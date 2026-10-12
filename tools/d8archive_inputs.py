#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Record/check the existing local browser codec; never generate or download."""
import argparse
import hashlib
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/d8archive'
SOURCES = ('firmware/src/d8p1.c', 'firmware/src/d8p1.h',
           'firmware/src/d8pool.c', 'firmware/src/d8pool.h',
           'tools/dabbl8_project_archive.c', 'web/build-d8archive.sh')
ARTIFACTS = ('dabbl8_project_archive.mjs', 'dabbl8_project_archive.wasm', 'compiler-version.txt')
def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def snapshot():
    return {'schema': 1, 'sources': {p: digest(ROOT / p) for p in SOURCES},
            'artifacts': {p: digest(OUT / p) for p in ARTIFACTS}}
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    current = snapshot()
    if args.record:
        (OUT / 'inputs.json').write_text(json.dumps(current, indent=2) + '\n')
    elif json.loads((OUT / 'inputs.json').read_text()) != current:
        raise SystemExit('Browser codec inputs changed: rebuild explicitly using the existing pinned compiler.')
    print('Local browser codec source/artifact identity PASS')
if __name__ == '__main__':
    main()
