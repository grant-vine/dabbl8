#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# /// script
# dependencies = []
# ///
"""Paused resource observations. Run python3 tools/dabbl8_resource_capture.py --help."""
from __future__ import annotations
import argparse
from dataclasses import asdict, dataclass
from enum import Enum
import hashlib
import json
import math
from pathlib import Path
import re
import time
from typing import Callable
from dabbl8_resource_report import Expected, ResourceReport, parse_report
from dabbl8_timing_capture import Console, MacCdc, RESOURCE_COMMANDS, package_identity, status


class ResourceCaptureError(ValueError):
    def __init__(self, operation: str, detail: str) -> None:
        self.operation, self.detail = operation, detail
        super().__init__(f'{operation}: {detail}')


@dataclass(frozen=True)
class Candidate:
    source_commit: str
    manifest_sha256: str
    diagnostic_package: str
    package_sha256: str
    expected: Expected
    version: str
    device_binary_identity_attested: bool = False


def defines(flags: list[str], name: str) -> list[str]:
    if '-D' in flags or '-U' in flags:
        raise ResourceCaptureError('manifest', 'split macro arguments unsupported')
    return [f for f in flags if re.match(r'-[DU]' + name + r'(?:$|=|\()', f)]


def boolean_define(flags: list[str], name: str, default: bool) -> bool:
    values = defines(flags, name)
    if not values: return default
    if values not in (['-D' + name + '=0'], ['-D' + name + '=1']):
        raise ResourceCaptureError('manifest', 'conflicting/bare/nonliteral define ' + name)
    return values[0].endswith('=1')


def candidate(manifest: Path) -> Candidate:
    identity = package_identity(manifest)
    with manifest.open('rb') as stream: raw = stream.read(2_000_001)
    if len(raw) > 2_000_000 or hashlib.sha256(raw).hexdigest() != identity['manifest_sha256']:
        raise ResourceCaptureError('manifest', 'identity metadata changed during validation')
    metadata = json.loads(raw)
    required = {'resource_diagnostic': True, 'resource_report_schema': 'D8R1',
                'resource_collection_initially_enabled': False}
    if any(type(metadata.get(k)) is not type(v) or metadata.get(k) != v for k, v in required.items()):
        raise ResourceCaptureError('manifest', 'explicit initially paused D8R1 resource kit required')
    profile, mask, uac = (metadata.get(k) for k in
        ('resource_engine_profile', 'resource_engine_mask', 'resource_uac_enabled'))
    if type(profile) is not int or type(mask) is not int or type(uac) is not bool:
        raise ResourceCaptureError('manifest', 'typed profile, mask and UAC fields required')
    expected = Expected(profile, mask, uac)
    flags, startup = metadata.get('compiler_flags'), metadata.get('startup_flags')
    if not isinstance(flags, list) or not isinstance(startup, list) or any(type(f) is not str for f in flags + startup):
        raise ResourceCaptureError('manifest', 'actual compiler/startup argument lists required')
    required_defines = {'D8_HEAVY_POOL_DIAGNOSTIC': '1', 'D8_STACK_WATERMARK_DIAGNOSTIC': '1',
               'DABBL8_IDENTITY': '1', 'DABBL8_TIMING_DIAGNOSTIC': '1', 'NPART': '8'}
    for name, value in required_defines.items():
        if defines(flags, name) != ['-D' + name + '=' + value]:
            raise ResourceCaptureError('manifest', 'missing/conflicting application define ' + name)
    if defines(startup, 'D8_STACK_WATERMARK_DIAGNOSTIC') != ['-DD8_STACK_WATERMARK_DIAGNOSTIC=1']:
        raise ResourceCaptureError('manifest', 'colored application startup required')
    profile_flags = defines(flags, 'D8BUILD_PROFILE')
    if profile_flags != ([] if profile == 0 else [f'-DD8BUILD_PROFILE={profile}']) and profile_flags != [f'-DD8BUILD_PROFILE={profile}']:
        raise ResourceCaptureError('manifest', 'profile define disagrees with metadata')
    if not boolean_define(flags, 'FELUCCA_CDC', True) or boolean_define(flags, 'FELUCCA_UAC', True) != uac:
        raise ResourceCaptureError('manifest', 'CDC/UAC defines disagree with resource kit')
    fm4, sliced = boolean_define(flags, 'FELUCCA_FM4', False), boolean_define(flags, 'FELUCCA_SLICE', True)
    effective = (0x1ffd | (0x2000 if sliced else 0) | (2 if fm4 else 0)) if profile == 0 else (0x1409, 0x3411)[profile - 1]
    if mask != effective or (profile and (fm4 or not sliced)) or defines(flags, 'D8BUILD_MASK') or defines(flags, 'D8ENGINE_EFFECTIVE_MASK'):
        raise ResourceCaptureError('manifest', 'effective engine mask disagrees with supported flags')
    if metadata.get('engine_profile') != ('full', 'core', 'breaks')[profile]:
        raise ResourceCaptureError('manifest', 'profile label disagrees with profile ID')
    return Candidate(identity['source_commit'], identity['manifest_sha256'],
                     identity['diagnostic_package'], identity['package_sha256'], expected,
                     'v' + identity['diagnostic_package'][7:-len('-timing-diagnostic.fwsc')] + '-timing')


