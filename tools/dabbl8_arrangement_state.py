# SPDX-License-Identifier: GPL-3.0-only
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
"""Strict D8A1 decoding; no transport, device or command implementation."""
from dataclasses import dataclass, replace
from enum import Enum
import math
import re
import struct
import zlib


@dataclass(frozen=True)
class ArrangementState:
    ticks: int
    boots: int
    usb_resets: int
    generation: int
    mode: int
    playing: int
    armed: int
    running: int
    row: int
    scene: int
    rows: int
    valid: int
    suspended: int
    pending: int
    fill_pending: int
    fill_active: int
    recording: int
    request: int
    count_in: int


def parse_arrangement(reply: str) -> ArrangementState:
    """Decode one exact CRC-bound snapshot within a bounded status response."""
    if len(reply) > 65536 or not reply.isascii():
        raise ValueError('Oversized or non-ASCII arrangement response')
    rows = [line for line in reply.splitlines() if line.startswith('d8a1')]
    if len(rows) != 1 or not re.fullmatch(r'd8a1(?: [0-9A-F]{8}){20}', rows[0]):
        raise ValueError('Missing, duplicate or malformed D8A1 frame')
    words = [int(v, 16) for v in rows[0].split()[1:]]
    checksum = zlib.crc32(struct.pack('<20I', 0x44384131, *words[:-1]))
    if checksum != words[-1]:
        raise ValueError('D8A1 CRC mismatch')
    state = ArrangementState(*words[:-1])
    if state.mode not in (0, 1, 2) or any(v not in (0, 1) for v in
            (state.playing, state.armed, state.running, state.valid, state.suspended, state.count_in)):
        raise ValueError('Unknown arrangement mode or boolean')
    if state.recording > 255 or state.request > 3 or state.fill_active > 2:
        raise ValueError('Invalid arrangement input state')
    if state.mode != 1:
        if (state.generation, state.row, state.scene, state.rows, state.valid, state.suspended,
            state.pending, state.fill_pending, state.fill_active) != (0, 255, 255, 0, 0, 0, 255, 255, 0):
            raise ValueError('Inactive native fields are not canonical')
    elif not 0 <= state.rows <= 16 or any(v != 255 and v >= 16 for v in
            (state.row, state.scene, state.pending, state.fill_pending)):
        raise ValueError('Native row shape outside bounds')
    return state


class Phase(str, Enum):
    WAITING = 'waiting-for-owner-play'
    RUNNING = 'observing-natural-rows'
    COMPLETE = 'all-rows-observed-and-stopped'


@dataclass(frozen=True)
class ArrangementObservation:
    """One bounded observation of the pinned16-row C proposal; no device IO."""
    boots: int
    usb_resets: int
    waiting_since: float
    last_host_seconds: float
    phase: Phase = Phase.WAITING
    generation: int = 0
    last_row: int = -1
    seen: int = 0
    started_at: float = 0

    @classmethod
    def begin(cls, state: ArrangementState, seconds: float) -> 'ArrangementObservation':
        if not math.isfinite(seconds) or seconds < 0 or any((state.playing, state.armed,
                state.running, state.recording, state.request, state.count_in)):
            raise ValueError('Initial observation must be finite and stopped/unarmed')
        return cls(state.boots, state.usb_resets, seconds, seconds)

    def observe(self, state: ArrangementState, seconds: float) -> 'ArrangementObservation':
        """Refuse lost coverage/epoch/control changes; never infer missing rows."""
        if self.phase == Phase.COMPLETE:
            raise ValueError('Completed observation is one-shot')
        if (not math.isfinite(seconds) or not self.last_host_seconds < seconds <= self.last_host_seconds + 1
                or (state.boots, state.usb_resets) != (self.boots, self.usb_resets)
                or state.recording or state.count_in):
            raise ValueError('Observation clock, coverage, epoch or recording changed')
        if self.phase == Phase.WAITING and not state.playing:
            if seconds - self.waiting_since > 60 or state.running:
                raise ValueError('PLAY not observed within the bounded owner window')
            if (state.armed, state.request) not in ((0, 0), (1, 1)):
                raise ValueError('Unexpected stopped or preparing transport state')
            return replace(self, last_host_seconds=seconds)
        if (state.mode != 1 or state.valid != 1 or state.rows != 16 or
                state.scene != state.row or state.armed or state.request or
                state.pending != 255 or state.fill_pending != 255 or state.fill_active):
            raise ValueError('Pinned natural arrangement state changed')
        if state.playing:
            if not state.running or state.suspended:
                raise ValueError('Inconsistent active native transport')
            first = self.phase == Phase.WAITING
            if (first and state.row != 0) or (not first and
                    (state.generation != self.generation or state.row not in (self.last_row, self.last_row + 1))):
                raise ValueError('Initial or intermediate arrangement rows were missed')
            origin = seconds if first else self.started_at
            if seconds - origin > 36:
                raise ValueError('Arrangement did not reach automatic Stop in time')
            return replace(self, phase=Phase.RUNNING, generation=state.generation,
                           last_row=state.row, seen=self.seen | (1 << state.row),
                           started_at=origin, last_host_seconds=seconds)
        if (state.running or not state.suspended or self.seen != 65535 or state.row != 15 or
                state.generation != ((self.generation + 1) & 0xffffffff) or
                seconds - self.started_at > 36):
            raise ValueError('Missing rows or unconfirmed natural final Stop')
        return replace(self, phase=Phase.COMPLETE, last_host_seconds=seconds)
