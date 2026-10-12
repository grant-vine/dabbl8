#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bounded exact-parent mixer comparison from local Git; no fetch or generation."""
import argparse, hashlib, io, json, shlex, subprocess, tarfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
PARENT = '8105e2aa45f40136c7a0bc64f57b509219e1a865'
def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out', required=True)
    p.add_argument('--cc', default='clang')
    args = p.parse_args()
    out = Path(args.out).resolve(); out.mkdir(parents=True, exist_ok=False)
    reference = out / 'reference'; reference.mkdir()
    archive = subprocess.check_output(['git', 'archive', PARENT, 'firmware', 'tests/hostsim.c'], cwd=ROOT)
    with tarfile.open(fileobj=io.BytesIO(archive)) as entries:
        for item in entries:
            path = Path(item.name)
            if path.is_absolute() or '..' in path.parts or not (item.isdir() or item.isfile()):
                raise RuntimeError('Unsafe parent archive path')
            dest = reference / path
            if item.isdir(): dest.mkdir(parents=True, exist_ok=True)
            else:
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(entries.extractfile(item).read())
    flags = ['-std=c11', '-D_POSIX_C_SOURCE=200809L', '-O1', '-g', '-w',
             '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
             '-fno-sanitize=signed-integer-overflow,shift', '-DFELUCCA_FM4=0',
             '-DFELUCCA_SLICE=1', '-Ibuild/gen', '-Ifirmware/src']
    reports = {}
    for name in ('parent', 'candidate'):
        command = shlex.split(args.cc) + flags
        if name == 'parent': command += ['-DTREE_HOSTSIM="' + str(reference / 'tests/hostsim.c') + '"']
        command += ['tests/native_dry_sends_test.c', '-lm', '-o', str(out / name)]
        for phase, cmd in [('compile', command), ('run', [str(out / name)])]:
            q = subprocess.run(cmd, cwd=ROOT, capture_output=True, timeout=120)
            (out / (name + '-' + phase + '.stdout')).write_bytes(q.stdout)
            (out / (name + '-' + phase + '.stderr')).write_bytes(q.stderr)
            if q.returncode: raise SystemExit(name + ' ' + phase + ' failed: ' + str(q.returncode))
        reports[name] = q.stdout.decode()
    if not reports['parent'].startswith('dry sends: cases=832 checks=40032 trace=') or reports['parent'] != reports['candidate']:
        raise SystemExit('Mixer/state trace differs from the frozen parent')
    receipt = {'parent_commit': PARENT, 'result': 'PASS', 'trace': reports['candidate'].strip(),
               'source_sha256': {f: hashlib.sha256((ROOT / f).read_bytes()).hexdigest() for f in
                    ('firmware/src/fx.c', 'tests/native_dry_sends_test.c')},
               'scope': 'Actual mixer, all13 selectable engines, all8 tracks/send masks, modulation, tails and shared8 voices; host only.'}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))
if __name__ == '__main__': main()
