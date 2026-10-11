# SPDX-License-Identifier: GPL-3.0-only
"""Validate paused D8T1 timing reports and nearest-rank percentile bounds.
No device transport, installation, clock calibration or hardware qualification.
"""
import argparse
import json
import re
import struct
import zlib
from pathlib import Path

BINS = 128
SHIFT = 10
U32_MAX = (1 << 32) - 1

def u32(value):
    if type(value) is not int or not 0 <= value <= U32_MAX:
        raise ValueError('unsigned 32-bit integer required')
    return value

def report(record):
    if set(record) != {'bins', 'count', 'max_ticks', 'saturated'}:
        raise ValueError('unexpected or missing histogram fields')
    bins = record['bins']
    if type(bins) is not list or len(bins) != BINS:
        raise ValueError('128 bins required')
    bins = [u32(n) for n in bins]
    count = u32(record['count'])
    maximum = u32(record['max_ticks'])
    saturated = u32(record['saturated'])
    if saturated not in (0, 1) or saturated:
        raise ValueError('saturated/lost samples invalidate complete distribution')
    if sum(bins) != count:
        raise ValueError('bin sum differs from sample count')
    if not count:
        if maximum:
            raise ValueError('empty histogram has a nonzero maximum')
        return {'samples': 0, 'maximum_ticks': None, 'p95_ticks': None, 'p99_ticks': None}
    max_bin = min(maximum >> SHIFT, BINS - 1)
    if not bins[max_bin] or any(bins[max_bin + 1:]):
        raise ValueError('maximum and histogram disagree')
    def percentile(percent):
        rank = (count * percent + 99) // 100
        cumulative = 0
        for index, n in enumerate(bins):
            cumulative += n
            if cumulative >= rank:
                # Last bin is unbounded, but the independently retained maximum
                # supplies an observed upper bound. Never report a bin midpoint
                # or interpolation as an exact percentile.
                lower = index << SHIFT
                upper = maximum if index == BINS - 1 else min(maximum, ((index + 1) << SHIFT) - 1)
                return {'rank': rank, 'lower': lower, 'upper': upper}
        raise ValueError('rank not covered')
    return {'samples': count, 'maximum_ticks': maximum,
            'p95_ticks': percentile(95), 'p99_ticks': percentile(99)}

HEX = re.compile(r'[0-9A-F]{8}')


