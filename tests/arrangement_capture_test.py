# SPDX-License-Identifier: GPL-3.0-only
"""Actual console framing with synthetic reports; no native ports or real wait."""
from dataclasses import astuple
import contextlib
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import dabbl8_arrangement_capture as capture
from dabbl8_arrangement_state import ArrangementState
from dabbl8_timing_capture import ARRANGEMENT_COMMANDS, Console

VERSION = 'v0.2.3-timing'


def wire(label, rows, names, magic):
    words = magic(rows)
    crc = zlib.crc32(struct.pack('<' + 'I' * len(words), *words))
    end = [*rows[0], crc]
    return ''.join(label + ' ' + name + ' ' + ' '.join(f'{v:08X}' for v in row) + '\r\n'
                   for name, row in zip(names, [*rows, end]))


def resource_frame():
    peaks = list(capture.PEAKS)
    rows = [[1, 17], [8, 0, 0x3ffd, 3, 0], [1,0,8,8,0,0,9,8,8,8,7120,56960],
            peaks, peaks, [1,0x1c74100,0x1c7a000,24320,24280,40],
            [1,0x1c7a100,0x1c7c000,7936,7920,16]]
    return wire('d8r1', rows, ['begin','config','pool','last','peak','user','supervisor','end'],
        lambda r: [0x44385231,*r[0],*r[1],*r[2],0x4c,*r[3],0x50,*r[4],0x55,*r[5],0x53,*r[6]])


def timing_frame():
    count, duration = 12000, (12000 - 1) * 69632 + 1000
    hist = [count,1000,count,*([0]*127)]
    rows = [[1,17], [8,128,44100,24,10,128], [1,count,0,0],
            [100,100+duration,duration,0,17,0,0,0,0,0,0,0,0], hist, hist]
    return wire('d8t2', rows, ['begin','config','state','window','render','inclusive','end'],
        lambda r: [0x44385432,*r[0],*r[1],*r[2],*r[3],0x52,*r[4],0x49,*r[5]])


class FakeClock:
    def __init__(self):
        self.seconds = 0.
    def now(self):
        return self.seconds
    def wait(self, seconds):
        self.seconds += seconds


