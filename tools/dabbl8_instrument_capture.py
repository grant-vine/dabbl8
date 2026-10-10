#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Collect version-1 read-only post-boot store/current-state frames; no restore."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import select
import subprocess
import tempfile
import time
import zlib

COMMAND = 78
HEADER = bytes((0xf0, 0x7d, 0x46, 0x4c, COMMAND))
RAW_MAP = ((0x97000, 8192, 0, 0), (0x99000, 8192, 0, 0),
           (0x9b000, 8192, 0, 0), (0x9d000, 8192, 0, 0),
           (0xe5000, 8192, 0, 0), (0xfc000, 8192, 0, 0),
           (0xdc000, 8192, 0, 0), (0xde000, 8192, 0, 0),
           (0x9f000, 4096, 0xfe000, 4096), (0xa0000, 81920, 0, 0),
           (0xb4000, 81920, 0, 0), (0xc8000, 81920, 0, 0))
NAMES = ('project-pair-0', 'project-pair-1', 'project-pair-2', 'project-pair-3',
         'persisted-autosave-pair', 'settings-pair', 'preset-pair-0', 'preset-pair-1',
         'fm6-pair', 'sample-allocation-0', 'sample-allocation-1', 'sample-allocation-2',
         'native-live', 'current-settings-per4', 'current-preset-bank-0',
         'current-preset-bank-1', 'current-fm6-presets')
ROOT = Path(__file__).resolve().parents[1]
CURRENT_LENGTHS = {13: 572, 14: 3080, 15: 3080, 16: 3728}


def u32(value):
    if type(value) is not int or not 0 <= value <= 0xffffffff:
        raise ValueError('Invalid unsigned wire integer')
    return bytes((value >> (7 * i)) & 127 for i in range(5))


def r32(data):
    if len(data) != 5 or any(x > 127 for x in data) or data[4] > 15:
        raise ValueError('Noncanonical unsigned wire integer')
    return sum(x << (7 * i) for i, x in enumerate(data))


def unpack(data, count):
    if len(data) != count + (count + 6) // 7:
        raise ValueError('Wrong packed chunk length')
    result = bytearray()
    pos = 0
    while len(result) < count:
        size = min(7, count - len(result))
        mask = data[pos]
        if mask >> size:
            raise ValueError('Noncanonical packed chunk mask')
        result.extend(data[pos + 1 + i] | (((mask >> i) & 1) << 7) for i in range(size))
        pos += size + 1
    return bytes(result)


class Bridge:
    """Unqualified persistent adapter: full SysEx space-separated two-digit hex bytes in/out."""
    def __init__(self, executable, arguments=('--bridge',), timeout=10):
        self.stderr = tempfile.TemporaryFile()
        self.stdout_log = tempfile.TemporaryFile()
        self.process = subprocess.Popen([str(Path(executable).resolve()), *arguments],
                                        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=self.stderr, bufsize=0, cwd=ROOT)
        self.timeout = timeout
        self.pending = bytearray()

    def line(self, line, maximum=4096):
        self.process.stdin.write(line.encode('ascii') + b'\n')
        deadline = time.monotonic() + self.timeout
        while b'\n' not in self.pending:
            remaining = deadline - time.monotonic()
            if remaining <= 0 or not select.select([self.process.stdout], [], [], max(0, remaining))[0]:
                raise ValueError('Adapter reply timed out')
            chunk = os.read(self.process.stdout.fileno(), 2048)
            if not chunk:
                raise ValueError('Adapter ended before a complete reply')
            self.stdout_log.write(chunk)
            self.pending.extend(chunk)
            if len(self.pending) > maximum:
                raise ValueError('Adapter reply exceeds bounded line size')
        answer, _, tail = self.pending.partition(b'\n')
        self.pending = bytearray(tail)
        return answer.decode('ascii')

    def exchange(self, frame):
        answer = self.line(frame.hex(' '))
        tokens = answer.split(' ')
        if len(tokens) > 600 or any(len(t) != 2 or any(c not in '0123456789abcdefABCDEF' for c in t) for t in tokens):
            raise ValueError('Adapter reply is not bounded canonical hex bytes')
        return bytes.fromhex(answer)

    def retain_diagnostics(self, destination):
        for name, stream in (('adapter-output.log', self.stdout_log), ('adapter-stderr.log', self.stderr)):
            stream.flush(); stream.seek(0)
            with (Path(destination) / name).open('xb') as output:
                while chunk := stream.read(65536):
                    output.write(chunk)

    def close(self):
        if self.process.poll() is None:
            self.process.stdin.close()
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.process.terminate()
                self.process.wait(timeout=2)
        self.process.stdout.close()
        self.stderr.close()
        self.stdout_log.close()

    def __enter__(self):
        return self

    def __exit__(self, *unused):
        self.close()


