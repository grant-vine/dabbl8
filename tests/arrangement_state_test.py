# SPDX-License-Identifier: GPL-3.0-only
"""Actual native C formatter interoperability and hostile frame refusals."""
import dataclasses
from pathlib import Path
import struct
import subprocess
import sys
import unittest
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from dabbl8_arrangement_state import ArrangementObservation, ArrangementState, Phase, parse_arrangement

ENABLED = Path(sys.argv.pop(1))
DISABLED = Path(sys.argv.pop(1))


def frame(state: ArrangementState) -> str:
    words = dataclasses.astuple(state)
    crc = zlib.crc32(struct.pack('<20I', 0x44384131, *words))
    return 'd8a1 ' + ' '.join(f'{v:08X}' for v in (*words, crc)) + '\r\n'


class StateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        run = subprocess.run([str(ENABLED)], capture_output=True, timeout=30, check=True)
        cls.lines = run.stdout.decode('ascii').splitlines(keepends=True)
        cls.states = [parse_arrangement(line) for line in cls.lines[:-1]]
        if len(cls.states) != 7 or not cls.lines[-1].startswith('d8a1 '):
            raise AssertionError('Actual C fixture did not produce expected complete/incomplete frames')
        cls.reference = cls.states[2]

    def test_actual_c_native_transitions_and_guard(self) -> None:
        self.assertEqual([s.mode for s in self.states], [0, 2, 1, 1, 1, 1, 1])
        self.assertEqual([s.row for s in self.states[2:]], [0, 0, 1, 2, 2])
        self.assertTrue(all(s.running == s.playing == 1 for s in self.states[2:6]))
        self.assertEqual((self.states[-1].running, self.states[-1].playing, self.states[-1].suspended), (0, 0, 1))
        self.assertEqual(self.states[-1].generation, self.reference.generation + 1)
        self.assertTrue(all(s.boots == 17 and s.usb_resets == 0 for s in self.states))

    def test_stalled_frame_never_accepted(self) -> None:
        with self.assertRaises(ValueError):
            parse_arrangement(self.lines[-1])

    def test_disabled_profile_has_no_snapshot(self) -> None:
        run = subprocess.run([str(DISABLED)], capture_output=True, timeout=30, check=True)
        self.assertEqual(run.stdout, b'')

    def test_status_context_and_exact_crc(self) -> None:
        line = frame(self.reference)
        self.assertEqual(parse_arrangement('status\r\ndabbl8 test\r\n' + line + '\r\n> '), self.reference)
        broken = line[:-12] + '00000000\r\n'
        for response in ('', line + line, ' ' + line, line.lower(), line.rstrip() + ' 0',
                         broken, line + '\N{SNOWMAN}', 'x' * 65537):
            with self.subTest(response=response[:24]), self.assertRaises(ValueError):
                parse_arrangement(response)

    def test_crc_valid_but_impossible_shapes_refused(self) -> None:
        for key, value in (('mode', 3), ('playing', 2), ('armed', 2), ('running', 2),
                           ('row', 16), ('scene', 16), ('rows', 17), ('valid', 2),
                           ('suspended', 2), ('pending', 16), ('fill_pending', 16),
                           ('fill_active', 3), ('recording', 256), ('request', 4), ('count_in', 2)):
            with self.subTest(field=key), self.assertRaises(ValueError):
                parse_arrangement(frame(dataclasses.replace(self.reference, **{key: value})))

    def test_inactive_union_is_explicitly_canonical(self) -> None:
        state = self.states[0]
        for key, value in (('generation', 1), ('row', 0), ('scene', 0), ('rows', 1),
                           ('valid', 1), ('suspended', 1), ('pending', 0),
                           ('fill_pending', 0), ('fill_active', 1)):
            with self.subTest(field=key), self.assertRaises(ValueError):
                parse_arrangement(frame(dataclasses.replace(state, **{key: value})))

    def progress(self, generation: int = 123) -> ArrangementObservation:
        base = dataclasses.replace(self.states[0], recording=0, request=0, count_in=0)
        progress = ArrangementObservation.begin(base, 0.)
        for tick in range(64):
            state = dataclasses.replace(self.reference, rows=16, row=tick // 4, scene=tick // 4,
                                        generation=generation)
            progress = progress.observe(state, (tick + 1) * .5)
        return progress

    def stopped(self, generation: int = 124) -> ArrangementState:
        return dataclasses.replace(self.states[-1], rows=16, row=15, scene=15, generation=generation)

    def test_natural_16_rows_then_stop_and_one_shot(self) -> None:
        progress = self.progress().observe(self.stopped(), 32.5)
        self.assertEqual((progress.phase, progress.seen), (Phase.COMPLETE, 65535))
        with self.assertRaises(ValueError):
            progress.observe(self.stopped(), 33.)

    def test_generation_rollover_has_explicit_stop_transition(self) -> None:
        self.assertEqual(self.progress(0xffffffff).observe(self.stopped(0), 32.5).phase, Phase.COMPLETE)

    def test_stale_stopped_data_cannot_be_success(self) -> None:
        progress = ArrangementObservation.begin(self.states[0], 0.)
        progress = progress.observe(self.states[-1], .5)
        self.assertEqual((progress.phase, progress.seen), (Phase.WAITING, 0))
        for tick in range(1, 120):
            progress = progress.observe(self.states[-1], (tick + 1) * .5)
        with self.assertRaises(ValueError):
            progress.observe(self.states[-1], 60.5)

    def test_unknown_rows_epochs_controls_and_poll_gaps_refuse(self) -> None:
        first = dataclasses.replace(self.reference, rows=16)
        progress = ArrangementObservation.begin(self.states[0], 0.).observe(first, .5)
        for key, value in (('boots', 18), ('usb_resets', 1), ('generation', first.generation + 1),
                           ('row', 2), ('scene', 1), ('rows', 15), ('pending', 1),
                           ('fill_pending', 1), ('fill_active', 1), ('recording', 1),
                           ('count_in', 1), ('request', 1), ('armed', 1), ('mode', 2),
                           ('valid', 0), ('suspended', 1), ('running', 0)):
            with self.subTest(field=key), self.assertRaises(ValueError):
                progress.observe(dataclasses.replace(first, **{key: value}), 1.)
        for time in (.5, 1.51, float('inf'), float('nan')):
            with self.subTest(seconds=time), self.assertRaises(ValueError):
                progress.observe(first, time)

    def test_skipped_start_early_stop_and_wrong_stop_generation_refuse(self) -> None:
        base = ArrangementObservation.begin(self.states[0], 0.)
        with self.assertRaises(ValueError):
            base.observe(dataclasses.replace(self.reference, rows=16, row=1, scene=1), .5)
        with self.assertRaises(ValueError):
            base.observe(dataclasses.replace(self.reference, rows=16), .5).observe(self.stopped(), 1.)
        with self.assertRaises(ValueError):
            self.progress().observe(self.stopped(123), 32.5)

    def test_running_forever_never_becomes_a_pass(self) -> None:
        progress = self.progress()
        state = dataclasses.replace(self.reference, rows=16, row=15, scene=15, generation=123)
        for time in (32.5, 33., 33.5, 34., 34.5, 35., 35.5, 36., 36.5):
            progress = progress.observe(state, time)
        with self.assertRaises(ValueError):
            progress.observe(state, 37.)

    def test_initial_readiness_and_preparing_are_bounded(self) -> None:
        for key in ('playing', 'armed', 'running', 'recording', 'request', 'count_in'):
            with self.subTest(field=key), self.assertRaises(ValueError):
                ArrangementObservation.begin(dataclasses.replace(self.states[0], **{key: 1}), 0.)
        progress = ArrangementObservation.begin(self.states[0], 0.)
        prepared = dataclasses.replace(self.reference, playing=0, running=0, armed=1, request=1, rows=16)
        self.assertEqual(progress.observe(prepared, .5).phase, Phase.WAITING)


if __name__ == '__main__':
    unittest.main()
