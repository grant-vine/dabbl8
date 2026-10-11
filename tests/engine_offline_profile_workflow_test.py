#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Real CLI + complete offline bundle masks; ordinary bytes stay unchanged."""
from pathlib import Path
import base64
import hashlib
import json
import struct
import subprocess
import sys
import tempfile
import zlib
root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'tools'))
import dabbl8_project_convert as project
import dabbl8_migration_bundle as migration
exe, initializer = map(lambda p: Path(p).resolve(), (sys.argv[1], sys.argv[3]))
baseline = None if sys.argv[2] == '-' else Path(sys.argv[2]).resolve()
fm4, slice_on = map(int, sys.argv[4:6])
full = sum(1 << i for i in range(14) if i != 1 or fm4)
if not slice_on:
    full &= ~(1 << 13)
receipts = json.loads((root / 'tests/fixtures/engine-profile-ordinary-cli.json').read_text())
assert receipts['format'] == 'dabbl8-ordinary-cli-parent-receipts' and receipts['version'] == 1
assert receipts['parent_commit'] == '370ac2d108be3de6cd258ded66cda179218198d8'
vectors = receipts['configs'][f'fm{fm4}-slice{slice_on}']['vectors']
fixtures = root / 'tests/fixtures/projects'
originals = sorted(p for p in fixtures.glob('*.bin') if '.expected-fun9' not in p.name)
pristine = {p: p.read_bytes() for p in fixtures.glob('*.bin')}
checks = 0

def check(yes, label):
    global checks
    checks += 1
    if not yes:
        raise AssertionError(label)

def cli(mode, source, refs=15, mask=None, tool=exe):
    args = [str(tool), mode, str(source), str(refs)]
    if mask is not None:
        args.append(str(mask))
    return subprocess.run(args, capture_output=True)

def identity(outcome, mode, original_name, source):
    actual = {'status': outcome.returncode, 'stdout_sha256': hashlib.sha256(outcome.stdout).hexdigest(),
              'stderr_sha256': hashlib.sha256(outcome.stderr).hexdigest()}
    check(actual == vectors[original_name][mode], 'ordinary exact frozen parent CLI receipt')
    if baseline is not None:
        old = cli(mode, source, tool=baseline)
        check((outcome.returncode, outcome.stdout, outcome.stderr) == (old.returncode, old.stdout, old.stderr), 'ordinary direct rebuilt-parent byte identity')

def checksum(data):
    h = 2166136261
    for byte in data[:-4]:
        h = ((h ^ byte) * 16777619) & 0xffffffff
    data[-4:] = struct.pack('<I', h)
    return bytes(data)

def source_for(engine=10, sentinel=0, references=True):
    data = bytearray(pristine[fixtures / 'fun9.bin'])
    for t in range(4):
        pos = 68 + t * 677
        data[pos + 99] = engine
        data[pos + 100] = sentinel
    if not references:
        pos = 68 + 4 * 677
        data[pos:pos + 36] = b'\0' * 36
    return checksum(data)

def chunks(data):
    result, pos = {}, 32
    check(data[:4] == b'D8P1', 'native header')
    while pos < len(data):
        ident, length, crc = struct.unpack_from('<HHI', data, pos)
        body = data[pos + 8:pos + 8 + length]
        check(len(body) == length and zlib.crc32(body) == crc, 'independent chunk CRC')
        result[ident & 32767] = body
        pos += 8 + length
    check(pos == len(data), 'exact native chunk extent')
    return result

def metadata(outcome):
    return json.loads(outcome.stderr)

def refusal(source, mask, status, refs=15):
    p = cli('legacy-profile-v1', source, refs, mask)
    meta = metadata(p)
    check(p.returncode == 1 and not p.stdout and not meta['accepted'], 'refusal emits zero proposed bytes')
    check(meta['profile_schema'] == 1 and meta['profile_status'] == status, 'structured refusal reason')
    return meta