@dataclass(frozen=True)
class Observation:
    boots: int
    usb_resets: int
    uac_enabled: bool
    uac_config: tuple[int, ...]
    generation: int
    audio_history: tuple[int, ...] = ()

    @property
    def stable(self) -> tuple[int, ...]:
        return (self.boots, self.usb_resets, int(self.uac_enabled), *self.uac_config,
                *self.audio_history)


def observation(reply: str, version: str, expected: Expected) -> Observation:
    baseline = status(reply, version, expected.uac)
    fields = {}
    for key in ('schema', 'helpers', 'tracks', 'profile', 'engine_mask', 'uac', 'running', 'generation'):
        rows = [line for line in reply.splitlines() if line.startswith('resource_' + key + ' ')]
        pattern = r'resource_' + key + (r' 0x[0-9A-F]{8}' if key in ('engine_mask', 'generation') else r' [0-9]+')
        if len(rows) != 1 or not re.fullmatch(pattern, rows[0]):
            raise ResourceCaptureError('status', 'missing/ambiguous resource capability ' + key)
        fields[key] = int(rows[0].split()[1], 0 if key in ('engine_mask', 'generation') else 10)
    if tuple(fields[k] for k in ('schema', 'helpers', 'tracks', 'profile', 'engine_mask', 'uac', 'running')) != (1, 3, 8, expected.profile, expected.mask, int(expected.uac), 0):
        raise ResourceCaptureError('status', 'capability/profile mismatch or existing active run')
    history = tuple(baseline[k] for k in ('audio_late', 'audio_late_entry', 'audio_late_boot')) if 'audio_late_boot' in baseline else ()
    return Observation(baseline['boots'], baseline['usb_resets'], expected.uac,
        tuple(baseline[k] for k in ('uac_hz', 'uac_alt', 'uac_rate_sets', 'uac_starts')) if expected.uac else (), fields['generation'], history)


class Ownership(str, Enum):
    NOT_OWNED = 'not-owned'
    UNKNOWN_START = 'start-ack-lost-or-malformed; inspect explicitly'
    OWNED = 'owned-running'
    UNKNOWN_STOP = 'stop-ack-lost-or-malformed; inspect explicitly'
    PAUSED = 'paused-acknowledged'


@dataclass
class Receipt:
    identity: Candidate
    workload: str
    version: str
    port: str
    requested_seconds: float
    accepted: bool = False
    ownership: Ownership = Ownership.NOT_OWNED
    host_elapsed_seconds: float | None = None
    failure: str | None = None
    cleanup_failure: str | None = None
    close_failure: str | None = None
    hardware_qualified: bool = False
    device_flashed: bool = False


def acknowledge(reply: str, words: tuple[str, ...], generation: int, boots: int) -> str:
    rows = [line for line in reply.splitlines()[1:] if line not in ('', '> ')]
    if len(rows) != 1 or rows[0] not in tuple(f'{word} {generation:08X} {boots:08X}' for word in words):
        raise ResourceCaptureError('ack', 'generation/boot-bound acknowledgement missing')
    return rows[0].rsplit(' ', 2)[0]


def report_payload(raw: bytes) -> bytes:
    prefix = b'resource report\r\n'
    if raw.startswith(prefix): raw = raw[len(prefix):]
    if raw.endswith(b'\r\n> '): raw = raw[:-2]
    return raw


