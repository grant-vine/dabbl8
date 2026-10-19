# SPDX-License-Identifier: GPL-3.0-only
"""Stdlib local tests. D8_RESOURCE_CONSOLE_BIN must name the compiled C fixture."""
from __future__ import annotations
from dataclasses import asdict
import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
import zlib
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import dabbl8_resource_capture as capture
from dabbl8_resource_report import Expected, parse_report

VERSION = 'v0.1.0-timing'
BANNER = b"\r\nDabbl8 v0.1.0-timing console - 'help'\r\n> "
EXPECTED = Expected(0, 0x3ffd, False)


def rows(idle=False, profile=0, uac=False):
    vectors = [0] * 14; vectors[12] = 0 if idle else 8
    count = 0 if idle else 10
    return [[1, 17], [8, profile, (0x3ffd, 0x1409, 0x3411)[profile], 3, int(uac)],
            [1, 0, count, 0 if idle else 8, 0 if idle else 1, 0 if idle else 1, count+1,
             sum(vectors), sum(vectors), 8, 7120, 56960], vectors[:], vectors[:],
            [1, 0x01c74100, 0x01c7a000, 24320, 24280, 40],
            [1, 0x01c7a100, 0x01c7c000, 7936, 7920, 16]]


def frame(values=None, **kwargs):
    values = values if values is not None else rows(**kwargs)
    words = [0x44385231, *values[0], *values[1], *values[2], 0x4c, *values[3],
             0x50, *values[4], 0x55, *values[5], 0x53, *values[6]]
    crc = zlib.crc32(struct.pack('<' + 'I' * len(words), *words))
    values = [*values, [*values[0], crc]]
    return ''.join('d8r1 ' + name + ' ' + ' '.join(f'{v:08X}' for v in row) + '\r\n'
        for name, row in zip(('begin', 'config', 'pool', 'last', 'peak', 'user', 'supervisor', 'end'), values))


def status(generation=0, *, running=0, uac=False):
    baseline = 'dabbl8 '+VERSION+'\r\nboots 17\r\nusb_resets 2\r\n'
    if uac: baseline += 'uac_hz 44100\r\nuac_alt 1\r\nuac_rate_sets 1\r\nuac_starts 1\r\n'
    return baseline + 'resource_schema 1\r\nresource_helpers 3\r\nresource_tracks 8\r\nresource_profile 0\r\nresource_engine_mask 0x00003FFD\r\nresource_uac '+str(int(uac))+'\r\nresource_running '+str(running)+'\r\nresource_generation '+f'0x{generation:08X}\r\n'


class Scripted:
    def __init__(self, changes=None, fragment=4096):
        self.replies = {'status': [status(), status(1)], 'resource start': 'resource started 00000001 00000011\r\n',
                        'resource stop': 'resource stopped 00000001 00000011\r\n', 'resource report': frame()}
        self.replies.update(changes or {}); self.queue = bytearray(BANNER)
        self.writes = []; self.fragment = fragment; self.status_count = 0; self.closed = False
    def read(self, timeout):
        if not self.queue: raise OSError('simulated disconnected/lost acknowledgement')
        data = self.queue[:self.fragment]; del self.queue[:self.fragment]; return bytes(data)
    def write(self, data):
        command = data.decode().rstrip('\r'); self.writes.append(command)
        if command == 'status': self.status_count += 1
        reply = self.replies[command]
        if isinstance(reply, list): reply = reply[min(self.status_count-1, len(reply)-1)]
        if isinstance(reply, Exception): raise reply
        separator = '' if command == 'resource report' and reply.endswith('\r\n') else '\r\n'
        self.queue.extend((command+'\r\n'+reply+separator+'> ').encode())
    def close(self): self.closed = True


