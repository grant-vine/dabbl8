#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Prepare an original-preserving migration proposal; never access a device."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import zlib
from dabbl8_project_convert import run_converter
from dabbl8_storage_plan import plan

ROOT = Path(__file__).resolve().parents[1]


def autosave_snapshot(data):
    """Select the last CRC-valid legacy autosave, retaining both copies externally."""
    if len(data) != 8192:
        raise ValueError('Autosave snapshot must contain exactly two raw 4096-byte sectors')
    if data == b'\xff' * 8192:
        return None, {'status': 'erased', 'source': 'validated-live-backup'}
    valid = []
    for copy in range(2):
        sector = data[copy * 4096:(copy + 1) * 4096]
        magic, kind, slot, sequence, length, crc, _, _, hcrc = struct.unpack('<IHH6I', sector[:32])
        if zlib.crc32(sector[:28]) != hcrc:
            continue  # Torn header; a valid other copy may still be recovered.
        if magic != 0x554C4546 or kind != 9 or slot != copy:
            raise ValueError('Committed foreign or unsupported data occupies the legacy autosave pair')
        if length > 3840:
            raise ValueError('Committed autosave exceeds the legacy payload bounds')
        body = sector[256:256 + length]
        if zlib.crc32(body) == crc:
            valid.append((copy, sequence, body))
    if not valid:
        raise ValueError('Nonempty autosave pair has no recoverable CRC-valid record; it is not proven erased')
    selected = valid[0]
    if len(valid) == 2:
        delta = (valid[1][1] - valid[0][1]) & 0xffffffff
        if delta == 0x80000000 or (delta == 0 and valid[1][2] != valid[0][2]):
            raise ValueError('Autosave generations are ambiguous; retain both originals for review')
        if 0 < delta < 0x80000000:
            selected = valid[1]
    return selected[2], {'status': 'recovered', 'copy': selected[0], 'sequence': selected[1],
                         'valid_copies': len(valid), 'source': 'persisted-autosave'}


