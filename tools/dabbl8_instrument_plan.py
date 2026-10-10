#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Derive a verified offline native pool from complete retained instrument originals."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import zlib
from dabbl8_instrument_migrate import (FILES, ROLE_LIMITS, bounded_file, digest,
                                      retain_archive, strict_json)
from dabbl8_project_archive import inspect_pool

FUN_SIZES = (688, 2552, 2584, 2680, 3352, 3388, 3388, 3584, 3648)


def frame(data):
    if len(data) < 12:
        raise ValueError('Truncated frozen FUN frame')
    tag, size = struct.unpack_from('<II', data)
    index = tag - 0x46554e31
    if not 0 <= index < 9 or len(data) != FUN_SIZES[index] or size != len(data):
        raise ValueError('Unsupported frozen FUN tag/extent')
    fnv = 2166136261
    for byte in data[:-4]:
        fnv = ((fnv ^ byte) * 16777619) & 0xffffffff
    if struct.unpack_from('<I', data, len(data)-4)[0] != fnv:
        raise ValueError('Damaged frozen FUN checksum')
    return index + 1


def recover(data, role):
    """Conservative source recovery; no torn nonerased sector is ignored."""
    if len(data) != 8192 or type(role) is not int or not 0 <= role < 5:
        raise ValueError('Invalid frozen project pair role/extent')
    records = []
    for slot in range(2):
        sector = data[slot*4096:(slot+1)*4096]
        if sector == b'\xff'*4096:
            continue
        magic, kind, physical, seq, n, crc, reserved0, reserved1, hcrc = struct.unpack('<IHH6I', sector[:32])
        if magic != 0x554c4546 or kind != (9 if role == 4 else role+1) or physical != slot or n > 3840 or zlib.crc32(sector[:28]) != hcrc:
            raise ValueError('Unsupported, foreign or torn nonerased legacy sector')
        body = sector[256:256+n]
        if zlib.crc32(body) != crc:
            raise ValueError('Damaged legacy body checksum')
        fmt = frame(body)
        records.append((slot, seq, body, fmt))
    if not records:
        return None, {'status': 'erased', 'role': role}
    selected = records[0]
    if len(records) == 2:
        delta = (records[1][1] - records[0][1]) & 0xffffffff
        if delta in (0, 0x80000000):
            raise ValueError('Equal or half-range legacy generations are ambiguous')
        if delta < 0x80000000:
            selected = records[1]
    return selected[2], dict(status='recovered', role=role, copy=selected[0],
                            sequence=selected[1], valid_copies=len(records),
                            source_format='FUN'+str(selected[3]), bytes=len(selected[2]),
                            sha256=digest(selected[2]))


def convert(executable, mode, path, mask):
    p = subprocess.run([str(Path(executable).resolve()), mode, str(path.resolve()), str(mask)],
                       capture_output=True, timeout=30)
    metadata = p.stdout if mode == 'check' else p.stderr
    if len(metadata) > 16384 or len(p.stdout) > 16384:
        raise ValueError('Converter response exceeds bounded size')
    result = strict_json(metadata.decode('utf-8'))
    if type(result) is not dict or result.get('format') != 'dabbl8-project-conversion' or type(result.get('version')) is not int or result['version'] != 1 or result.get('accepted') is not True or result.get('device_writes') is not False or type(result.get('available_project_mask')) is not int or result.get('available_project_mask') != mask or p.returncode:
        raise ValueError('Actual converter refused source/context or returned unsupported evidence')
    return p.stdout, result