class ResourceTests(unittest.TestCase):
    def test_new_protocol_observation_retains_history_and_refuses_new_events(self):
        # Given resource capabilities and a v025 counter tuple.
        version = 'v0.2.5-timing'
        history = 'audio_late 2\r\naudio_late_entry 2\r\naudio_late_boot 0\r\n'
        reply = status().replace(VERSION, version) + history
        # When parsing observations of unchanged versus replaced history.
        first = capture.observation(reply, version, EXPECTED)
        same = capture.observation(reply, version, EXPECTED)
        changed = capture.observation(reply.replace('audio_late 2', 'audio_late 3')
            .replace('audio_late_entry 2', 'audio_late_entry 3'), version, EXPECTED)
        # Then stable identity includes the retained baseline, not only boot/USB.
        self.assertEqual(first.stable, same.stable)
        self.assertNotEqual(first.stable, changed.stable)
        with self.assertRaises(ValueError):
            capture.observation(reply.replace('audio_late 2', 'audio_late 3')
                .replace('audio_late_boot 0', 'audio_late_boot 1'), version, EXPECTED)

    def setUp(self):
        parent = os.environ.get('D8_RESOURCE_CAPTURE_EVIDENCE_OUT')
        if parent:
            Path(parent).mkdir(parents=True, exist_ok=True)
        self.output = Path(tempfile.mkdtemp(prefix=self._testMethodName+'-', dir=parent))
        self.identity = capture.Candidate('a'*40, 'b'*64, 'dabbl8-0.1.0-timing-diagnostic.fwsc', 'c'*64, EXPECTED, VERSION)
    def run_capture(self, transport, *, fail=None, sleep=lambda _: None, identity=None):
        receipt = capture.Receipt(identity or self.identity, 'simulated unit workload', VERSION, 'simulated', 10)
        with (self.output/'journal.jsonl').open('x') as log:
            def journal(direction, data):
                log.write(json.dumps({'direction': direction, 'hex': data.hex()})+'\n'); log.flush()
            console = capture.Console(transport, journal, commands=capture.RESOURCE_COMMANDS)
            try:
                result = capture.collect(console, self.output, receipt, sleep=sleep)
                receipt.accepted = True
            except Exception as error:
                receipt.failure = str(error)
                if fail is None: raise
                self.assertIsInstance(error, fail); result = None
            finally:
                (self.output/'receipt.json').write_text(json.dumps(asdict(receipt), indent=2)+'\n')
        return result, receipt
    def manifest(self):
        name = 'dabbl8-0.1.0-timing-diagnostic.fwsc'; data = b'local fixture not firmware'
        (self.output/name).write_bytes(data)
        value = {'source_commit': 'a'*40, 'candidates': {'diagnostic': {'package': name,
            'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data)}},
            'resource_diagnostic': True, 'resource_report_schema': 'D8R1',
            'resource_collection_initially_enabled': False, 'resource_engine_profile': 0,
            'resource_engine_mask': 0x3ffd, 'resource_uac_enabled': False, 'engine_profile': 'full',
            'compiler_flags': ['-DD8_HEAVY_POOL_DIAGNOSTIC=1', '-DD8_STACK_WATERMARK_DIAGNOSTIC=1',
                '-DDABBL8_IDENTITY=1', '-DDABBL8_TIMING_DIAGNOSTIC=1', '-DNPART=8', '-DFELUCCA_UAC=0'],
            'startup_flags': ['-DD8_STACK_WATERMARK_DIAGNOSTIC=1']}
        path = self.output/'manifest.json'; path.write_text(json.dumps(value)); return path, value
    def test_fragmented_valid_capture(self):
        for size in (1, 7, 4096):
            with self.subTest(size=size):
                output = self.output; self.output = output/str(size); self.output.mkdir()
                t = Scripted(fragment=size); result, receipt = self.run_capture(t)
                self.assertTrue(receipt.accepted); self.assertFalse(result.hardware_qualified)
                self.assertEqual(result.pool.peak_live_owned_slots, 8)
                self.assertEqual(t.writes, list(capture.RESOURCE_COMMANDS)+['status'])
                self.assertGreater((self.output/'report.response').stat().st_size, 0); self.output = output
    def test_collect_rejects_original_malformed_row_endings(self):
        good=frame(); parent=self.output; cases={'lf-only':good.replace('\r\n','\n'),
            'mixed':good.replace('\r\n','\n',1), 'footer-lf':good[:-2]+'\n',
            'cr-only':good.replace('\r\n','\r'),
            'vertical-tab':good.replace('\r\n','\v',1),
            'extra-blank-row':good.replace('\r\n','\r\n\r\n',1),
            'unknown-row':good.replace('\r\n','\r\nforeign error row\r\n',1)}
        for name, malformed in cases.items():
            with self.subTest(ending=name):
                self.output=parent/name; self.output.mkdir()
                t=Scripted({'resource report':malformed},fragment=7)
                _, receipt=self.run_capture(t,fail=ValueError)
                self.assertFalse(receipt.accepted)
                response=(self.output/'report.response').read_bytes()
                self.assertEqual((self.output/'report.frame').read_bytes(),response[len(b'resource report\r\n'):-2])
                self.assertIn(malformed.encode('ascii'),response)
                self.assertEqual(t.writes,list(capture.RESOURCE_COMMANDS))
                self.assertFalse((self.output/'report.json').exists())
    def test_missing_footer_crlf_preserves_unclosed_transport_bytes(self):
        payload=frame()[:-2].encode('ascii')+b'> '
        t=Scripted(fragment=7); original=t.write
        def unclosed(data):
            if data==b'resource report\r':
                t.writes.append('resource report');t.queue.extend(b'resource report\r\n'+payload)
            else: original(data)
        t.write=unclosed
        _, receipt=self.run_capture(t,fail=OSError)
        self.assertFalse(receipt.accepted)
        self.assertEqual((self.output/'report.frame').read_bytes(),payload)
        self.assertEqual((self.output/'report.response').read_bytes(),b'resource report\r\n'+payload)
        self.assertEqual(t.writes,list(capture.RESOURCE_COMMANDS))
    def test_idle_initial_census_is_valid_limited_observation(self):
        report, _ = self.run_capture(Scripted({'resource report': frame(idle=True)}))
        self.assertEqual((report.pool.requests, report.pool.censuses), (0, 1))
    def test_actual_c_console_frame_interoperates(self):
        binary = os.environ.get('D8_RESOURCE_CONSOLE_BIN')
        self.assertTrue(binary, 'D8_RESOURCE_CONSOLE_BIN required; compile actual fixture first')
        proc = subprocess.run([binary, '--frame'], check=True, capture_output=True)
        (self.output/'actual-console.frame').write_bytes(proc.stdout)
        report, _ = self.run_capture(Scripted({'resource report': proc.stdout.decode('ascii')}))
        self.assertEqual((report.pool.requests, report.pool.failures), (10, 1))
        self.assertEqual((self.output/'report.frame').read_bytes(),proc.stdout)
    def test_profiles_and_stack_scopes(self):
        for profile in range(3):
            report = parse_report(frame(profile=profile), Expected(profile, (0x3ffd,0x1409,0x3411)[profile], False), 1, 17)
            self.assertEqual(report.user.observed_written_bytes_since_boot, 40)
        value = rows(); value[3][12] = 1; value[2][7:9] = [1, 1]; value[4][12] = 1; value[4][10] = 1
        self.assertEqual(sum(parse_report(frame(value), EXPECTED, 1, 17).independent_peak_engine), 2)
    def test_transport_crc_order_unknown_truncation_refused(self):
        good = frame()
        for bad in (good[:-3], good.replace('d8r1','d8t2'), good.replace('8429D750','00000000'), good+good,
                    good.replace('d8r1 peak','d8r1 last'), good.replace('00000011 8429D750','00000012 8429D750')):
            with self.assertRaises(ValueError): parse_report(bad, EXPECTED, 1, 17)
        for gen, boots in ((2,17), (1,18), (0,17)):
            with self.assertRaises(ValueError): parse_report(good, EXPECTED, gen, boots)
    def test_recomputed_crc_cannot_hide_numeric_inconsistency(self):
        changes = [(2,1,1),(2,1,2),(2,1,4),(2,1,8),(2,1,16),(2,2,0xffffffff),(2,6,12),
            (2,9,9),(2,10,0),(2,11,1),(3,0,1),(3,12,9),(4,12,7),(5,0,2),(5,1,0),
            (5,4,24281),(6,5,20),(1,3,1),(1,4,1)]
        for row,col,value in changes:
            with self.subTest(row=row,col=col,value=value):
                values = rows(); values[row][col] = value
                with self.assertRaises(ValueError): parse_report(frame(values), EXPECTED, 1, 17)
    def test_invalid_autopaused_retains_frame_then_rejects(self):
        values = rows(); values[2][1] = 2
        t = Scripted({'resource stop':'resource paused 00000001 00000011\r\n','resource report':frame(values)})
        _, receipt = self.run_capture(t, fail=ValueError)
        self.assertFalse(receipt.accepted); self.assertEqual(receipt.ownership, capture.Ownership.PAUSED)
        self.assertIn('d8r1 end', (self.output/'report.frame').read_text()); self.assertNotIn('report.json', [p.name for p in self.output.iterdir()])
    def test_start_invalid_retains_invalid_report_without_wait(self):
        values = rows(idle=True); values[2][1] = 4
        t = Scripted({'resource start':'resource invalid 00000001 00000011\r\n','resource report':frame(values)})
        _, receipt = self.run_capture(t, fail=ValueError, sleep=lambda _: self.fail('invalid start must not wait'))
        self.assertEqual(t.writes, ['status','resource start','resource report']); self.assertEqual(receipt.ownership,capture.Ownership.PAUSED)
    def test_lost_start_ack_has_unknown_ownership_no_speculative_stop(self):
        _, receipt = self.run_capture(t := Scripted({'resource start': OSError('start ACK lost')}), fail=OSError)
        self.assertEqual(t.writes,['status','resource start']); self.assertEqual(receipt.ownership,capture.Ownership.UNKNOWN_START)
    def test_driver_timeout_start_ack_not_retried(self):
        _, receipt = self.run_capture(t := Scripted({'resource start':TimeoutError('simulated driver timeout')}),fail=TimeoutError)
        self.assertEqual(t.writes,['status','resource start']);self.assertEqual(receipt.ownership,capture.Ownership.UNKNOWN_START)
    def test_uac_configuration_change_is_rejected(self):
        identity=capture.Candidate('a'*40,'b'*64,self.identity.diagnostic_package,'c'*64,Expected(0,0x3ffd,True),VERSION)
        t=Scripted({'status':[status(uac=True),status(1,uac=True).replace('uac_alt 1','uac_alt 0')],'resource report':frame(uac=True)})
        _, receipt=self.run_capture(t,fail=ValueError,identity=identity);self.assertFalse(receipt.accepted)
    def test_partial_report_disconnect_preserves_bytes(self):
        t=Scripted()
        original=t.write
        def partial(data):
            if data==b'resource report\r':
                t.writes.append('resource report');t.queue.extend(b'resource report\r\nd8r1 begin 00000001 00000011\r\nd8r1 config 0000')
            else: original(data)
        t.write=partial
        self.run_capture(t,fail=OSError)
        self.assertIn(b'd8r1 config 0000',(self.output/'report.frame').read_bytes())
        self.assertGreater((self.output/'report.response').stat().st_size,0)
    def test_lost_stop_ack_never_replayed(self):
        _, receipt = self.run_capture(t := Scripted({'resource stop': OSError('stop ACK lost')}), fail=OSError)
        self.assertEqual(t.writes,['status','resource start','resource stop']); self.assertEqual(receipt.ownership,capture.Ownership.UNKNOWN_STOP)
    def test_malformed_ack_preserves_ambiguity(self):
        _, receipt = self.run_capture(t := Scripted({'resource start':'resource started 00000002 00000011\r\n'}), fail=ValueError)
        self.assertEqual(receipt.ownership,capture.Ownership.UNKNOWN_START);self.assertNotIn('resource stop',t.writes)
    def test_contradictory_start_ack_remains_unknown(self):
        t=Scripted({'resource start':'resource start refused\r\nresource started 00000001 00000011\r\n'})
        _, receipt=self.run_capture(t,fail=ValueError)
        self.assertEqual(receipt.ownership,capture.Ownership.UNKNOWN_START);self.assertNotIn('resource stop',t.writes)
    def test_owned_failure_cleanup_pauses_once(self):
        def interrupted(_): raise RuntimeError('simulated workload interruption')
        _, receipt = self.run_capture(t := Scripted(), fail=RuntimeError, sleep=interrupted)
        self.assertEqual(receipt.ownership,capture.Ownership.PAUSED);self.assertEqual(t.writes.count('resource stop'),1)
    def test_owned_failure_cleanup_failure_retained(self):
        def interrupted(_): raise RuntimeError('workload interruption')
        _, receipt = self.run_capture(t := Scripted({'resource stop':OSError('cleanup disconnect')}),fail=RuntimeError,sleep=interrupted)
        self.assertTrue(receipt.cleanup_failure);self.assertEqual(t.writes.count('resource stop'),1)
    def test_boot_usb_generation_changes_rejected(self):
        for changed in (status(1).replace('boots 17','boots 18'),status(1).replace('usb_resets 2','usb_resets 3'),status(2)):
            self.output = self.output/'change';self.output.mkdir()
            _, receipt = self.run_capture(Scripted({'status':[status(),changed]}),fail=ValueError)
            self.assertFalse(receipt.accepted)
    def test_missing_caps_active_run_wrong_profile_refused_before_start(self):
        for changed in (status().replace('resource_helpers 3','resource_helpers 1'),status(running=1),status().replace('resource_profile 0','resource_profile 1')):
            self.output=self.output/'case';self.output.mkdir();t=Scripted({'status':changed})
            self.run_capture(t,fail=ValueError);self.assertEqual(t.writes,['status'])
    def test_manifest_flags_and_types_refused_before_device_open(self):
        path, good = self.manifest(); self.assertEqual(capture.candidate(path).expected,EXPECTED)
        for key,value in (('resource_diagnostic',False),('resource_diagnostic',1),('resource_engine_profile',True),('resource_uac_enabled',0),('resource_engine_mask',0x1409),('startup_flags',[])):
            bad = {**good,key:value};path.write_text(json.dumps(bad))
            with mock.patch.object(capture,'MacCdc') as device:
                with self.assertRaises(ValueError): capture.candidate(path)
                device.assert_not_called()
        for flag in ('-DD8_HEAVY_POOL_DIAGNOSTIC','-UD8_HEAVY_POOL_DIAGNOSTIC','-DD8_STACK_WATERMARK_DIAGNOSTIC=0','-DFELUCCA_FM4=1','-DFELUCCA_UAC','-D'):
            path.write_text(json.dumps({**good,'compiler_flags':good['compiler_flags']+[flag]}))
            with self.assertRaises(ValueError): capture.candidate(path)
    def test_default_plan_never_opens_port(self):
        path,_=self.manifest()
        with mock.patch.object(capture,'MacCdc') as device, mock.patch.object(sys,'argv',['resource','--manifest',str(path),'--version',VERSION,'--workload','idle']), contextlib.redirect_stdout(io.StringIO()) as text:
            self.assertEqual(capture.main(),0);device.assert_not_called()
            (self.output/'plan.json').write_text(text.getvalue())
    def test_simulated_cli_receipt_and_close_failure(self):
        path,_=self.manifest();t=Scripted();t.close=lambda: (_ for _ in ()).throw(OSError('driver restore failed'))
        output=self.output/'cli'
        with mock.patch.object(capture,'MacCdc',return_value=t),mock.patch.object(capture.time,'sleep',lambda _:None),mock.patch.object(sys,'argv',['resource','--manifest',str(path),'--version',VERSION,'--workload','idle','--capture','--port','/dev/cu.usbmodemfixture','--output',str(output)]),contextlib.redirect_stdout(io.StringIO()):
            # Explicit injected sleep avoids an actual ten-second wait in collect's bound default.
            original=capture.collect
            with mock.patch.object(capture,'collect',side_effect=lambda c,o,r: original(c,o,r,sleep=lambda _:None)):
                self.assertEqual(capture.main(),1)
        receipt=json.loads((output/'receipt.json').read_text());self.assertFalse(receipt['accepted']);self.assertTrue(receipt['close_failure'])


if __name__ == '__main__': unittest.main()
