# SPDX-License-Identifier: GPL-3.0-only
"""Scripted console transport only; never open MIDI, USB or a tty."""
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock
import contextlib
import io
import zlib
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import dabbl8_timing_capture as capture

VERSION = 'v0.1.0-timing'
BANNER = b"\r\nDabbl8 v0.1.0-timing console - 'help'\r\n> "
STATUS = 'dabbl8 v0.1.0-timing\r\nboots 17\r\nusb_resets 2\r\nuac_hz 44100\r\nuac_alt 1\r\nuac_rate_sets 1\r\nuac_starts 1\r\n'


def frame(uac_enabled=True):
    # Independent literal words and CRC; plausible data are not hardware evidence.
    def hist(values):
        bins = [0] * 128
        for value in values:
            bins[value >> 10] += 1
        return [len(values), max(values), *bins]
    begin, config, state = [1, 17], [8, 128, 44100, 24, 10, 128], [1, 3, 0, 0]
    window = [100, 200100, 200000, 0, 17, 0, 0, 0, 0, 0, 0, 0, int(uac_enabled)]
    render, inclusive = hist([1000, 1200, 1400]), hist([1200, 1400, 1800])
    words = [0x44385432, *begin, *config, *state, *window, 0x52, *render, 0x49, *inclusive]
    end = [*begin, zlib.crc32(struct.pack('<' + 'I' * len(words), *words))]
    return ''.join('d8t2 ' + label + ' ' + ' '.join(f'{x:08X}' for x in values) + '\r\n'
                   for label, values in zip(['begin', 'config', 'state', 'window', 'render', 'inclusive', 'end'], [begin, config, state, window, render, inclusive, end]))


class Scripted:
    def __init__(self, changes=None, fragment=4096):
        self.replies = {'status': STATUS, 'timing start': 'timing started\r\n',
                        'timing stop': 'timing stopped\r\n', 'timing report': frame()}
        self.replies.update(changes or {})
        self.queue = bytearray(BANNER); self.writes = []; self.fragment = fragment; self.status_count = 0

    def read(self, timeout):
        if not self.queue:
            raise OSError('scripted disconnect')
        data = self.queue[:self.fragment]; del self.queue[:self.fragment]; return bytes(data)

    def write(self, data):
        command = data.decode().rstrip('\r'); self.writes.append(command)
        if command == 'status':
            self.status_count += 1
        reply = self.replies[command]
        if isinstance(reply, list):
            reply = reply[min(self.status_count - 1, len(reply) - 1)]
        self.queue.extend((command + '\r\n' + reply + '\r\n> ').encode())


