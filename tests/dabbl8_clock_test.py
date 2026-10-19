# SPDX-License-Identifier: GPL-3.0-only
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
# How to run: python3 tests/dabbl8_clock_test.py (offline; no native MIDI/CDC).
"""Deterministic clock math, exact replies and stop-on-first-failure controls."""
from dataclasses import replace
from pathlib import Path
import contextlib
import hashlib
import io
import json
import struct
import sys
import tempfile
from unittest.mock import patch
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from dabbl8_clock_bounds import ClockObservation, ClockPacket, ClockRefused, clock_bounds, parse_clock
from dabbl8_clock_capture import ClockRequest, ClockSource, collect_clock
import dabbl8_clock_capture as capture
from dabbl8_timing_capture import CLOCK_COMMANDS, COMMANDS, Console, command_profile

VERSION = 'v0.2.3-timing'
BASE = ClockPacket(100, 17, 2, 9, 0xFFFFFFFF, 0)


def reply(packet: ClockPacket) -> str:
    words = (packet.ticks, packet.boots, packet.usb_resets, packet.config, packet.period, packet.uac)
    crc = zlib.crc32(struct.pack('<7I', 0x44384331, *words))
    return 'timing clock\r\nd8c1 ' + ' '.join(f'{w:08X}' for w in (*words, crc)) + '\r\n> '


def rejects(operation) -> None:
    try:
        operation()
    except (ClockRefused, ValueError):
        return
    raise AssertionError('Expected refusal')


