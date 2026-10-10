#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Actual C byte codec/pool readback, immutable synthetic fixtures, no device API."""
import base64
import copy
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
import dabbl8_project_archive as archive
adapter = Path(sys.argv[1]).resolve()
checks = 0


def check(ok, label):
    global checks
    checks += 1
    if not ok:
        raise AssertionError(label)


def fix(data):
    data = bytearray(data)
    pos = 32
    for _ in range(data[20]):
        n = struct.unpack_from('<H', data, pos + 2)[0]
        struct.pack_into('<I', data, pos + 4, zlib.crc32(data[pos + 8:pos + 8 + n]))
        pos += 8 + n
    data[12:16] = b'\0' * 4
    struct.pack_into('<I', data, 12, zlib.crc32(data))
    return bytes(data)


def maximum():
    data = bytearray((ROOT / 'tests/fixtures/d8p1/maximum.d8p').read_bytes())
    pos = 32
    for _ in range(data[20]):
        kind, n = struct.unpack_from('<HH', data, pos)
        if kind & 32767 == 7:
            for b in range(data[pos + 8]):
                for t in range(8):
                    data[pos + 9 + b * 28 + 12 + t * 2] %= 3
        pos += 8 + n
    return fix(data)


minimal = (ROOT / 'tests/fixtures/d8p1/minimal.d8p').read_bytes()
unknown = (ROOT / 'tests/fixtures/d8p1/unknown-optional.d8p').read_bytes()
full = maximum()
with tempfile.TemporaryDirectory() as td:
    td = Path(td)
    live = td / 'live.d8p'
    live.write_bytes(minimal)
    files = [td / f'object-{o}.d8p' for o in range(4)]
    for o, p in enumerate(files):
        p.write_bytes(full if o == 0 else minimal)
    # All sixteen sparse persisted sets, including absent shared autosave.
    for mask in range(16):
        pool = td / f'pool-{mask}.bin'
        inputs = [p if mask & (1 << o) else '-' for o, p in enumerate(files)]
        # Full metadata references all three projects: use it only with mask7.
        if mask & 7 != 7:
            inputs = [live if mask & (1 << o) else '-' for o in range(4)]
        archive.ask(adapter, 'build', pool, mask, *inputs)
        before = [hashlib.sha256(p.read_bytes()).hexdigest() for p in (pool, live)]
        report = archive.prepare('export', pool, td / f'export-{mask}', adapter, live)
        check(report['accepted'], report.get('reason', f'export sparse{mask}'))
        exported = td / f'export-{mask}' / 'verified/project-set.d8bak'
        imported = archive.prepare('import', exported, td / f'import-{mask}', adapter)
        check(imported['accepted'], imported.get('reason', f'import sparse{mask}'))
        check(imported['present'] == mask, 'explicit absence and autosave presence')
        check(before == [hashlib.sha256(p.read_bytes()).hexdigest() for p in (pool, live)], 'source bytes unchanged')
        check(exported.read_bytes() == (td / f'import-{mask}/original.d8bak').read_bytes(), 'exact original archive retained')
        check(pool.read_bytes() == (td / f'import-{mask}/original-pool.bin').read_bytes(), 'exact original pool retained')
        a, objects, _ = archive.parse_archive(exported.read_bytes())
        check(objects[0] == minimal, 'live distinct from persisted autosave')
        for o in range(4):
            check(bool(a['objects'][o + 1]['blob']['present']) == bool(mask & (1 << o)), 'every persisted identity explicit')
            raw = archive.ask(adapter, 'read', td / f'import-{mask}/verified/proposed-virtual-pool.bin', o, binary=True) if mask & (1 << o) else b''
            check(raw == objects[o + 1], 'whole set byte-exact readback')
        if mask == 15:
            good = json.loads(exported.read_bytes())
            check(objects[1] == full, 'banks/scenes/rows/names/all motion exact')
            check(objects[1] != objects[0] and objects[1] != objects[4], 'live, stored and autosave not substituted')
            check(report['objects'][1]['banks'] == 4 and report['objects'][1]['scenes'] == 16, 'maximum scene metadata retained')
            try:
                archive.prepare('export', pool, td / f'export-{mask}', adapter, live)
            except FileExistsError:
                check(True, 'existing output refusal')
            else:
                raise AssertionError('existing output overwritten')
            again = archive.prepare('export', pool, td / 'repeat', adapter, live)
            check(again['accepted'] and (td / 'repeat/verified/project-set.d8bak').read_bytes() == exported.read_bytes(), 'deterministic archive')
    # A known but noncanonical wire must not be decoded/re-encoded or normalized.
    raw = bytearray(minimal)
    chunks = []; pos = 32
    for _ in range(raw[20]):
        kind, n = struct.unpack_from('<HH', raw, pos)
        chunk = bytearray(raw[pos:pos + 8 + n])
        if kind & 32767 == 1:
            struct.pack_into('<hh', chunk, 8, -32768, 32767)
        if kind & 32767 == 6:
            chunk[8:20] = b'RAW "TEST"' + b'\0' * 2
        chunks.append(chunk); pos += 8 + n
    noncanonical = fix(raw[:32] + b''.join(reversed(chunks)))
    live.write_bytes(noncanonical)
    lossless = archive.prepare('export', td / 'pool-0.bin', td / 'noncanonical', adapter, live)
    check(lossless['accepted'], lossless.get('reason', 'noncanonical accepted'))
    decoded = archive.parse_archive((td / 'noncanonical/verified/project-set.d8bak').read_bytes())[1]
    check(decoded[0] == noncanonical, 'noncanonical order/extreme signed globals/name bytes retained')
    restored = archive.prepare('import', td / 'noncanonical/verified/project-set.d8bak', td / 'noncanonical-import', adapter)
    check(restored['accepted'] and (td / 'noncanonical-import/verified/live.d8p').read_bytes() == noncanonical, 'noncanonical import exact bytes')
    live.write_bytes(minimal)
    missing = archive.prepare('export', td / 'pool-0.bin', td / 'missing-adapter', td / 'absent-adapter', live)
    check(not missing['accepted'] and not (td / 'missing-adapter/verified').exists(), 'missing dependency explicit refusal')
    check((td / 'missing-adapter/original-live.bin').read_bytes() == minimal, 'missing dependency retains original')
    cases = []
    def mutate(label, fn):
        a = copy.deepcopy(good)
        fn(a)
        cases.append((label, json.dumps(a).encode()))
    mutate('version', lambda a: a.update(version=2))
    mutate('limits', lambda a: a['limits'].update(project_bytes=7937))
    mutate('bool-limits', lambda a: a['limits'].update(autosaves=True))
    mutate('extra-header', lambda a: a.update(future=True))
    mutate('missing-object', lambda a: a['objects'].pop())
    mutate('extra-object', lambda a: a['objects'].append(a['objects'][0]))
    mutate('duplicate-id', lambda a: a['objects'][2].update(role='project-0'))
    mutate('fourth-project', lambda a: a['objects'][4].update(role='project-3'))
    mutate('unsafe-path', lambda a: a['source'].update(pool_name='../other'))
    mutate('absolute-path', lambda a: a['source'].update(live_name='/private/input'))
    mutate('bool-mask', lambda a: a.update(present=True))
    mutate('absence', lambda a: a['objects'][2]['blob'].update(present=False))
    mutate('size', lambda a: a['objects'][1]['blob'].update(bytes=7937))
    mutate('crc', lambda a: a['objects'][1]['blob'].update(crc32=0))
    mutate('sha', lambda a: a['objects'][1]['blob'].update(sha256='0' * 64))
    mutate('encoding', lambda a: a['objects'][1]['blob'].update(data='!'))
    mutate('unknown-optional', lambda a: a['objects'][0].update(blob=archive.blob(unknown)))
    required = bytearray(unknown)
    pos = 32
    for _ in range(required[20]):
        kind, n = struct.unpack_from('<HH', required, pos)
        if kind & 32767 > 8:
            struct.pack_into('<H', required, pos, kind | 32768)
        pos += 8 + n
    mutate('unknown-required', lambda a: a['objects'][0].update(blob=archive.blob(fix(required))))
    mutate('unsupported-engine', lambda a: a['objects'][0].update(blob=archive.blob(b'D8P1')))
    mutate('pool-provenance', lambda a: a['objects'][1].update(blob=archive.blob(minimal)))
    original_fourth = (ROOT / 'tests/fixtures/d8p1/maximum.d8p').read_bytes()
    mutate('fourth-unresolved-reference', lambda a: a['objects'][0].update(blob=archive.blob(original_fourth)))
    cases += [('deep-json', b'[' * 2000 + b']' * 2000), ('duplicate-json', b'{"format":0,"format":1}'), ('truncated', b'{'), ('oversize', b' ' * (archive.ARCHIVE_LIMIT + 1))]
    for i, (label, data) in enumerate(cases):
        source = td / f'bad-{i}.d8bak'
        source.write_bytes(data)
        out = td / f'refusal-{i}'
        report = archive.prepare('import', source, out, adapter)
        check(not report['accepted'] and bool(report.get('reason')), label + ' explicit refusal')
        check(not (out / 'verified').exists() and not list(out.glob('.work-*')), label + ' no partial usable output')
        check(source.read_bytes() == data, label + ' original untouched')
        if len(data) <= archive.ARCHIVE_LIMIT:
            check((out / 'original.d8bak').read_bytes() == data, label + ' exact original retained')
    # A damaged only-copy header must not become a false absent role when
    # a different current object survives. Full known sets may retain torn spare.
    sparse = (td / 'pool-9.bin').read_bytes()
    for label, offset in [('magic', 0), ('header-crc', 28)]:
        damaged = bytearray(sparse); damaged[offset] ^= 1
        source = td / f'damaged-{label}.pool'; source.write_bytes(damaged)
        out = td / f'damaged-{label}-export'
        result = archive.prepare('export', source, out, adapter, live)
        check(not result['accepted'] and 'absence' in result.get('reason', ''), 'damaged sole ' + label + ' sparse refusal')
        check(not (out / 'verified').exists() and (out / 'original-pool.bin').read_bytes() == bytes(damaged), 'damaged sparse original retained')
    def record(block, obj, sequence, payload):
        result = bytearray(b'\xff' * 8192)
        struct.pack_into('<I4B5I', result, 0, 0x50533844, 1, block, 0, 0, obj, sequence, len(payload), zlib.crc32(payload), 0xffffffff)
        struct.pack_into('<I', result, 28, zlib.crc32(result[:28]))
        result[256:256 + len(payload)] = payload
        return result
    # Valid headers identify an already-present role even with a bad payload;
    # retain fallback bytes/history rather than infer any absent role from it.
    fallback = bytearray(sparse); bad = record(4, 0, 2, minimal); bad[256] ^= 1
    fallback[4 * 8192:] = bad
    source = td / 'sparse-fallback.pool'; source.write_bytes(fallback)
    result = archive.prepare('export', source, td / 'sparse-fallback', adapter, live)
    check(result['accepted'] and result['present'] == 9, 'recognized corrupt copy uses valid same-role predecessor')
    check((td / 'sparse-fallback/original-pool.bin').read_bytes() == bytes(fallback), 'corrupt copy and fallback raw history retained')
    for label, payload in [('optional', unknown), ('required', fix(required))]:
        data = bytearray((td / 'pool-15.bin').read_bytes()); data[:8192] = record(0, 0, 1, payload)
        source = td / f'stored-{label}.pool'; source.write_bytes(data)
        result = archive.prepare('export', source, td / f'stored-{label}', adapter, live)
        check(not result['accepted'] and not (td / f'stored-{label}/verified').exists(), 'stored unknown ' + label + ' refuses usable proposal')
        check((td / f'stored-{label}/original-pool.bin').read_bytes() == bytes(data), 'stored unknown raw original retained')
    torn = bytearray((td / 'pool-15.bin').read_bytes()); torn[4 * 8192 + 256] = 0
    source = td / 'full-with-torn-spare.pool'; source.write_bytes(torn)
    result = archive.prepare('export', source, td / 'full-with-torn-spare', adapter, live)
    check(result['accepted'] and result['present'] == 15, 'full known set retains legitimate torn spare')
    check((td / 'full-with-torn-spare/original-pool.bin').read_bytes() == bytes(torn), 'torn spare raw bytes preserved')
    # Pool errors preserve exact pool and live snapshots before refusal.
    orphan = bytearray((td / 'pool-8.bin').read_bytes()); orphan[256] ^= 1
    residue = bytearray(b'\xff' * archive.POOL_BYTES); residue[256] = 0
    for label, data in [('short', b'x'), ('foreign', b'\0' * archive.POOL_BYTES), ('orphan', bytes(orphan)), ('unproven-empty', bytes(residue))]:
        source = td / f'{label}.pool'
        source.write_bytes(data)
        out = td / f'{label}-export'
        result = archive.prepare('export', source, out, adapter, live)
        check(not result['accepted'] and not (out / 'verified').exists(), label + ' pool refusal')
        check((out / 'original-pool.bin').read_bytes() == data and (out / 'original-live.bin').read_bytes() == minimal, label + ' both originals retained')
    # Exercise the documented command line, including argument and refusal status.
    command = [sys.executable, str(ROOT / 'tools/dabbl8_project_archive.py')]
    result = subprocess.run(command + ['--help'], capture_output=True, text=True)
    check(result.returncode == 0 and '--live' in result.stdout, 'CLI help exposes required snapshot')
    result = subprocess.run(command + ['export', str(td / 'pool-15.bin'), str(td / 'cli-missing-live')], capture_output=True, text=True)
    check(result.returncode == 2 and not (td / 'cli-missing-live').exists(), 'CLI requires live before output')
    result = subprocess.run(command + ['export', str(td / 'pool-15.bin'), str(td / 'cli-export'), '--live', str(live), '--adapter', str(adapter)], capture_output=True, text=True)
    check(result.returncode == 0 and json.loads(result.stdout)['accepted'], 'documented CLI export')
    result = subprocess.run(command + ['import', str(td / 'cli-export/verified/project-set.d8bak'), str(td / 'cli-import'), '--adapter', str(adapter)], capture_output=True, text=True)
    check(result.returncode == 0 and json.loads(result.stdout)['accepted'], 'documented CLI import')
    result = subprocess.run(command + ['export', str(td / 'short.pool'), str(td / 'cli-refusal'), '--live', str(live), '--adapter', str(adapter)], capture_output=True, text=True)
    check(result.returncode == 1 and not json.loads(result.stdout)['accepted'] and not (td / 'cli-refusal/verified').exists(), 'CLI refusal status and no usable outputs')
print(f'Native project-set archive: {checks} checks, zero failures; all16 sparse masks, actual C exact-byte pool readback; no device or full-instrument backup claim')
