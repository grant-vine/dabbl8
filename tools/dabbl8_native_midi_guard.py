# SPDX-License-Identifier: GPL-3.0-only
"""Bind Mac MIDI access to the locally tested repaired extension before import."""
from __future__ import annotations

import hashlib
from importlib.machinery import PathFinder
from pathlib import Path
import platform
import sys
from typing import Final

QUALIFIED_FILES: Final[tuple[tuple[str, str], ...]] = (
    ('__init__.py', '0b6882c35205dc3cdbb72c8a48c82e4e98676bc4fa9afeecf07831f822abb707'),
    ('version.py', '80f4e6cb7b596a91f2d3a013909ae80a75d9ea52b22fc72b42c3ae4c62042ebe'),
    ('_rtmidi.cpython-39-darwin.so', '7b92ee608464cb1a1b34fef81906df90ffe807774bd786a496ed489b0f3c819e'),
)


class NativeMidiQualificationError(RuntimeError):
    def __init__(self, detail: str) -> None:
        super().__init__('Native CoreMIDI capture is unqualified and disabled: ' + detail)


def require_native_qualification() -> Path:
    """Inspect package files without executing its initializer or native loader."""
    if platform.system() != 'Darwin' or platform.machine() != 'arm64' or sys.version_info[:2] != (3, 9):
        raise NativeMidiQualificationError('requires reviewed arm64 CPython 3.9 build')
    spec = PathFinder.find_spec('rtmidi', sys.path)
    if spec is None or spec.origin is None or spec.submodule_search_locations is None:
        raise NativeMidiQualificationError('rtmidi package missing or not a filesystem package')
    package = Path(spec.origin).resolve().parent
    for name, expected in QUALIFIED_FILES:
        path = package / name
        try:
            actual = hashlib.sha256(path.read_bytes()).hexdigest()
        except OSError as error:
            raise NativeMidiQualificationError('cannot read ' + name) from error
        if actual != expected:
            raise NativeMidiQualificationError('unreviewed bytes in ' + name)
    for name, filename in (('rtmidi', '__init__.py'), ('rtmidi._rtmidi', QUALIFIED_FILES[2][0])):
        loaded = sys.modules.get(name)
        if loaded is not None:
            origin = loaded.__file__
            if origin is None or Path(origin).resolve() != (package / filename).resolve():
                raise NativeMidiQualificationError('foreign module already loaded: ' + name)
    return package


def verify_loaded_native() -> None:
    package = require_native_qualification()
    module = sys.modules.get('rtmidi._rtmidi')
    if module is None or module.__file__ is None:
        raise NativeMidiQualificationError('native module was not loaded by reviewed package')
    if Path(module.__file__).resolve() != (package / QUALIFIED_FILES[2][0]).resolve():
        raise NativeMidiQualificationError('loaded extension differs from reviewed file')