class Scripted:
    def __init__(self, changes=None, failure=None, close_failure=False, fragment=4096):
        self.clock = FakeClock()
        self.changes = changes or {}
        self.failure, self.close_failure, self.fragment = failure, close_failure, fragment
        self.queue = bytearray(f"\r\nDabbl8 {VERSION} console - 'help'\r\n> ".encode())
        self.writes, self.closed, self.resource, self.generation = [], 0, 0, 0
        self.ready_at = None

    def ready(self):
        self.ready_at = self.clock.now()

    def snapshot(self):
        elapsed = -1 if self.ready_at is None else self.clock.now() - self.ready_at - 1
        active, stopped = 0 <= elapsed < 32, elapsed >= 32
        row = min(15, int(elapsed // 2)) if elapsed >= 0 else 255
        return ArrangementState(int(self.clock.now()*24e6),17,2,
            124 if stopped else 123 if active else 0, 1 if elapsed >= 0 else 0,
            int(active),0,int(active),row,row,16 if elapsed >= 0 else 0,
            int(elapsed >= 0),int(stopped),255,255,0,0,0,0)

    def status(self):
        state = self.snapshot()
        words = astuple(state)
        crc = zlib.crc32(struct.pack('<20I', 0x44384131, *words))
        frame = 'd8a1 ' + ' '.join(f'{v:08X}' for v in (*words,crc)) + '\r\n'
        return (f'dabbl8 {VERSION}\r\nboots 17\r\nusb_resets 2\r\n'
            'audio_late 0\r\nvoices_shed 0\r\nflash 1\r\nfl_failure 0\r\nbpm 120\r\n'
            'resource_schema 1\r\nresource_helpers 3\r\nresource_tracks 8\r\n'
            'resource_profile 0\r\nresource_uac 0\r\nresource_engine_mask 0x00003FFD\r\n'
            f'resource_running {self.resource}\r\nresource_generation 0x{self.generation:08X}\r\n' + frame)

    def read(self, _timeout):
        if not self.queue:
            raise OSError('scripted disconnect')
        reply = self.queue[:self.fragment]
        del self.queue[:self.fragment]
        return bytes(reply)

    def write(self, data):
        command = data.decode('ascii').rstrip('\r')
        if command not in ARRANGEMENT_COMMANDS:
            raise AssertionError('Non-diagnostic send')
        self.writes.append(command)
        if command == self.failure:
            raise OSError('lost reply: ' + command)
        if command == 'resource start':
            self.resource, self.generation = 1, 1
        if command == 'resource stop':
            self.resource = 0
        values = {'status': self.status(), 'timing start': 'timing started\r\n',
            'timing stop': 'timing stopped\r\n', 'timing report': timing_frame(),
            'resource start': 'resource started 00000001 00000011\r\n',
            'resource stop': 'resource stopped 00000001 00000011\r\n', 'resource report': resource_frame()}
        reply = self.changes.get(command, values[command])
        if callable(reply):
            reply = reply(values[command])
        self.queue.extend((command + '\r\n' + reply + '> ').encode('ascii'))

    def close(self):
        self.closed += 1
        if self.close_failure:
            raise OSError('close restoration failed')


class CaptureTests(unittest.TestCase):
    def run_capture(self, transport):
        receipt = capture.Receipt(VERSION)
        result = capture.run_transport(lambda: transport, receipt,
            clock=capture.Clock(transport.clock.now, transport.clock.wait), ready=transport.ready)
        self.assertEqual(transport.closed, 1)
        return result, receipt

    def test_whole_natural_run_both_observers_closed_before_readback(self):
        for fragment in (1, 7, 4096):
            with self.subTest(fragment=fragment):
                transport = Scripted(fragment=fragment)
                result, receipt = self.run_capture(transport)
                self.assertIsNotNone(result)
                self.assertTrue(receipt.accepted and receipt.snapshot_may_be_attempted)
                self.assertEqual((receipt.rows_seen, receipt.timing, receipt.resource),
                                 (65535, capture.Lease.PAUSED, capture.Lease.PAUSED))
                self.assertFalse(receipt.hardware_qualified or receipt.device_write_commands_sent)
                self.assertEqual(transport.writes[-5:], ['resource stop','timing stop','resource report','timing report','status'])

    def test_bad_banner_and_unsafe_initial_status_send_no_starts(self):
        for transport in (Scripted({'status': lambda s:s.replace('flash 1','flash 0')}), Scripted()):
            if not transport.changes:
                transport.queue = bytearray(b'Wrong banner\r\n> ')
            _, receipt = self.run_capture(transport)
            self.assertFalse(receipt.accepted)
            self.assertFalse(any('start' in c or 'stop' in c for c in transport.writes))

    def test_refused_start_preserves_existing_run(self):
        for kind in ('timing', 'resource'):
            transport = Scripted({kind+' start': kind+' start refused\r\n'})
            _, receipt = self.run_capture(transport)
            self.assertFalse(receipt.accepted)
            self.assertNotIn(kind+' stop', transport.writes)
            if kind == 'resource':
                self.assertEqual(transport.writes.count('timing stop'), 1)

    def test_unconfirmed_start_never_pauses_unknown_run(self):
        for kind in ('timing', 'resource'):
            transport = Scripted(failure=kind+' start')
            _, receipt = self.run_capture(transport)
            self.assertFalse(receipt.accepted)
            self.assertNotIn(kind+' stop', transport.writes)
            self.assertEqual(getattr(receipt,kind), capture.Lease.UNKNOWN_START)

    def test_lost_stop_ack_never_retries_and_stops_remaining_owned_run(self):
        for kind in ('timing', 'resource'):
            transport = Scripted(failure=kind+' stop')
            _, receipt = self.run_capture(transport)
            self.assertFalse(receipt.accepted)
            self.assertEqual(transport.writes.count(kind+' stop'), 1)
            self.assertEqual(getattr(receipt,kind), capture.Lease.UNKNOWN_STOP)
            self.assertFalse(any(c.endswith('report') for c in transport.writes))

    def test_primary_failure_and_cleanup_failure_both_survive(self):
        transport = Scripted(failure='resource stop')
        def failed_ready():
            raise RuntimeError('owner action cancelled')
        transport.ready = failed_ready
        _, receipt = self.run_capture(transport)
        self.assertIn('owner action cancelled', receipt.failure)
        self.assertTrue(any('lost reply' in s for s in receipt.cleanup_failures))
        self.assertEqual(transport.writes.count('timing stop'), 1)

    def test_close_failure_refuses_readback_even_after_valid_observation(self):
        _, receipt = self.run_capture(Scripted(close_failure=True))
        self.assertFalse(receipt.accepted or receipt.snapshot_may_be_attempted)
        self.assertIn('close restoration failed', receipt.close_failure)

    def test_running_health_failure_pauses_only_owned_diagnostics_and_preserves_close_error(self):
        transport = Scripted(close_failure=True)
        transport.changes['status'] = lambda s:s.replace('audio_late 0','audio_late 1') if transport.ready_at is not None else s
        _, receipt = self.run_capture(transport)
        self.assertIn('audio_late', receipt.failure)
        self.assertIn('close restoration failed', receipt.close_failure)
        self.assertEqual(transport.writes.count('resource stop'), 1)
        self.assertEqual(transport.writes.count('timing stop'), 1)
        self.assertFalse(any(c.endswith('report') for c in transport.writes))
        self.assertFalse(receipt.accepted or receipt.snapshot_may_be_attempted)

    def test_malformed_reports_stop_before_next_report(self):
        for kind in ('resource', 'timing'):
            transport = Scripted({kind+' report': 'broken\r\n'})
            _, receipt = self.run_capture(transport)
            self.assertFalse(receipt.accepted)
            if kind == 'resource':
                self.assertNotIn('timing report', transport.writes)

    def test_no_owner_play_times_out_and_pauses_owned_diagnostics(self):
        transport = Scripted()
        transport.ready = lambda: None
        _, receipt = self.run_capture(transport)
        self.assertFalse(receipt.accepted)
        self.assertEqual(transport.writes.count('resource stop'), 1)
        self.assertEqual(transport.writes.count('timing stop'), 1)
        self.assertLessEqual(transport.clock.now(), 61)

    def test_reused_receipt_refused_before_transport_acquisition(self):
        receipt = capture.Receipt(VERSION, timing=capture.Lease.UNKNOWN_START)
        def forbidden():
            raise AssertionError('Port factory reached')
        capture.run_transport(forbidden, receipt)
        self.assertIn('fresh, unowned', receipt.failure)

    def test_fixed_profile_rejects_all_non_diagnostic_commands(self):
        transport = Scripted()
        console = Console(transport, lambda *_:None, commands=ARRANGEMENT_COMMANDS)
        for command in ('uboot yes','memr 0','flr 0','save','play','timing start\ruboot yes','resource report junk'):
            with self.assertRaises(ValueError):
                console.request(command)
        self.assertEqual(transport.writes, [])

    def test_CLI_plan_and_refusals_never_acquire_a_port(self):
        # Given a real candidate manifest/package, the exact project gate is
        # separately exercised below; only its large generated input is stubbed.
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            name = 'dabbl8-0.2.3-timing-diagnostic.fwsc'
            (root/name).write_bytes(b'fixture')
            flags = ['-D'+key+'='+value for key,value in {
                'D8_HEAVY_POOL_DIAGNOSTIC':'1','D8_STACK_WATERMARK_DIAGNOSTIC':'1',
                'DABBL8_IDENTITY':'1','DABBL8_TIMING_DIAGNOSTIC':'1','NPART':'8',
                'FELUCCA_UAC':'0'}.items()]
            manifest = {'source_commit':'a'*40,'resource_diagnostic':True,'resource_report_schema':'D8R1',
                'resource_collection_initially_enabled':False,'resource_engine_profile':0,
                'resource_engine_mask':0x3ffd,'resource_uac_enabled':False,'engine_profile':'full',
                'compiler_flags':flags,'startup_flags':['-DD8_STACK_WATERMARK_DIAGNOSTIC=1'],
                'source_inputs':{'firmware/src/arrangement_diagnostic_console.h':capture.HEADER_SHA},
                'candidates':{'diagnostic':{'package':name,'sha256':hashlib.sha256(b'fixture').hexdigest(),'bytes':7}}}
            path = root/'manifest.json'
            path.write_text(json.dumps(manifest))
            base = ['capture','--manifest',str(path),'--project',str(root/'project.d8p')]
            existing = root/'existing'
            existing.mkdir()
            cases = [([], 0), (['--capture'], 2),
                (['--capture','--owner-ready','--port','/dev/tty.other','--output',str(root/'out')],2),
                (['--capture','--owner-ready','--port','/dev/cu.usbmodemfixture','--output',str(existing)],FileExistsError)]
            with mock.patch.object(capture,'MacCdc',side_effect=AssertionError('Native port reached')) as native, \
                 mock.patch.object(capture,'admit_project'):
                for extra, expected in cases:
                    with self.subTest(extra=extra), mock.patch.object(sys,'argv',base+extra), \
                         contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                        if expected == 0:
                            self.assertEqual(capture.main(),0)
                        elif expected == 2:
                            with self.assertRaises(SystemExit) as error:
                                capture.main()
                            self.assertEqual(error.exception.code,2)
                        else:
                            with self.assertRaises(expected):
                                capture.main()
                manifest['source_inputs'] = {}
                path.write_text(json.dumps(manifest))
                with mock.patch.object(sys,'argv',base+['--capture','--owner-ready']), \
                     contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
                    capture.main()
                native.assert_not_called()

    def test_project_admission_refuses_wrong_size_hash_and_symlink(self):
        # Given only invalid local inputs, admission fails without native access.
        with tempfile.TemporaryDirectory() as d:
            path = Path(d)/'proposal.d8p'
            for data in (b'', b'x'*7385, b'x'*7386):
                path.write_bytes(data)
                with self.assertRaises(ValueError):
                    capture.admit_project(path)
            link = Path(d)/'link.d8p'
            link.symlink_to(path)
            with self.assertRaises(ValueError):
                capture.admit_project(link)


if __name__ == '__main__':
    unittest.main()
