#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Validate a candidate engine card; emit configuration, never firmware."""
import argparse
import json
import re
from pathlib import Path
REGISTRY = Path(__file__).resolve().parents[1] / 'firmware/src/d8card.def'


def registry():
    rows = re.findall(r'^D8_ENGINE\(([A-Z0-9]+), (\d+), (\d+)\)$', REGISTRY.read_text(), re.M)
    result = {name: (int(identity), int(deps)) for name, identity, deps in rows}
    if len(result) != 13 or len({v[0] for v in result.values()}) != 13 or sum(1 << v[0] for v in result.values()) != 0x3ffd:
        raise ValueError('Invalid stable engine registry')
    return result


def pairs(items):
    result = {}
    for key, value in items:
        if key in result:
            raise ValueError('Duplicate JSON key')
        result[key] = value
    return result


def load(path):
    with Path(path).open('rb') as source:
        raw = source.read(4097)
    if len(raw) > 4096:
        raise ValueError('Profile exceeds 4096 bytes')
    return json.loads(raw.decode('utf-8'), object_pairs_hook=pairs)


def resolve(profile):
    if not isinstance(profile, dict) or set(profile) != {'version', 'name', 'engines'}:
        raise ValueError('Expected version, name and engines only')
    if type(profile['version']) is not int or profile['version'] != 1:
        raise ValueError('Unsupported card schema')
    name = profile['name']
    if not isinstance(name, str) or not re.fullmatch(r'[a-z][a-z0-9-]{0,31}', name):
        raise ValueError('Card name must be 1–32 lowercase letters, digits or hyphens')
    selected = profile['engines']
    if not isinstance(selected, list) or not 1 <= len(selected) <= 13 or any(not isinstance(x, str) for x in selected):
        raise ValueError('Select 1–13 engine names')
    if len(set(selected)) != len(selected):
        raise ValueError('Duplicate engine')
    engines = registry()
    if any(x not in engines for x in selected):
        raise ValueError('Unknown or retired engine')
    selected = sorted(selected, key=lambda x: engines[x][0])
    mask = sum(1 << engines[x][0] for x in selected)
    components = 0
    for item in selected:
        components |= engines[item][1]
    return {'schema': 'dabbl8-card-candidate', 'version': 1, 'name': name,
            'engines': selected, 'engine_mask': mask, 'required_components': components,
            'tracks': 8, 'shared_sounding_voices': 8,
            'status': 'candidate', 'firmware_generated': False,
            'runtime_enforced': False, 'hardware_qualified': False}


def emit(profile, destination):
    result = resolve(profile)  # validate before creating a destination
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=False)
    (destination / 'card.json').write_text(json.dumps(result, indent=2) + '\n')
    (destination / 'card.h').write_text(
        '/* SPDX-License-Identifier: GPL-3.0-only */\n'
        '/* Candidate configuration only; shipping build does not consume these macros. */\n'
        '#ifndef DABBL8_REQUESTED_CARD_H\n#define DABBL8_REQUESTED_CARD_H\n'
        f'#define D8CARD_REQUESTED_ENGINE_MASK {result["engine_mask"]}u\n'
        f'#define D8CARD_REQUIRED_COMPONENTS {result["required_components"]}u\n#endif\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('profile', type=Path)
    parser.add_argument('destination', type=Path, help='New directory; existing paths refuse')
    args = parser.parse_args()
    try:
        result = emit(load(args.profile), args.destination)
    except (OSError, ValueError, UnicodeError) as error:
        parser.exit(1, f'Card refused: {error}\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
