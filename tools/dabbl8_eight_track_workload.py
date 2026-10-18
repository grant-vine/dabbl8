# SPDX-License-Identifier: GPL-3.0-only
from typing import Final
from tools.dabbl8_midi_workload import ConsolePort, Event, MidiPort, WorkloadError, health, preflight, recording_clear

NOTES: Final = (60, 64, 67, 36, 72, 76, 79, 84)
DURATION: Final = 18.0
EVENTS: Final = tuple(Event(burst * .5 + offset, note, velocity, channel)
    for burst in range(16) for offset, velocity in ((0., 32), (.125, 0))
    for channel, note in enumerate(NOTES))


def preflight_eight(midi: MidiPort, console: ConsolePort,
                    dumps: tuple[tuple[int, ...], ...]) -> tuple[int, int]:
    """Compare track dumps read-only; a fresh full live snapshot is an outer gate."""
    if len(dumps) != 8 or any(len(dump) < 3 or dump[0] != index
                             for index, dump in enumerate(dumps)):
        raise WorkloadError('invalid local eight-track contract')
    counters = preflight(midi, console, dumps[3])
    for index, dump in enumerate(dumps):
        if index != 3 and midi.query(29, (index,)) != dump:
            raise WorkloadError('track differs from validated contract: ' + str(index + 1))
    recording_clear(midi)
    if midi.query(2, (1, 14)) != (1, 14, 0, 64):
        raise WorkloadError('routing changed during all-track preflight')
    if health(console.request('status')) != counters:
        raise WorkloadError('counters changed during all-track preflight')
    return counters
