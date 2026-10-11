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


class ProfileConversionError(ValueError):
    """A structured optional-profile refusal, distinct from ordinary conversion."""
    def __init__(self, metadata):
        super().__init__(metadata.get('reason', 'Profile conversion refused the input'))
        self.metadata = metadata


def profile_mask_value(mask):
    if type(mask) is not int or not 0 < mask <= 16383:
        raise ValueError('Engine mask must be an integer from 1 through 16383')
    return mask


def run_converter(executable, mode, original, mask, engine_mask=None):
    command = [str(Path(executable).resolve()), mode, str(original.resolve()), str(mask)]
    if mode == 'legacy-profile-v1':
        command.append(str(profile_mask_value(engine_mask)))
    elif engine_mask is not None:
        raise ValueError('Engine mask requires explicit legacy-profile-v1 mode')
    p = subprocess.run(command, capture_output=True)
    meta = json.loads((p.stdout if mode == 'check' else p.stderr).decode('utf-8'))
    if not isinstance(meta, dict):
        raise ValueError('Unrecognized converter result')
    if meta.get('format') != 'dabbl8-project-conversion' or meta.get('version') != 1:
        raise ValueError('Unrecognized converter result')
    if mode == 'legacy-profile-v1':
        code = meta.get('profile_status_code')
        statuses = ['ok', 'bad_data', 'invalid_mask', 'unavailable_engine', 'unsupported_source_engine', 'bad_argument']
        if (type(meta.get('profile_schema')) is not int or meta.get('profile_schema') != 1 or
                type(meta.get('engine_mask')) is not int or meta.get('engine_mask') != engine_mask or
                type(meta.get('available_project_mask')) is not int or meta.get('available_project_mask') != mask or
                type(meta.get('accepted')) is not bool or meta.get('device_writes') is not False or
                meta.get('firmware_package') is not False or
                meta.get('experimental_offline_profile') is not True or meta.get('qualified_firmware') is not False or
                type(code) is not int or not 0 <= code < len(statuses) or meta.get('profile_status') != statuses[code] or
                (meta.get('accepted') is True and code != 0) or (meta.get('accepted') is not True and code == 0)):
            raise ValueError('Unrecognized profile-v1 converter result')
    if p.returncode or meta.get('accepted') is not True:
        if mode == 'legacy-profile-v1':
            raise ProfileConversionError(meta)
        raise ValueError(meta.get('reason', 'Converter refused the input'))
    if mode == 'legacy-profile-v1':
        if (type(meta.get('track_count')) is not int or meta.get('track_count') != 8 or
                type(meta.get('shared_voices')) is not int or meta.get('shared_voices') != 8):
            raise ValueError('Invalid profile-v1 track/voice metadata')
        defaults = meta.get('inserted_defaults')
        if (meta.get('profile_status_code') != 0 or meta.get('profile_status') != 'ok' or
                not isinstance(defaults, list) or len(defaults) != 4):
            raise ValueError('Invalid profile-v1 default metadata')
        for track, item in zip(range(5, 9), defaults):
            if not isinstance(item, dict):
                raise ValueError('Invalid profile-v1 inserted-track defaults')
            engine, preset = item.get('engine'), item.get('preset')
            if (type(item.get('track')) is not int or item.get('track') != track or
                    type(item.get('requested_engine')) is not int or item.get('requested_engine') != 0 or
                    type(engine) is not int or not 0 <= engine < 14 or not engine_mask & (1 << engine) or
                    type(preset) is not int or not 0 <= preset <= 255 or
                    item.get('fallback') is not (engine != 0) or
                    (engine == 0 and (preset != 255 or item.get('default_kind') != 'historical_deferred')) or
                    (engine != 0 and (preset != 0 or item.get('default_kind') != 'factory_preset_0'))):
                raise ValueError('Invalid profile-v1 inserted-track defaults')
    return p.stdout, meta


def prepare(source, destination, converter, references=None, *, engine_mask=None):
    if engine_mask is not None:
        profile_mask_value(engine_mask)
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
    if engine_mask is not None:
        report.update({'profile_schema': 1, 'engine_mask': engine_mask,
                       'experimental_offline_profile': True, 'qualified_firmware': False})
    mode = 'legacy-profile-v1' if engine_mask is not None else 'legacy'
    profile_args = () if engine_mask is None else (engine_mask,)
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
            data, meta = run_converter(converter, mode, original_ref, mask, *profile_args)
            pending.append((f'slot-{slot}.d8p', data))
            report['references'][str(slot)]['conversion'] = meta
        data, meta = run_converter(converter, mode, original, mask, *profile_args)
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
        if isinstance(error, ProfileConversionError):
            report['profile_refusal'] = error.metadata
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
    p.add_argument('--experimental-engine-mask-v1', type=int, help='Optional offline engine mask; not qualified reduced firmware')
    p.add_argument('--reference', action='append', default=[], metavar='SLOT=FILE', help='Actual saved project for original slot 0..3')
    args = p.parse_args()
    if args.experimental_engine_mask_v1 is not None:
        try:
            profile_mask_value(args.experimental_engine_mask_v1)
        except ValueError as error:
            p.error(str(error))
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
    report = prepare(args.source, args.destination, args.converter, references, engine_mask=args.experimental_engine_mask_v1)
    print(json.dumps(report, indent=2))
    return 0 if report['accepted'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
