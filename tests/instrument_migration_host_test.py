#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Strict complete capture consumer and actual NOR simulation integration tests."""
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import dabbl8_instrument_migrate as host

checks = 0

def check(ok, label):
    global checks
    checks += 1
    if not ok:
        raise AssertionError(label)


def refusal(fn, label):
    try:
        fn()
    except (ValueError, OSError, UnicodeError):
        check(True, label)
    else:
        check(False, label)


def encoded(m):
    return json.dumps(m, sort_keys=True).encode()


def manifest(roles):
    m = dict(format='dabbl8-instrument-capture', version=1, role_table=1,
             mode='full', token=1, post_boot=True, current_state_complete=True,
             raw_bytes=319488, provenance='Synthetic post-boot fixture; no device provenance',
             **{key: False for key in host.DENIED_AUTHORITIES})
    m['objects'] = []
    for r, data in enumerate(roles):
        a, n, b, k = host.RAW_MAP[r] if r < 12 else (0, 0, 0, 0)
        m['objects'].append(dict(role=r, name=host.NAMES[r], file=host.FILES[r],
                                bytes=len(data), crc32=zlib.crc32(data), sha256=host.digest(data),
                                segments=[dict(address=a, bytes=n), dict(address=b, bytes=k)]))
    return m


def archive(path, image):
    path.mkdir()
    roles = []
    for a, n, b, k in host.RAW_MAP:
        roles.append(image[a:a+n] + image[b:b+k])
    roles.append((ROOT / 'tests/fixtures/d8p1/minimal.d8p').read_bytes())
    # Unsaved opaque current-state bytes must not be normalized or inferred from
    # raw stores. Deliberately include unknown/inert bytes and stale inner CRCs.
    for role in range(13, 17):
        roles.append(bytes((i * 17 + role) & 255 for i in range(host.ROLE_LIMITS[role])))
    for name, data in zip(host.FILES, roles):
        (path / name).write_bytes(data)
    m = manifest(roles)
    (path / 'manifest.json').write_bytes(encoded(m))
    return tuple(roles), m


def hashes(path):
    return {p.name: host.digest(p.read_bytes()) for p in path.iterdir() if p.is_file()}


