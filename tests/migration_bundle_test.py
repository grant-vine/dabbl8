#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Synthetic complete archives and real C conversion/pool writer, no device I/O."""
import base64
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import dabbl8_migration_bundle as migration
converter, initializer = map(lambda p: Path(p).resolve(), sys.argv[1:3])
checks = 0


def check(condition, label):
    global checks
    checks += 1
    if not condition:
        raise AssertionError(label)


def sum_project(data):
    h = 2166136261
    for byte in data[:-4]:
        h = ((h ^ byte) * 16777619) & 0xffffffff
    data[-4:] = struct.pack('<I', h)
    return bytes(data)


original = (ROOT / 'tests/fixtures/projects/fun9.bin').read_bytes()
# Historical fixture references slots 0 and 3. Redirect explicitly for this synthetic
# accepted input only; immutable original fixtures and golden evidence stay untouched.
project = bytearray(original)
project[68 + 4 * (99 + 2 + 64 * 9) + 6] = 1
project = sum_project(project)
other = bytearray(project)
other[16] ^= 1
other = sum_project(other)
IDS = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 32, 33, 34]


def archive(projects=None, current=project, sample=b''):
    values = {0: current, 1: b'synthetic-settings', 2: project, 3: project, 4: project,
              5: original, 6: b'synthetic-presets', 7: b'other-presets', 8: b'',
              9: b'fm6-presets', 32: sample, 33: b'', 34: b''}
    if projects:
        values.update(projects)
    return {'format': 'felucca-backup', 'version': 1, 'objects': [
        {'id': i, 'size': len(values[i]), 'crc': zlib.crc32(values[i]),
         'data': base64.b64encode(values[i]).decode()} for i in IDS]}


def sector(copy, sequence, body, kind=9):
    data = bytearray(b'\xff' * 4096)
    header = struct.pack('<IHH5I', 0x554C4546, kind, copy, sequence, len(body), zlib.crc32(body), 0, 0)
    data[:32] = header + struct.pack('<I', zlib.crc32(header))
    data[256:256 + len(body)] = body
    return bytes(data)