def prepare(source, destination, converter, initializer, adapter, autosave_source):
    snapshot, inputs = retain_archive(source, destination)
    destination = Path(destination)
    report = dict(format='dabbl8-source-derived-instrument-plan', version=1, accepted=False,
                  source_encoded_conversion_verified=False, source_runtime_musical_equivalence=False,
                  device_writes=False, production_ownership=False, hardware_qualified=False,
                  migration_executed=False, restore_qualified=False, review_required=True,
                  autosave_source=autosave_source, source_manifest_sha256=inputs.get('manifest_sha256'),
                  warnings=['Complete post-boot originals remain retained; this is not a pre-boot backup.',
                            'Frozen conversion can normalize inactive/reserved bytes and reports engine/motion changes.',
                            'Original fourth manual slot is archival only; references are never remapped.',
                            'Encoded conversion/readback establishes source derivation, not runtime musical or device equivalence.',
                            'No current RAM adoption, persistence polling release or production ownership is granted.'])
    staging = destination/'private-staging'
    try:
        if snapshot is None:
            raise ValueError(inputs.get('reason', 'Complete instrument archive refused'))
        if autosave_source not in ('persisted', 'current'):
            raise ValueError('Explicit autosave source persisted/current required')
        retained = destination/'retained'
        tools = {key: Path(value).resolve() for key, value in [('converter',converter), ('initializer',initializer), ('adapter',adapter)]}
        report['tools'] = {key: {'name': path.name, 'sha256': digest(bounded_file(path, 32*1024*1024))} for key,path in tools.items()}
        selections = [recover(snapshot['roles'][r], r) for r in range(5)]
        report['legacy_sources'] = [item[1] for item in selections]
        mask = sum(1<<r for r in range(3) if selections[r][0] is not None)
        original_mask = mask | (8 if selections[3][0] is not None else 0)
        report['available_project_mask'] = mask
        report['original_project_mask'] = original_mask
        staging.mkdir()
        pending = {}
        report['conversions'] = {}
        for role, (body, metadata) in enumerate(selections):
            if body is None:
                continue
            path = destination/f'selected-legacy-role-{role}.bin'
            path.write_bytes(body)
            # Validate the selected generation of every nonempty pair with the C importer;
            # older generations received frozen frame/FNV checks during recovery.
            # including unselected persisted autosave and archival slot four.
            context = original_mask if role == 3 or (role == 4 and autosave_source == 'current') else mask
            data, meta = convert(tools['converter'], 'legacy', path, context)
            if bounded_file(path,3840) != body:
                raise ValueError('Selected source changed during conversion')
            report['conversions'][str(role)] = dict(source_file=path.name, source_sha256=digest(body), metadata=meta,
                                                   encoded_sha256=digest(data), encoded_bytes=len(data), archived_only=role==3)
            converted = staging/f'legacy-role-{role}.d8p'; converted.write_bytes(data)
            canonical, unused = convert(tools['converter'], 'canonical', converted, context)
            unused, checked = convert(tools['converter'], 'check', converted, context)
            if bounded_file(converted,7936) != data:
                raise ValueError('Converted source changed during canonical/check validation')
            if canonical != data or not data.startswith(b'D8P1') or not 32 <= len(data) <= 7936:
                raise ValueError('Source-derived conversion failed exact canonical verification')
            report['conversions'][str(role)]['verification'] = checked
            if role < 3:
                pending[role] = data
            elif role == 4 and autosave_source == 'persisted':
                pending[3] = data
        if autosave_source == 'persisted' and selections[4][0] is None:
            raise ValueError('Persisted autosave is proven erased; explicitly choose current, never implicit bootstrap')
        if autosave_source == 'current':
            live = retained/FILES[12]
            data, meta = convert(tools['converter'], 'canonical', live, mask)
            unused, checked = convert(tools['converter'], 'check', live, mask)
            if data != snapshot['roles'][12]:
                raise ValueError('Current native autosave is not exact canonical source bytes')
            pending[3] = data
            report['current_autosave'] = dict(role=12, source_sha256=digest(data), encoded_sha256=digest(data), bytes=len(data), verification=checked)
        for role,data in pending.items():
            (staging/f'object-{role}.d8p').write_bytes(data)
        image = staging/'proposed-native-pool.bin'
        slots = [str((staging/f'object-{r}.d8p').resolve()) if r in pending else '-' for r in range(3)]
        p = subprocess.run([str(tools['initializer']), str(image.resolve()), str(mask), *slots,
                            str((staging/'object-3.d8p').resolve())], capture_output=True, timeout=30)
        if p.returncode or len(p.stdout)>4096:
            raise ValueError('Actual pool initializer refused complete converted set')
        initialized = strict_json(p.stdout.decode('utf-8'))
        if type(initialized) is not dict or set(initialized) != {'format','version','device_writes','present','bytes'} or type(initialized.get('version')) is not int or type(initialized.get('present')) is not int or type(initialized.get('bytes')) is not int or initialized.get('device_writes') is not False or initialized != {'format':'dabbl8-virtual-pool','version':1,'device_writes':False,'present':mask|8,'bytes':40960}:
            raise ValueError('Unsupported initializer result')
        pool = bounded_file(image,40960)
        present, objects, inventory = inspect_pool(tools['adapter'], image)
        if len(pool)!=40960 or type(present) is not int or present != mask|8 or any(objects[r] != pending.get(r,b'') for r in range(4)):
            raise ValueError('Actual C pool readback differs from exact source-derived set')
        if any(bounded_file(destination/f'selected-legacy-role-{r}.bin',3840) != body for r,(body,unused) in enumerate(selections) if body is not None) or any(bounded_file(staging/f'object-{r}.d8p',7936) != data for r,data in pending.items()):
            raise ValueError('Selected or converted source artifacts changed during pool preparation')
        # Retained inputs and fixed executable identities must survive execution.
        if bounded_file(retained/'manifest.json',16384) != snapshot['manifest_bytes'] or any(bounded_file(retained/name, ROLE_LIMITS[r]) != snapshot['roles'][r] for r,name in enumerate(FILES)) or any(digest(bounded_file(path,32*1024*1024)) != report['tools'][key]['sha256'] for key,path in tools.items()):
            raise ValueError('Retained originals or conversion tools changed')
        # Publish derived files only after whole-set conversion and actual C readback.
        for role,data in pending.items():
            (destination/f'object-{role}.d8p').write_bytes(data)
            if bounded_file(destination/f'object-{role}.d8p',7936) != data:
                raise ValueError('Published source-derived object readback mismatch')
        output = destination/'proposed-native-pool.bin'; output.write_bytes(pool)
        if bounded_file(output,40960) != pool:
            raise ValueError('Published source-derived pool readback mismatch')
        report['objects'] = {str(role):{'file':f'object-{role}.d8p','bytes':len(data),'sha256':digest(data)} for role,data in pending.items()}
        report['pool'] = dict(file=output.name, bytes=len(pool), sha256=digest(pool), actual_c_inventory=inventory,
                              initializer_readback=initialized)
        report['source_encoded_conversion_verified'] = True
        report['accepted'] = True
    except (OSError, ValueError, TypeError, KeyError, UnicodeError, struct.error, subprocess.TimeoutExpired) as error:
        report['reason'] = str(error)
        for path in [*destination.glob('object-*.d8p'), destination/'proposed-native-pool.bin']:
            path.unlink(missing_ok=True)
        report.pop('objects',None); report.pop('pool',None)
    (destination/'plan-report.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source',type=Path); p.add_argument('destination',type=Path)
    p.add_argument('--autosave-source',choices=('persisted','current'),required=True)
    p.add_argument('--converter',type=Path,required=True)
    p.add_argument('--initializer',type=Path,required=True)
    p.add_argument('--adapter',type=Path,required=True)
    a=p.parse_args()
    try:
        report=prepare(a.source,a.destination,a.converter,a.initializer,a.adapter,a.autosave_source)
    except FileExistsError:
        p.error('destination exists; originals are never overwritten')
    print(json.dumps(report,indent=2));return 0 if report['accepted'] else 1

if __name__ == '__main__':
    raise SystemExit(main())