def raw_expected(image, roles, apply_plan=None):
    # Independent full-image oracle; no production expected_image helper.
    result = bytearray(image)
    for r in range(12 if apply_plan is None else 5):
        data = roles[r] if apply_plan is None else apply_plan[r*8192:(r+1)*8192]
        a, n, b, k = host.RAW_MAP[r]
        result[a:a+n] = data[:n]
        if k:
            result[b:b+k] = data[n:]
    return bytes(result)


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: instrument_migration_host_test.py EXECUTOR POOL_INITIALIZER')
    executor, initializer = map(lambda x: Path(x).resolve(), sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix='dabbl8-instrument-migration-') as tmp:
        t = Path(tmp)
        pattern = bytes((a ^ (a >> 9) ^ 0xa5) & 255 for a in range(1048576))
        raw = t / 'patterned'
        roles, m = archive(raw, pattern)
        original_hashes = hashes(raw)
        check(host.validate_bytes(encoded(m), roles)['roles'] == roles, 'all17 accepted unchanged')
        for key, value in [('version', 2), ('version', True), ('role_table', 2), ('mode', 'raw'),
                           ('post_boot', False), ('current_state_complete', False), ('token', -1),
                           ('token', 1 << 32), ('raw_bytes', 319487), ('provenance', 'x'*201)]:
            bad = copy.deepcopy(m); bad[key] = value
            refusal(lambda: host.parse_manifest(encoded(bad)), 'strict header ' + key)
        for key in host.DENIED_AUTHORITIES:
            for value in (True, 0):
                bad = copy.deepcopy(m); bad[key] = value
                refusal(lambda: host.parse_manifest(encoded(bad)), 'no inferred authority ' + key)
        for r in range(17):
            for key, value in [('role', (r+1)%17), ('name', 'unknown'), ('file', '../escape'),
                               ('bytes', 0), ('crc32', True), ('sha256', 'A'*64)]:
                bad = copy.deepcopy(m); bad['objects'][r][key] = value
                refusal(lambda: host.parse_manifest(encoded(bad)), 'role identity/limits')
            bad = list(roles); bad[r] = bad[r][:-1]
            refusal(lambda: host.validate_bytes(encoded(m), tuple(bad)), 'every role required')
            bad = list(roles); bad[r] = bytes([bad[r][0]^1]) + bad[r][1:]
            refusal(lambda: host.validate_bytes(encoded(m), tuple(bad)), 'every role CRC/SHA')
            bad = copy.deepcopy(m); bad['objects'][r]['segments'][0]['address'] += 4096
            refusal(lambda: host.parse_manifest(encoded(bad)), 'fixed map')
        bad = copy.deepcopy(m); bad['objects'][0]['sha256'] = '0'*64
        refusal(lambda: host.validate_bytes(encoded(bad), roles), 'SHA independently verified despite correct CRC')
        bad = copy.deepcopy(m); bad['objects'][0]['crc32'] ^= 1
        refusal(lambda: host.validate_bytes(encoded(bad), roles), 'CRC independently verified despite correct SHA')
        for payload in (b'{"version":1,"version":1}', b'{"version":NaN}', b'\xff', b' '*16385):
            refusal(lambda: host.parse_manifest(payload), 'bounded duplicate/nonfinite/encoding')
        for changed in ('extra-field', 'missing-field', 'roles12', 'reordered', 'extra-segment'):
            bad = copy.deepcopy(m)
            if changed == 'extra-field': bad['authority'] = True
            elif changed == 'missing-field': del bad['post_boot']
            elif changed == 'roles12': bad['objects'] = bad['objects'][:12]
            elif changed == 'reordered': bad['objects'][0], bad['objects'][1] = bad['objects'][1], bad['objects'][0]
            else: bad['objects'][8]['segments'].append(dict(address=0, bytes=0))
            refusal(lambda: host.parse_manifest(encoded(bad)), changed)
        retained, report = host.retain_archive(raw, t/'retained-ok')
        check(report['archive_validated'] and retained['roles'] == roles, 'exact originals retained')
        check(hashes(t/'retained-ok/retained') == original_hashes, 'copy identity all17+manifest')
        refusal(lambda: host.retain_archive(raw, t/'retained-ok'), 'exclusive destination')
        for fault in ('missing', 'corrupt', 'oversize', 'symlink', 'unexpected', 'badmanifest'):
            source = t / ('fault-'+fault); shutil.copytree(raw, source)
            target = source / host.FILES[16]
            if fault == 'missing': target.unlink()
            elif fault == 'corrupt': target.write_bytes(b'wrong')
            elif fault == 'oversize': target.write_bytes(b'x'*(host.ROLE_LIMITS[16]+1))
            elif fault == 'symlink': target.unlink(); target.symlink_to(raw/host.FILES[16])
            elif fault == 'unexpected': (source/'private-unrecognized.bin').write_bytes(b'keep')
            else: (source/'manifest.json').write_bytes(b'{}')
            before = hashes(source)
            destination = t / ('refused-'+fault)
            result = host.simulate(source, destination, '/does/not/exist', '/does/not/exist', 'restore')
            check(not result['accepted'] and not (destination/'executor-output.log').exists(), 'reject before executor '+fault)
            check(hashes(source) == before, 'source untouched '+fault)
            check((destination/'retained'/host.FILES[0]).read_bytes() == roles[0], 'available originals retained '+fault)
            if fault in ('oversize', 'symlink', 'missing'):
                check(host.FILES[16] in json.loads((destination/'input-report.json').read_text())['unretained'], 'explicit unretained')
        current = t/'current.bin'; current.write_bytes(b'\x3c'*1048576)
        restore = host.simulate(raw, t/'restore', executor, current, 'restore')
        check(restore['accepted'] and restore['full_image_readback_verified'], 'actual restore complete')
        check((t/'restore/simulated-result.bin').read_bytes() == raw_expected(current.read_bytes(), roles), 'whole NOR including fragmented FM6/sample tails')
        check(hashes(t/'restore/retained') == original_hashes, 'restore retains all current logical originals too')
        # Structurally supported legacy source, not a source-to-plan musical proof.
        legacy = bytearray(pattern)
        body = (ROOT/'tests/fixtures/projects/fun9.bin').read_bytes()
        check(len(body) == 3648, 'immutable FUN9 fixture')
        for r in range(5):
            sector = bytearray(b'\xff'*4096)
            header = struct.pack('<IHH5I', 0x554c4546, r+1 if r<4 else 9, 0, 1,
                                 len(body), zlib.crc32(body), 0xffffffff, 0xffffffff)
            sector[:32] = header + struct.pack('<I', zlib.crc32(header))
            sector[256:256+len(body)] = body
            a = host.RAW_MAP[r][0]
            legacy[a:a+8192] = sector + b'\xff'*4096
        supported = t/'supported'; goodroles, unused = archive(supported, bytes(legacy))
        goodcurrent = t/'supported-current.bin'; goodcurrent.write_bytes(legacy)
        plan = t/'reviewed-plan.bin'
        minimal = ROOT/'tests/fixtures/d8p1/minimal.d8p'
        p = subprocess.run([str(initializer), str(plan), '7', *([str(minimal)]*4)], capture_output=True)
        check(p.returncode == 0 and len(plan.read_bytes()) == 40960, 'actual initializer reviewed structural plan')
        apply = host.simulate(supported, t/'apply', executor, goodcurrent, 'apply', plan, reviewed_plan=True)
        check(apply['accepted'], 'actual APPLY complete')
        check((t/'apply/simulated-result.bin').read_bytes() == raw_expected(bytes(legacy), goodroles, plan.read_bytes()), 'actual plan exact and outside preserved')
        check(apply['source_to_plan_musical_equivalence'] is False and apply['executor_receipt']['source_plan_equivalence'] is False, 'no source musical equivalence claim')
        for label, source, image, proposal, reviewed in (
                ('unreviewed', supported, goodcurrent, plan, False),
                ('unsupported', raw, t/'pattern-current.bin', plan, True),
                ('source-changed', supported, current, plan, True),
                ('invalid-plan', supported, goodcurrent, t/'bad-plan.bin', True)):
            (t/'pattern-current.bin').write_bytes(pattern)
            (t/'bad-plan.bin').write_bytes(b'\x55'*40960)
            out = t/label
            r = host.simulate(source, out, executor, image, 'apply', proposal, reviewed_plan=reviewed)
            check(not r['accepted'], 'actual refusal '+label)
            if label != 'unreviewed':
                check((out/'simulated-result.bin').read_bytes() == image.read_bytes(), 'zero mutation preflight '+label)
                check(r['executor_receipt']['mutation_calls'] == 0, 'no IO writes before validated '+label)
            else: check(not (out/'executor-output.log').exists(), 'review decision before executor')
        # Meaningful uncertain prefixes; originals make each interrupted state
        # recoverable without accepting a subset or granting production authority.
        for cut, partial in ((1, 0), (1, 73), (2, 127), (7, 9), (17, 256)):
            out = t/f'cut-{cut}-{partial}'
            r = host.simulate(supported, out, executor, goodcurrent, 'apply', plan,
                              cut=cut, partial=partial, reviewed_plan=True)
            check(not r['accepted'] and r['executor_receipt']['mutation_calls'] == cut, 'actual interrupted cut')
            check(hashes(out/'retained') == hashes(supported), 'cut originals unchanged')
            recovered = t/f'recovered-{cut}-{partial}'
            rr = host.simulate(supported, recovered, executor, out/'simulated-result.bin', 'restore')
            check(rr['accepted'], 'restore interrupted mapped plan')
            check((recovered/'simulated-result.bin').read_bytes() == bytes(legacy), 'whole exact original restoration')
        # Receipt parser independent malformed/replayed/false-authority cases.
        receipt = restore['executor_receipt']
        for key, value in [('version', 2), ('version', True), ('operation', 'apply'),
                           ('completed', 1), ('quarantined', False), ('device_writes', True),
                           ('production_ownership', True), ('hardware_qualified', True),
                           ('source_plan_equivalence', True), ('plan_review_required', False),
                           ('mutation_calls', -1), ('cut_operation', 1), ('partial_bytes', 1),
                           ('reads', True), ('status', 'unknown')]:
            bad = dict(receipt); bad[key] = value
            refusal(lambda: host.parse_receipt(encoded(bad), 'restore', 0, 0, 0), 'strict receipt '+key)
        for payload in (b'{}', b'[]', b'\xff', b' '*4097, b'{"version":1,"version":1}'):
            refusal(lambda: host.parse_receipt(payload, 'restore', 0, 0, 0), 'malformed receipt')
        refusal(lambda: host.parse_receipt(encoded(receipt), 'restore', 0, 0, 1), 'exit contradiction')
        # A supplied executable is unqualified. Complete receipts alone cannot
        # certify results; independent entire-image readback must reject these.
        for kind in ('false-complete', 'outside-scope', 'truncated', 'bad-json', 'retained-change'):
            fake = t/('fake-'+kind)
            text = '#!/usr/bin/env python3\nimport sys,json\nfrom pathlib import Path\n'
            text += f'r={receipt!r}\n'
            text += 'r.update(erases=0,programs=0,mutation_calls=0)\n'
            text += 'data=bytearray(Path(sys.argv[4]).read_bytes())\n'
            if kind == 'outside-scope': text += 'data[0]^=1\n'
            if kind == 'truncated': text += 'data=data[:-1]\n'
            if kind == 'retained-change':
                text += f"Path(sys.argv[3], {host.FILES[16]!r}).write_bytes(b'x')\n"
            text += 'Path(sys.argv[5]).write_bytes(data)\n'
            text += "print('{}')\n" if kind == 'bad-json' else 'print(json.dumps(r))\n'
            fake.write_text(text); fake.chmod(0o700)
            r = host.simulate(raw, t/('fake-result-'+kind), fake, current, 'restore')
            check(not r['accepted'], 'independent result refusal '+kind)
        for extra in (['--help'], ['--operation','restore'], ['--reviewed-plan'], ['--cut-operation','1']):
            cmd = [sys.executable, str(ROOT/'tools/dabbl8_instrument_migrate.py')]
            if extra != ['--help']: cmd += [str(raw), str(t/'never-cli')]
            p = subprocess.run(cmd+extra, capture_output=True)
            check(p.returncode == (0 if extra == ['--help'] else 2), 'CLI explicit simulation inputs')
        existing = subprocess.run([sys.executable, str(ROOT/'tools/dabbl8_instrument_migrate.py'), str(raw), str(t/'restore')], capture_output=True)
        check(existing.returncode == 2 and hashes(t/'restore/retained') == original_hashes, 'CLI existing evidence preserved')
        check(hashes(raw) == original_hashes, 'all source originals immutable')
        check(current.read_bytes() == b'\x3c'*1048576, 'supplied simulation input immutable')
        check(goodcurrent.read_bytes() == bytes(legacy), 'supported source image immutable')
    print(f'PASS: {checks} instrument migration host checks; simulation only, no musical equivalence or hardware qualification')

if __name__ == '__main__':
    main()
