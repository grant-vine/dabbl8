#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run real optional profile staging and preserve every frozen historical byte."""
from pathlib import Path
import hashlib
import subprocess
import sys
root = Path(__file__).resolve().parents[1]
fixtures = root / 'tests/fixtures/projects'
originals = sorted(p for p in fixtures.glob('*.bin') if '.expected-fun9' not in p.name)
assert len(originals) == 13
pristine = {p: hashlib.sha256(p.read_bytes()).digest() for p in fixtures.glob('*.bin')}
subprocess.run([str(Path(sys.argv[1]).resolve()), str(fixtures / 'fun9.bin'), *map(str, originals)], cwd=root, check=True)
assert pristine == {p: hashlib.sha256(p.read_bytes()).digest() for p in fixtures.glob('*.bin')}
print('Offline profile staging: all 13 historical originals and canonical references remain pristine')
