#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
# How to run: python3 tools/dabbl8_clock_capture.py --help (plan-only by default).
"""Read-only clock comparison; default plan opens no device or native port."""
import argparse
from dataclasses import asdict, dataclass
import hashlib
import json
from pathlib import Path
import re
import time
from typing import Callable, Final, Protocol

from dabbl8_clock_bounds import ClockBounds, ClockObservation, ClockRefused, clock_bounds, parse_clock
from dabbl8_timing_capture import CLOCK_COMMANDS, Console, MacCdc, package_identity, status


class ClockConsole(Protocol):
    def receive(self) -> str: ...
    def request(self, command: str) -> str: ...


@dataclass(frozen=True, slots=True)
class ClockRun:
    points: tuple[ClockObservation, ...]
    bounds: ClockBounds


@dataclass(frozen=True, slots=True)
class ClockRequest:
    version: str
    uac_enabled: bool = False


@dataclass(frozen=True, slots=True)
class ClockSource:
    now: Callable[[], int] = time.monotonic_ns
    wait: Callable[[float], None] = time.sleep


HOST_CLOCK: Final = ClockSource()


def require_health(reply: str) -> None:
    """Do not accept a rate comparison on a failed flash/audio diagnostic epoch."""
    lines = reply.splitlines()
    for field in ('audio_late 0', 'flash 1', 'fl_failure 0'):
        if lines.count(field) != 1 or sum(line.startswith(field.split()[0] + ' ') for line in lines) != 1:
            raise ClockRefused('Clock status health guard failed: ' + field.split()[0])


def collect_clock(console: ClockConsole, request: ClockRequest,
                  clock: ClockSource = HOST_CLOCK) -> ClockRun:
    """One bounded status/three-sample/status sequence; never retry or reset anything."""
    banner = console.receive()
    if banner != f"\r\nDabbl8 {request.version} console - 'help'\r\n> ":
        raise ClockRefused('Expected one fresh console greeting')
    reply = console.request('status')
    require_health(reply)
    before = status(reply, request.version, request.uac_enabled)
    observations: list[ClockObservation] = []
    for index in range(3):
        if index:
            clock.wait(5)
        sent = clock.now()
        reply = console.request('timing clock')
        received = clock.now()
        point = ClockObservation(sent, received, parse_clock(reply))
        if (point.packet.boots, point.packet.usb_resets, point.packet.uac) != (
            before['boots'], before['usb_resets'], int(request.uac_enabled)
        ):
            raise ClockRefused('Clock sample disagrees with admitted status')
        observations.append(point)
        if len(observations) >= 2:
            _ = clock_bounds(tuple(observations))  # Refuse before another request.
    reply = console.request('status')
    require_health(reply)
    after = status(reply, request.version, request.uac_enabled)
    if before != after:
        raise ClockRefused('Boot/USB profile changed during comparison')
    points = tuple(observations)
    return ClockRun(points, clock_bounds(points))


class ClockArgs(argparse.Namespace):
    """Mutable argument-parsing destination; required fields are set by argparse."""
    manifest: Path = Path()
    version: str = ''
    uac_enabled: bool = False
    port: str | None = None
    output: Path | None = None
    capture: bool = False


def main() -> int:
    """Preserve raw traffic and cleanup errors; explicit capture remains owner-gated."""
    parser = argparse.ArgumentParser(description=__doc__)
    _ = parser.add_argument('--manifest', type=Path, required=True)
    _ = parser.add_argument('--version', required=True)
    _ = parser.add_argument('--uac-enabled', action='store_true')
    _ = parser.add_argument('--port')
    _ = parser.add_argument('--output', type=Path)
    _ = parser.add_argument('--capture', action='store_true')
    args = ClockArgs()
    _ = parser.parse_args(namespace=args)
    if not re.fullmatch(r'v[0-9]+\.[0-9]+\.[0-9]+-timing', args.version):
        parser.error('Expected vX.Y.Z-timing identity')
    identity = package_identity(args.manifest)
    header = Path(__file__).resolve().parents[1] / 'firmware/src/audio_clock_console.h'
    capable = identity['clock_source_sha256'] == hashlib.sha256(header.read_bytes()).hexdigest()
    expected_name = 'dabbl8-' + args.version[1:].replace('-timing', '-timing-diagnostic.fwsc')
    if identity['diagnostic_package'] != expected_name:
        parser.error('Version must match the pinned diagnostic package')
    if not args.capture:
        print(json.dumps({'mode': 'plan-only; no device access', 'identity': identity,
                          'commands': CLOCK_COMMANDS, 'samples': 3,
                          'candidate_supports_d8c1': capable,
                          'hardware_qualified': False}, indent=2))
        return 0
    if not args.port or args.output is None:
        parser.error('--capture requires explicit --port and new --output')
    if not capable:
        parser.error('Package does not pin the reviewed D8C1 diagnostic; no port opened')
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = {**identity, 'accepted': False, 'hardware_qualified': False,
               'device_flashed': False, 'version': args.version,
               'reference': 'host monotonic_ns; oscillator accuracy/drift not certified',
               'assumptions': 'stable TIMER4, 1..100 MHz; complete fresh request/reply brackets',
               'full_isr_or_stack_qualification': False}
    with (args.output / 'journal.jsonl').open('x', encoding='utf-8') as log:
        def journal(direction: str, data: bytes) -> None:
            _ = log.write(json.dumps({'host_ns': time.monotonic_ns(), 'direction': direction,
                                  'hex': data.hex()}) + '\n')
            log.flush()
        transport = None
        try:
            transport = MacCdc(args.port, commands=CLOCK_COMMANDS)
            result = collect_clock(Console(transport, journal, commands=CLOCK_COMMANDS),
                                   ClockRequest(args.version, args.uac_enabled))
            _ = (args.output / 'comparison.json').write_text(
                json.dumps(asdict(result), indent=2) + '\n', encoding='utf-8')
            receipt['accepted'] = True
        except (OSError, ValueError, RuntimeError) as error:
            receipt['failure'] = str(error)
        finally:
            if transport is not None:
                try:
                    transport.close()
                except (OSError, ValueError) as error:
                    receipt['close_failure'] = str(error)
                    receipt['accepted'] = False
            _ = (args.output / 'receipt.json').write_text(
                json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(receipt, indent=2))
    return 0 if receipt['accepted'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
