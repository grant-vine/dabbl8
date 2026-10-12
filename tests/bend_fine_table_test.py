#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the actual bend arithmetic against an independent integer64 oracle.

This is an arithmetic check, not renderer or target timing qualification. The
MIDI target bound is the current 24.99-semitone RPN maximum, in Q8 semitones.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, default=Path(__file__).resolve().parents[1] / 'firmware/src/voice.c')
parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
parser.add_argument('--sanitize', action='store_true', help='Strict ASan/UBSan; no inherited DSP exclusions')
args = parser.parse_args()
source = args.source.read_text()
fragments = re.findall(r'^    bend16 = .*?^    uint32_t nr =', source, re.M | re.S)
if len(fragments) != 1:
    parser.error('expected one identifiable production bend arithmetic block')
fragment = fragments[0].rsplit('    uint32_t nr =', 1)[0]
program = '''/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
typedef struct { int32_t whole, fine; } result_t;
static result_t actual(int32_t bend) {
    int32_t bend16, bend_fine;
''' + fragment + '''    return (result_t){bend16, bend_fine};
}
static unsigned checks;
static int verify(int32_t bend) {
    int64_t residue = (int64_t)bend % 16;
    if (residue < 0) residue += 16;
    int64_t whole = ((int64_t)bend - residue) / 16;
    int64_t fine = residue * 2367 / 2560;
    result_t got = actual(bend); checks++;
    if (got.whole != whole || got.fine != fine) {
        fprintf(stderr, "FAIL bend=%d whole=%d/%lld fine=%d/%lld\\n",
                bend, got.whole, (long long)whole, got.fine, (long long)fine);
        return 1;
    }
    return 0;
}
int main(void) {
    int failed = 0;
    for (int32_t bend = -6397; bend <= 6397; bend++) failed |= verify(bend);
    /* -bend+15 in the inherited floor expression is defined starting here.
     * More-negative arbitrary int32 inputs are outside the supported MIDI range
     * and are deliberately not promoted to a safety claim. */
    const int32_t edges[] = {-2147483632,-2147483631,-1073741825,-65537,
                            65537,1073741825,2147483646,2147483647};
    for (unsigned i=0;i<sizeof edges/sizeof edges[0];i++) failed |= verify(edges[i]);
    printf("bend arithmetic: NTRK=%u values=%u failures=%u\\n", NTRK, checks, failed);
    return failed;
}
'''
with tempfile.TemporaryDirectory(prefix='dabbl8-bend-arithmetic-') as directory:
    root = Path(directory)
    c = root / 'probe.c'
    c.write_text(program)
    for tracks in (4, 8):
        exe = root / ('probe' + str(tracks))
        flags = ['-std=c11', '-O1', '-Wall', '-Wextra', '-Werror', '-DNTRK=' + str(tracks)]
        if args.sanitize:
            flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
        subprocess.run(shlex.split(args.cc) + flags + [str(c), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
