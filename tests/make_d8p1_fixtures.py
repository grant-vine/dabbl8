#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Independent synthetic D8P1 references; standard Python packing/CRC, no C codec."""
import binascii
import struct
import sys
from pathlib import Path


def name(text):
    return text.encode('ascii').ljust(12, b'\0')


def chunks(maximum):
    tracks = bytearray()
    for track in range(8):
        tracks.extend((track * 99 + p) % 192 for p in range(99))
        tracks.extend([track, 255 if track == 7 else track])
        for step in range(64):
            ratchet = step % 4
            tracks.extend([(step + n * 12) % 128 for n in range(4)])
            tracks.extend([step % 5 | (step % 3) << 3 | (step % 4) << 5,
                           (step * 3) % 128 | (ratchet & 1) << 7,
                           255, 1 << (step % 8), step % 102 | (ratchet >> 1) << 7])
    motion = bytearray(b'D8M1' + bytes([64 if maximum else 0, 255, 0, 0]))
    if maximum:
        for i in range(64):
            motion.extend(bytes([i % 8, i // 8, i % 17 | (i % 2) << 7]))
            motion.extend(struct.pack('<h', (i * 3) % 192 - 64))
    result = [(0x8001, struct.pack('<27h', *range(-13, 14))),
              (0x8002, bytes(tracks)), (0x8003, bytes(range(128)) * 8),
              (0x8004, bytes(motion)),
              (0x8005, bytes([16]) + bytes(x for i in range(16) for x in [i, i + 1]) if maximum else b'\0'),
              (0x8006, name('CODEC TEST') + bytes([7, 2, 0, 0]))]
    if maximum:
        banks = bytes([4]) + b''.join(name(f'BANK {i}') + bytes(x for t in range(8) for x in [i, t]) for i in range(4))
        scenes = bytes([16]) + b''.join(name(f'SCENE {i}') + bytes([i % 4, 7, i]) + bytes(x for t in range(8) for x in [(i + t) % 128, (i * 7 + t) % 128, (i + t) % 49]) for i in range(16))
        result.extend([(7, banks), (8, scenes)])
    return result


def encode(parts):
    payload = b''.join(struct.pack('<HHI', kind, len(data), binascii.crc32(data)) + data for kind, data in parts)
    header = bytearray(b'D8P1' + struct.pack('<HHII', 1, 32, 32 + len(payload), 0) + bytes([8, 64, 99, 27, len(parts), 1, 1]) + bytes(9))
    result = header + payload
    struct.pack_into('<I', result, 12, binascii.crc32(result))
    return bytes(result)


if __name__ == '__main__':
    out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
    for filename, parts in [('minimal.d8p', chunks(False)), ('maximum.d8p', chunks(True)),
                            ('unknown-optional.d8p', chunks(False) + [(42, b'KEEP THIS DATA')])]:
        raw = encode(parts)
        (out / filename).write_bytes(raw)
        print(filename, len(raw))
    assert len(encode(chunks(True))) == 7705
