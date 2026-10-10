#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare committed references with an independent standard-library encoder."""
from pathlib import Path
import tempfile
import subprocess
import sys
with tempfile.TemporaryDirectory() as directory:
    subprocess.run([sys.executable, 'tests/make_d8p1_fixtures.py', directory], check=True)
    for name in ['minimal.d8p', 'maximum.d8p', 'unknown-optional.d8p']:
        assert (Path(directory) / name).read_bytes() == (Path('tests/fixtures/d8p1') / name).read_bytes(), name
print('D8P1: 3 independent Python reference files match committed synthetic fixtures')
