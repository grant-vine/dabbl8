#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Source-derived plan and actual apply/restore simulation; never device I/O."""
import copy
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import dabbl8_instrument_plan as plan
import dabbl8_instrument_migrate as host
from instrument_migration_host_test import archive,hashes,raw_expected
checks=0

def check(ok,label):
    global checks
    checks+=1
    if not ok:raise AssertionError(label)

def refused(fn,label):
    try:fn()
    except ValueError:check(True,label)
    else:check(False,label)

def pair(body,role,slot=0,seq=1):
    result=bytearray(b'\xff'*8192)
    h=struct.pack('<IHH5I',0x554c4546,9 if role==4 else role+1,slot,seq,len(body),zlib.crc32(body),0xffffffff,0xffffffff)
    result[slot*4096:slot*4096+32]=h+struct.pack('<I',zlib.crc32(h))
    result[slot*4096+256:slot*4096+256+len(body)]=body
    return bytes(result)

def checksum(b):
    h=2166136261
    for x in b[:-4]:h=((h^x)*16777619)&0xffffffff
    struct.pack_into('<I',b,len(b)-4,h)
    return bytes(b)

def standalone(n):
    # Explicit synthetic variants of immutable fixtures: remove only their
    # fourth-slot song chain, retaining layout and test music. Never modify files
    # or goldens; original immutable chain fixtures separately prove refusal.
    body=bytearray((ROOT/f'tests/fixtures/projects/fun{n}.bin').read_bytes())
    if n>=6:
        offset=3346 if n==6 else 68+4*(body[66]+2+64*9)
        body[offset:offset+36]=b'\0'*36
        return checksum(body)
    return bytes(body)

def fixture(t,label,bodies,auto=True):
    image=bytearray(bytes((a^(a>>9)^0xa5)&255 for a in range(1048576)))
    for r in range(5):
        data=b'\xff'*8192 if r not in bodies or (r==4 and not auto) else pair(bodies[r],r)
        a=host.RAW_MAP[r][0];image[a:a+8192]=data
    source=t/label;roles,m=archive(source,bytes(image));current=t/(label+'-current.bin');current.write_bytes(image)
    return source,current,roles,m

