#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Replay the shipping restore/USB producer order against the real C boot path."""
from pathlib import Path
import hashlib
import json
import os
import re
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'ci'))
from local_checks import recipes


def main(argv: list[str]) -> int:
    """Compile only the local boot fixture; injected MIDI never opens a port."""
    if len(argv) != 2:
        raise SystemExit('usage: native_boot_startup_order_test.py MAIN_SOURCE OUTPUT_DIR')
    source, out = Path(argv[0]).resolve(), Path(argv[1]).resolve()
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', source.read_text(), flags=re.S)
    body = text.split('static void fm1_main(void)', 1)[1].split('\n}\n', 1)[0]
    calls = re.findall(r'^\s*(usb_start\(\)|autosave_boot\(boot_clean\))\s*;', body, re.M)
    if len(calls) != 2 or len(set(calls)) != 2:
        raise SystemExit('Expected exactly one USB start and one boot restore in fm1_main')
    out.mkdir(parents=True, exist_ok=True)
    header = out / 'shipping-boot-order.h'
    replacements = {'usb_start()': 'boot_open_external_midi()',
                    'autosave_boot(boot_clean)': '(void)autosave_boot(1)'}
    header.write_text(''.join(replacements[call] + ';\n' for call in calls))
    command = next(step['argv'] for step in recipes({'readonly-boot'},
        shlex.split(os.environ.get('CC', 'clang')), out, 'HEAD')
        if step['name'] == 'readonly-boot-8-compile')
    command.insert(1, '-DD8_NATIVE_BOOT_STARTUP_ORDER="' + str(header) + '"')
    (out / 'commands.json').write_text(json.dumps({'source': str(source),
        'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
        'calls': calls, 'compile': command, 'run': [command[-1]]}, indent=2) + '\n')
    for name, invocation in (('compile', command), ('run', [command[-1]])):
        result = subprocess.run(invocation, cwd=ROOT, capture_output=True, timeout=120,
            env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0:halt_on_error=1',
                 'UBSAN_OPTIONS': 'halt_on_error=1'}, check=False)
        (out / (name + '.stdout')).write_bytes(result.stdout)
        (out / (name + '.stderr')).write_bytes(result.stderr)
        (out / (name + '.exit')).write_text(str(result.returncode) + '\n')
        sys.stdout.buffer.write(result.stdout)
        sys.stderr.buffer.write(result.stderr)
        if result.returncode:
            return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1:]))
