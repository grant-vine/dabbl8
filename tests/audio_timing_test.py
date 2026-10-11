# SPDX-License-Identifier: GPL-3.0-only
"""Host-only diagnostic checks; all clocks and console transport are simulated."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

SOURCE = Path(__file__).resolve().parents[1]
PARENT = SOURCE / 'build/host/timing-evidence'
PARENT.mkdir(parents=True, exist_ok=True)
OUT = Path(tempfile.mkdtemp(prefix='run-', dir=PARENT))
CC = shlex.split(os.environ.get('CC', 'cc'))
SAN = os.environ.get('SANITIZE', '1') != '0'
ENV = dict(os.environ, ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
           UBSAN_OPTIONS='halt_on_error=1')
source_paths = ['firmware/src/audio.c', 'firmware/src/audio_timing.h', 'firmware/src/console.c',
                'firmware/src/audio_timing_console.h', 'tools/build.py', 'tools/gen_samples.py',
                'tools/dabbl8_timing_report.py', 'tests/audio_test.c', 'tests/audio_timing_isr_test.c',
                'tests/audio_timing_session_test.c', 'tests/audio_timing_console_test.py',
                'tests/dabbl8_timing_report_test.py', 'tests/timing_build_isolation_test.py',
                'tests/audio_timing_test.py', 'tests/dabbl8_timing_window_test.py', 'tests/audio_timing_window_test.c', 'docs/dabbl8/evidence/2026-10-11-timing-diagnostic/host/console/normal-flag1.frame']
receipt = {'source_hashes': {name: hashlib.sha256((SOURCE / name).read_bytes()).hexdigest()
                             for name in source_paths}, 'scope': 'Simulated clocks/DMA/CDC only; no device or target timing proof.',
           'result': 'FAIL', 'checks': [], 'skips': []}

def run(name, command, env=None):
    proc = subprocess.run(command, cwd=SOURCE, env=env or ENV,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (OUT / (name + '.log')).write_bytes(proc.stdout)
    receipt['checks'].append({'name': name, 'command': command,
                             'exit': proc.returncode,
                             'log_sha256': hashlib.sha256(proc.stdout).hexdigest()})
    if proc.returncode:
        sys.stdout.buffer.write(proc.stdout)
        raise RuntimeError(f'{name} failed; evidence: {OUT}')
    return proc.stdout

try:
    modes = [('normal', ['-O2'])]
    if SAN:
        modes.append(('san', ['-O1', '-g', '-fsanitize=address,undefined',
                             '-fno-sanitize-recover=undefined']))
    else:
        receipt['skips'].append('SANITIZE=0: session, ISR and console sanitizer runs skipped')
    for mode, flags in modes:
        binary = OUT / ('session-' + mode)
        run('session-' + mode + '-build', [*CC, *flags, '-Wall',
            'tests/audio_timing_session_test.c', '-o', str(binary)])
        run('session-' + mode, [str(binary)])
    for mode, flags in modes:
        binary = OUT / ('window-' + mode)
        run('window-' + mode + '-build', [*CC, *flags, '-Wall', 'tests/audio_timing_window_test.c', '-o', str(binary)])
        run('window-' + mode, [str(binary)])
    for tracks in (4, 8):
        for mode, flags in modes:
            # The actual DSP uses upstream's intentional wrap/XIP exclusions.
            irq_flags = flags + (['-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow']
                                 if mode == 'san' else [])
            traces = []
            for enabled in (0, 1):
                label = f'irq-{tracks}-{mode}-flag{enabled}'
                binary = OUT / label
                run(label + '-build', [*CC, *irq_flags, '-w', f'-DNPART={tracks}',
                    f'-DDABBL8_TIMING_DIAGNOSTIC={enabled}', '-Ibuild/gen', '-Ifirmware/src',
                    'tests/audio_timing_isr_test.c', '-lm', '-o', str(binary)])
                for collecting in ([False, True] if enabled else [False]):
                    data = run(label + ('-collect' if collecting else '-paused'),
                               [str(binary)] + (['collect'] if collecting else []))
                    traces.append(next(line for line in data.splitlines()
                                       if line.startswith(b'actual audio parity:')))
            if len(set(traces)) != 1:
                raise RuntimeError(f'ISR output/state parity differs: {tracks}/{mode}')
    console_out = OUT / 'console'
    run('console', [sys.executable, 'tests/audio_timing_console_test.py'],
        dict(ENV, D8_TIMING_CONSOLE_OUT=str(console_out)))
    run('parser', [sys.executable, 'tests/dabbl8_timing_report_test.py'],
        dict(ENV, D8_TIMING_TEST_FRAME=str(SOURCE / 'docs/dabbl8/evidence/2026-10-11-timing-diagnostic/host/console/normal-flag1.frame')))
    run('parser-window', [sys.executable, 'tests/dabbl8_timing_window_test.py'],
        dict(ENV, D8_TIMING_TEST_FRAME=str(console_out / 'normal-flag1.frame')))
    run('output-isolation', [sys.executable, 'tests/timing_build_isolation_test.py'])
    receipt['result'] = 'PASS'
    print(f'Audio timing diagnostics PASS; actual ISR off/paused/collect parity, session, console, parser and output isolation; simulated only. Evidence: {OUT}')
finally:
    (OUT / 'results.json').write_text(json.dumps(receipt, indent=2) + '\n')
