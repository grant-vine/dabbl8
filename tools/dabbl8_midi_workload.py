# SPDX-License-Identifier: GPL-3.0-only
from dataclasses import dataclass
import math
import time
from typing import Callable, Final, Protocol

VERSION: Final = 'v0.1.8-timing'
NOTES: Final = (36, 38, 39, 42, 46, 45, 37, 56)
DURATION: Final = 18.0


class WorkloadError(RuntimeError):
    def __init__(self, criterion: str) -> None:
        self.criterion: str = criterion
        super().__init__(criterion)


class MidiPort(Protocol):
    def query(self, command: int, args: tuple[int, ...]) -> tuple[int, ...]: ...
    def note(self, channel: int, note: int, velocity: int) -> None: ...


class ConsolePort(Protocol):
    def request(self, command: str) -> str: ...


@dataclass(frozen=True)
class Event:
    seconds: float
    note: int
    velocity: int
    channel: int = 3


EVENTS: Final = tuple(Event(burst * .5 + offset, note, velocity)
    for burst in range(16) for offset, velocity in ((0., 32), (.125, 0)) for note in NOTES)


def field(reply: str, key: str) -> int:
    rows = [line for line in reply.splitlines() if line.startswith(key + ' ')]
    if len(rows) != 1:
        raise WorkloadError('missing or ambiguous status: ' + key)
    return int(rows[0].split()[1], 0)


def health(reply: str) -> tuple[int, int]:
    if reply.splitlines().count('dabbl8 ' + VERSION) != 1:
        raise WorkloadError('wrong firmware identity')
    for key, wanted in (('playing', 0), ('audio_late', 0), ('voices_shed', 0),
                        ('flash', 1), ('fl_failure', 0)):
        if field(reply, key) != wanted:
            raise WorkloadError('unsafe status: ' + key)
    return field(reply, 'boots'), field(reply, 'usb_resets')


def recording_clear(midi: MidiPort) -> None:
    """Read TRACK without selecting anything; reject armed or malformed state."""
    state = midi.query(27, ())
    if len(state) != 50 or not 0 <= state[0] < 8 or state[1] != 8:
        raise WorkloadError('invalid eight-track recording state')
    if any(not 0 <= value <= 127 for value in state):
        raise WorkloadError('noncanonical recording state')
    for track in range(8):
        if state[2 + track * 6 + 5] != 0:
            raise WorkloadError('recording is armed on track ' + str(track + 1))


def preflight(midi: MidiPort, console: ConsolePort,
              expected_dump: tuple[int, ...]) -> tuple[int, int]:
    counters = health(console.request('status'))
    caps = midi.query(75, ())
    if len(caps) != 15 or caps[:6] != (68, 56, 1, 8, 64, 8):
        raise WorkloadError('wrong eight-track capabilities')
    if midi.query(2, (1, 14)) != (1, 14, 0, 64):
        raise WorkloadError('routing is not CH1-8')
    recording_clear(midi)
    if len(expected_dump) < 3 or expected_dump[:2] != (3, 10):
        raise WorkloadError('invalid local DRUM contract')
    if midi.query(29, (3,)) != expected_dump:
        raise WorkloadError('track 4 differs from validated DRUM contract')
    if health(console.request('status')) != counters:
        raise WorkloadError('counters changed during preflight')
    return counters


def play(midi: MidiPort, console: ConsolePort, counters: tuple[int, int], *,
         clock: Callable[[], float] = time.monotonic,
         sleep: Callable[[float], None] = time.sleep,
         events: tuple[Event, ...] = EVENTS,
         duration: float = DURATION) -> None:
    if not math.isfinite(duration) or not 0 < duration <= 60 or not 0 < len(events) <= 256:
        raise WorkloadError('unbounded workload schedule')
    scheduled: set[tuple[int, int]] = set()
    previous = -1.
    for event in events:
        if (not math.isfinite(event.seconds) or not 0 <= event.seconds <= duration or event.seconds < previous
                or not 0 <= event.channel < 8 or not 0 <= event.note < 128
                or event.velocity not in (0, 32)):
            raise WorkloadError('invalid workload event')
        key = (event.channel, event.note)
        if event.velocity:
            if key in scheduled:
                raise WorkloadError('duplicate scheduled note-on')
            scheduled.add(key)
        else:
            if key not in scheduled:
                raise WorkloadError('unmatched scheduled note-off')
            scheduled.remove(key)
        if len(scheduled) > 8:
            raise WorkloadError('scheduled notes exceed eight voices')
        previous = event.seconds
    if scheduled:
        raise WorkloadError('scheduled notes lack cleanup')
    held: set[tuple[int, int]] = set()
    start = clock()
    next_status = start

    def check() -> None:
        if health(console.request('status')) != counters:
            raise WorkloadError('boot/USB counters changed during notes')
        recording_clear(midi)

    def wait(until: float) -> None:
        nonlocal next_status
        while clock() < until:
            if clock() >= next_status:
                check()
                next_status = clock() + .5
            remaining = until - clock()
            if remaining > 0:
                sleep(min(remaining, .125))

    try:
        check()
        for event in events:
            wait(start + event.seconds)
            if clock() - start - event.seconds > .25:
                raise WorkloadError('host event scheduling exceeded 250 ms')
            if event.velocity:
                held.add((event.channel, event.note))
            midi.note(event.channel, event.note, event.velocity)
            if not event.velocity:
                held.discard((event.channel, event.note))
        wait(start + duration)
        check()
        if held:
            raise WorkloadError('workload left notes held')
    finally:
        failures: list[str] = []
        for channel, note in sorted(held):
            try:
                midi.note(channel, note, 0)
            except (OSError, RuntimeError, ValueError) as error:
                failures.append(str(error))
        if failures:
            raise WorkloadError('individual note-off cleanup uncertain: ' + '; '.join(failures))