def collect(exchange, destination, mode='full', provenance='unverified supplied adapter'):
    """Retain every received frame/partial role; publish only complete verified receipt."""
    if mode not in ('full', 'raw-store') or type(provenance) is not str or len(provenance) > 200:
        raise ValueError('Unsupported completeness mode or provenance label')
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=False)
    report = {'format': 'dabbl8-instrument-capture-report', 'version': 1, 'accepted': False,
              'mode': mode, 'post_boot': True, 'current_state_complete': False,
              'full_physical_flash': False, 'pre_boot_original': False,
              'device_writes': False, 'hardware_qualified': False, 'restore_qualified': False,
              'migration_authorized': False, 'ownership_authorized': False,
              'provenance': provenance, 'originals': {}, 'warnings': [
                  'Known musical allocations and declared current logical state only; not vendor recovery or full physical flash.',
                  'Adapter identity and physical capture/recovery remain unqualified; completion grants no migration authority.',
                  'Raw-store mode deliberately omits unsaved current project/settings/preset state.']}
    token = None
    requests = 0
    transcript_path = destination / 'received-frames.jsonl'
    with transcript_path.open('x') as transcript:
        def ask(op, suffix=b'', phase=None, begin=False):
            nonlocal requests, token
            requests += 1
            if requests > 4096:
                raise ValueError('Capture exceeds bounded request count')
            args = bytes((1, op)) + (b'' if op < 2 else u32(token)) + suffix
            request = HEADER + args + b'\xf7'
            record = {'sequence': requests, 'request_hex': request.hex()}
            try:
                reply = exchange(request)
            except Exception as error:
                record['error'] = str(error)
                transcript.write(json.dumps(record) + '\n'); transcript.flush()
                raise
            if type(reply) is not bytes:
                record['error'] = 'Adapter must return exact bytes'
                transcript.write(json.dumps(record) + '\n'); transcript.flush()
                raise ValueError(record['error'])
            record['response_hex'] = reply.hex()
            transcript.write(json.dumps(record) + '\n'); transcript.flush()
            if len(reply) < 15 or len(reply) > 600 or not reply.startswith(HEADER) or reply[-1] != 0xf7 or any(x > 127 for x in reply[5:-1]):
                raise ValueError('Malformed, truncated or non-seven-bit capture frame')
            data = reply[5:-1]
            if data[:3] != bytes((1, op, 0)):
                raise ValueError('Wrong capture version/operation or refused status')
            received_token = r32(data[3:8])
            if begin:
                if received_token != (token + 1) & 0xffffffff:
                    raise ValueError('Stale or replayed capture BEGIN token')
                token = received_token
            elif token is not None and received_token != token:
                raise ValueError('Stale or replayed capture token')
            if phase is not None and data[8] != phase:
                raise ValueError('Unexpected capture phase')
            return data[8], data[9:], received_token

        def scan(initial):
            for role, mapping in enumerate(RAW_MAP):
                length = mapping[1] + mapping[3]
                for off in range(0, length, 256):
                    end = off + min(256, length - off)
                    last = end == length
                    done = role == 11 and last
                    expected_phase = (2 if initial else 4) if done else (1 if initial else 3)
                    _, payload, _ = ask(2, phase=expected_phase)
                    expected_role, expected_offset = (role + 1, 0) if last else (role, end)
                    if len(payload) != 6 or payload[0] != expected_role or r32(payload[1:]) != expected_offset:
                        raise ValueError('Nonprogressing, reordered or truncated scan reply')

        def descriptor(role, phase):
            _, payload, _ = ask(3, bytes((role,)), phase=phase)
            if len(payload) != 31 or payload[0] != role:
                raise ValueError('Wrong descriptor identity/order/length')
            fields = tuple(r32(payload[i:i + 5]) for i in range(1, 31, 5))
            length, crc, *mapping = fields
            if role < 12:
                expected = RAW_MAP[role]
                if tuple(mapping) != expected or length != expected[1] + expected[3]:
                    raise ValueError('Unsupported fixed physical role map')
            elif any(mapping) or (role == 12 and not 0 < length <= 7936) or (role > 12 and length != CURRENT_LENGTHS[role]):
                raise ValueError('Unsupported current-state role length/map')
            return {'role': role, 'name': NAMES[role], 'bytes': length, 'crc32': crc,
                    'segments': [{'address': mapping[0], 'bytes': mapping[1]}, {'address': mapping[2], 'bytes': mapping[3]}]}

        try:
            phase, payload, token = ask(0)
            if phase not in range(5) or payload != bytes((1, 12, 17, 0, 2, 1)):
                raise ValueError('Unsupported capture capabilities/role table')
            _, payload, _ = ask(1, bytes((mode == 'full',)), phase=1, begin=True)
            if payload:
                raise ValueError('Unexpected BEGIN payload')
            scan(True)
            roles = [descriptor(role, 2) for role in range(17 if mode == 'full' else 12)]
            for item in roles:
                path = destination / ('original-' + item['name'] + '.bin')
                with path.open('xb') as original:
                    crc = 0
                    for off in range(0, item['bytes'], 256):
                        count = min(256, item['bytes'] - off)
                        _, payload, _ = ask(4, bytes((item['role'],)) + u32(off) + bytes((count & 127, count >> 7)), phase=2)
                        if len(payload) < 8 or payload[0] != item['role'] or r32(payload[1:6]) != off or payload[6:8] != bytes((count & 127, count >> 7)):
                            raise ValueError('Wrong chunk role/offset/count echo')
                        raw = unpack(payload[8:], count)
                        original.write(raw); original.flush()
                        crc = zlib.crc32(raw, crc)
                    if crc != item['crc32']:
                        raise ValueError('Original role checksum mismatch')
                raw = path.read_bytes()
                if len(raw) != item['bytes'] or zlib.crc32(raw) != item['crc32']:
                    raise ValueError('Original output readback mismatch')
                item['file'] = path.name
                item['sha256'] = hashlib.sha256(raw).hexdigest()
            _, payload, _ = ask(5, phase=3)
            if payload:
                raise ValueError('Unexpected END payload')
            scan(False)
            final = [descriptor(role, 4) for role in range(len(roles))]
            if any({key: item[key] for key in final_item} != final_item for item, final_item in zip(roles, final)):
                raise ValueError('Final complete manifest changed')
            with tempfile.TemporaryDirectory(prefix='.verified-', dir=destination) as stage:
                stage = Path(stage)
                manifest = {'format': 'dabbl8-instrument-capture', 'version': 1, 'role_table': 1,
                            'mode': mode, 'token': token, 'post_boot': True,
                            'current_state_complete': mode == 'full', 'raw_bytes': 319488,
                            'hardware_qualified': False, 'restore_qualified': False,
                            'migration_authorized': False, 'ownership_authorized': False,
                            'full_physical_flash': False, 'pre_boot_original': False,
                            'provenance': provenance, 'objects': roles}
                for item in roles:
                    data = (destination / item['file']).read_bytes()
                    if len(data) != item['bytes'] or hashlib.sha256(data).hexdigest() != item['sha256']:
                        raise ValueError('Retained original changed before publication')
                    target = stage / item['file']
                    target.write_bytes(data)
                    if target.read_bytes() != data:
                        raise ValueError('Verified output copy differs from original')
                (stage / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
                stage.rename(destination / 'verified')
            report['accepted'] = True
            report['current_state_complete'] = mode == 'full'
            report['token'] = token
            report['objects'] = roles
        except Exception as error:
            report['reason'] = str(error)
        finally:
            if token is not None:
                try:
                    ask(6)
                except Exception:
                    pass  # Preserve the original refusal; abort cannot grant completion.
    for path in sorted(destination.glob('original-*.bin')):
        data = path.read_bytes()
        report['originals'][path.name] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    report['requests'] = requests
    report['transcript_sha256'] = hashlib.sha256(transcript_path.read_bytes()).hexdigest()
    (destination / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=Path, help='New private directory; existing destinations refuse')
    parser.add_argument('--mode', choices=('full', 'raw-store'), default='full')
    parser.add_argument('--adapter', required=True, type=Path, help='Persistent full-frame hex-line transport executable')
    parser.add_argument('--adapter-arg', action='append', default=None)
    parser.add_argument('--provenance', default='unverified supplied adapter')
    args = parser.parse_args()
    bridge = None
    def exchange(frame):
        nonlocal bridge
        if bridge is None:
            bridge = Bridge(args.adapter, args.adapter_arg if args.adapter_arg is not None else ('--bridge',))
        return bridge.exchange(frame)
    try:
        report = collect(exchange, args.destination, args.mode, args.provenance)
        if bridge is not None:
            bridge.retain_diagnostics(args.destination)
    except FileExistsError:
        parser.error('destination already exists; original files are never overwritten')
    finally:
        if bridge is not None:
            bridge.close()
    print(json.dumps(report, indent=2))
    return 0 if report['accepted'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
