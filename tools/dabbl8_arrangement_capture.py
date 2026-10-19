#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
"""Bounded diagnostic-only arrangement capture. Default planning opens no port."""
import argparse
from dataclasses import asdict, dataclass, field
from enum import Enum
import hashlib
import json
from pathlib import Path
import re
import time
from typing import Callable, Final, Protocol, TypeAlias, TypedDict, cast

from dabbl8_arrangement_state import ArrangementObservation, ArrangementState, Phase, parse_arrangement
from dabbl8_resource_capture import Candidate, acknowledge, candidate, report_payload
from dabbl8_resource_report import Expected, ResourceReport, parse_report as parse_resource
from dabbl8_timing_capture import ARRANGEMENT_COMMANDS, Console, MacCdc, status
import dabbl8_timing_report as timing_report

PROJECT_SHA: Final = '25ce16e8ae7b6fab1d5b4020df95e41fd0b1df6525c735adb698ed6ce54dbe61'
HEADER_SHA: Final = '690d815ee325884eac6db5955e8e0c780ec413302972d5b053db97b91a40ca27'
EXPECTED: Final = Expected(0, 0x3ffd, False)
PEAKS: Final = (0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 6, 0)
Json: TypeAlias = str | int | float | bool | None | list['Json'] | dict[str, 'Json']


class ConsolePort(Protocol):
    def receive(self) -> str: ...
    def request(self, command: str) -> str: ...


class TransportPort(Protocol):
    def read(self, timeout: float) -> bytes: ...
    def write(self, data: bytes) -> None: ...
    def close(self) -> None: ...


class Lease(str, Enum):
    NOT_OWNED = 'not-owned'
    UNKNOWN_START = 'start-unconfirmed'
    OWNED = 'owned-running'
    UNKNOWN_STOP = 'stop-unconfirmed'
    PAUSED = 'paused-acknowledged'


@dataclass
class Receipt:
    version: str
    candidate_identity: Candidate | None = None
    project_sha256: str = PROJECT_SHA
    timing: Lease = Lease.NOT_OWNED
    resource: Lease = Lease.NOT_OWNED
    resource_generation: int = 0
    boots: int = 0
    usb_resets: int = 0
    audio_history: tuple[int, ...] = ()
    rows_seen: int = 0
    observation_complete: bool = False
    accepted: bool = False
    failure: str | None = None
    cleanup_failures: list[str] = field(default_factory=list)
    close_failure: str | None = None
    snapshot_may_be_attempted: bool = False
    quiet_guard_still_required: bool = True
    device_write_commands_sent: bool = False
    hardware_qualified: bool = False


@dataclass(frozen=True)
class Clock:
    now: Callable[[], float] = time.monotonic
    wait: Callable[[float], None] = time.sleep


class TimingWindow(TypedDict):
    uac_enabled: bool
    late_publications: int
    taint: int
    alnk_aux_pending_observations: list[int]


class TimingReport(TypedDict):
    format: str
    tracks: int
    samples: int
    window: TimingWindow


class TimingParser(Protocol):
    def __call__(self, text: str, expected_generation: int | None = None,
                 expected_boots: int | None = None) -> TimingReport: ...


# Explicit legacy boundary: the existing validated report parser returns a
# richer JSON mapping. Admit D8T2 before using its required window fields.
parse_timing = cast(TimingParser, timing_report.parse_report)


@dataclass(frozen=True)
class Result:
    progress: ArrangementObservation
    resources: ResourceReport
    timing: TimingReport


def number(reply: str, name: str, *, hexadecimal: bool = False) -> int:
    """Read one canonical unsigned status field, never coalesce duplicates."""
    rows = [s for s in reply.splitlines() if s.startswith(name + ' ')]
    pattern = name + (r' 0x[0-9A-F]{8}' if hexadecimal else r' [0-9]+')
    if len(rows) != 1 or not re.fullmatch(pattern, rows[0]):
        raise ValueError('Missing/ambiguous status: ' + name)
    value = int(rows[0].split()[1], 16 if hexadecimal else 10)
    if value > 0xffffffff:
        raise ValueError('Status outside32bits: ' + name)
    return value


