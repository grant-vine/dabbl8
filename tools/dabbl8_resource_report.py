# SPDX-License-Identifier: GPL-3.0-only
"""Strict D8R1 wire model. CRC protects framing; it does not attest the device."""
from __future__ import annotations
from dataclasses import dataclass
import re
import struct
import zlib


class ResourceReportError(ValueError):
    def __init__(self, criterion: str) -> None:
        self.criterion = criterion
        super().__init__('D8R1 rejected: ' + criterion)


@dataclass(frozen=True)
class Expected:
    profile: int
    mask: int
    uac: bool

    def __post_init__(self) -> None:
        masks = ((0x1ffd, 0x1fff, 0x3ffd, 0x3fff), (0x1409,), (0x3411,))
        if type(self.profile) is not int or not 0 <= self.profile <= 2 or type(self.mask) is not int or self.mask not in masks[self.profile] or type(self.uac) is not bool:
            raise ResourceReportError('unsupported typed engine profile/mask/UAC')


@dataclass(frozen=True)
class Pool:
    schema: int
    invalid: int
    requests: int
    allocations: int
    reuses: int
    failures: int
    censuses: int
    last_live_owned_slots: int
    peak_live_owned_slots: int
    capacity_slots: int
    body_bytes: int
    reserved_bytes: int


@dataclass(frozen=True)
class Stack:
    status: int
    low: int
    top: int
    capacity_bytes: int
    untouched_bytes: int
    observed_written_bytes_since_boot: int


@dataclass(frozen=True)
class ResourceReport:
    generation: int
    boots: int
    config: Expected
    pool: Pool
    last_engine: tuple[int, ...]
    independent_peak_engine: tuple[int, ...]
    user: Stack
    supervisor: Stack
    format: str = 'D8R1'
    hardware_qualified: bool = False


def parse_report(frame: str, expected: Expected, generation: int, boots: int) -> ResourceReport:
    labels = ('begin', 'config', 'pool', 'last', 'peak', 'user', 'supervisor', 'end')
    counts = (2, 5, 12, 14, 14, 6, 6, 3)
    lines = frame.split('\r\n')
    if len(lines) != 9 or lines[-1] != '':
        raise ResourceReportError('exact ordered CRLF frame required')
    rows: list[tuple[int, ...]] = []
    for line, label, count in zip(lines[:-1], labels, counts):
        if not re.fullmatch('d8r1 ' + label + r'( [0-9A-F]{8}){' + str(count) + '}', line):
            raise ResourceReportError('unknown, duplicated, truncated or out-of-order row ' + label)
        rows.append(tuple(int(word, 16) for word in line.split()[2:]))
    begin, config, pool_words, last, peaks, user_words, super_words, end = rows
    crc_words = [0x44385231, *begin, *config, *pool_words, 0x4c, *last, 0x50, *peaks,
                 0x55, *user_words, 0x53, *super_words]
    crc = zlib.crc32(struct.pack('<' + 'I' * len(crc_words), *crc_words))
    if end != (*begin, crc): raise ResourceReportError('closing generation/boot or CRC mismatch')
    if not 1 <= generation <= 0xffffffff or not 0 <= boots <= 0xffffffff or begin != (generation, boots):
        raise ResourceReportError('run generation/boot binding')
    if config != (8, expected.profile, expected.mask, 3, int(expected.uac)):
        raise ResourceReportError('eight-track/helper/profile/UAC configuration mismatch')
    pool = Pool(*pool_words)
    if pool.schema != 1 or pool.invalid:
        raise ResourceReportError('unavailable schema or invalid/autopaused pool observation')
    if pool.requests != pool.allocations + pool.reuses + pool.failures or pool.censuses != pool.requests + 1:
        raise ResourceReportError('canonical request/outcome/census counters inconsistent')
    if pool.capacity_slots != 8 or not 0 < pool.body_bytes <= 65536 or pool.reserved_bytes != 8 * pool.body_bytes:
        raise ResourceReportError('fixed pool reservation inconsistent')
    if not 0 <= pool.last_live_owned_slots <= pool.peak_live_owned_slots <= 8:
        raise ResourceReportError('ownership counts exceed fixed capacity')
    if sum(last) != pool.last_live_owned_slots or sum(peaks) < pool.peak_live_owned_slots:
        raise ResourceReportError('engine vector/total observations inconsistent')
    for engine, current, peak in zip(range(14), last, peaks):
        if not 0 <= current <= peak <= pool.peak_live_owned_slots:
            raise ResourceReportError('independent engine peak inconsistent')
        if (engine not in (7, 8, 9, 10, 12, 13) or not expected.mask & (1 << engine)) and (current or peak):
            raise ResourceReportError('unavailable/non-heavy engine owns reported slots')
    stacks = []
    for words, bounds in zip((user_words, super_words), ((0x01c74100, 0x01c7a000), (0x01c7a100, 0x01c7c000))):
        stack = Stack(*words)
        if stack.status != 1 or (stack.low, stack.top) != bounds or stack.capacity_bytes != stack.top - stack.low:
            raise ResourceReportError('unavailable/invalid hardware stack descriptor')
        if stack.untouched_bytes + stack.observed_written_bytes_since_boot != stack.capacity_bytes or (stack.untouched_bytes | stack.observed_written_bytes_since_boot) & 3:
            raise ResourceReportError('stack surviving-write lower bound inconsistent')
        stacks.append(stack)
    return ResourceReport(generation, boots, expected, pool, last, peaks, stacks[0], stacks[1])
