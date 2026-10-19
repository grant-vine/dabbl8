# SPDX-License-Identifier: GPL-3.0-only
"""Actual virtual NOR snapshot plus release/deadline refusals; no native MIDI."""
from __future__ import annotations

import json
import io
import contextlib
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
from unittest.mock import patch
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.dabbl8_instrument_capture import Bridge
from tools.dabbl8_readonly_snapshot import capture, SnapshotRefused
from tools import dabbl8_readonly_snapshot as snapshot
from tools.dabbl8_native_midi_guard import NativeMidiQualificationError

with patch.object(sys, 'argv', ['snapshot']), \
     patch.object(snapshot, 'require_native_qualification', side_effect=AssertionError('plan touched native gate')), \
     patch.object(snapshot, 'StrictCoreMidiBackend', side_effect=AssertionError('plan created backend')), \
     contextlib.redirect_stdout(io.StringIO()) as plan:
    snapshot.main()
    assert json.loads(plan.getvalue())['device_access'] is False

fixture = Path(sys.argv[1]).resolve()
for corrupt_release in (False, True):
    with TemporaryDirectory() as temp, Bridge(fixture) as actual:
        def exchange(frame: bytes) -> bytes:
            reply = actual.exchange(frame)
            if corrupt_release and frame[6] == 6:
                reply = reply[:13] + b'\x03' + reply[14:]
            return reply
        try:
            capture(exchange, Path(temp) / 'capture')
        except SnapshotRefused:
            assert corrupt_release
        else:
            assert not corrupt_release
        stats = json.loads(actual.line('#stats'))
        assert stats['writes'] == 0 and stats['unchanged']
        manifest = json.loads((Path(temp) / 'capture/verified/manifest.json').read_text())
        assert len(manifest['objects']) == 17

with TemporaryDirectory() as temp, Bridge(fixture) as actual:
    with patch('tools.dabbl8_readonly_snapshot.monotonic', side_effect=[0., 121.]):
        try:
            capture(actual.exchange, Path(temp) / 'expired')
        except SnapshotRefused:
            pass
        else:
            raise AssertionError('expired deadline accepted')
    stats = json.loads(actual.line('#stats'))
    assert stats['writes'] == 0 and stats['unchanged']

with TemporaryDirectory() as temp, Bridge(fixture) as actual:
    observed_ops = []
    def exchange_then_expire(frame: bytes) -> bytes:
        observed_ops.append(frame[6])
        return actual.exchange(frame)
    with patch('tools.dabbl8_readonly_snapshot.monotonic', side_effect=[0., 0., 0., 121.]):
        try:
            capture(exchange_then_expire, Path(temp) / 'expire-active')
        except SnapshotRefused:
            pass
        else:
            raise AssertionError('active expired capture accepted')
    assert observed_ops == [0, 1, 6]
    stats = json.loads(actual.line('#stats'))
    assert stats['writes'] == 0 and stats['unchanged']

with TemporaryDirectory() as temp:
    target = Path(temp) / 'must-not-exist'
    with patch.object(sys, 'argv', ['snapshot', '--capture', str(target)]), \
         patch.object(snapshot, 'require_native_qualification', side_effect=NativeMidiQualificationError('test')), \
         patch.object(snapshot, 'StrictCoreMidiBackend', side_effect=AssertionError('gate bypass')):
        try:
            snapshot.main()
        except NativeMidiQualificationError:
            pass
        else:
            raise AssertionError('native refusal bypassed')
    assert not target.exists()

print('PASS snapshot: 17 actual C roles, zero NOR writes, bad release and deadline refused; no MIDI')

# Simulate the observed missing second-scan reply through the real collector.
# A cleanup error must not hide which request first failed or publish a snapshot.
with TemporaryDirectory() as temp, Bridge(fixture) as actual:
    sent = []
    def lose_verification_reply(frame: bytes) -> bytes:
        sent.append(frame[6])
        if len(sent) == 3692:
            raise TimeoutError('Capture reply timed out; reconnect required')
        if frame[6] == 6:
            raise OSError('closed link cannot acknowledge release')
        return actual.exchange(frame)
    try:
        capture(lose_verification_reply, Path(temp) / 'lost-reply')
    except SnapshotRefused as error:
        assert 'Capture reply timed out; reconnect required' in str(error), str(error)
        assert 'request 3692' in str(error) and 'operation 2' in str(error), str(error)
    else:
        raise AssertionError('missing verification reply accepted')
    assert len(sent) == 3693 and sent[-1] == 6
    assert not (Path(temp) / 'lost-reply/verified').exists()
    report = json.loads((Path(temp) / 'lost-reply/report.json').read_text())
    assert report['reason'] == 'Capture reply timed out; reconnect required'
    assert report['failed_request']['sequence'] == 3692
    assert report['failed_request']['operation'] == 2
    assert report['failed_request']['scan_role'] == 11
    assert report['failed_request']['scan_offset'] == 45312
    assert report['failed_request']['scan_pass'] == 'verification'
    assert report['failed_request']['finished_seconds'] >= report['failed_request']['started_seconds']
    assert not report['release_acknowledged']
    assert report['release_error'] == 'closed link cannot acknowledge release'
    stats = json.loads(actual.line('#stats'))
    assert stats['writes'] == 0 and stats['unchanged']


# Failed acquisition and successful host cleanup are independent outcomes.
class ClosedFakeLink:
    def __init__(self) -> None:
        self.closed = False
        self.cleanup_succeeded = False
        self.cleanup_error = None

    def close(self) -> None:
        self.closed = True
        self.cleanup_succeeded = True

    def exchange(self, frame: bytes) -> bytes:
        raise AssertionError('mock capture must not send any request')

with TemporaryDirectory() as temp:
    target = Path(temp) / 'failed-capture'
    fake = ClosedFakeLink()
    with patch.object(sys, 'argv', ['snapshot', '--capture', str(target)]), \
         patch.object(snapshot, 'require_native_qualification', return_value=None), \
         patch.dict(sys.modules, {'mido': SimpleNamespace(Backend=lambda name: None, Message=None)}), \
         patch.object(snapshot, 'StrictCoreMidiBackend', return_value=None), \
         patch.object(snapshot, 'ReadOnlyMidi', return_value=fake), \
         patch.object(snapshot, 'capture', side_effect=SnapshotRefused('specific timeout')):
        try:
            snapshot.main()
        except SnapshotRefused as error:
            assert 'specific timeout' in str(error)
        else:
            raise AssertionError('failed snapshot CLI accepted')
    receipt = json.loads((target / 'receipt.json').read_text())
    assert not receipt['accepted'] and receipt['native_ports_closed_successfully']
    assert 'specific timeout' in receipt['failure']
    assert receipt['notes_sent'] == 0 and not receipt['saved_data_writes']

print('PASS timeout provenance: real-C second-scan missing reply, no accepted snapshot, separate cleanup evidence; no device access')