def checked_status(console: ConsolePort, receipt: Receipt, *, initial: bool = False) -> ArrangementState:
    """Require healthy epoch/profile; transport state comes from coherent D8A1."""
    reply = console.request('status')
    epoch = status(reply, receipt.version, False)
    late_key = 'audio_late_boot' if 'audio_late_boot' in epoch else 'audio_late'
    history = tuple(epoch[k] for k in ('audio_late', 'audio_late_entry', 'audio_late_boot')) if 'audio_late_boot' in epoch else ()
    expected = {late_key: 0, 'voices_shed': 0, 'flash': 1, 'fl_failure': 0,
                'resource_schema': 1, 'resource_helpers': 3, 'resource_tracks': 8,
                'resource_profile': 0, 'resource_uac': 0, 'bpm': 120,
                'resource_running': int(receipt.resource == Lease.OWNED)}
    for key, wanted in expected.items():
        if number(reply, key) != wanted:
            raise ValueError('Health/profile failed: ' + key)
    if number(reply, 'resource_engine_mask', hexadecimal=True) != EXPECTED.mask:
        raise ValueError('Wrong engine mask')
    generation = number(reply, 'resource_generation', hexadecimal=True)
    state = parse_arrangement(reply)
    if (state.boots, state.usb_resets) != (epoch['boots'], epoch['usb_resets']):
        raise ValueError('Snapshot/outer epoch changed')
    if initial:
        if generation == 0xffffffff:
            raise ValueError('Resource generation exhausted; no diagnostic start')
        receipt.boots, receipt.usb_resets = state.boots, state.usb_resets
        receipt.resource_generation = generation + 1
        receipt.audio_history = history
    elif ((state.boots, state.usb_resets) != (receipt.boots, receipt.usb_resets)
          or generation != receipt.resource_generation or history != receipt.audio_history):
        raise ValueError('Epoch, audio history or resource generation changed')
    return state


def timing_ack(reply: str, command: str, word: str) -> None:
    """Admit one exact legacy timing acknowledgement, not unrelated text."""
    if not reply.startswith(command + '\r\n') or not reply.endswith('\r\n> '):
        raise ValueError('Timing acknowledgement framing failed')
    if [s for s in reply.splitlines()[1:] if s not in ('', '> ')] != [word]:
        raise ValueError('Timing acknowledgement failed')


def pause(console: ConsolePort, receipt: Receipt, kind: str) -> None:
    """One attempt only, exclusively for an acknowledged owned diagnostic run."""
    lease = receipt.timing if kind == 'timing' else receipt.resource
    if kind not in ('timing', 'resource') or lease != Lease.OWNED:
        raise ValueError('Cannot pause an unowned/uncertain run')
    if kind == 'timing':
        receipt.timing = Lease.UNKNOWN_STOP
        timing_ack(console.request('timing stop'), 'timing stop', 'timing stopped')
        receipt.timing = Lease.PAUSED
    else:
        receipt.resource = Lease.UNKNOWN_STOP
        acknowledge(console.request('resource stop'), ('resource stopped', 'resource paused'),
                    receipt.resource_generation, receipt.boots)
        receipt.resource = Lease.PAUSED


