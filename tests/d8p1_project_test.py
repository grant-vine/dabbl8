#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Execute actual C state/legacy round trips and verify pristine source hashes."""
from pathlib import Path
import hashlib
import subprocess
import sys
root = Path(__file__).resolve().parents[1]
fixtures = root / 'tests/fixtures/projects'
legacy = sorted(p for p in fixtures.glob('*.bin') if '.expected-fun9' not in p.name)
assert len(legacy) == 13
all_files = list(fixtures.glob('*.bin')) + list((root / 'tests/fixtures/d8p1').glob('*.d8p'))
before = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in all_files}
args = [str(Path(sys.argv[1]).resolve())] + [str(root / 'tests/fixtures/d8p1' / n) for n in ['minimal.d8p', 'maximum.d8p', 'unknown-optional.d8p']] + [str(p) for p in legacy]
subprocess.run(args, cwd=root, check=True)
assert before == {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in all_files}
print('D8P1 project fixtures: 13 legacy originals and all canonical references remain pristine')