def observations(rate: int, origin: int = 0) -> tuple[ClockObservation, ...]:
    # Deliberately asymmetric and varying delays; exact sample times are known only to this fixture.
    return tuple(ClockObservation(send, send + width, replace(BASE,
        ticks=(origin + sample * rate // 1_000_000_000) % (1 << 32)))
        for send, width, sample in ((0, 8_000_000, 7_000_000),
                                   (5_000_000_000, 2_000_000, 5_000_100_000),
                                   (10_000_000_000, 4_000_000, 10_003_000_000)))


def test_rate_bounds() -> None:
    for rate in (23_900_000, 24_000_000, 24_100_000):
        for origin in (0, 0xFFF00000):
            bounds = clock_bounds(observations(rate, origin))
            assert bounds.lower_hz <= rate <= bounds.upper_hz
            assert bounds.upper_hz - bounds.lower_hz < 30_000
            assert bounds.comparisons == 3
    exact = (ClockObservation(0, 1, replace(BASE, ticks=0)),
             ClockObservation(10_000_000_000, 10_000_000_001, replace(BASE, ticks=240_000_000)))
    bounds = clock_bounds(exact)
    assert bounds.lower_hz == 23_999_999 and bounds.upper_hz == 24_000_001


def test_invalid_observations() -> None:
    points = observations(24_000_000)
    rejects(lambda: clock_bounds(points[:1]))
    rejects(lambda: ClockObservation(2, 1, BASE))
    rejects(lambda: ClockObservation(0, 1_000_000_001, BASE))
    for field, value in (('boots', 18), ('usb_resets', 3), ('uac', 1)):
        changed = replace(points[1], packet=replace(points[1].packet, **{field: value}))
        rejects(lambda: clock_bounds((points[0], changed)))
    rejects(lambda: clock_bounds(tuple(reversed(points))))
    rejects(lambda: clock_bounds((points[0], replace(points[0], sent_ns=1, received_ns=2))))
    rejects(lambda: clock_bounds((points[0], replace(points[1], sent_ns=50_000_000_000,
                                                   received_ns=50_000_000_001))))
    rejects(lambda: clock_bounds((points[0], replace(points[1], packet=points[0].packet))))
    changed = replace(points[2], packet=replace(points[2].packet, ticks=800_000_000))
    rejects(lambda: clock_bounds((points[0], points[1], changed)))
    for config, period in ((8, 0xFFFFFFFF), (0x19, 0xFFFFFFFF), (9, 0xFFFFFFFE)):
        rejects(lambda: ClockPacket(100, 17, 2, config, period, 0))


def test_wire_parser() -> None:
    frame = reply(BASE)
    assert parse_clock(frame) == BASE
    for invalid in (frame[:-1], frame + frame, frame.replace('00000011', '00000012'),
                    frame.replace('FFFFFFFF', 'ffffffff'), frame.replace('timing clock', 'status'),
                    frame.replace('d8c1', 'd8c2'), frame.replace('d8c1', 'timing clock changed')):
        rejects(lambda: parse_clock(invalid))
    for command in ('timing start', 'timing stop', 'uboot yes', 'timing clock\ruboot yes'):
        class Unused:
            def write(self, data: bytes) -> None:
                raise AssertionError('Forbidden command reached transport')
        rejects(lambda: Console(Unused(), lambda *args: None, commands=CLOCK_COMMANDS).request(command))
    assert command_profile(COMMANDS) == COMMANDS


class ScriptedConsole:
    """Mutable command journal; all responses and time are injected."""
    def __init__(self, packets: tuple[ClockPacket, ...], *, version: str = VERSION) -> None:
        self.packets = iter(packets)
        self.version = version
        self.commands: list[str] = []
        self.statuses = 0

    def receive(self) -> str:
        return f"\r\nDabbl8 {self.version} console - 'help'\r\n> "

    def request(self, command: str) -> str:
        self.commands.append(command)
        if command == 'status':
            self.statuses += 1
            return f'dabbl8 {self.version}\r\nboots 17\r\nusb_resets 2\r\naudio_late 0\r\nflash 1\r\nfl_failure 0\r\n'
        assert command == 'timing clock'
        return reply(next(self.packets))


def test_collection_and_failure_stop() -> None:
    points = observations(24_000_000)
    console = ScriptedConsole(tuple(p.packet for p in points))
    times = iter(n for p in points for n in (p.sent_ns, p.received_ns))
    result = collect_clock(console, ClockRequest(VERSION), ClockSource(lambda: next(times), lambda n: None))
    assert result.points == points and result.bounds.lower_hz <= 24_000_000 <= result.bounds.upper_hz
    assert console.commands == ['status', 'timing clock', 'timing clock', 'timing clock', 'status']
    for change in (replace(points[1].packet, boots=18), points[0].packet):
        console = ScriptedConsole((points[0].packet, change))
        times = iter(n for p in points for n in (p.sent_ns, p.received_ns))
        rejects(lambda: collect_clock(console, ClockRequest(VERSION),
                ClockSource(lambda: next(times), lambda n: None)))
        assert console.commands == ['status', 'timing clock', 'timing clock']


def test_cli_access_and_cleanup() -> None:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        name = 'dabbl8-0.2.3-timing-diagnostic.fwsc'
        (root / name).write_bytes(b'offline fixture')
        manifest = root / 'manifest.json'
        manifest.write_text(json.dumps({'source_commit': 'a' * 40, 'timing_diagnostic': True,
            'source_inputs': {'firmware/src/audio_clock_console.h': hashlib.sha256(
                (Path(__file__).resolve().parents[1] / 'firmware/src/audio_clock_console.h').read_bytes()).hexdigest()},
            'candidates': {'diagnostic': {
            'package': name, 'bytes': 15, 'sha256': hashlib.sha256(b'offline fixture').hexdigest()}}}))
        arguments = ['clock', '--manifest', str(manifest), '--version', VERSION]
        with patch.object(sys, 'argv', arguments), patch.object(capture, 'MacCdc') as native:
            native.side_effect = AssertionError('Plan opened a native port')
            with contextlib.redirect_stdout(io.StringIO()) as output:
                assert capture.main() == 0
            assert json.loads(output.getvalue())['mode'].startswith('plan-only')
            native.assert_not_called()
        with patch.object(sys, 'argv', [*arguments, '--capture', '--port', '/dev/cu.usbmodemFake',
                                       '--output', str(root / 'attempt')]), \
             patch.object(capture, 'MacCdc') as native, patch.object(capture, 'collect_clock') as collect:
            collect.side_effect = ClockRefused('primary capture failure')
            native.return_value.close.side_effect = OSError('cleanup failure')
            with contextlib.redirect_stdout(io.StringIO()):
                assert capture.main() == 1
            receipt = json.loads((root / 'attempt/receipt.json').read_text())
            assert receipt['failure'] == 'primary capture failure'
            assert receipt['close_failure'] == 'cleanup failure'
            assert not receipt['accepted'] and not receipt['hardware_qualified']
            assert native.call_count == 1 and native.return_value.close.call_count == 1
        metadata = json.loads(manifest.read_text())
        del metadata['source_inputs']
        manifest.write_text(json.dumps(metadata))
        with patch.object(sys, 'argv', [*arguments, '--capture', '--port', '/dev/cu.usbmodemFake',
                                       '--output', str(root / 'older')]), patch.object(capture, 'MacCdc') as native:
            with contextlib.redirect_stderr(io.StringIO()):
                try:
                    capture.main()
                except SystemExit as error:
                    assert error.code == 2
                else:
                    raise AssertionError('Older package admitted')
            native.assert_not_called()
            assert not (root / 'older').exists()


def test_health_failure_refuses_acceptance() -> None:
    points = observations(24_000_000)
    for when in (1, 2):
        console = ScriptedConsole(tuple(p.packet for p in points))
        original = console.request

        def failing_status(command: str) -> str:
            text = original(command)
            if command == 'status' and console.statuses == when:
                return text.replace('audio_late 0', 'audio_late 1')
            return text

        times = iter(n for p in points for n in (p.sent_ns, p.received_ns))
        with patch.object(console, 'request', failing_status):
            rejects(lambda: collect_clock(console, ClockRequest(VERSION),
                    ClockSource(lambda: next(times), lambda n: None)))
        assert len(console.commands) == (1 if when == 1 else 5)


def test_boot_audio_history_and_failures() -> None:
    # Given the new producer protocol, retained events precede this healthy boot.
    version = 'v0.2.5-timing'
    history = 'audio_late 2\r\naudio_late_entry 2\r\naudio_late_boot 0'
    cases = ((history, history, 0),
             (history.replace('audio_late_boot 0', 'audio_late_boot 1'), history, 1),
             (history.replace('audio_late 2', 'audio_late 3').replace('audio_late_boot 0', 'audio_late_boot 1'), history, 1),
             ('audio_late 0', history, 1),
             (history + '\r\naudio_late_boot 0', history, 1),
             (history, history.replace('audio_late 2', 'audio_late 3').replace('audio_late_boot 0', 'audio_late_boot 1'), 5),
             (history, history.replace(' 2', ' 3'), 5))
    for first, last, refusal_commands in cases:
        points = observations(24_000_000)
        console = ScriptedConsole(tuple(p.packet for p in points), version=version)
        original = console.request

        def counter_status(command: str) -> str:
            text = original(command)
            if command == 'status':
                return text.replace('audio_late 0', first if console.statuses == 1 else last)
            return text

        times = iter(n for p in points for n in (p.sent_ns, p.received_ns))
        # When the actual collector runs against only injected wire responses.
        with patch.object(console, 'request', counter_status):
            operation = lambda: collect_clock(console, ClockRequest(version),
                ClockSource(lambda: next(times), lambda _: None))
            if refusal_commands:
                rejects(operation)
            else:
                result = operation()
                assert result.bounds.lower_hz <= 24_000_000 <= result.bounds.upper_hz
        # Then a failed admission sends nothing further, and no retries occur.
        assert len(console.commands) == (refusal_commands or 5)


if __name__ == '__main__':
    for check in (test_rate_bounds, test_invalid_observations, test_wire_parser,
                  test_collection_and_failure_stop, test_cli_access_and_cleanup,
                  test_health_failure_refuses_acceptance, test_boot_audio_history_and_failures):
        check()
        print(check.__name__, 'PASS')
