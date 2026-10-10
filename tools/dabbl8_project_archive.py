#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exact native project-set archives and offline virtual readback; no device access."""
import argparse
import base64
import binascii
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

LIMIT = 7936
POOL_BYTES = 40960
ARCHIVE_LIMIT = 262144
ROLES = ('live', 'project-0', 'project-1', 'project-2', 'autosave')
LIMITS = {'project_bytes': LIMIT, 'virtual_pool_bytes': POOL_BYTES, 'projects': 3, 'autosaves': 1}


def blob(data, present=True):
    return {'present': present, 'bytes': len(data), 'crc32': zlib.crc32(data),
            'sha256': hashlib.sha256(data).hexdigest(), 'data': base64.b64encode(data).decode('ascii')}


def unblob(item, maximum):
    if type(item) is not dict or set(item) != {'present', 'bytes', 'crc32', 'sha256', 'data'}:
        raise ValueError('Unsupported archive blob fields')
    if type(item['present']) is not bool or type(item['bytes']) is not int or not 0 <= item['bytes'] <= maximum:
        raise ValueError('Invalid archive size or presence')
    if type(item['crc32']) is not int or not 0 <= item['crc32'] <= 0xffffffff or type(item['sha256']) is not str:
        raise ValueError('Invalid archive checksum')
    if type(item['data']) is not str or len(item['data']) != 4 * ((item['bytes'] + 2) // 3):
        raise ValueError('Invalid archive encoding bounds')
    data = base64.b64decode(item['data'], validate=True)
    if blob(data, item['present']) != item or (not item['present'] and data):
        raise ValueError('Archive checksum, encoding or absence mismatch')
    return data


def snapshot(source, destination, maximum):
    with Path(source).open('rb') as f:
        data = f.read(maximum + 1)
    if len(data) > maximum:
        raise ValueError('Source exceeds bounded size; retain it separately')
    destination.write_bytes(data)
    return data


def ask(adapter, *args, binary=False):
    p = subprocess.run([str(Path(adapter).resolve()), *map(str, args)], capture_output=True)
    if p.returncode:
        raise ValueError(p.stderr.decode('utf8').strip() or 'Native archive adapter refused input')
    if binary:
        return p.stdout
    result = json.loads(p.stdout)
    if result.get('version') != 1 or result.get('format') not in ('dabbl8-project-archive-pool', 'dabbl8-project-archive-check'):
        raise ValueError('Unsupported adapter result')
    return result


def metadata(data):
    # Inspection after the actual C codec accepted all fields; never re-encode.
    result = {'name': '', 'banks': 0, 'scenes': 0, 'rows': 0}
    pos = 32
    for _ in range(data[20]):
        kind, n = struct.unpack_from('<HH', data, pos)
        p = data[pos + 8:pos + 8 + n]
        if kind & 32767 == 6:
            result['name'] = p[:12].split(b'\0')[0].decode('ascii')
        if kind & 32767 in (5, 7, 8):
            result[{5: 'rows', 7: 'banks', 8: 'scenes'}[kind & 32767]] = p[0]
        pos += 8 + n
    return result


def inspect_pool(adapter, path):
    info = ask(adapter, 'inventory', path)
    present = info.get('present')
    if type(present) is not int or not 0 <= present <= 15 or type(info.get('records')) is not list or len(info['records']) != 4:
        raise ValueError('Invalid native inventory result')
    objects = [ask(adapter, 'read', path, o, binary=True) if present & (1 << o) else b'' for o in range(4)]
    return present, objects, info


def parse_archive(data):
    def pairs(values):
        result = {}
        for key, value in values:
            if key in result:
                raise ValueError('Duplicate archive field')
            result[key] = value
        return result
    a = json.loads(data, object_pairs_hook=pairs)
    if type(a) is not dict or set(a) != {'format', 'version', 'limits', 'present', 'objects', 'original_pool', 'source'}:
        raise ValueError('Unsupported native project-set header')
    if a['format'] != 'dabbl8-project-set' or type(a['version']) is not int or a['version'] != 1 or type(a['limits']) is not dict or a['limits'] != LIMITS or any(type(v) is not int for v in a['limits'].values()):
        raise ValueError('Unsupported native project-set schema or limits')
    if type(a['present']) is not int or not 0 <= a['present'] <= 15:
        raise ValueError('Invalid persisted presence mask')
    if type(a['source']) is not dict or set(a['source']) != {'pool_name', 'live_name'}:
        raise ValueError('Invalid source provenance')
    for name in a['source'].values():
        if type(name) is not str or not name or len(name) > 255 or name in ('.', '..') or any(c in name for c in '/\\\0'):
            raise ValueError('Invalid source name; paths are not accepted')
    if type(a['objects']) is not list or len(a['objects']) != len(ROLES):
        raise ValueError('Missing or extra project-set objects')
    objects = []
    for i, item in enumerate(a['objects']):
        if type(item) is not dict or set(item) != {'role', 'blob'} or item['role'] != ROLES[i]:
            raise ValueError('Unknown, duplicate or reordered project identity')
        raw = unblob(item['blob'], LIMIT)
        expected = True if not i else bool(a['present'] & (1 << (i - 1)))
        if item['blob']['present'] != expected or (expected and not raw):
            raise ValueError('Project-set presence mismatch')
        objects.append(raw)
    pool = unblob(a['original_pool'], POOL_BYTES)
    if not a['original_pool']['present'] or len(pool) != POOL_BYTES:
        raise ValueError('Missing original virtual pool')
    return a, objects, pool


def prepare(mode, source, destination, adapter, live=None):
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=False)
    report = {'format': 'dabbl8-project-set-report', 'version': 1, 'accepted': False,
              'device_writes': False, 'firmware_package': False, 'hardware_qualified': False,
              'full_instrument_backup': False, 'mode': mode, 'originals': {},
              'warnings': ['Offline project set only: settings, presets, samples and hardware capture are not represented.',
                           'Virtual pool bytes are not a contiguous physical flash image or an installer.',
                           'No runtime adoption, engine normalization or legacy fourth-slot remapping is performed.']}
    try:
        if mode == 'export':
            pool = snapshot(source, destination / 'original-pool.bin', POOL_BYTES)
            raw_live = snapshot(live, destination / 'original-live.bin', LIMIT)
            present, stored, inventory = inspect_pool(adapter, destination / 'original-pool.bin')
            objects = [raw_live, *stored]
            archive = {'format': 'dabbl8-project-set', 'version': 1, 'limits': LIMITS,
                       'present': present, 'objects': [{'role': role, 'blob': blob(raw, i == 0 or bool(present & (1 << (i - 1))))}
                                                     for i, (role, raw) in enumerate(zip(ROLES, objects))],
                       'original_pool': blob(pool), 'source': {'pool_name': Path(source).name, 'live_name': Path(live).name}}
            archive_bytes = (json.dumps(archive, sort_keys=True, separators=(',', ':')) + '\n').encode()
            parse_archive(archive_bytes)
        elif mode == 'import':
            archive_bytes = snapshot(source, destination / 'original.d8bak', ARCHIVE_LIMIT)
            archive, objects, pool = parse_archive(archive_bytes)
            (destination / 'original-pool.bin').write_bytes(pool)
            (destination / 'original-live.bin').write_bytes(objects[0])
            present, stored, inventory = inspect_pool(adapter, destination / 'original-pool.bin')
            if present != archive['present'] or stored != objects[1:]:
                raise ValueError('Archive payloads disagree with the original pool provenance')
        else:
            raise ValueError('Unsupported archive operation')
        # Snapshot every decoded original before structural/ref validation.
        for role, raw in zip(ROLES, objects):
            (destination / f'original-{role}.bin').write_bytes(raw)
        with tempfile.TemporaryDirectory(prefix='.work-', dir=destination) as temp:
            temp = Path(temp)
            for role, raw in zip(ROLES, objects):
                if raw:
                    path = temp / f'{role}.d8p'
                    path.write_bytes(raw)
                    checked = ask(adapter, 'check', path, present & 7)
                    if checked.get('bytes') != len(raw):
                        raise ValueError('Adapter length mismatch')
            paths = [temp / f'{role}.d8p' if raw else '-' for role, raw in zip(ROLES[1:], objects[1:])]
            rebuilt = temp / 'proposed-virtual-pool.bin'
            result = ask(adapter, 'build', rebuilt, present, *paths)
            again, reloaded, _ = inspect_pool(adapter, rebuilt)
            if result.get('present') != present or again != present or reloaded != objects[1:]:
                raise ValueError('Whole project-set exact-byte readback failed')
            (temp / 'project-set.d8bak').write_bytes(archive_bytes)
            report['objects'] = [{'role': role, 'present': i == 0 or bool(present & (1 << (i - 1))),
                                  'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest(),
                                  **(metadata(raw) if raw else {})} for i, (role, raw) in enumerate(zip(ROLES, objects))]
            report['present'] = present
            report['source_inventory'] = inventory
            report['adapter_sha256'] = hashlib.sha256(Path(adapter).read_bytes()).hexdigest()
            report['archive_sha256'] = hashlib.sha256(archive_bytes).hexdigest()
            report['proposed_pool_sha256'] = hashlib.sha256(rebuilt.read_bytes()).hexdigest()
            # One publication after ALL validation/readback, including sparse absence.
            temp.rename(destination / 'verified')
            report['accepted'] = True
    except (OSError, ValueError, KeyError, TypeError, UnicodeError, binascii.Error, struct.error, RecursionError) as error:
        report['reason'] = str(error)
        report.pop('objects', None)
    for path in destination.glob('original*'):
        if path.is_file():
            raw = path.read_bytes()
            report['originals'][path.name] = {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}
    (destination / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('export', 'import'))
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path, help='New private directory; existing paths refuse')
    parser.add_argument('--live', type=Path, help='Required exact live D8P1 snapshot for export')
    parser.add_argument('--adapter', type=Path, default=Path('build/host/dabbl8_project_archive'))
    args = parser.parse_args()
    if args.mode == 'export' and args.live is None:
        parser.error('export requires --live; persisted autosave is never substituted')
    report = prepare(args.mode, args.source, args.destination, args.adapter, args.live)
    print(json.dumps(report, indent=2))
    return 0 if report['accepted'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