with tempfile.TemporaryDirectory() as directory:
    temp = Path(directory)
    src, raw = temp / 'backup.fm1bak', temp / 'autosave.bin'
    sample = bytearray(514)
    struct.pack_into('<IH', sample, 0, 0x504d5346, 1); sample[6] = 1
    sample[512:] = b'\x12\x34'
    struct.pack_into('<II', sample, 16, 2, zlib.crc32(sample[512:]))
    struct.pack_into('<5I', sample, 32, 0, 4, 0, 3, 32768); sample[58] = 127
    src.write_text(json.dumps(archive(sample=bytes(sample))))
    # Persisted autosave must win over the current live snapshot.
    raw.write_bytes(sector(0, 4, project) + sector(1, 5, other))
    source_hashes = [hashlib.sha256(p.read_bytes()).hexdigest() for p in (src, raw)]
    result = migration.prepare(src, raw, temp / 'accepted', converter, initializer)
    check(result['accepted'], result.get('reason', 'complete proposal accepted'))
    dst = temp / 'accepted'
    check(source_hashes == [hashlib.sha256(p.read_bytes()).hexdigest() for p in (src, raw)], 'source archive and raw sectors untouched')
    check((dst / 'original-object-32.bin').read_bytes() == bytes(sample), 'validated ADPCM sample retained byte exact')
    repeat = migration.prepare(src, raw, temp / 'repeat', converter, initializer)
    check(repeat['accepted'] and (temp / 'repeat/proposed-virtual-pool.bin').read_bytes() == (dst / 'proposed-virtual-pool.bin').read_bytes(), 'identical input yields identical virtual pool bytes')
    check(result['autosave_selection']['copy'] == 1 and result['autosave_selection']['sequence'] == 5, 'newest persisted autosave selected')
    check((dst / 'original-selected-autosave.bin').read_bytes() == other, 'live project does not replace persisted autosave')
    check((dst / 'original-object-5.bin').read_bytes() == original and not (dst / 'project-3.d8p').exists(), 'fourth original retained without alias')
    for i in IDS:
        check((dst / result['objects'][str(i)]['file']).exists(), 'all original objects including empty presets/samples retained')
    check((dst / 'original.fm1bak').read_bytes() == src.read_bytes(), 'archive byte exact')
    check((dst / 'original-autosave-pair.bin').read_bytes() == raw.read_bytes(), 'both sectors byte exact')
    check(result['available_project_mask'] == 7 and result['pool']['writer_readback']['present'] == 15, 'three projects plus one autosave')
    image = (dst / 'proposed-virtual-pool.bin').read_bytes()
    check(len(image) == 40960 and image[-8192:] == b'\xff' * 8192, 'one shared spare left erased')
    check(result['pool']['sha256'] == hashlib.sha256(image).hexdigest(), 'pool hash recorded')
    for obj in range(4):
        block = image[obj * 8192:(obj + 1) * 8192]
        name = f'project-{obj}.d8p' if obj < 3 else 'autosave.d8p'
        wire = (dst / name).read_bytes()
        check(block[:4] == b'D8SP' and struct.unpack_from('<I', block, 8)[0] == obj, 'native header object identity')
        check(block[256:256 + len(wire)] == wire, 'real writer preserves converted object')
    check(not result['device_writes'] and not result['migration_executed'] and result['review_required'], 'offline review contract')
    check(json.loads((dst / 'report.json').read_text()) == result, 'report retained')
    try:
        migration.prepare(src, raw, dst, converter, initializer)
    except FileExistsError:
        check(True, 'existing bundle untouched')
    else:
        raise AssertionError('existing output overwritten')

    cases = [
        ('missing', None, archive()),
        ('short', b'\xff' * 8191, archive()),
        ('trailing', b'\xff' * 8193, archive()),
        ('foreign', sector(0, 4, project, 7) + sector(1, 5, other), archive()),
        ('ambiguous', sector(0, 4, project) + sector(1, 4, other), archive()),
        ('half-range', sector(0, 0, project) + sector(1, 0x80000000, other), archive()),
        ('corrupt', b'\0' * 8192, archive()),
        ('fourth-ref-live', b'\xff' * 8192, archive(current=original)),
        ('fourth-ref-slot', b'\xff' * 8192, archive({3: original})),
        ('fourth-ref-autosave', sector(0, 1, original) + b'\xff' * 4096, archive()),
        ('missing-ref', b'\xff' * 8192, archive({3: b''})),
        ('bad-project', b'\xff' * 8192, archive({4: b'broken'})),
        ('bad-fourth', b'\xff' * 8192, archive({5: b'broken'})),
        ('bad-sample', b'\xff' * 8192, archive(sample=b'not-a-sample')),
    ]
    bad_crc = archive(); bad_crc['objects'][0]['crc'] ^= 1
    cases.append(('archive-crc', b'\xff' * 8192, bad_crc))
    old_archive = archive(); del old_archive['objects'][9]
    cases.append(('incomplete-archive', b'\xff' * 8192, old_archive))
    for name, snapshot, contents in cases:
        src.write_text(json.dumps(contents))
        if snapshot is not None:
            raw.write_bytes(snapshot)
        dest = temp / name
        result = migration.prepare(src, None if snapshot is None else raw, dest, converter, initializer)
        check(not result['accepted'] and bool(result['reason']), name + ': explicit refusal')
        check(not list(dest.glob('*.d8p')) and not (dest / 'proposed-virtual-pool.bin').exists(), name + ': no usable converted outputs')
        check((dest / 'original.fm1bak').read_bytes() == src.read_bytes(), name + ': archive retained')
        if snapshot is not None:
            check((dest / 'original-autosave-pair.bin').read_bytes() == snapshot, name + ': both raw originals retained')
        if name.startswith('fourth-ref') or name == 'bad-project':
            check(len(list(dest.glob('original-object-*.bin'))) == 13, name + ': all decoded originals retained before refusal')

    # Generation wrap, corruption fallback, identical generation copies, erased pair.
    corrupted = bytearray(sector(1, 20, other)); corrupted[256] ^= 1
    for name, data, expected, copy in [
        ('wrap', sector(0, 0xffffffff, project) + sector(1, 0, other), other, 1),
        ('fallback', sector(0, 4, project) + bytes(corrupted), project, 0),
        ('equal', sector(0, 4, project) + sector(1, 4, project), project, 0),
        ('erased', b'\xff' * 8192, project, None),
    ]:
        src.write_text(json.dumps(archive())); raw.write_bytes(data)
        dest = temp / name
        result = migration.prepare(src, raw, dest, converter, initializer)
        check(result['accepted'], name + ': recoverable proposal')
        check((dest / 'original-selected-autosave.bin').read_bytes() == expected, name + ': exact selected snapshot')
        check(result['autosave_selection'].get('copy') == copy, name + ': source reported')
    # Original third-slot references keep identity 2; they must never alias to 1/3.
    third = bytearray(project); third[2776 + 6] = 2; third = sum_project(third)
    src.write_text(json.dumps(archive({2: third, 3: third, 4: third}, current=third)))
    raw.write_bytes(sector(0, 8, third) + b'\xff' * 4096)
    result = migration.prepare(src, raw, temp / 'third-slot', converter, initializer)
    check(result['accepted'], result.get('reason', 'third native slot accepted'))
    for name in ['live.d8p', 'project-0.d8p', 'project-1.d8p', 'project-2.d8p', 'autosave.d8p']:
        meta = result['conversions'][name]
        check(meta['referenced_project_mask'] == 5 and meta['slot_to_scene'][2] != meta['slot_to_scene'][0], 'third original identity remains independently referenced')
    # Initializer itself refuses unavailable references, invalid masks, bad wire
    # and existing output rather than bypassing the bundle validator.
    valid_wire = dst / 'project-0.d8p'
    for mask, args in [('8', [str(valid_wire)] * 4), ('0', [str(valid_wire)] * 4),
                       ('7', ['-'] * 3 + [str(valid_wire)]), ('7', [str(src)] * 4)]:
        target = temp / ('initializer-refused-' + str(checks))
        result = subprocess.run([str(initializer), str(target), mask, *args], capture_output=True)
        check(result.returncode != 0 and not target.exists(), 'initializer refuses before creating a partial image')
    target = dst / 'proposed-virtual-pool.bin'
    before = target.read_bytes()
    result = subprocess.run([str(initializer), str(target), '7', *[str(dst / f'project-{i}.d8p') for i in range(3)], str(dst / 'autosave.d8p')], capture_output=True)
    check(result.returncode != 0 and target.read_bytes() == before, 'initializer never overwrites an existing proposal')
    # Empty all native slots initializes only autosave with a genuine no-reference project.
    standalone = bytearray(project); standalone[2776] = 0; standalone = sum_project(standalone)
    src.write_text(json.dumps(archive({2: b'', 3: b'', 4: b'', 5: b''}, current=standalone)))
    raw.write_bytes(b'\xff' * 8192)
    result = migration.prepare(src, raw, temp / 'empty-native', converter, initializer)
    check(result['accepted'] and result['pool']['writer_readback']['present'] == 8, result.get('reason', 'empty native slots retain autosave'))

print(f'Migration bundle: {checks} checks, 0 failures; synthetic archives, actual C conversion/pool readback; no device I/O')
