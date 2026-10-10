#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Real C dispatch round trips and hostile response refusals; simulated reads only."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import zlib
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import dabbl8_instrument_capture as capture
bridge_path = Path(sys.argv[1]).resolve()
checks = 0


def check(ok, label):
    global checks
    checks += 1
    if not ok:
        raise AssertionError(label)


def reference(mapping):
    return bytes((a ^ (a >> 9) ^ 0xa5) & 255
                 for base, length in ((mapping[0], mapping[1]), (mapping[2], mapping[3]))
                 for a in range(base, base + length))


def mutate(exchange, predicate, alteration):
    done = False
    def wrapped(frame):
        nonlocal done
        reply = exchange(frame)
        if not done and predicate(frame, reply):
            done = True
            return alteration(reply)
        return reply
    return wrapped


def changed(reply, index, value):
    data = bytearray(reply)
    data[index] = value
    return bytes(data)


def op(number):
    return lambda frame, reply: frame[6] == number


with tempfile.TemporaryDirectory() as directory:
    td = Path(directory)
    binary_hash = hashlib.sha256(bridge_path.read_bytes()).hexdigest()
    for mode in ('full', 'raw-store'):
        with capture.Bridge(bridge_path) as bridge:
            logical = {role: bytes.fromhex(bridge.line('#reference ' + str(role), maximum=24000))
                       for role in range(12, 17)} if mode == 'full' else {}
            result = capture.collect(bridge.exchange, td / mode, mode, 'actual C virtual-NOR test fixture')
            stats = json.loads(bridge.line('#stats'))
            check(stats['writes'] == 0 and stats['unchanged'], 'actual all-NOR equality and zero mutation calls')
        check(result['accepted'], result.get('reason', mode + ' real capture'))
        check(result['current_state_complete'] == (mode == 'full'), 'explicit full versus raw completeness')
        check(not any(result[k] for k in ('device_writes', 'hardware_qualified', 'restore_qualified', 'migration_authorized', 'ownership_authorized', 'full_physical_flash', 'pre_boot_original')), 'receipt grants no physical/recovery/migration authority')
        dest = td / mode
        manifest = json.loads((dest / 'verified/manifest.json').read_text())
        check(len(manifest['objects']) == (17 if mode == 'full' else 12), 'exact ordered role count')
        for item in manifest['objects']:
            role = item['role']; raw = (dest / item['file']).read_bytes()
            check(item['name'] == capture.NAMES[role] and item['file'] == 'original-' + capture.NAMES[role] + '.bin', 'fixed role provenance paths')
            check(len(raw) == item['bytes'] and zlib.crc32(raw) == item['crc32'] and hashlib.sha256(raw).hexdigest() == item['sha256'], 'each original size CRC SHA')
            check((dest / 'verified' / item['file']).read_bytes() == raw, 'verified output exact copy of retained original')
            if role < 12:
                check(raw == reference(capture.RAW_MAP[role]), 'actual fixed allocation bytes independently match NOR fixture')
            else:
                check(raw == logical[role], 'actual logical role exact independent pre-BEGIN reference')
        check(sum(item['bytes'] for item in manifest['objects'][:12]) == 319488, 'all raw allocations including full samples')
        records = [json.loads(line) for line in (dest / 'received-frames.jsonl').read_text().splitlines()]
        check(all(bytes.fromhex(x['request_hex']).startswith(capture.HEADER) for x in records), 'collector submits binary full SysEx through actual bridge')
        check(len([x for x in records if bytes.fromhex(x['request_hex'])[6] == 2]) == 2496, 'bounded complete BEGIN and END scans')
        check(bytes.fromhex(records[-1]['request_hex'])[6] == 6, 'explicit abort releases completed private session')
        try:
            capture.collect(lambda unused: b'', dest)
        except FileExistsError:
            check(True, 'existing destination refuses before adapter access')
        else:
            raise AssertionError('existing destination overwritten')
    # Invalid frames are retained before parsing and never publish usable outputs.
    variants = [
        ('truncated-cap', op(0), lambda p: p[:-1]),
        ('wrong-command', op(0), lambda p: changed(p, 4, 77)),
        ('wrong-version', op(0), lambda p: changed(p, 5, 2)),
        ('high-bit', op(0), lambda p: changed(p, 14, 128)),
        ('unknown-map-version', op(0), lambda p: changed(p, 14, 2)),
        ('wrong-raw-role-count', op(0), lambda p: changed(p, 15, 11)),
        ('wrong-full-role-count', op(0), lambda p: changed(p, 16, 16)),
        ('wrong-chunk-limit', op(0), lambda p: changed(p, 18, 1)),
        ('write-capability', op(0), lambda p: changed(p, 19, 0)),
        ('replayed-begin', op(1), lambda p: changed(p, 8, 0)),
        ('bad-u32-top', op(1), lambda p: changed(p, 12, 16)),
        ('wrong-begin-phase', op(1), lambda p: changed(p, 13, 2)),
        ('nonprogress-scan', op(2), lambda p: p[:15] + capture.u32(0) + p[-1:]),
        ('wrong-scan-token', op(2), lambda p: changed(p, 8, (p[8] + 1) & 127)),
        ('reordered-descriptor', op(3), lambda p: changed(p, 14, 1)),
        ('bad-descriptor-size', op(3), lambda p: p[:-2] + p[-1:]),
        ('wrong-fixed-map', op(3), lambda p: changed(p, 25, p[25] ^ 1)),
        ('wrong-role-length', op(3), lambda p: changed(p, 15, p[15] ^ 1)),
        ('wrong-get-echo', op(4), lambda p: changed(p, 15, p[15] ^ 1)),
        ('truncated-data', op(4), lambda p: p[:-2] + p[-1:]),
        ('unused-pack-mask', op(4), lambda p: changed(p, len(p) - 6, p[-6] | 16)),
        ('mutated-data', op(4), lambda p: changed(p, 23, p[23] ^ 1)),
        ('refused-response', op(4), lambda p: changed(p, 7, 4)),
        ('false-end-complete', op(5), lambda p: changed(p, 13, 4)),
        ('changed-final-crc', lambda f, r: f[6] == 3 and r[13] == 4,
         lambda p: changed(p, 20, p[20] ^ 1)),
    ]
    for label, predicate, alteration in variants:
        with capture.Bridge(bridge_path) as bridge:
            result = capture.collect(mutate(bridge.exchange, predicate, alteration), td / label, 'raw-store')
        check(not result['accepted'] and not (td / label / 'verified').exists(), label + ' refuses complete publication')
        check((td / label / 'received-frames.jsonl').stat().st_size > 0 and (td / label / 'report.json').exists(), label + ' exact response evidence retained')
        for item in result['originals'].values():
            check(item['bytes'] <= 81920, label + ' partial original remains bounded')
    # Actual firmware invalidations, not a mock status response.
    controls = [
        ('raw-change-in-begin', '#change 0', lambda f: f[6] == 2),
        ('raw-change-at-get', '#change 4', lambda f: f[6] == 4),
        ('raw-change-at-end', '#change 9', lambda f: f[6] == 5),
        ('usb-deconfig-cycle', '#usb-cycle', lambda f: f[6] == 4),
        ('usb-reset', '#reset', lambda f: f[6] == 4),
        ('idle-timeout', '#timeout', lambda f: f[6] == 4),
        ('conflicting-upgrade', '#ota', lambda f: f[6] == 4),
        ('live-change', '#change 12', lambda f: f[6] == 4 and f[12] == 12),
        ('per4-change', '#change 13', lambda f: f[6] == 4 and f[12] == 13),
        ('bank0-change', '#change 14', lambda f: f[6] == 4 and f[12] == 14),
        ('bank1-change', '#change 15', lambda f: f[6] == 4 and f[12] == 15),
        ('fm6-change', '#change 16', lambda f: f[6] == 4 and f[12] == 16),
    ]
    for label, command, predicate in controls:
        with capture.Bridge(bridge_path) as bridge:
            triggered = False
            def controlled(frame):
                global triggered
                if not triggered and predicate(frame):
                    check(bridge.line(command) == 'ok', label + ' actual fixture injection')
                    triggered = True
                return bridge.exchange(frame)
            result = capture.collect(controlled, td / label, 'full')
            check(triggered, label + ' intended observable boundary reached')
            check(json.loads(bridge.line('#stats'))['writes'] == 0, label + ' no physical write call')
        check(not result['accepted'] and not (td / label / 'verified').exists(), label + ' actual C completion refused')
        check((td / label / 'received-frames.jsonl').exists(), label + ' failed receipt evidence retained')
    with capture.Bridge(bridge_path) as bridge:
        counter = 0
        def disconnected(frame):
            global counter
            counter += 1
            if counter == 1300:
                raise OSError('synthetic adapter disconnect')
            return bridge.exchange(frame)
        result = capture.collect(disconnected, td / 'disconnected', 'raw-store')
    check(not result['accepted'] and not (td / 'disconnected/verified').exists(), 'adapter disconnect refuses publication')
    check(any(item['bytes'] > 0 for item in result['originals'].values()), 'disconnect retains already received originals')
    command = [sys.executable, str(ROOT / 'tools/dabbl8_instrument_capture.py')]
    run = subprocess.run(command + [str(td / 'cli'), '--adapter', str(bridge_path), '--mode', 'raw-store'], capture_output=True, text=True)
    check(run.returncode == 0 and json.loads(run.stdout)['accepted'], 'documented CLI actual raw collector')
    check((td / 'cli/adapter-output.log').stat().st_size > 0 and (td / 'cli/adapter-stderr.log').exists(), 'CLI retains exact adapter stream diagnostics')
    run = subprocess.run(command + [str(td / 'cli'), '--adapter', str(td / 'missing-adapter')], capture_output=True, text=True)
    check(run.returncode == 2 and 'already exists' in run.stderr, 'CLI existing destination refuses before missing adapter starts')
    run = subprocess.run(command + [str(td / 'missing-adapter-output'), '--adapter', str(td / 'missing-adapter')], capture_output=True, text=True)
    check(run.returncode == 1 and not json.loads(run.stdout)['accepted'], 'missing adapter creates explicit refusal report')
    check((td / 'missing-adapter-output/report.json').exists() and not (td / 'missing-adapter-output/verified').exists(), 'missing adapter cannot create complete receipt')
    check(hashlib.sha256(bridge_path.read_bytes()).hexdigest() == binary_hash, 'collector never modifies actual bridge input')
    for bad in (b'\0' * 4, b'\0\0\0\0\x10', b'\x80\0\0\0\0'):
        try:
            capture.r32(bad)
        except ValueError:
            check(True, 'noncanonical unsigned integer refuses')
        else:
            raise AssertionError('invalid unsigned integer accepted')
    try:
        capture.unpack(b'\x02\0', 1)
    except ValueError:
        check(True, 'unused pack mask bits refuse')
    else:
        raise AssertionError('noncanonical mask accepted')
print(f'Instrument capture collector: {checks} checks passed; actual C dispatch/NOR and hostile responses; no physical qualification')
