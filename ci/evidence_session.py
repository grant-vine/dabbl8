# SPDX-License-Identifier: GPL-3.0-only
"""Invocation-owned evidence directories; never delete or reuse earlier logs."""
import tempfile
from pathlib import Path


def create_evidence_directory(root):
    root = Path(root)
    root.mkdir(parents=True, exist_ok=True)
    return Path(tempfile.mkdtemp(prefix='run-', dir=root))