class CaptureTests(unittest.TestCase):
    def test_uac_disabled_profile_captures_without_uac_status_fields(self):
        minimal = 'dabbl8 ' + VERSION + '\r\nboots 17\r\nusb_resets 2\r\nfl_state 2\r\n'
        t = Scripted({'status': minimal, 'timing report': frame(False)})
        with tempfile.TemporaryDirectory() as d:
            report = capture.collect(capture.Console(t, lambda *a: None), Path(d), VERSION,
                                     10, sleep=lambda n: None, uac_enabled=False)
            self.assertFalse(report['window']['uac_enabled'])
            self.assertEqual(report['host_observation']['before']['uac_enabled'], 0)
            self.assertFalse(report['hardware_qualified'])
            self.assertEqual(t.writes, ['status', 'timing start', 'timing stop', 'timing report', 'status'])

    def test_uac_status_absence_never_implicitly_selects_disabled_profile(self):
        for disabled, reply in [(False, 'dabbl8 ' + VERSION + '\r\nboots 17\r\nusb_resets 2\r\n'),
                                (True, STATUS),
                                (True, 'dabbl8 ' + VERSION + '\r\nusb_resets 2\r\n')]:
            t = Scripted({'status': reply})
            with tempfile.TemporaryDirectory() as d:
                with self.assertRaises(ValueError):
                    capture.collect(capture.Console(t, lambda *a: None), Path(d), VERSION,
                                    10, sleep=lambda n: None, uac_enabled=not disabled)
            self.assertEqual(t.writes, ['status'])

    def test_disabled_profile_refuses_enabled_report_and_keeps_raw_frame(self):
        t = Scripted({'status': 'dabbl8 ' + VERSION + '\r\nboots 17\r\nusb_resets 2\r\n'})
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaises(ValueError):
                capture.collect(capture.Console(t, lambda *a: None), Path(d), VERSION,
                                10, sleep=lambda n: None, uac_enabled=False)
            self.assertTrue((Path(d) / 'report.frame').exists())
            self.assertFalse((Path(d) / 'report.json').exists())

    def run_script(self, transport):
        with tempfile.TemporaryDirectory() as directory:
            events = []
            con = capture.Console(transport, lambda *args: events.append(args))
            result = capture.collect(con, Path(directory), VERSION, 10, sleep=lambda n: None)
            self.assertFalse(result['hardware_qualified'])
            self.assertFalse(result['host_observation']['device_binary_identity_attested'])
            self.assertEqual(result['samples'], 3)
            self.assertTrue((Path(directory) / 'report.frame').exists())
            self.assertTrue(events)

    def test_fragmented_valid_frame(self):
        for size in (1, 7, 4096):
            with self.subTest(size=size):
                t = Scripted(fragment=size); self.run_script(t)
                self.assertEqual(t.writes, ['status', 'timing start', 'timing stop', 'timing report', 'status'])

    def test_non_allowlisted_commands_never_sent(self):
        t = Scripted(); con = capture.Console(t, lambda *a: None)
        for command in ('uboot yes', 'flr 0x93000', 'memr 0', 'timing start\ruboot yes', 'timing report junk'):
            with self.assertRaises(ValueError): con.request(command)
        self.assertEqual(t.writes, [])

    def test_wrong_banner_no_commands(self):
        t = Scripted(); t.queue = bytearray(BANNER.replace(b'Dabbl8', b'Felucca'))
        with self.assertRaises(ValueError): self.run_script(t)
        self.assertEqual(t.writes, [])

    def test_start_refusal_does_not_stop_or_reset_existing_run(self):
        t = Scripted({'timing start': 'timing start refused\r\n'})
        with self.assertRaises(ValueError): self.run_script(t)
        self.assertEqual(t.writes, ['status', 'timing start'])

    def test_crc_and_truncation_refusal_preserves_raw_report(self):
        for invalid in (frame().replace('00000011', '00000012', 1), frame().split('d8t2 end')[0]):
            t = Scripted({'timing report': invalid})
            with tempfile.TemporaryDirectory() as d:
                with self.assertRaises(ValueError): capture.collect(capture.Console(t, lambda *a: None), Path(d), VERSION, 10, sleep=lambda n: None)
                self.assertTrue((Path(d) / 'report.frame').exists())
                self.assertFalse((Path(d) / 'report.json').exists())

    def test_boot_reset_or_usb_change_refused(self):
        for field, old, new in [('boots', '17', '18'), ('usb_resets', '2', '3'), ('uac_hz', '44100', '48000'), ('uac_rate_sets', '1', '2')]:
            with self.subTest(field=field):
                t = Scripted({'status': [STATUS, STATUS.replace(field + ' ' + old, field + ' ' + new)]})
                with self.assertRaises(ValueError): self.run_script(t)

    def test_bad_status_identity_prevents_start(self):
        t = Scripted({'status': STATUS.replace('dabbl8', 'felucca')})
        with self.assertRaises(ValueError): self.run_script(t)
        self.assertEqual(t.writes, ['status'])

    def test_stop_ack_failure_attempts_only_owned_stop(self):
        t = Scripted({'timing stop': 'timing not running\r\n'})
        with self.assertRaises(ValueError): self.run_script(t)
        self.assertEqual(t.writes, ['status', 'timing start', 'timing stop', 'timing stop'])

    def test_transport_bounds_and_disconnect(self):
        for raw in (b'a' * 65537, b'\xff\r\n> ', BANNER + BANNER, b''):
            t = Scripted(); t.queue = bytearray(raw)
            with self.assertRaises((ValueError, OSError)): capture.Console(t, lambda *a: None).receive()

    def test_timeout_has_no_retry(self):
        class Silent:
            def read(self, timeout): return b''
        tick = iter(range(100))
        with self.assertRaises(TimeoutError): capture.Console(Silent(), lambda *a: None, clock=lambda: next(tick)).receive()

    def test_stale_reply_echo_refused(self):
        t = Scripted(); con = capture.Console(t, lambda *a: None)
        con.receive(); original = t.write
        def stale(data):
            original(data); t.queue = t.queue.replace(b'status\r\n', b'timing report\r\n', 1)
        t.write = stale
        with self.assertRaises(ValueError): con.request('status')

    def test_native_write_guard_rejects_before_os_write(self):
        native = object.__new__(capture.MacCdc)
        with mock.patch.object(capture.os, 'write') as write:
            with self.assertRaises(ValueError): native.write(b'uboot yes\r')
            write.assert_not_called()

    def test_plan_mode_never_opens_port(self):
        with tempfile.TemporaryDirectory() as d:
            f = Path(d)/'manifest.json'
            with mock.patch.object(capture, 'package_identity', return_value={}), mock.patch.object(capture, 'MacCdc') as device, mock.patch.object(sys, 'argv', ['capture', '--manifest', str(f), '--workload', 'plan']), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(capture.main(), 0)
                device.assert_not_called()

    def test_candidate_integrity_before_device_access(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d); name = 'dabbl8-0.1.0-timing-diagnostic.fwsc'; (p/name).write_bytes(b'fixture')
            m = {'source_commit': 'a'*40, 'candidates': {'diagnostic': {'package': name, 'sha256': hashlib.sha256(b'fixture').hexdigest(), 'bytes': 7}}}
            f = p/'manifest.json'; f.write_text(json.dumps(m)); self.assertFalse(capture.package_identity(f)['device_binary_identity_attested'])
            (p/name).write_bytes(b'changed')
            with self.assertRaises(ValueError): capture.package_identity(f)
            m['candidates']['diagnostic']['package'] = '../private'; f.write_text(json.dumps(m))
            with self.assertRaises(ValueError): capture.package_identity(f)


if __name__ == '__main__': unittest.main()
