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
