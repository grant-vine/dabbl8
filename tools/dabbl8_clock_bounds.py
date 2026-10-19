# SPDX-License-Identifier: GPL-3.0-only
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
# How to run: imported by dabbl8_clock_capture.py; offline tests in tests/dabbl8_clock_test.py.
"""Conservative TIMER4 rate intervals relative to a host monotonic clock."""
from dataclasses import dataclass
from fractions import Fraction
import re
import struct
from typing import Final
import zlib

MODULUS: Final = 1 << 32
NS_PER_SECOND: Final = 1_000_000_000
MAX_RATE_HZ: Final = 100_000_000  # Explicit admission assumption, not a measured fact.
MIN_RATE_HZ: Final = 1_000_000
FRAME: Final = re.compile(r'timing clock\r\nd8c1' + r' ([0-9A-F]{8})' * 7 + r'\r\n> ')


class ClockRefused(ValueError):
    """An observation cannot support a bounded comparison."""
    reason: str
    def __init__(self, reason: str) -> None:
        self.reason = reason
        super().__init__(reason)


@dataclass(frozen=True, slots=True)
class ClockPacket:
    ticks: int
    boots: int
    usb_resets: int
    config: int
    period: int
    uac: int

    def __post_init__(self) -> None:
        if any(not 0 <= v < MODULUS for v in (
            self.ticks, self.boots, self.usb_resets, self.config, self.period, self.uac
        )) or self.config & 255 != 9 or self.period != MODULUS - 1 or self.uac not in (0, 1):
            raise ClockRefused('Unsupported timer configuration or word range')


def parse_clock(reply: str) -> ClockPacket:
    """Require the exact command echo, one frame, CRC and final prompt."""
    match = FRAME.fullmatch(reply)
    if match is None:
        raise ClockRefused('Malformed, stale or truncated D8C1 reply')
    words = tuple(int(word, 16) for word in match.groups())
    if zlib.crc32(struct.pack('<7I', 0x44384331, *words[:6])) != words[6]:
        raise ClockRefused('D8C1 checksum mismatch')
    return ClockPacket(*words[:6])


@dataclass(frozen=True, slots=True)
class ClockObservation:
    sent_ns: int
    received_ns: int
    packet: ClockPacket

    def __post_init__(self) -> None:
        if self.sent_ns < 0 or not 0 < self.received_ns - self.sent_ns <= NS_PER_SECOND:
            raise ClockRefused('Invalid or excessively wide host bracket')


@dataclass(frozen=True, slots=True)
class ClockBounds:
    lower_hz: int
    upper_hz: int
    comparisons: int


def clock_bounds(points: tuple[ClockObservation, ...]) -> ClockBounds:
    """Intersect all pairs, accounting for transit time, wrap and one tick quantization.

    The sample executes somewhere inside each host request/reply bracket. This
    does not assume symmetric USB latency or treat the midpoint as an exact time.
    A stable clock in 1..100 MHz is an explicit precondition. A host clock has
    independent drift/error; these intervals are relative to it, not SI calibration.
    """
    if len(points) < 2:
        raise ClockRefused('At least two observations are required')
    reference = points[0].packet
    for point in points:
        p = point.packet
        if (p.boots, p.usb_resets, p.config & 255, p.period, p.uac) != (
            reference.boots, reference.usb_resets, reference.config & 255,
            reference.period, reference.uac
        ):
            raise ClockRefused('Boot, USB epoch or timer/profile changed')
    low, high = Fraction(MIN_RATE_HZ), Fraction(MAX_RATE_HZ)
    pairs = 0
    for index, left in enumerate(points):
        for right in points[index + 1:]:
            shortest = right.sent_ns - left.received_ns
            longest = right.received_ns - left.sent_ns
            if shortest < NS_PER_SECOND or longest * MAX_RATE_HZ >= MODULUS * NS_PER_SECOND:
                raise ClockRefused('Overlapping/short brackets or ambiguous full counter wrap')
            delta = (right.packet.ticks - left.packet.ticks) % MODULUS
            # A zero/reversed counter must not become a plausible rate by choosing another wrap.
            pair_low = Fraction((delta - 1) * NS_PER_SECOND, longest)
            pair_high = Fraction((delta + 1) * NS_PER_SECOND, shortest)
            low, high = max(low, pair_low), min(high, pair_high)
            if low > high:
                raise ClockRefused('Counter movement or clock stationarity is inconsistent')
            pairs += 1
    return ClockBounds(low.numerator // low.denominator,
                       -(-high.numerator // high.denominator), pairs)
