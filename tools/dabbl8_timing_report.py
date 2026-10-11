# SPDX-License-Identifier: GPL-3.0-only
"""Validate paused D8T1/D8T2 timing reports and nearest-rank percentile bounds.
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


def _parse_d8t1(text, expected_generation=None, expected_boots=None):
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


def parse_report(text, expected_generation=None, expected_boots=None):
    """D8T1 historical decoding is unchanged; D8T2 adds an aligned logical window."""
    if not isinstance(text, str) or 'd8t2 begin' not in text:
        return _parse_d8t1(text, expected_generation, expected_boots)
    if len(text) > 65536 or not text.isascii() or any(ord(c)<32 and c not in '\r\n' for c in text):
        raise ValueError('bounded ASCII console text required')
    if not re.search(r'(?m)^d8t2 end [0-9A-F]{8} [0-9A-F]{8} [0-9A-F]{8}\r?\n', text):
        raise ValueError('complete terminated footer required')
    lines = text.splitlines()
    starts = [i for i,l in enumerate(lines) if l.startswith(('d8t1 begin','d8t2 begin'))]
    if len(starts)!=1: raise ValueError('exactly one timing frame required')
    start=starts[0];frame=lines[start:start+7]
    if len(frame)!=7 or any(l.startswith(('d8t1 ','d8t2 ')) for l in lines[:start]+lines[start+7:]):
        raise ValueError('extra or truncated timing frame data')
    data=[]
    for line,label,size in zip(frame,['begin','config','state','window','render','inclusive','end'],[2,6,4,13,BINS+2,BINS+2,3]):
        words=line.split(' ')
        if words[:2]!=['d8t2',label] or len(words)!=size+2 or not all(HEX.fullmatch(w) for w in words[2:]):
            raise ValueError('invalid D8T2 row')
        data.append([int(w,16) for w in words[2:]])
    begin,config,state,window,render,inclusive,end=data
    words=[0x44385432,*begin,*config,*state,*window,0x52,*render,0x49,*inclusive]
    if zlib.crc32(struct.pack('<'+'I'*len(words),*words))!=end[2] or begin!=end[:2]:
        raise ValueError('timing report CRC/identity mismatch')
    first,last,lo,hi,boots,late,under,over,aux0,aux1,aux2,taint,uac=window
    if taint or uac not in (0,1) or (not uac and (under or over)):
        raise ValueError('tainted/unsupported timing window')
    # Reuse the historical histogram/paired-sequence validation exactly after
    # independently validating D8T2's own ordered words and CRC.
    oldwords=[0x44385431,*begin,*config,*state,0x52,*render,0x49,*inclusive]
    oldcrc=zlib.crc32(struct.pack('<'+'I'*len(oldwords),*oldwords))
    labels=['begin','config','state','render','inclusive','end']
    oldrows=[begin,config,state,render,inclusive,[*begin,oldcrc]]
    legacy=''.join('d8t1 '+label+' '+ ' '.join(f'{n:08X}' for n in row)+'\r\n' for label,row in zip(labels,oldrows))
    result=_parse_d8t1(legacy,expected_generation,expected_boots)
    elapsed=(hi<<32)|lo
    if result['samples']:
        if boots!=begin[1] or ((last-first)&U32_MAX)!=lo or elapsed<inclusive[1] or elapsed>result['samples']*0x7fffffff or late>result['samples']:
            raise ValueError('inconsistent elapsed/boot window')
    elif any(window[:-1]):
        raise ValueError('empty run has a nonempty window')
    result['format']='D8T2'
    result['window']={'first_ticks':first,'last_ticks':last,'elapsed_ticks':elapsed,
        'nominal_elapsed_seconds':elapsed/(config[3]*1000000),
        'late_publications':late,'uac_enabled':bool(uac),
        'uac_underrun_publications':under if uac else None,
        'uac_overrun_publications':over if uac else None,
        'alnk_aux_pending_observations':[aux0,aux1,aux2], 'taint':taint,
        'boundary':'First accepted half entry snapshot through last accepted half endpoint snapshot; neither console command time nor lifetime counters.'}
    result['limits'] += [
        'D8T2 snapshots use one bounded counter double-read around each timer read; any observed change taints the run.',
        'Snapshot metadata before the entry timer read and after the endpoint read, recorder code and assembly are excluded from inclusive durations.',
        'Unsigned modular deltas require each observation gap/counter delta below 2^31; unseen whole wraps, ABA or externally reset values that alias wraps cannot be excluded.',
        'ALNK auxiliary bits are observed pending flags, not verified DMA error meanings. Late/UAC counts are software counter publications within the logical window.',
        'External elapsed/reset evidence and pinned diagnostic overhead remain mandatory; this is not physical deadline qualification.',
    ]
    return result

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
