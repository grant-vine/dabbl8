#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Real offline bundles, original preservation and whole-bundle refusal."""
from pathlib import Path
import hashlib
import importlib.util
import json
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('converter', root / 'tools/dabbl8_project_convert.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
exe = Path(sys.argv[1]).resolve()
fixtures = root / 'tests/fixtures/projects'
originals = sorted(p for p in fixtures.glob('*.bin') if '.expected-fun9' not in p.name)
assert len(originals) == 13
checks = 0

def check(yes, label):
    global checks
    checks += 1
    if not yes:
        raise AssertionError(label)

with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    refs = {i: fixtures / 'fun9.bin' for i in range(4)}
    pristine = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in fixtures.glob('*.bin')}
    for source in originals:
        dst = tmp / source.stem
        report = module.prepare(source, dst, exe, refs)
        check(report['accepted'], source.stem + ' converts into a validated proposed bundle')
        check((dst / 'original.bin').read_bytes() == source.read_bytes(), 'primary original exact')
        check(report['original_sha256'] == hashlib.sha256(source.read_bytes()).hexdigest(), 'primary original hash')
        check(not report['device_writes'] and not report['firmware_package'], 'offline project-only scope')
        check(report['conversion']['track_count'] == 8 and report['conversion']['shared_voices'] == 8, 'eight tracks retain shared voices')
        check(json.loads((dst / 'report.json').read_text()) == report, 'concrete report retained')
        for slot in range(4):
            row = report['references'][str(slot)]
            check((dst / row['original_file']).read_bytes() == refs[slot].read_bytes(), 'reference snapshot exact')
            check(row['original_sha256'] == hashlib.sha256(refs[slot].read_bytes()).hexdigest(), 'reference original hash')
            data = (dst / row['converted_file']).read_bytes()
            check(data[:4] == b'D8P1' and len(data) <= 7936 and row['converted_sha256'] == hashlib.sha256(data).hexdigest(), 'reference output bounded/hash verified')
        data = (dst / 'project.d8p').read_bytes()
        check(report['converted_sha256'] == hashlib.sha256(data).hexdigest(), 'primary proposed output hash')
        canonical = source.with_name(source.stem + '.expected-fun9.bin')
        expected, _ = module.run_converter(exe, 'legacy', canonical, 15)
        check(data == expected, 'new file equals unchanged upstream canonical legacy reference semantics')
        try:
            module.prepare(source, dst, exe, refs)
        except FileExistsError:
            check(True, 'existing bundle never overwritten')
        else:
            raise AssertionError('existing bundle overwritten')
    check(bool(json.loads((tmp / 'fun9-digital/report.json').read_text())['conversion']['engine_changes']), 'DIGITAL migration reported')
    check(bool(json.loads((tmp / 'fun4-phys-drum/report.json').read_text())['conversion']['engine_changes']), 'PHYS DRUM migration reported')
    # An actual chain cannot claim absent or invalid related projects.
    for label, context in [('missing', {}), ('missing-slot3', {0: refs[0]}), ('missing-slot0', {3: refs[3]})]:
        dst = tmp / label
        report = module.prepare(fixtures / 'fun9.bin', dst, exe, context)
        check(not report['accepted'] and not list(dst.glob('*.d8p')), 'unavailable references refuse the whole bundle')
        check((dst / 'original.bin').read_bytes() == (fixtures / 'fun9.bin').read_bytes(), 'refused primary retained')
    bad = tmp / 'bad-reference.bin'
    corrupted = bytearray(refs[0].read_bytes()); corrupted[100] ^= 1; bad.write_bytes(corrupted)
    context = dict(refs); context[3] = bad
    dst = tmp / 'bad-reference'
    report = module.prepare(fixtures / 'fun9.bin', dst, exe, context)
    check(not report['accepted'] and not list(dst.glob('*.d8p')), 'invalid reference refuses otherwise valid primary')
    check((dst / 'slot-3-original.bin').read_bytes() == corrupted, 'invalid reference original remains recoverable')
    check(len(list(dst.glob('*original.bin'))) == 5, 'all supplied originals saved before conversion refusal')
    good = (fixtures / 'fun9.bin').read_bytes()
    future = bytearray(good); future[:4] = (0x46554E41).to_bytes(4, 'little')
    for i, data in enumerate([b'', b'FUN', good[:-1], good + b'\0', bytes(future), b'x' * 9000, bytes(corrupted)]):
        src = tmp / f'refused-{i}.bin'; src.write_bytes(data)
        dst = tmp / f'refused-{i}'
        report = module.prepare(src, dst, exe, refs)
        check(not report['accepted'] and not list(dst.glob('*.d8p')), 'unsupported/damaged/oversize has no proposed output')
        check((dst / 'original.bin').read_bytes() == data, 'every refused input retained byte exact')
        check(report['original_sha256'] == hashlib.sha256(data).hexdigest(), 'refused original hash')
    # Preserve original DIGITAL automation even when the migrated engine drops it.
    changed = bytearray((fixtures / 'fun9-digital.bin').read_bytes())
    motion_offset = 68 + 4 * (99 + 2 + 64 * 9) + 36
    changed[motion_offset:motion_offset + 260] = bytes(260)
    changed[motion_offset:motion_offset + 4] = bytes([2, 1, 0, 0])
    changed[motion_offset + 4:motion_offset + 8] = bytes([1, 91, 64, 0])
    changed[motion_offset + 8:motion_offset + 12] = bytes([255, 0x87, 224, 255])
    fnv = 2166136261
    for byte in changed[:-4]:
        fnv = ((fnv ^ byte) * 16777619) & 0xffffffff
    changed[-4:] = fnv.to_bytes(4, 'little')
    src = tmp / 'digital-motion.bin'; src.write_bytes(changed)
    dst = tmp / 'digital-motion'
    report = module.prepare(src, dst, exe, refs)
    check(report['accepted'] and report['conversion']['removed_motion_records'] == 1, 'incompatible DIGITAL automation removal reported')
    check((dst / 'original.bin').read_bytes() == changed, 'removed automation still recoverable in exact original')
    data = (dst / 'project.d8p').read_bytes(); pos = 32; motion = None
    for _ in range(data[20]):
        kind = int.from_bytes(data[pos:pos+2], 'little') & 32767
        size = int.from_bytes(data[pos+2:pos+4], 'little')
        if kind == 4:
            motion = data[pos+8:pos+8+size]
        pos += 8 + size
    check(motion is not None and motion[4] == 1 and motion[8:] == bytes([3, 63, 0x87, 224, 255]), 'unrelated track4 step64 signed lock remains at exact address/value')
    unknown = root / 'tests/fixtures/d8p1/unknown-optional.d8p'
    dst = tmp / 'unknown-optional'
    report = module.prepare(unknown, dst, exe, refs)
    check(not report['accepted'] and not list(dst.glob('*.d8p')) and (dst / 'original.bin').read_bytes() == unknown.read_bytes(), 'unknown optional proposed data is preserved and never silently dropped')
    # A late readback/canonical failure cleans up every converted artifact.
    actual = module.run_converter
    def failed_canonical(executable, mode, original, mask):
        data, meta = actual(executable, mode, original, mask)
        if mode == 'canonical' and original.name == 'project.d8p':
            return data + b'bad', meta
        return data, meta
    module.run_converter = failed_canonical
    try:
        dst = tmp / 'late-verification'
        report = module.prepare(fixtures / 'fun9.bin', dst, exe, refs)
    finally:
        module.run_converter = actual
    check(not report['accepted'] and not list(dst.glob('*.d8p')), 'late verification refusal removes all converted artifacts')
    check(len(list(dst.glob('*original.bin'))) == 5, 'late refusal preserves every original')
    check('converted_file' not in report and all('converted_file' not in row for row in report['references'].values()), 'late refusal report cannot claim deleted output files')
    dst = tmp / 'missing-converter'
    report = module.prepare(fixtures / 'fun9.bin', dst, tmp / 'not-an-executable', refs)
    check(not report['accepted'] and (dst / 'original.bin').exists() and not list(dst.glob('*.d8p')), 'missing tool still retains original/report')
    check(pristine == {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in fixtures.glob('*.bin')}, 'all frozen fixtures remain pristine')
print(f'Offline D8P1 conversion: {checks} checks passed; 13 real legacy fixtures, snapshots and verified context, no device writes')