def parse_report(text, expected_generation=None, expected_boots=None):
    """Accept exactly one complete frame, including its canonical numeric CRC.

    Console echoes/prompts outside the frame are allowed. No serial connection,
    firmware upload, storage writes or claims of physical qualification.
    """
    if not isinstance(text, str) or len(text) > 65536:
        raise ValueError('bounded console text required')
    try:
        text.encode('ascii')
    except UnicodeEncodeError as exc:
        raise ValueError('ASCII console text required') from exc
    if any(ord(char) < 32 and char not in '\r\n' for char in text):
        raise ValueError('unexpected console control byte')
    if not re.search(r'(?m)^d8t1 end [0-9A-F]{8} [0-9A-F]{8} [0-9A-F]{8}\r?\n', text):
        raise ValueError('complete terminated footer required')
    lines = text.splitlines()
    starts = [i for i, line in enumerate(lines) if line.startswith('d8t1 begin')]
    if len(starts) != 1:
        raise ValueError('exactly one timing frame required')
    start = starts[0]
    if len(lines) < start + 6:
        raise ValueError('truncated timing frame')
    frame = lines[start:start + 6]
    if any(line.startswith('d8t1 ') for line in lines[:start] + lines[start + 6:]):
        raise ValueError('extra timing frame data')
    def row(index, label, count):
        words = frame[index].split(' ')
        if words[:2] != ['d8t1', label] or len(words) != count + 2:
            raise ValueError(f'invalid {label} row')
        if not all(HEX.fullmatch(word) for word in words[2:]):
            raise ValueError('canonical eight-digit hexadecimal words required')
        return [int(word, 16) for word in words[2:]]
    begin = row(0, 'begin', 2)
    config = row(1, 'config', 6)
    state = row(2, 'state', 4)
    render = row(3, 'render', BINS + 2)
    inclusive = row(4, 'inclusive', BINS + 2)
    end = row(5, 'end', 3)
    if begin != end[:2] or not begin[0]:
        raise ValueError('run/footer identity mismatch')
    if expected_generation is not None and begin[0] != u32(expected_generation):
        raise ValueError('unexpected run generation')
    if expected_boots is not None and begin[1] != u32(expected_boots):
        raise ValueError('unexpected boot counter')
    crc_words = [0x44385431, *begin, *config, *state, 0x52, *render, 0x49, *inclusive]
    crc = zlib.crc32(struct.pack('<' + 'I' * len(crc_words), *crc_words))
    if crc != end[2]:
        raise ValueError('timing report CRC mismatch')
    tracks, half_frames, fs, ticks_per_us, shift, bins = config
    if tracks not in (4, 8) or half_frames != 128 or fs != 44100 or ticks_per_us != 24 or shift != SHIFT or bins != BINS:
        raise ValueError('unsupported timing configuration')
    first, last, saturated, invalid = state
    if invalid or saturated:
        raise ValueError('lost/invalid samples invalidate distribution')
    if render[0] != inclusive[0]:
        raise ValueError('paired histogram counts differ')
    count = render[0]
    if count:
        if ((last - first + 1) & U32_MAX) != count:
            raise ValueError('serviced-half sequence does not cover samples')
    elif first or last:
        raise ValueError('empty run has nonzero half sequence')
    def metric(words):
        return report({'count': words[0], 'max_ticks': words[1], 'bins': words[2:], 'saturated': saturated})
    metrics = {'render': metric(render), 'inclusive': metric(inclusive)}
    cumulative_render = cumulative_inclusive = 0
    for render_bin, inclusive_bin in zip(render[2:], inclusive[2:]):
        cumulative_render += render_bin
        cumulative_inclusive += inclusive_bin
        if cumulative_render < cumulative_inclusive:
            raise ValueError('render distribution exceeds paired inclusive durations')
    if metrics['render']['maximum_ticks'] is not None and render[1] > inclusive[1]:
        raise ValueError('render maximum exceeds inclusive maximum')
    return {
        'format': 'D8T1', 'generation': begin[0], 'boot_counter': begin[1],
        'tracks': tracks, 'half_frames': half_frames, 'nominal_sample_rate': fs,
        'nominal_timer_ticks_per_us': ticks_per_us, 'clock_calibrated': False,
        'samples': count, 'first_serviced_half': first, 'last_serviced_half': last,
        'crc32': f'{crc:08X}', 'metrics': metrics,
        'measurement_status': 'nonempty_consistent_report' if count else 'no_samples',
        'hardware_qualified': False,
        'limits': [
            'Percentiles are nearest-rank tick intervals; no interpolated exact values.',
            'Render uses legacy nested-TIMER5-subtracted microseconds reexpanded to ticks.',
            'Inclusive interval excludes recorder overhead, early C prologue and assembly entry/exit.',
            'Contiguous serviced-half numbers do not prove every DMA deadline was served.',
            'Physical clock calibration, diagnostic overhead, complete IRQ/deadline and recovery qualification remain required.',
            'Source/package identity must be tied to separately retained build and installation evidence; this report is not binary attestation.',
        ],
    }


def main():
    parser = argparse.ArgumentParser(description='Validate a paused diagnostic console report; no device access.')
    parser.add_argument('report', type=Path)
    parser.add_argument('--generation', type=lambda value: int(value, 0))
    parser.add_argument('--boots', type=lambda value: int(value, 0))
    args = parser.parse_args()
    # Limit before decoding; do not consume an arbitrary-size file as a console report.
    with args.report.open('rb') as stream:
        raw = stream.read(65537)
    if len(raw) > 65536:
        parser.error('report exceeds 65536 bytes')
    try:
        result = parse_report(raw.decode('ascii'), args.generation, args.boots)
    except (ValueError, UnicodeDecodeError) as exc:
        parser.error(str(exc))
    print(json.dumps(result, indent=2))
    return 0 if result['samples'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