def main():
    if len(sys.argv)!=5:raise SystemExit('usage: instrument_plan_test.py CONVERTER INITIALIZER ARCHIVE_ADAPTER EXECUTOR')
    converter,initializer,adapter,executor=[Path(p).resolve() for p in sys.argv[1:]]
    with tempfile.TemporaryDirectory(prefix='dabbl8-source-plan-') as tmp:
        t=Path(tmp)
        for n in range(1,10):
            frozen=(ROOT/f'tests/fixtures/projects/fun{n}.bin').read_bytes()
            check(plan.frame(frozen)==n,'immutable FUN framing')
            body=standalone(n)
            check(plan.frame(body)==n,'standalone synthetic framing')
            source,current,roles,m=fixture(t,f'fun{n}',{r:body for r in range(5)})
            before=hashes(source)
            destination=t/f'plan{n}'
            result=plan.prepare(source,destination,converter,initializer,adapter,'persisted')
            check(result['accepted'],f'source-derived FUN{n}: '+str(result.get('reason')))
            check(result['source_encoded_conversion_verified'] and not result['source_runtime_musical_equivalence'],'encoded only')
            check(result['available_project_mask']==7 and result['original_project_mask']==15,'three slots and archived fourth')
            for r in range(4):
                expected,meta=plan.convert(converter,'legacy',destination/f'selected-legacy-role-{r if r<3 else 4}.bin',7)
                check((destination/f'object-{r}.d8p').read_bytes()==expected,'actual selected conversion binding')
                check(result['objects'][str(r)]['sha256']==host.digest(expected),'output hash binding')
            check(hashes(destination/'retained')==before,'all17 exact source retention')
            check(not (destination/'object-4.d8p').exists(),'no fourth alias')
            if n in (1,6,9):
                applied=t/f'apply{n}'
                ar=host.simulate(source,applied,executor,current,'apply',destination/'proposed-native-pool.bin',reviewed_plan=True)
                check(ar['accepted'],'actual source-derived apply '+str(ar.get('reason')))
                check((applied/'simulated-result.bin').read_bytes()==raw_expected(current.read_bytes(),roles,(destination/'proposed-native-pool.bin').read_bytes()),'whole apply image')
                restored=t/f'restore{n}'
                rr=host.simulate(source,restored,executor,applied/'simulated-result.bin','restore')
                check(rr['accepted'] and (restored/'simulated-result.bin').read_bytes()==current.read_bytes(),'whole exact original restoration')
            check(hashes(source)==before,'external source unchanged')
        body=standalone(9)
        source,current,roles,m=fixture(t,'sparse',{0:body,4:body})
        r=plan.prepare(source,t/'sparse-plan',converter,initializer,adapter,'persisted')
        check(r['accepted'] and r['available_project_mask']==1 and set(r['objects'])=={'0','3'},'sparse exact absence')
        source,current,roles,m=fixture(t,'erased-auto',{0:body,1:body,2:body},auto=False)
        r=plan.prepare(source,t/'erased-refusal',converter,initializer,adapter,'persisted')
        check(not r['accepted'] and not (t/'erased-refusal/proposed-native-pool.bin').exists(),'erased auto no implicit bootstrap')
        r=plan.prepare(source,t/'explicit-current',converter,initializer,adapter,'current')
        check(r['accepted'] and (t/'explicit-current/object-3.d8p').read_bytes()==roles[12],'explicit canonical current exact autosave')
        # Every original source with fourth-slot references refuses surviving
        # manual/autosave conversion; archival fourth alone keeps that identity.
        for n in range(6,10):
            frozen=(ROOT/f'tests/fixtures/projects/fun{n}.bin').read_bytes()
            source,current,roles,m=fixture(t,f'refs{n}',{0:frozen,1:body,2:body,3:body,4:body})
            r=plan.prepare(source,t/f'refs-refuse{n}',converter,initializer,adapter,'persisted')
            check(not r['accepted'] and not (t/f'refs-refuse{n}/proposed-native-pool.bin').exists(),'no fourth reference remap')
        frozen=(ROOT/'tests/fixtures/projects/fun9.bin').read_bytes()
        source,current,roles,m=fixture(t,'fourth-refs',{0:body,1:body,2:body,3:frozen,4:body})
        r=plan.prepare(source,t/'fourth-only',converter,initializer,adapter,'persisted')
        check(r['accepted'] and r['conversions']['3']['metadata']['referenced_project_mask']==9,'fourth independently validated original context')
        # Wrap selection, both directions, preserves every unused generation.
        a=pair(standalone(1),0,0,0xffffffff);b=pair(body,0,1,0)
        selected,meta=plan.recover(a[:4096]+b[4096:],0)
        check(selected==body and meta['copy']==1 and meta['sequence']==0,'rollover generation')
        selected,meta=plan.recover(pair(body,0,0,0)[:4096]+pair(standalone(1),0,1,0xffffffff)[4096:],0)
        check(selected==body and meta['copy']==0,'reverse rollover')
        # Complete archive rollover recovery is bound through actual conversion,
        # whole-set writer/readback, physical mapped APPLY and exact RESTORE.
        source,current,roles,m=fixture(t,'rollover',{r:body for r in range(5)})
        image=bytearray(current.read_bytes());a=host.RAW_MAP[0][0]
        image[a:a+8192]=pair(standalone(1),0,0,0xffffffff)[:4096]+pair(body,0,1,0)[4096:]
        current.write_bytes(image)
        (source/host.FILES[0]).write_bytes(bytes(image[a:a+8192]))
        roles=tuple((source/name).read_bytes() for name in host.FILES)
        from instrument_migration_host_test import manifest,encoded
        (source/'manifest.json').write_bytes(encoded(manifest(roles)))
        r=plan.prepare(source,t/'rollover-plan',converter,initializer,adapter,'persisted')
        check(r['accepted'] and r['legacy_sources'][0]['copy']==1 and r['legacy_sources'][0]['sequence']==0,'complete rollover provenance')
        ar=host.simulate(source,t/'rollover-apply',executor,current,'apply',t/'rollover-plan/proposed-native-pool.bin',reviewed_plan=True)
        check(ar['accepted'],'actual rollover mapped apply')
        rr=host.simulate(source,t/'rollover-restore',executor,t/'rollover-apply/simulated-result.bin','restore')
        check(rr['accepted'] and (t/'rollover-restore/simulated-result.bin').read_bytes()==bytes(image),'rollover originals including unused old generation')
        # Header/body/FUN refusals also occur with complete archive retention and
        # no partially published derived objects, rather than parser-only tests.
        source,current,roles,m=fixture(t,'damaged-archive',{r:body for r in range(5)})
        raw=bytearray(roles[1]);raw[28]^=1
        (source/host.FILES[1]).write_bytes(raw)
        roles=tuple((source/name).read_bytes() for name in host.FILES)
        (source/'manifest.json').write_bytes(encoded(manifest(roles)))
        before=hashes(source)
        r=plan.prepare(source,t/'damaged-plan',converter,initializer,adapter,'persisted')
        check(not r['accepted'] and hashes(t/'damaged-plan/retained')==before,'damaged raw originals retained exactly')
        check(not list((t/'damaged-plan').glob('object-*.d8p')) and not (t/'damaged-plan/proposed-native-pool.bin').exists(),'no failed partial publication')
        for seq in (0,0x80000000):
            refused(lambda:plan.recover(pair(body,0,0,0)[:4096]+pair(body,0,1,seq)[4096:],0),'equal/half-range ambiguity')
        for fault in ('magic','hcrc','bcrc','fun-checksum','foreign','physical','oversize','FUN-tag'):
            raw=bytearray(pair(body,0))
            if fault=='magic':raw[0]^=1
            elif fault=='hcrc':raw[28]^=1
            elif fault=='bcrc':raw[300]^=1
            elif fault=='fun-checksum':raw[256+len(body)-1]^=1;struct.pack_into('<I',raw,16,zlib.crc32(raw[256:256+len(body)]));struct.pack_into('<I',raw,28,zlib.crc32(raw[:28]))
            elif fault=='foreign':struct.pack_into('<H',raw,4,9);struct.pack_into('<I',raw,28,zlib.crc32(raw[:28]))
            elif fault=='physical':struct.pack_into('<H',raw,6,1);struct.pack_into('<I',raw,28,zlib.crc32(raw[:28]))
            elif fault=='oversize':struct.pack_into('<I',raw,12,3841);struct.pack_into('<I',raw,28,zlib.crc32(raw[:28]))
            else:raw[256]^=1;struct.pack_into('<I',raw,16,zlib.crc32(raw[256:256+len(body)]));struct.pack_into('<I',raw,28,zlib.crc32(raw[:28]))
            refused(lambda:plan.recover(bytes(raw),0),'conservative '+fault)
            # Another valid copy does not turn unsupported residue into absence.
            refused(lambda:plan.recover(bytes(raw[:4096])+pair(body,0,1,2)[4096:],0),'torn other copy '+fault)
        source,current,roles,m=fixture(t,'tool-tamper',{0:body,1:body,2:body,3:body,4:body})
        fake=t/'tamper-converter'
        fake.write_text('#!/usr/bin/env python3\nimport subprocess,sys\nfrom pathlib import Path\n'+
                        f'p=subprocess.run([{str(converter)!r},*sys.argv[1:]],capture_output=True)\n'+
                        "Path(sys.argv[2]).write_bytes(b'changed')\nsys.stdout.buffer.write(p.stdout)\nsys.stderr.buffer.write(p.stderr)\nsys.exit(p.returncode)\n")
        fake.chmod(0o700)
        r=plan.prepare(source,t/'tamper-refused',fake,initializer,adapter,'persisted')
        check(not r['accepted'] and 'changed during conversion' in r['reason'],'selected source mutation refuses')
        check(hashes(t/'tamper-refused/retained')==hashes(source),'raw originals survive changed selected input')
        try:plan.prepare(source,t/'tamper-refused',converter,initializer,adapter,'persisted')
        except FileExistsError:check(True,'existing output refused')
        else:check(False,'existing output refused')
        # Even plausible JSON booleans cannot masquerade as protocol integers.
        source,current,roles,m=fixture(t,'boolean-tools',{0:body,4:body})
        fake=t/'bool-converter'
        fake.write_text('#!/usr/bin/env python3\nimport subprocess,sys,json\n'+
                        f'p=subprocess.run([{str(converter)!r},*sys.argv[1:]],capture_output=True)\n'+
                        "m=json.loads(p.stdout if sys.argv[1]=='check' else p.stderr);m['available_project_mask']=True\n"+
                        "sys.stdout.buffer.write((json.dumps(m)+'\\n').encode() if sys.argv[1]=='check' else p.stdout)\n"+
                        "sys.stderr.buffer.write(b'' if sys.argv[1]=='check' else (json.dumps(m)+'\\n').encode());sys.exit(p.returncode)\n")
        fake.chmod(0o700)
        r=plan.prepare(source,t/'bool-converter-refuse',fake,initializer,adapter,'persisted')
        check(not r['accepted'],'converter boolean mask refuses')
        fake=t/'bool-initializer'
        fake.write_text('#!/usr/bin/env python3\nimport subprocess,sys,json\n'+
                        f'p=subprocess.run([{str(initializer)!r},*sys.argv[1:]],capture_output=True)\n'+
                        "m=json.loads(p.stdout);m['version']=True;print(json.dumps(m));sys.exit(p.returncode)\n")
        fake.chmod(0o700)
        r=plan.prepare(source,t/'bool-initializer-refuse',converter,fake,adapter,'persisted')
        check(not r['accepted'] and not (t/'bool-initializer-refuse/proposed-native-pool.bin').exists(),'initializer boolean version refuses')
        r=plan.prepare(source,t/'no-choice',converter,initializer,adapter,None)
        check(not r['accepted'],'explicit autosave choice mandatory')
    print(f'PASS: {checks} source-derived instrument plan checks; encoded provenance only, no runtime/device equivalence')

if __name__=='__main__':main()
