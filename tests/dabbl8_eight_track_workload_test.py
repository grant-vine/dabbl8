# SPDX-License-Identifier: GPL-3.0-only
from __future__ import annotations
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tests.dabbl8_midi_workload_test import Clock, Console, refuses
from tools.dabbl8_midi_workload import Event, play
from tools.dabbl8_eight_track_workload import DURATION, EVENTS, preflight_eight

DUMPS = tuple((track, 10 if track == 3 else 0, 0, 0, 64) for track in range(8))


class MidiEight:
    def __init__(self) -> None:
        self.messages: list[tuple[int, int, int]] = []
        self.held: set[tuple[int, int]] = set()
        self.dumps: tuple[tuple[int, ...], ...] = DUMPS
        self.rec_queries: int = 0
        self.arm_after: int = 0
        self.fail_on: int = 0

    def query(self, command: int, args: tuple[int, ...]) -> tuple[int, ...]:
        if command == 75 and not args:
            return (68, 56, 1, 8, 64, 8) + (0,) * 9
        if command == 2 and args == (1, 14):
            return (1, 14, 0, 64)
        if command == 29 and len(args) == 1 and 0 <= args[0] < 8:
            return self.dumps[args[0]]
        if command == 27 and not args:
            self.rec_queries += 1
            state = [0, 8] + [0, 0, 0, 64, 0, 0] * 8
            if self.arm_after and self.rec_queries >= self.arm_after:
                state[49] = 1
            return tuple(state)
        raise AssertionError('query outside read-only contract')

    def note(self, channel: int, note: int, velocity: int) -> None:
        assert 0 <= channel < 8
        self.messages.append((channel, note, velocity))
        key = (channel, note)
        if velocity:
            self.held.add(key)
            if sum(bool(v) for _, _, v in self.messages) == self.fail_on:
                raise OSError('uncertain note-on')
        else:
            self.held.discard(key)


def main() -> None:
    for track in range(8):
        midi, console = MidiEight(), Console()
        midi.dumps = tuple((index, 13) if index == track else dump for index, dump in enumerate(DUMPS))
        assert refuses(lambda: preflight_eight(midi, console, DUMPS))
        assert not midi.messages
    midi, console, clock = MidiEight(), Console(), Clock()
    counters = preflight_eight(midi, console, DUMPS)
    play(midi, console, counters, clock=clock.clock, sleep=clock.sleep, events=EVENTS, duration=DURATION)
    assert len(midi.messages) == 256 and not midi.held and clock.now == 18.
    assert {channel for channel, _, velocity in midi.messages if velocity} == set(range(8))
    for channel in range(8):
        assert sum(c == channel and bool(v) for c, _, v in midi.messages) == 16
    for fail_on, arm_after, expected_ons in ((5, 0, 5), (0, 2, 8)):
        midi, console, clock = MidiEight(), Console(), Clock()
        midi.fail_on, midi.arm_after = fail_on, arm_after
        assert refuses(lambda: play(midi, console, (1, 1), clock=clock.clock,
            sleep=clock.sleep, events=EVENTS, duration=DURATION))
        assert not midi.held
        assert sum(bool(v) for _, _, v in midi.messages) == expected_ons
    bad_schedules = (
        (), (Event(-.1, 60, 32, 0),), (Event(0., 60, 32, 8),),
        (Event(0., 60, 32, 0),), (Event(0., 60, 0, 0),),
        (Event(0., 60, 32, 0), Event(0., 60, 32, 0)),
        (Event(1., 60, 32, 0), Event(.5, 60, 0, 0)),
    )
    for events in bad_schedules:
        midi, console = MidiEight(), Console()
        assert refuses(lambda: play(midi, console, (1, 1), events=events, duration=18.))
        assert not midi.messages and not console.requests
    print('PASS eight-channel schedule: exact routing/sounds, preflight refusal, partial sends, late REC abort and malformed schedules; no native I/O')


if __name__ == '__main__':
    main()