with tempfile.TemporaryDirectory(prefix='d8-offline-profile-workflow-') as temp:
    temp = Path(temp)
    for original in originals:
        ordinary = cli('legacy', original)
        identity(ordinary, 'legacy', original.name, original)
        optional = cli('legacy-profile-v1', original, mask=full)
        check(optional.returncode == 0 and optional.stdout == ordinary.stdout, 'full profile output identical')
        meta = metadata(optional)
        for key, value in metadata(ordinary).items():
            check(meta[key] == value, 'ordinary migration metadata preserved')
        check(meta['engine_mask'] == full and not meta['qualified_firmware'], 'mask/version and experimental qualification explicit')
        check(all(r['engine'] == 0 and r['preset'] == 255 and not r['fallback'] for r in meta['inserted_defaults']), 'full profile retains deferred historical defaults')
        native = temp / 'native.d8p'; native.write_bytes(ordinary.stdout)
        for mode in ('check', 'canonical'):
            identity(cli(mode, native), mode, original.name, native)
    src = temp / 'source.bin'; src.write_bytes(source_for())
    original_wire = cli('legacy', src).stdout
    old_chunks = chunks(original_wire)
    targets = [12] + ([1] if fm4 else []) + ([13] if slice_on else [])
    for expected in targets:
        mask = (1 << expected) | (1 << 10)
        out = cli('legacy-profile-v1', src, mask=mask)
        check(out.returncode == 0, 'compiled profile permits required original DRUM')
        meta, wire = metadata(out), chunks(out.stdout)
        check(wire[2][:4 * 677] == old_chunks[2][:4 * 677] and wire[3][:512] == old_chunks[3][:512], 'four original tracks/private patches exact')
        check(all(wire.get(i) == old_chunks.get(i) for i in (1, 4, 5, 6, 7, 8)), 'global/motion/chain/name/banks/scenes exact')
        for t, row in zip(range(4, 8), meta['inserted_defaults']):
            check(row == {'track': t + 1, 'requested_engine': 0, 'engine': expected, 'preset': 0, 'fallback': True, 'default_kind': 'factory_preset_0'}, 'independent actual default metadata')
            check(wire[2][t * 677 + 99:t * 677 + 101] == bytes([expected, 0]), 'metadata matches actual stored sound')
        refs = {0: src, 3: src}
        dst = temp / f'profile-{expected}'
        report = project.prepare(src, dst, exe, refs, engine_mask=mask)
        check(report['accepted'] and report['engine_mask'] == mask, report.get('reason', 'whole profile bundle'))
        for entry in [report, *report['references'].values()]:
            check((dst / entry['original_file']).read_bytes() == src.read_bytes(), 'bundle original retained')
            check(entry['conversion']['engine_mask'] == mask, 'same profile primary and references')
            check(entry['converted_sha256'] == hashlib.sha256((dst / entry['converted_file']).read_bytes()).hexdigest(), 'converted hash actual')
        try:
            project.prepare(src, dst, exe, refs, engine_mask=mask)
        except FileExistsError:
            check(True, 'no overwrite')
        else:
            raise AssertionError('bundle overwritten')
    # Refuse primary or any supplied reference without publishing a partial set.
    missing = temp / 'missing.bin'; missing.write_bytes(source_for(0))
    for source, refs in [(missing, {0: src, 3: src}), (src, {0: src, 3: missing})]:
        dst = temp / f'refusal-{checks}'
        report = project.prepare(source, dst, exe, refs, engine_mask=(1 << 12) | (1 << 10))
        check(not report['accepted'] and report['profile_refusal']['profile_status'] == 'unavailable_engine', 'missing-original engine structured refusal')
        check(not list(dst.glob('*.d8p')) and (dst / 'original.bin').read_bytes() == source.read_bytes(), 'whole-set refusal retains primary and no outputs')
        for slot, value in refs.items():
            check((dst / f'slot-{slot}-original.bin').read_bytes() == value.read_bytes(), 'all supplied originals captured before refusal')
    for value in (0, 2 if not fm4 else 32768, 1 << 30, '-1', '+1', '1x', '999999999999999999999999999'):
        refusal(src, value, 'invalid_mask')
    refusal(missing, 1 << 12, 'unavailable_engine')
    corrupt = temp / 'corrupt.bin'; data = bytearray(src.read_bytes()); data[100] ^= 1; corrupt.write_bytes(data)
    refusal(corrupt, full, 'bad_data')
    unsupported = temp / 'unsupported.bin'; unsupported.write_bytes(source_for(14))
    refusal(unsupported, full, 'unsupported_source_engine')
    refusal(src, full, 'bad_data', refs=0)
    # Actual late verification and malformed profile metadata must clean up
    # every proposed output while retaining all originals.
    actual_run = project.run_converter
    def late_verify(executable, mode, original, mask, *profile_args):
        data, meta = actual_run(executable, mode, original, mask, *profile_args)
        if mode == 'canonical' and original.name == 'project.d8p':
            return data + b'bad', meta
        return data, meta
    project.run_converter = late_verify
    try:
        dst = temp / 'profile-late-refusal'
        late = project.prepare(src, dst, exe, {0: src, 3: src}, engine_mask=(1 << 12) | (1 << 10))
    finally:
        project.run_converter = actual_run
    check(not late['accepted'] and not list(dst.glob('*.d8p')), 'late profile readback failure removes all outputs')
    check(len(list(dst.glob('*original.bin'))) == 3 and 'converted_file' not in late and all('converted_file' not in row for row in late['references'].values()), 'late refusal retains originals and removes stale output metadata')
    actual_process = project.subprocess.run
    def changed_metadata(kind):
        def run(command, **kwargs):
            process = actual_process(command, **kwargs)
            if command[1] == 'legacy-profile-v1' and process.returncode == 0:
                meta = json.loads(process.stderr)
                if kind == 'mask': meta['engine_mask'] = 1
                elif kind == 'schema': meta['profile_schema'] = True
                elif kind == 'status': meta['profile_status_code'] = True
                elif kind == 'defaults': meta['inserted_defaults'][0] = None
                elif kind == 'slot-mask': meta['available_project_mask'] = 1
                elif kind == 'writes': meta['device_writes'] = True
                elif kind == 'firmware': meta['firmware_package'] = True
                elif kind == 'requested-bool': meta['inserted_defaults'][0]['requested_engine'] = False
                elif kind == 'engine-bool': meta['inserted_defaults'][0]['engine'] = False
                elif kind == 'tracks': meta['track_count'] = 4
                elif kind == 'voices': meta['shared_voices'] = 64
                elif kind == 'track-float': meta['inserted_defaults'][0]['track'] = 5.0
                elif kind == 'slot-bool': meta['available_project_mask'] = True
                else: meta['inserted_defaults'][0]['engine'] = 0
                process.stderr = json.dumps(meta).encode()
            return process
        return run
    for kind in ('mask', 'schema', 'status', 'defaults', 'identity', 'slot-mask', 'writes', 'firmware', 'requested-bool', 'engine-bool', 'tracks', 'voices', 'track-float', 'slot-bool'):
        project.subprocess.run = changed_metadata(kind)
        try:
            dst = temp / ('malformed-' + kind)
            bad_meta = project.prepare(src, dst, exe, {0: src, 3: src}, engine_mask=(1 << 12) | (1 << 10))
        finally:
            project.subprocess.run = actual_process
        check(not bad_meta['accepted'] and not list(dst.glob('*.d8p')) and len(list(dst.glob('*original.bin'))) == 3, 'malformed profile metadata fails whole bundle closed')
    for invalid in (0, False, -1, 16384, '4096'):
        dst = temp / f'invalid-argument-{checks}'
        try:
            project.prepare(src, dst, exe, engine_mask=invalid)
        except ValueError:
            check(not dst.exists(), 'invalid API argument creates no output directory')
        else:
            raise AssertionError('invalid profile argument accepted')
    # Synthetic archival migration: ordinary and optional full profile are exact.
    clean = source_for(references=False)
    ids = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 32, 33, 34]
    values = {i: b'' for i in ids}
    values.update({0: clean, 1: b'synthetic-settings', 2: clean, 3: clean, 4: clean, 5: source_for(0, references=False)})
    archive = {'format': 'felucca-backup', 'version': 1, 'objects': [{'id': i, 'size': len(values[i]), 'crc': zlib.crc32(values[i]), 'data': base64.b64encode(values[i]).decode()} for i in ids]}
    backup = temp / 'backup.fm1bak'; backup.write_text(json.dumps(archive))
    autosave = temp / 'autosave.bin'; autosave.write_bytes(b'\xff' * 8192)
    ordinary_dir, profile_dir = temp / 'migration-ordinary', temp / 'migration-full'
    ordinary = migration.prepare(backup, autosave, ordinary_dir, exe, initializer)
    optional = migration.prepare(backup, autosave, profile_dir, exe, initializer, engine_mask=full)
    check(ordinary['accepted'] and optional['accepted'], 'actual migration full profile accepted')
    check((ordinary_dir / 'proposed-virtual-pool.bin').read_bytes() == (profile_dir / 'proposed-virtual-pool.bin').read_bytes(), 'full migration pool bytes exact')
    limited_dir = temp / 'migration-limited'
    limited = migration.prepare(backup, autosave, limited_dir, exe, initializer, engine_mask=(1 << 12) | (1 << 10))
    check(limited['accepted'] and limited['archival_validation']['accepted'], 'archival-only unavailable ANALOG preserved/validated ordinarily, never substituted or emitted')
    check(all(row['engine_mask'] == ((1 << 12) | (1 << 10)) for row in limited['conversions'].values()), 'all emitted migration objects receive same profile')
    values[4] = source_for(0, references=False)
    for obj in archive['objects']:
        if obj['id'] == 4:
            obj.update({'size': len(values[4]), 'crc': zlib.crc32(values[4]), 'data': base64.b64encode(values[4]).decode()})
    backup.write_text(json.dumps(archive)); dst = temp / 'migration-refused'
    refused = migration.prepare(backup, autosave, dst, exe, initializer, engine_mask=(1 << 12) | (1 << 10))
    check(not refused['accepted'] and refused['profile_refusal']['profile_status'] == 'unavailable_engine', 'migration missing-original source refuses')
    check(not list(dst.glob('*.d8p')) and not (dst / 'proposed-virtual-pool.bin').exists(), 'migration publishes zero native/pool artifacts on refusal')
    check((dst / 'original.fm1bak').read_bytes() == backup.read_bytes() and (dst / 'original-autosave-pair.bin').read_bytes() == autosave.read_bytes(), 'migration retains original archive/sectors')
check(pristine == {p: p.read_bytes() for p in pristine}, 'all historical references pristine')
print(f'Offline profile workflow: {checks} checks PASS; FM4={fm4} SLICE={slice_on}; C CLI/Python proposals only')
