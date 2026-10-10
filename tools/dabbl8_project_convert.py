#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Preserve legacy originals and produce a verified offline proposed D8P1 bundle."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def snapshot(source, destination):
    digest = hashlib.sha256()
    size = 0
    with Path(source).open('rb') as src, destination.open('xb') as dst:
        for block in iter(lambda: src.read(65536), b''):
            dst.write(block)
            digest.update(block)
            size += len(block)
    return {'original_file': destination.name, 'original_bytes': size, 'original_sha256': digest.hexdigest(), 'source_name': Path(source).name}


def run_converter(executable, mode, original, mask):
    p = subprocess.run([str(Path(executable).resolve()), mode, str(original.resolve()), str(mask)], capture_output=True)
    meta = json.loads((p.stdout if mode == 'check' else p.stderr).decode('utf-8'))
    if meta.get('format') != 'dabbl8-project-conversion' or meta.get('version') != 1:
        raise ValueError('Unrecognized converter result')
    if p.returncode or meta.get('accepted') is not True:
        raise ValueError(meta.get('reason', 'Converter refused the input'))
    return p.stdout, meta


def prepare(source, destination, converter, references=None):
    references = dict(references or {})
    if any(type(slot) is not int or slot < 0 or slot > 3 for slot in references):
        raise ValueError('Reference slots must be unique integers 0 through 3')
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=False)
    report = {'format': 'dabbl8-project-bundle-report', 'version': 1, 'accepted': False,
              'device_writes': False, 'firmware_package': False, 'references': {},
              'warnings': ['D8P1 is a proposed project format, not firmware or a qualified device upload.',
                           'Every original is retained byte for byte; conversion is semantic and may normalize reserved or inactive data.',
                           'DIGITAL/SAMPLE-PERC/PHYS migrations use the pinned importer; incompatible engine automation removals are reported.',
                           'Chain references require supplied, converted project snapshots; absent slots are never aliased.',
                           'No flash map, runtime arrangement playback, backup transport or hardware recovery is qualified.']}
    try:
        original = destination / 'original.bin'
        report.update(snapshot(source, original))
        mask = sum(1 << slot for slot in references)
        report['available_project_mask'] = mask
        originals = {}
        # Preserve every supplied reference before validating any conversion.
        for slot, path in sorted(references.items()):
            name = f'slot-{slot}-original.bin'
            originals[slot] = destination / name
            report['references'][str(slot)] = snapshot(path, originals[slot])
        pending = []
        for slot, original_ref in originals.items():
            data, meta = run_converter(converter, 'legacy', original_ref, mask)
            pending.append((f'slot-{slot}.d8p', data))
            report['references'][str(slot)]['conversion'] = meta
        data, meta = run_converter(converter, 'legacy', original, mask)
        pending.append(('project.d8p', data))
        # No converted artifact is published until the complete bundle validates.
        for name, data in pending:
            if not data.startswith(b'D8P1') or not 32 <= len(data) <= 7936:
                raise ValueError('Converter emitted an invalid bounded project')
        for name, data in pending:
            path = destination / name
            with path.open('xb') as output:
                output.write(data)
            _, checked = run_converter(converter, 'check', path, mask)
            canonical, _ = run_converter(converter, 'canonical', path, mask)
            if canonical != data:
                raise ValueError('Converted project is not canonical')
            entry = report if name == 'project.d8p' else report['references'][name.split('-')[1].split('.')[0]]
            entry.update({'converted_file': name, 'converted_bytes': len(data), 'converted_sha256': hashlib.sha256(data).hexdigest(), 'verification': checked})
        report['conversion'] = meta
        report['accepted'] = True
    except (OSError, ValueError, KeyError, UnicodeError) as error:
        report['reason'] = str(error)
        # Only files created by this operation in its exclusive new directory.
        for path in destination.glob('*.d8p'):
            path.unlink()
        for key in ['converted_file', 'converted_bytes', 'converted_sha256', 'verification']:
            report.pop(key, None)
        for entry in report['references'].values():
            for key in ['converted_file', 'converted_bytes', 'converted_sha256', 'verification']:
                entry.pop(key, None)
    with (destination / 'report.json').open('x', encoding='utf-8') as output:
        json.dump(report, output, indent=2)
        output.write('\n')
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path)
    p.add_argument('destination', type=Path, help='New directory; existing paths are refused')
    p.add_argument('--converter', type=Path, default=Path('build/host/dabbl8_project_convert'))
    p.add_argument('--reference', action='append', default=[], metavar='SLOT=FILE', help='Actual saved project for original slot 0..3')
    args = p.parse_args()
    references = {}
    for item in args.reference:
        try:
            slot, filename = item.split('=', 1)
            slot = int(slot)
            if not 0 <= slot <= 3 or not filename:
                raise ValueError()
        except ValueError:
            p.error('References must be SLOT=FILE with slot 0 through 3')
        if slot in references:
            p.error('Duplicate reference slot')
        references[slot] = Path(filename)
    report = prepare(args.source, args.destination, args.converter, references)
    print(json.dumps(report, indent=2))
    return 0 if report['accepted'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
