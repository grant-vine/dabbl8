# SPDX-License-Identifier: GPL-3.0-only
"""Offline qualification refusals; no native module or endpoint is constructed."""
from __future__ import annotations

import hashlib
from importlib.util import spec_from_file_location
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
from types import ModuleType, SimpleNamespace
from unittest.mock import patch
from collections.abc import Callable

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools import dabbl8_native_midi_guard as guard
from tools.dabbl8_readonly_midi import StrictCoreMidiBackend


def refuses(call: Callable[[], Path | StrictCoreMidiBackend | None]) -> None:
    try:
        call()
    except guard.NativeMidiQualificationError:
        return
    raise AssertionError('unqualified native backend was accepted')


with TemporaryDirectory() as temp:
    folder = Path(temp).resolve() / 'rtmidi'
    folder.mkdir()
    names = ('__init__.py', 'version.py', '_rtmidi.cpython-39-darwin.so')
    for name in names:
        (folder / name).write_bytes(b'fixture bytes; never imported')
    qualified = tuple((name, hashlib.sha256((folder / name).read_bytes()).hexdigest()) for name in names)
    spec = spec_from_file_location('rtmidi', folder / '__init__.py', submodule_search_locations=[str(folder)])
    assert spec is not None
    with patch.object(guard.platform, 'system', return_value='Darwin'), \
         patch.object(guard.platform, 'machine', return_value='arm64'), \
         patch.object(guard.sys, 'version_info', (3, 9)), \
         patch.object(guard.PathFinder, 'find_spec', return_value=spec), \
         patch.object(guard, 'QUALIFIED_FILES', qualified), \
         patch.dict(sys.modules):
        sys.modules.pop('rtmidi', None)
        sys.modules.pop('rtmidi._rtmidi', None)
        assert guard.require_native_qualification() == folder
        refuses(guard.verify_loaded_native)
        for name in names:
            original = (folder / name).read_bytes()
            (folder / name).write_bytes(original + b'tampered')
            refuses(guard.require_native_qualification)
            # Direct constructor must refuse before backend/module/native access.
            refuses(lambda: StrictCoreMidiBackend(SimpleNamespace()))
            (folder / name).write_bytes(original)
        module = ModuleType('rtmidi._rtmidi')
        module.__file__ = str(folder / names[2])
        sys.modules['rtmidi._rtmidi'] = module
        guard.verify_loaded_native()
        module.__file__ = str(folder / 'foreign.so')
        refuses(guard.require_native_qualification)
        with patch.object(guard.PathFinder, 'find_spec', return_value=None):
            refuses(guard.require_native_qualification)
        with patch.object(guard.platform, 'machine', return_value='x86_64'):
            refuses(guard.require_native_qualification)

print('PASS native MIDI shared guard: exact bytes, ABI, missing/foreign modules and direct-constructor refusal; no native imports')
