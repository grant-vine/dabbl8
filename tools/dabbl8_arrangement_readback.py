#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
"""Offline C-context comparison from two existing complete instrument snapshots."""
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import re
from typing import Callable, Final, TypeAlias, TypedDict, cast
import zlib

from dabbl8_instrument_capture import CURRENT_LENGTHS, NAMES, RAW_MAP
import dabbl8_project_archive as project_archive

HEAVY_SHA: Final = '678dba08d7f40c4a5e8c8223d703511466ef0aeb4e4d80987e406ef2b8f375c3'
ARRANGEMENT_SHA: Final = '25ce16e8ae7b6fab1d5b4020df95e41fd0b1df6525c735adb698ed6ce54dbe61'
Json: TypeAlias = str | int | float | bool | None | list['Json'] | dict[str, 'Json']


class ExportReport(TypedDict):
    accepted: bool


class ArchiveHeader(TypedDict):
    present: int


# These old codec/archive APIs are separately validated. Keep their untyped
# internals at this boundary; the adapter still verifies every actual payload.
prepare = cast(Callable[[str, Path, Path, Path, Path | None], ExportReport], project_archive.prepare)
parse_archive = cast(Callable[[bytes], tuple[ArchiveHeader, list[bytes], bytes]], project_archive.parse_archive)


@dataclass(frozen=True)
class Context:
    initial_live_sha256: str = HEAVY_SHA
    arrangement_sha256: str = ARRANGEMENT_SHA


def digest(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def bounded(path: Path, maximum: int) -> bytes:
    """Keep corrupted artifact metadata or symlinks out of bounded readback."""
    if path.is_symlink() or not path.is_file():
        raise ValueError('Expected regular local artifact')
    with path.open('rb') as stream:
        raw = stream.read(maximum + 1)
    if len(raw) > maximum:
        raise ValueError('Artifact exceeds bound')
    return raw


def snapshot(directory: Path) -> tuple[bytes, ...]:
    """Recheck accepted full capture, release and all original bytes before reuse."""
    raw = bounded(directory / 'verified/manifest.json', 65536)
    manifest = cast(Json, json.loads(raw))
    report = cast(Json, json.loads(bounded(directory / 'report.json', 65536)))
    if not isinstance(manifest, dict) or not isinstance(report, dict):
        raise ValueError('Snapshot metadata must be mappings')
    if (manifest.get('format') != 'dabbl8-instrument-capture' or type(manifest.get('version')) is not int or manifest.get('version') != 1 or
            type(manifest.get('role_table')) is not int or manifest.get('role_table') != 1 or manifest.get('mode') != 'full' or
            manifest.get('current_state_complete') is not True or manifest.get('raw_bytes') != 319488 or
            report.get('format') != 'dabbl8-instrument-capture-report' or type(report.get('version')) is not int or report.get('version') != 1 or
            report.get('accepted') is not True or report.get('current_state_complete') is not True or
            report.get('release_acknowledged') is not True):
        raise ValueError('Complete accepted/released snapshot required')
    objects = manifest.get('objects')
    if not isinstance(objects, list) or len(objects) != 17:
        raise ValueError('Exactly17ordered objects required')
    token = manifest.get('token')
    if (report.get('mode') != 'full' or report.get('objects') != objects or
            type(token) is not int or not 0 < token <= 0xffffffff or type(report.get('token')) is not int or report.get('token') != token):
        raise ValueError('Snapshot receipt and publication identity differ')
    values: list[bytes] = []
    for role, item in enumerate(objects):
        name = NAMES[role]
        if not isinstance(item, dict) or type(item.get('role')) is not int or item.get('role') != role or item.get('name') != name or item.get('file') != 'original-' + name + '.bin':
            raise ValueError('Snapshot object identity changed')
        path = directory / 'verified' / ('original-' + name + '.bin')
        cap = RAW_MAP[role][1] + RAW_MAP[role][3] if role < 12 else CURRENT_LENGTHS.get(role, 7936)
        if path.is_symlink() or not path.is_file():
            raise ValueError('Snapshot object is not a regular file')
        with path.open('rb') as stream:
            data = stream.read(cap + 1)
        if (len(data) > cap or len(data) != item.get('bytes') or digest(data) != item.get('sha256') or
                zlib.crc32(data) != item.get('crc32') or
                (role != 12 and len(data) != cap)):
            raise ValueError('Snapshot bytes/CRC/SHA differ: ' + name)
        values.append(data)
    return tuple(values)


def compare(before: Path, after: Path, adapter: Path, adapter_sha256: str, output: Path,
            context: Context = Context()) -> dict[str, str | bool]:
    """Decode existing raw snapshots offline; no extra device export or retry."""
    if not re.fullmatch('[0-9a-f]{64}', adapter_sha256) or digest(bounded(adapter, 16*1024*1024)) != adapter_sha256:
        raise ValueError('Pinned native archive adapter changed')
    initial, final = snapshot(before), snapshot(after)
    protected = (*range(5, 12), *range(13, 17))
    if any(initial[role] != final[role] for role in protected):
        raise ValueError('Settings/presets/samples changed')
    output.mkdir(mode=0o700, parents=True, exist_ok=False)
    logical: list[tuple[bytes, ...]] = []
    for label, values in (('before', initial), ('after', final)):
        # d8pool_mapped_address uses these exact five8192-byte allocations in
        # this order. Physical block identity is not logical project identity.
        pool = output / (label + '-virtual-pool.bin')
        live = output / (label + '-live.d8p')
        pool.write_bytes(b''.join(values[:5]))
        live.write_bytes(values[12])
        result = prepare('export', pool, output / label, adapter, live)
        if result.get('accepted') is not True:
            raise ValueError('Native codec/pool validation refused: ' + label)
        archive = output / label / 'verified/project-set.d8bak'
        header, objects, _ = parse_archive(archive.read_bytes())
        if header['present'] != 15:
            raise ValueError('Expected A/B/C/shared-autosave context missing')
        logical.append(tuple(objects))
    a, b = logical
    if digest(a[0]) != context.initial_live_sha256 or digest(a[4]) != context.initial_live_sha256:
        raise ValueError('Initial MIDI live/shared-autosave workspace differs')
    if digest(a[3]) != context.arrangement_sha256 or a[1:4] != b[1:4]:
        raise ValueError('Protected A/B/C changed')
    if digest(b[0]) != context.arrangement_sha256 or digest(b[4]) not in (context.initial_live_sha256, context.arrangement_sha256):
        raise ValueError('Unknown final live/shared-autosave context')
    return {'accepted': True, 'protected_A_B_C_equal': True, 'protected_ancillary_equal': True,
            'live_equals_C_proposal': True, 'final_autosave_sha256': digest(b[4]),
            'autosave_matches_C': digest(b[4]) == context.arrangement_sha256,
            'extra_device_exports_requested': False, 'hardware_qualified': False,
            'scope': 'Exact logical projects/ancillary bytes from already verified full snapshots; not full flash or device identity attestation.'}
