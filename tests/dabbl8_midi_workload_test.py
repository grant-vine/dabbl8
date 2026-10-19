# SPDX-License-Identifier: GPL-3.0-only
from __future__ import annotations
from pathlib import Path
import sys
from typing import Callable
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.dabbl8_midi_workload import EVENTS, WorkloadError, play, preflight

DUMP = (3, 10, 7, 0, 64)
STATUS = 'dabbl8 v0.1.8-timing\nplaying 0\naudio_late 0\nvoices_shed 0\nflash 1\nfl_failure 0\nboots 1\nusb_resets 1\n'


class Clock:
    def __init__(self) -> None:
        self.now: float = 0.
    def clock(self) -> float:
        return self.now
    def sleep(self, seconds: float) -> None:
        self.now += seconds


class Midi:
    def __init__(self) -> None:
        self.messages: list[tuple[int, int, int]] = []
        self.held: set[int] = set()
        self.replies: dict[int, tuple[int, ...]] = {75: (68, 56, 1, 8, 64, 8) + (0,) * 9,
                        2: (1, 14, 0, 64), 29: DUMP,
                        27: (0, 8) + (0, 0, 0, 64, 0, 0) * 8}
        self.fail_on: int = 0
        self.fail_cleanup: bool = False
    def query(self, command: int, args: tuple[int, ...]) -> tuple[int, ...]:
        assert (command, args) in ((75, ()), (2, (1, 14)), (29, (3,)), (27, ()))
        return self.replies[command]
    def note(self, channel: int, note: int, velocity: int) -> None:
        assert channel == 3
        self.messages.append((channel, note, velocity))
        if velocity:
            self.held.add(note)
            if sum(bool(v) for _, _, v in self.messages) == self.fail_on:
                raise OSError('uncertain note-on delivery')
        else:
            if self.fail_cleanup:
                raise OSError('cleanup failed')
            self.held.discard(note)


class Console:
    def __init__(self) -> None:
        self.reply: str = STATUS
        self.requests: int = 0
        self.fail_after: int = 0
    def request(self, command: str) -> str:
        assert command == 'status'
        self.requests += 1
        if self.fail_after and self.requests >= self.fail_after:
            return self.reply.replace('audio_late 0', 'audio_late 1')
        return self.reply


def refuses(operation: Callable[[], tuple[int, int] | None]) -> str:
    try:
        _ = operation()
    except (WorkloadError, OSError) as error:
        return str(error)
    raise AssertionError('unsafe operation accepted')


def main() -> None:
    for track in range(8):
        midi, console = Midi(), Console()
        armed = list(midi.replies[27])
        armed[2 + track * 6 + 5] = 1
        midi.replies[27] = tuple(armed)
        assert 'recording is armed' in refuses(lambda: preflight(midi, console, DUMP))
        assert not midi.messages
    for bad in ((), (0, 4), (8, 8) + (0,) * 48, (0, 8) + (128,) * 48):
        midi, console = Midi(), Console()
        midi.replies[27] = bad
        assert refuses(lambda: preflight(midi, console, DUMP))
        assert not midi.messages
    for key, bad in ((75, (0,)), (2, (1, 14, 1, 64)), (29, (3, 0))):
        midi, console = Midi(), Console()
        midi.replies[key] = bad
        assert refuses(lambda: preflight(midi, console, DUMP))
        assert not midi.messages
    for good, bad in (('v0.1.8', 'v0.1.7'), ('playing 0', 'playing 1'),
                      ('flash 1', 'flash 0'), ('fl_failure 0', 'fl_failure 1')):
        midi, console = Midi(), Console()
        console.reply = STATUS.replace(good, bad)
        assert refuses(lambda: preflight(midi, console, DUMP))
        assert not midi.messages
    midi, console, clock = Midi(), Console(), Clock()
    counters = preflight(midi, console, DUMP)
    play(midi, console, counters, clock=clock.clock, sleep=clock.sleep)
    assert len(midi.messages) == len(EVENTS) == 256 and not midi.held
    assert sum(bool(v) for _, _, v in midi.messages) == 128 and clock.now == 18.
    for cleanup_fail in (False, True):
        midi, console, clock = Midi(), Console(), Clock()
        midi.fail_on, midi.fail_cleanup = 5, cleanup_fail
        error = refuses(lambda: play(midi, console, (1, 1), clock=clock.clock, sleep=clock.sleep))
        assert sum(bool(v) for _, _, v in midi.messages) == 5
        assert len([v for _, _, v in midi.messages if not v]) == 5
        assert bool(midi.held) == cleanup_fail
        assert ('cleanup uncertain' in error) == cleanup_fail
    midi, console, clock = Midi(), Console(), Clock()
    console.fail_after = 2
    assert refuses(lambda: play(midi, console, (1, 1), clock=clock.clock, sleep=clock.sleep))
    assert sum(bool(v) for _, _, v in midi.messages) == 8 and not midi.held
    print('PASS: valid schedule, eight-track REC refusal, malformed state, identity/route/track refusals, partial sends, health abort and uncertain cleanup; no hardware')


if __name__ == '__main__':
    main()