def collect(console: Console, output: Path, receipt: Receipt, *,
            sleep: Callable[[float], None] = time.sleep,
            clock: Callable[[], float] = time.monotonic) -> ResourceReport:
    original_journal, recording = console.journal, False
    raw = bytearray()
    def journal(direction: str, data: bytes) -> None:
        nonlocal recording
        original_journal(direction, data)
        if direction == 'send': recording = data == b'resource report\r'
        elif direction == 'receive' and recording:
            raw.extend(data)
            (output / 'report.response').write_bytes(raw)
    console.journal = journal
    (output / 'report.response').write_bytes(b'')
    origin = clock()
    generation = boots = 0
    try:
        banner = console.receive()
        if banner.splitlines().count("Dabbl8 " + receipt.version + " console - 'help'") != 1:
            raise ResourceCaptureError('banner', 'fresh diagnostic banner required')
        before = observation(console.request('status'), receipt.version, receipt.identity.expected)
        boots, generation = before.boots, before.generation + 1
        receipt.ownership = Ownership.UNKNOWN_START
        started = console.request('resource start')
        if [line for line in started.splitlines()[1:] if line not in ('', '> ')] == ['resource start refused']:
            receipt.ownership = Ownership.NOT_OWNED
            raise ResourceCaptureError('start', 'refused; existing run preserved')
        ack = acknowledge(started, ('resource started', 'resource invalid'), generation, boots)
        receipt.ownership = Ownership.OWNED if ack == 'resource started' else Ownership.PAUSED
        if receipt.ownership == Ownership.OWNED:
            sleep(receipt.requested_seconds)
            receipt.ownership = Ownership.UNKNOWN_STOP
            acknowledge(console.request('resource stop'), ('resource stopped', 'resource paused'), generation, boots)
            receipt.ownership = Ownership.PAUSED
        receipt.host_elapsed_seconds = clock() - origin
        reply = console.request('resource report')
        frame = report_payload(reply.encode('ascii'))
        (output / 'report.frame').write_bytes(frame)
        report = parse_report(frame.decode('ascii'), receipt.identity.expected, generation, boots)
        after = observation(console.request('status'), receipt.version, receipt.identity.expected)
        if before.stable != after.stable or after.generation != generation:
            raise ResourceCaptureError('status', 'boot/USB/UAC/generation changed in outer observation')
        payload = {'report': asdict(report), 'host_observation': {'before': asdict(before), 'after': asdict(after),
            'workload': receipt.workload, 'host_elapsed_seconds': receipt.host_elapsed_seconds,
            'requested_seconds': receipt.requested_seconds}, 'identity': asdict(receipt.identity),
            'hardware_qualified': False, 'scope': 'Pool observed ownership since start; stack surviving writes since boot, nonatomic lower bound. Identity is observational.'}
        (output / 'report.json').write_text(json.dumps(payload, indent=2) + '\n')
        return report
    finally:
        if receipt.ownership == Ownership.OWNED:
            receipt.ownership = Ownership.UNKNOWN_STOP
            try:
                acknowledge(console.request('resource stop'), ('resource stopped', 'resource paused'), generation, boots)
                receipt.ownership = Ownership.PAUSED
            except Exception as error:
                receipt.cleanup_failure = str(error)
                original_journal('cleanup-failed', b'Owned run pause unconfirmed; inspect explicitly. No retry.')
        console.journal = original_journal
        if not (output / 'report.frame').exists():
            (output / 'report.frame').write_bytes(report_payload(bytes(raw)))


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--manifest', type=Path, required=True)
    p.add_argument('--version', required=True)
    p.add_argument('--workload', required=True)
    p.add_argument('--seconds', type=float, default=10)
    p.add_argument('--port'); p.add_argument('--output', type=Path)
    p.add_argument('--capture', action='store_true')
    args = p.parse_args()
    if not re.fullmatch(r'v[0-9]+\.[0-9]+\.[0-9]+-timing', args.version) or not math.isfinite(args.seconds) or not 1 <= args.seconds <= 60 or not args.workload.strip():
        p.error('Expected vX.Y.Z-timing version, workload label and finite 1..60 seconds')
    identity = candidate(args.manifest)  # All package/config checks precede any port open.
    if args.version != identity.version: p.error('Version must match the verified diagnostic package name')
    if not args.capture:
        print(json.dumps({'mode': 'plan-only; no device access', 'identity': asdict(identity), 'commands': RESOURCE_COMMANDS, 'workload': args.workload}, indent=2)); return 0
    if not args.port or args.output is None: p.error('--capture requires explicit port and new output directory')
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = Receipt(identity, args.workload, args.version, args.port, args.seconds)
    transport = None
    origin = time.monotonic()
    with (args.output / 'journal.jsonl').open('x') as log:
        def journal(direction: str, data: bytes) -> None:
            log.write(json.dumps({'host_seconds': time.monotonic()-origin, 'direction': direction, 'hex': data.hex(), 'ascii': data.decode('ascii', errors='backslashreplace')})+'\n'); log.flush()
        try:
            transport = MacCdc(args.port, commands=RESOURCE_COMMANDS)
            collect(Console(transport, journal, commands=RESOURCE_COMMANDS), args.output, receipt)
            receipt.accepted = True
        except Exception as error: receipt.failure = str(error)
        finally:
            if transport:
                try: transport.close()
                except Exception as error: receipt.close_failure = str(error); receipt.accepted = False
            for name in ('report.frame', 'report.response'):
                if not (args.output / name).exists(): (args.output / name).write_bytes(b'')
            (args.output / 'receipt.json').write_text(json.dumps(asdict(receipt), indent=2)+'\n')
    print(json.dumps(asdict(receipt), indent=2))
    return 0 if receipt.accepted else 1


if __name__ == '__main__': raise SystemExit(main())
