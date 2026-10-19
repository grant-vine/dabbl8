# SPDX-License-Identifier: GPL-3.0-only
"""Actual native archive codec over synthetic full snapshots; no device backend."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import dabbl8_arrangement_readback as readback
from dabbl8_instrument_capture import CURRENT_LENGTHS, NAMES, RAW_MAP

ROOT = Path(__file__).resolve().parents[1]
ADAPTER = Path(sys.argv.pop(1))
ADAPTER_SHA = hashlib.sha256(ADAPTER.read_bytes()).hexdigest()


def project(name):
    data = bytearray((ROOT/'tests/fixtures/d8p1/minimal.d8p').read_bytes())
    pos = 32
    while pos < len(data):
        kind, size = struct.unpack_from('<HH', data, pos)
        if kind & 32767 == 6:
            data[pos+8:pos+20] = name.encode().ljust(12, b'\0')
            struct.pack_into('<I', data, pos+4, zlib.crc32(data[pos+8:pos+8+size]))
        pos += size+8
    struct.pack_into('<I', data, 12, 0)
    struct.pack_into('<I', data, 12, zlib.crc32(data))
    return bytes(data)


class ReadbackTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.a, self.b, self.c, self.heavy, self.other = [project(x) for x in ('A','B','C','MIDI','OTHER')]
        self.context = readback.Context(readback.digest(self.heavy), readback.digest(self.c))

    def pool(self, label, *, a=None, b=None, c=None, autosave=None, relocate=False):
        parts = [self.a if a is None else a, self.b if b is None else b,
                 self.c if c is None else c, self.heavy if autosave is None else autosave]
        paths = []
        for index, data in enumerate(parts):
            p = self.root/(label+str(index)+'.d8p')
            p.write_bytes(data)
            paths.append(str(p))
        destination = self.root/(label+'pool.bin')
        subprocess.run([str(ADAPTER),'build',str(destination),'15',*paths], capture_output=True, check=True, timeout=10)
        data = destination.read_bytes()
        if relocate:
            data = data[16384:24576]+data[8192:16384]+data[:8192]+data[24576:]
            data = bytearray(data)
            for block in range(5):
                offset = block*8192
                if data[offset:offset+4] == b'D8SP':
                    data[offset+5] = block
                    struct.pack_into('<I',data,offset+28,zlib.crc32(data[offset:offset+28]))
            data = bytes(data)
        return data

    def snapshot(self, name, pool, live, *, changed_role=None, release=True):
        directory = self.root/name
        verified = directory/'verified'
        verified.mkdir(parents=True)
        objects = []
        for role, label in enumerate(NAMES):
            size = RAW_MAP[role][1]+RAW_MAP[role][3] if role < 12 else CURRENT_LENGTHS.get(role,7936)
            data = pool[8192*role:8192*(role+1)] if role < 5 else live if role == 12 else bytes(size)
            if changed_role == role:
                data = bytes([data[0]^1])+data[1:]
            filename = 'original-'+label+'.bin'
            (verified/filename).write_bytes(data)
            objects.append({'role':role,'name':label,'file':filename,'bytes':len(data),
                            'sha256':readback.digest(data),'crc32':zlib.crc32(data)})
        (verified/'manifest.json').write_text(json.dumps({'format':'dabbl8-instrument-capture','version':1,
            'role_table':1,'mode':'full','token':17,'current_state_complete':True,'raw_bytes':319488,'objects':objects}))
        (directory/'report.json').write_text(json.dumps({'format':'dabbl8-instrument-capture-report','version':1,
            'mode':'full','token':17,'objects':objects,'accepted':True,'current_state_complete':True,'release_acknowledged':release}))
        return directory

    def compare(self, before, after):
        return readback.compare(before,after,ADAPTER,ADAPTER_SHA,self.root/'result',self.context)

    def pair(self, **kwargs):
        before = self.snapshot('before',self.pool('before'),self.heavy)
        after = self.snapshot('after',self.pool('after',**kwargs),self.c)
        return before, after

    def test_known_C_context_with_original_autosave_is_accepted(self):
        result = self.compare(*self.pair())
        self.assertTrue(result['accepted'])
        self.assertFalse(result['autosave_matches_C'] or result['extra_device_exports_requested'])

    def test_C_autosave_and_native_block_relocation_preserve_logical_projects(self):
        result = self.compare(*self.pair(autosave=self.c,relocate=True))
        self.assertTrue(result['protected_A_B_C_equal'] and result['autosave_matches_C'])
        self.assertFalse(result['hardware_qualified'])

    def test_each_changed_protected_project_is_rejected(self):
        for role in ('a','b','c'):
            with self.subTest(role=role), tempfile.TemporaryDirectory() as d:
                self.root = Path(d)
                with self.assertRaisesRegex(ValueError,'Protected A/B/C'):
                    self.compare(*self.pair(**{role:self.other}))

    def test_unknown_autosave_and_live_contexts_are_rejected(self):
        with self.assertRaisesRegex(ValueError,'Unknown final'):
            self.compare(*self.pair(autosave=self.other))

    def test_stale_MIDI_live_state_cannot_be_called_C_playback(self):
        before = self.snapshot('before',self.pool('before'),self.heavy)
        after = self.snapshot('after',self.pool('after'),self.heavy)
        with self.assertRaisesRegex(ValueError,'Unknown final'):
            self.compare(before,after)

    def test_every_ancillary_raw_and_current_role_is_protected(self):
        for role in (*range(5,12),*range(13,17)):
            with self.subTest(role=role), tempfile.TemporaryDirectory() as d:
                self.root = Path(d)
                before = self.snapshot('before',self.pool('before'),self.heavy)
                after = self.snapshot('after',self.pool('after'),self.c,changed_role=role)
                with self.assertRaisesRegex(ValueError,'Settings/presets/samples'):
                    self.compare(before,after)

    def test_unconfirmed_snapshot_release_stops_before_codec_or_publication(self):
        before = self.snapshot('before',self.pool('before'),self.heavy,release=False)
        after = self.snapshot('after',self.pool('after'),self.c)
        with self.assertRaisesRegex(ValueError,'accepted/released'):
            self.compare(before,after)
        self.assertFalse((self.root/'result').exists())

    def test_corrupted_original_or_missing_role_is_refused(self):
        before, after = self.pair()
        (after/'verified/original-current-fm6-presets.bin').write_bytes(b'bad')
        with self.assertRaisesRegex(ValueError,'bytes/CRC/SHA'):
            self.compare(before,after)

    def test_foreign_adapter_hash_or_reused_output_refuses(self):
        before, after = self.pair()
        with self.assertRaisesRegex(ValueError,'adapter changed'):
            readback.compare(before,after,ADAPTER,'0'*64,self.root/'result',self.context)
        self.compare(before,after)
        with self.assertRaises(FileExistsError):
            self.compare(before,after)

    def test_boolean_schema_version_role_or_token_cannot_alias_an_integer(self):
        before, _ = self.pair()
        for relative, field in (('verified/manifest.json','version'),('verified/manifest.json','role_table'),
                                ('report.json','version'),('report.json','token'),('verified/manifest.json','role')):
            with self.subTest(field=field):
                path = before/relative
                raw = path.read_bytes()
                metadata = json.loads(raw)
                if field == 'role':
                    metadata['objects'][0]['role'] = False
                else:
                    metadata[field] = True
                path.write_text(json.dumps(metadata))
                with self.assertRaises(ValueError):
                    readback.snapshot(before)
                path.write_bytes(raw)


if __name__ == '__main__':
    unittest.main()
