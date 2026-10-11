#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Preprocess the actual core header: full builds stay compatible; profiles refuse.

No compiler execution of firmware, generated sample dependency or target build.
"""
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
CC = shlex.split(os.environ.get('CC', 'cc'))
checks = 0
for fm4 in (0, 1):
    for slice_ in (0, 1):
        expected = 0x1ffd | (0x2000 if slice_ else 0) | (2 if fm4 else 0)
        text = f'#include "core.h"\n#if D8ENGINE_EFFECTIVE_MASK != {expected}u\n#error incorrect observation mask\n#endif\n#if NENGINES != {13 + slice_}\n#error changed stable engine count\n#endif\n#if NENG_SHOWN != {12 + slice_ + fm4}\n#error changed visible count\n#endif\n'
        command = [*CC, '-E', '-x', 'c', '-Ifirmware/src', f'-DFELUCCA_FM4={fm4}', f'-DFELUCCA_SLICE={slice_}', '-']
        proc = subprocess.run(command, input=text, text=True, capture_output=True, cwd=ROOT)
        assert proc.returncode == 0, proc.stderr
        checks += 1
        for macro in ('D8CARD_REQUESTED_ENGINE_MASK', 'D8ENGINE_EFFECTIVE_MASK'):
            # Both full and reduced attempted overrides must fail, even when
            # a candidate header is injected before core.h rather than -D.
            for value in (0x3ffd, 1):
                for prefix in (f'#define {macro} {value}u\n', ''):
                    cmd = command if prefix else [*command[:-1], f'-D{macro}={value}u', '-']
                    proc = subprocess.run(cmd, input=prefix + text, text=True, capture_output=True, cwd=ROOT)
                    assert proc.returncode != 0 and 'runtime enforcement is incomplete' in proc.stderr, proc.stderr
                    checks += 1
print(f'engine availability build guard: {checks} checks passed; preprocessing only')