def collect(console: ConsolePort, receipt: Receipt, *, clock: Clock = Clock(),
            ready: Callable[[], None] = lambda: None,
            record_state: Callable[[ArrangementState, float], None] = lambda _s, _t: None,
            record_frame: Callable[[str, str], None] = lambda _n, _s: None) -> Result:
    """Own both observers around one owner PLAY; failures only pause known leases."""
    if receipt != Receipt(receipt.version, receipt.candidate_identity):
        raise ValueError('A fresh, unowned diagnostic receipt is required')
    try:
        if console.receive() != f"\r\nDabbl8 {receipt.version} console - 'help'\r\n> ":
            raise ValueError('Expected one fresh console greeting')
        initial = checked_status(console, receipt, initial=True)
        ArrangementObservation.begin(initial, clock.now())
        receipt.timing = Lease.UNKNOWN_START
        reply = console.request('timing start')
        if [s for s in reply.splitlines()[1:] if s not in ('', '> ')] == ['timing start refused']:
            receipt.timing = Lease.NOT_OWNED
            raise ValueError('Timing start refused; existing run preserved')
        timing_ack(reply, 'timing start', 'timing started')
        receipt.timing = Lease.OWNED
        receipt.resource = Lease.UNKNOWN_START
        reply = console.request('resource start')
        if [s for s in reply.splitlines()[1:] if s not in ('', '> ')] == ['resource start refused']:
            receipt.resource = Lease.NOT_OWNED
            raise ValueError('Resource start refused; existing run preserved')
        ack = acknowledge(reply, ('resource started', 'resource invalid'), receipt.resource_generation, receipt.boots)
        receipt.resource = Lease.OWNED if ack == 'resource started' else Lease.PAUSED
        if receipt.resource != Lease.OWNED:
            raise ValueError('Resource observation invalid at start')
        baseline = checked_status(console, receipt)
        progress = ArrangementObservation.begin(baseline, clock.now())
        ready()
        while progress.phase != Phase.COMPLETE:
            clock.wait(.25)
            state = checked_status(console, receipt)
            seconds = clock.now()
            record_state(state, seconds)
            progress = progress.observe(state, seconds)
            receipt.rows_seen = progress.seen
        receipt.observation_complete = True
        clock.wait(12)  # Matched native proposal's bounded tail; quiet still requires firmware admission.
        state = checked_status(console, receipt)
        require_final_state(progress, state)
        pause(console, receipt, 'resource')
        pause(console, receipt, 'timing')
        resource_reply = console.request('resource report')
        record_frame('resource', resource_reply)
        resources = parse_resource(report_payload(resource_reply.encode('ascii')).decode('ascii'),
                                   EXPECTED, receipt.resource_generation, receipt.boots)
        if resources.pool.failures or resources.pool.peak_live_owned_slots != 8 or resources.independent_peak_engine != PEAKS:
            raise ValueError('Expected eight-body observation absent or failed')
        timing_reply = console.request('timing report')
        record_frame('timing', timing_reply)
        timing = parse_timing(timing_reply, expected_boots=receipt.boots)
        if timing['format'] != 'D8T2':
            raise ValueError('D8T2 is required for an aligned observation')
        window = timing['window']
        if (timing['tracks'] != 8 or timing['samples'] < 11000 or
                window['uac_enabled'] or window['taint'] or window['late_publications'] or
                any(window['alnk_aux_pending_observations'])):
            raise ValueError('Timing window failed or cannot cover the known arrangement')
        final = checked_status(console, receipt)
        require_final_state(progress, final)
        return Result(progress, resources, timing)
    except BaseException as error:
        receipt.failure = f'{type(error).__name__}: {error}'
        raise
    finally:
        for kind in ('resource', 'timing'):
            if (receipt.resource if kind == 'resource' else receipt.timing) == Lease.OWNED:
                try:
                    pause(console, receipt, kind)
                except Exception as error:
                    receipt.cleanup_failures.append(f'{kind}: {type(error).__name__}: {error}')


def require_final_state(progress: ArrangementObservation, state: ArrangementState) -> None:
    """Validate frozen final native state; allow only timer advancement between samples."""
    if ((state.boots, state.usb_resets) != (progress.boots, progress.usb_resets) or
            (state.mode, state.playing, state.armed, state.running, state.row, state.scene, state.rows,
             state.valid, state.suspended, state.pending, state.fill_pending, state.fill_active,
             state.recording, state.request, state.count_in) != (1,0,0,0,15,15,16,1,1,255,255,0,0,0,0)
            or state.generation != ((progress.generation + 1) & 0xffffffff)):
        raise ValueError('Final native state is not the observed suspended Stop')


def run_transport(factory: Callable[[], TransportPort], receipt: Receipt, *, clock: Clock = Clock(),
                  journal: Callable[[str, bytes], None] = lambda _d, _b: None,
                  ready: Callable[[], None] = lambda: None,
                  record_state: Callable[[ArrangementState, float], None] = lambda _s, _t: None,
                  record_frame: Callable[[str, str], None] = lambda _n, _s: None) -> Result | None:
    """Acquire once and close once; success never hides cleanup/restoration failure."""
    transport, result = None, None
    try:
        if receipt != Receipt(receipt.version, receipt.candidate_identity):
            raise ValueError('A fresh, unowned diagnostic receipt is required before acquisition')
        transport = factory()
        console = cast(ConsolePort, Console(transport, journal, clock.now, commands=ARRANGEMENT_COMMANDS))
        result = collect(console, receipt, clock=clock, ready=ready,
                         record_state=record_state, record_frame=record_frame)
    except Exception as error:
        receipt.failure = receipt.failure or f'{type(error).__name__}: {error}'
        if error.__context__ is not None:
            receipt.failure += f'; context: {type(error.__context__).__name__}: {error.__context__}'
    finally:
        if transport is not None:
            try:
                transport.close()
            except Exception as error:
                receipt.close_failure = f'{type(error).__name__}: {error}'
        receipt.accepted = result is not None and receipt.failure is None and not receipt.cleanup_failures and receipt.close_failure is None
        receipt.snapshot_may_be_attempted = receipt.accepted
    return result