def preserve(path, output, maximum):
    with Path(path).open('rb') as f:
        data = f.read(maximum + 1)
    if len(data) > maximum:
        raise ValueError('Input exceeds bounded migration size; source remains untouched')
    output.write_bytes(data)  # Output is inside this operation's exclusive directory.
    return data, {'file': output.name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def prepare(backup, autosave, destination, converter, initializer, node='node'):
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=False)
    report = {'format': 'dabbl8-migration-proposal', 'version': 1, 'accepted': False,
              'device_writes': False, 'firmware_package': False, 'migration_executed': False,
              'hardware_qualified': False, 'review_required': True, 'originals': {}, 'objects': {},
              'warnings': ['A standard Felucca archive does not contain persisted autosave; a separate raw snapshot is mandatory.',
                           'Original slot 4 is archived only, never aliased into the three native slots.',
                           'The virtual image is a migration proposal, not an installer, firmware package or qualified restore path.',
                           'Capture and provenance of device snapshots, physical backend, interruption-safe migration and hardware recovery remain unqualified.']}
    try:
        _, report['originals']['backup'] = preserve(backup, destination / 'original.fm1bak', 2 * 1024 * 1024)
        if autosave is None:
            raise ValueError('A separate raw autosave snapshot is required; live music is not persisted autosave evidence')
        raw, report['originals']['autosave'] = preserve(autosave, destination / 'original-autosave-pair.bin', 16384)
        p = subprocess.run([node, str(ROOT / 'tools/dabbl8_backup_inspect.mjs'), str((destination / 'original.fm1bak').resolve())], capture_output=True)
        if p.returncode:
            raise ValueError(p.stderr.decode('utf8').strip() or 'Backup validation failed')
        inspection = json.loads(p.stdout)
        if inspection.get('format') != 'dabbl8-backup-inspection' or inspection.get('version') != 1:
            raise ValueError('Unsupported backup inspection result')
        objects = {}
        # Preserve every decoded archive object before attempting any conversion.
        for item in inspection['objects']:
            data = base64.b64decode(item['base64'], validate=True)
            name = f"original-object-{item['id']}.bin"
            (destination / name).write_bytes(data)
            objects[item['id']] = data
            report['objects'][str(item['id'])] = {'file': name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
        persisted, report['autosave_selection'] = autosave_snapshot(raw)
        if persisted is None:
            persisted = objects[0]  # Only an entirely erased pair allows this explicit initialization.
        (destination / 'original-selected-autosave.bin').write_bytes(persisted)
        report['autosave_selection'].update({'file': 'original-selected-autosave.bin',
            'bytes': len(persisted), 'sha256': hashlib.sha256(persisted).hexdigest()})
        mask = sum(1 << slot for slot in range(3) if objects[slot + 2])
        report['available_project_mask'] = mask
        report['archival_only_object'] = 5
        if objects[5]:
            # Verify the fourth original with its original identity/context, but
            # never emit a native pool slot or remap references to that identity.
            _, report['archival_validation'] = run_converter(
                converter, 'legacy', destination / 'original-object-5.bin', mask | 8)
        pending = [('live.d8p', destination / 'original-object-0.bin')]
        pending += [(f'project-{slot}.d8p', destination / f'original-object-{slot + 2}.bin') for slot in range(3) if mask & (1 << slot)]
        pending.append(('autosave.d8p', destination / 'original-selected-autosave.bin'))
        report['tools'] = {name: {'file': Path(executable).name,
            'sha256': hashlib.sha256(Path(executable).read_bytes()).hexdigest()}
            for name, executable in [('converter', converter), ('initializer', initializer)]}
        converted = []
        report['conversions'] = {}
        for name, source in pending:
            data, meta = run_converter(converter, 'legacy', source, mask)
            if not data.startswith(b'D8P1') or not 32 <= len(data) <= 7936:
                raise ValueError('Converter emitted an invalid bounded project')
            converted.append((name, data))
            report['conversions'][name] = meta
        for name, data in converted:
            path = destination / name
            path.write_bytes(data)
            canonical, _ = run_converter(converter, 'canonical', path, mask)
            _, checked = run_converter(converter, 'check', path, mask)
            if canonical != data:
                raise ValueError('Converted object failed canonical verification')
            report['conversions'][name].update({'file': name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(), 'verification': checked})
        image = destination / 'proposed-virtual-pool.bin'
        slots = [str((destination / f'project-{slot}.d8p').resolve()) if mask & (1 << slot) else '-' for slot in range(3)]
        p = subprocess.run([str(Path(initializer).resolve()), str(image.resolve()), str(mask), *slots,
                            str((destination / 'autosave.d8p').resolve())], capture_output=True)
        if p.returncode:
            raise ValueError('Real pool writer/reader refused the proposed initial set')
        result = json.loads(p.stdout)
        if result.get('format') != 'dabbl8-virtual-pool' or result.get('present') != mask | 8 or result.get('bytes') != 40960:
            raise ValueError('Pool initializer returned an unsupported result')
        data = image.read_bytes()
        if len(data) != 40960:
            raise ValueError('Virtual pool image has the wrong size')
        report['pool'] = {'file': image.name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
                          'writer_readback': result, 'geometry': plan()}
        report['accepted'] = True
    except (OSError, ValueError, KeyError, UnicodeError, struct.error) as error:
        report['reason'] = str(error)
        for path in [*destination.glob('*.d8p'), destination / 'proposed-virtual-pool.bin']:
            path.unlink(missing_ok=True)
        report.pop('pool', None)
        for item in report.get('conversions', {}).values():
            for key in ['file', 'bytes', 'sha256', 'verification']:
                item.pop(key, None)
    (destination / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('backup', type=Path)
    parser.add_argument('destination', type=Path, help='New private directory; existing paths refused')
    parser.add_argument('--autosave', type=Path, help='Raw two-sector legacy autosave snapshot; required for acceptance')
    parser.add_argument('--converter', type=Path, default=Path('build/host/dabbl8_project_convert'))
    parser.add_argument('--initializer', type=Path, default=Path('build/host/dabbl8_pool_initialize'))
    parser.add_argument('--node', default='node')
    args = parser.parse_args()
    report = prepare(args.backup, args.autosave, args.destination, args.converter, args.initializer, args.node)
    print(json.dumps(report, indent=2))
    return 0 if report['accepted'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