def admit_project(path: Path) -> None:
    """Pin bounded regular input bytes; never use a private arbitrary workspace."""
    if path.is_symlink() or not path.is_file():
        raise ValueError('Project must be a regular local file')
    with path.open('rb') as stream:
        raw = stream.read(7386)
    if len(raw) != 7385 or hashlib.sha256(raw).hexdigest() != PROJECT_SHA:
        raise ValueError('Expected exact locally verified C proposal')


class Args(argparse.Namespace):
    manifest: Path = Path()
    project: Path = Path()
    port: str | None = None
    output: Path | None = None
    capture: bool = False
    owner_ready: bool = False


def main() -> int:
    """Require exact local candidate/proposal before explicit native acquisition."""
    parser = argparse.ArgumentParser(description=__doc__)
    _ = parser.add_argument('--manifest', type=Path, required=True)
    _ = parser.add_argument('--project', type=Path, required=True)
    _ = parser.add_argument('--port')
    _ = parser.add_argument('--output', type=Path)
    _ = parser.add_argument('--capture', action='store_true')
    _ = parser.add_argument('--owner-ready', action='store_true')
    args = Args()
    _ = parser.parse_args(namespace=args)
    identity = candidate(args.manifest)
    if identity.expected != EXPECTED:
        parser.error('Only the pinned full-profile/UAC-off C arrangement is supported')
    admit_project(args.project)
    # This is the same pinned source manifest already checked by candidate().
    # No source-directory layout is required in a frozen standalone host kit.
    with args.manifest.open('rb') as stream:
        raw = stream.read(2_000_001)
    if len(raw) > 2_000_000 or hashlib.sha256(raw).hexdigest() != identity.manifest_sha256:
        parser.error('Manifest changed after qualification; no port opened')
    metadata = cast(Json, json.loads(raw))
    if not isinstance(metadata, dict):
        parser.error('Expected manifest mapping; no port opened')
    sources = metadata.get('source_inputs')
    capable = isinstance(sources, dict) and sources.get('firmware/src/arrangement_diagnostic_console.h') == HEADER_SHA
    if not args.capture:
        print(json.dumps({'mode': 'plan-only; no device access', 'identity': asdict(identity),
                          'commands': ARRANGEMENT_COMMANDS, 'candidate_supports_d8a1': capable,
                          'project_sha256': PROJECT_SHA, 'owner_play_window_seconds': 60,
                          'playback_window_seconds': 36, 'tail_seconds': 12,
                          'hardware_ready': False, 'hardware_qualified': False}, indent=2))
        return 0
    if not capable or not args.owner_ready:
        parser.error('Reviewed D8A1 candidate and currently ready owner required; no port opened')
    if not args.port or not re.fullmatch(r'/dev/cu\.usbmodem[A-Za-z0-9_-]+', args.port) or args.output is None:
        parser.error('Explicit USB console port and new output directory required')
    output = args.output
    output.mkdir(mode=0o700, parents=True, exist_ok=False)
    receipt, result = Receipt(identity.version, identity), None
    with (output / 'journal.jsonl').open('x', encoding='utf-8') as log, (output / 'rows.jsonl').open('x', encoding='utf-8') as rows:
        def journal(direction: str, data: bytes) -> None:
            _ = log.write(json.dumps({'host_seconds': time.monotonic(), 'direction': direction, 'hex': data.hex()}) + '\n')
            log.flush()
        def record_state(state: ArrangementState, seconds: float) -> None:
            _ = rows.write(json.dumps({'host_seconds': seconds, 'state': asdict(state)}) + '\n')
            rows.flush()
        def record_frame(name: str, reply: str) -> None:
            if name not in ('resource', 'timing'):
                raise ValueError('Unknown report artifact')
            _ = (output / (name + '.response')).write_text(reply, encoding='ascii')
        try:
            result = run_transport(lambda: cast(TransportPort, MacCdc(args.port, commands=ARRANGEMENT_COMMANDS)),
                receipt, journal=journal, ready=lambda: print('READY: load C; hold SEQ; press PLAY once.', flush=True),
                record_state=record_state, record_frame=record_frame)
            if result is not None:
                _ = (output / 'observation.json').write_text(json.dumps(asdict(result), indent=2) + '\n')
        except Exception as error:
            receipt.failure = receipt.failure or f'{type(error).__name__}: {error}'
            receipt.accepted = receipt.snapshot_may_be_attempted = False
        finally:
            _ = (output / 'receipt.json').write_text(json.dumps(asdict(receipt), indent=2) + '\n')
    print(json.dumps(asdict(receipt), indent=2))
    return 0 if receipt.accepted else 1


if __name__ == '__main__':
    raise SystemExit(main())
