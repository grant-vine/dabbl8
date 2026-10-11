#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Actual collector/command79/driver bridge; virtual NOR, no device transport."""
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import dabbl8_instrument_capture as capture
import dabbl8_instrument_migrate as migration
import dabbl8_instrument_session as session
bridge_path,converter,initializer,adapter,production=map(lambda x:Path(x).resolve(),sys.argv[1:6])
checks=0

def check(ok,label):
    global checks
    checks+=1
    if not ok:raise AssertionError(label)

def originals(dest):
    manifest=json.loads((dest/'verified/manifest.json').read_text())
    check(len(manifest['objects'])==17,'full17 publication')
    for item in manifest['objects']:
        raw=(dest/item['file']).read_bytes()
        check(len(raw)==item['bytes'] and hashlib.sha256(raw).hexdigest()==item['sha256'],'original exact hash/size')
        check((dest/'verified'/item['file']).read_bytes()==raw,'published original exact')
        check((dest/'independent-originals/retained'/item['file']).read_bytes()==raw,'independently retained original exact')
    return manifest

def run(bridge,dest,source='current',operation='apply',exchange=None):
    return session.run(exchange or bridge.exchange,dest,operation=operation,autosave_source=source,
                       confirm=lambda:check(bridge.line('#consent')=='ok','actual private panel consent'),
                       converter=converter,initializer=initializer,adapter=adapter)

def mutate(reply,index,value):
    out=bytearray(reply);out[index]=value;return bytes(out)

with tempfile.TemporaryDirectory() as directory:
    td=Path(directory)
    for source in ('current','persisted'):
        with capture.Bridge(bridge_path) as bridge:
            dest=td/source;report=run(bridge,dest,source)
            check(report['completed'] and report['observed_own_operations']==180,'actual source-derived apply completes')
            check(all(not report[x] for x in ('device_writes','hardware_qualified','production_ownership','reboot_permission_retained')),'no physical or persistent permission claim')
            originals(dest)
            frames=[json.loads(x) for x in (dest/'received-frames.jsonl').read_text().splitlines()]
            check(not any(bytes.fromhex(x['request_hex'])[6]==6 for x in frames),'successful live handoff consumes capture before ordinary cleanup')
            before=json.loads(bridge.line('#stats'));check(before['phase']==9,'actual READY dispatch')
            check(bridge.line('#reboot')=='ok','actual session metadata reset')
            after=json.loads(bridge.line('#stats'));check(after['phase']==0 and after['nor_crc']==before['nor_crc'],'reboot preserves NOR and loses session')
            resume=td/(source+'-resume');rr=run(bridge,resume,'persisted','resume')
            check(rr['completed'] and rr['observed_own_operations']==0,'explicit native rotating-inventory resume read-only')
            stats=json.loads(bridge.line('#stats'));check(stats['nor_crc']==before['nor_crc'],'resume entire NOR unchanged')
            originals(resume)
    # Strict parser/correlation failures occur after complete retained originals,
    # but before any physical callback. No usable migration receipt is produced.
    variants=(('truncated',lambda p:p[:-1]),('wrong-version',lambda p:mutate(p,5,2)),
              ('high-bit',lambda p:mutate(p,23,128)),('wrong-generation',lambda p:mutate(p,8,0)),
              ('wrong-sequence',lambda p:mutate(p,13,1)),('noncanonical-u32',lambda p:mutate(p,12,16)),
              ('unknown-phase',lambda p:mutate(p,7,11)),('wrong-command',lambda p:mutate(p,4,78)))
    for name,alter in variants:
        with capture.Bridge(bridge_path) as bridge:
            before=json.loads(bridge.line('#stats'));done=[False]
            def exchange(frame):
                reply=bridge.exchange(frame)
                if frame[4]==79 and not done[0]:done[0]=True;return alter(reply)
                return reply
            dest=td/name;report=run(bridge,dest,exchange=exchange)
            check(done[0] and not report['completed'],'malformed/correlation refusal '+name)
            originals(dest)
            check(bridge.line('#tick')=='ok','actual main-loop cleanup after refusal')
            stats=json.loads(bridge.line('#stats'))
            check(stats['writes']==0 and stats['nor_crc']==before['nor_crc'],'refused handoff whole NOR unchanged '+name)
    with capture.Bridge(production) as bridge:
        dest=td/'shipping';before=json.loads(bridge.line('#stats'));report=run(bridge,dest)
        check(not report['completed'],'shipping quarantine denies migration callback')
        originals(dest);stats=json.loads(bridge.line('#stats'))
        check(stats['physical_calls']==0 and stats['nor_crc']==before['nor_crc'],'shipping no writes exact originals')
    with capture.Bridge(bridge_path) as bridge:
        check(bridge.line('#cut 0')=='ok','private first erase failure')
        dest=td/'cut';report=run(bridge,dest)
        check(not report['completed'],'uncertain destructive attempt refuses')
        originals(dest);stats=json.loads(bridge.line('#stats'))
        check(stats['phase']==10 and stats['physical_calls']==1,'actual failed/offline completion after attempt')
    called=[]
    try:session.run(lambda frame:called.append(frame),td/'invalid',operation='resume',autosave_source='current',confirm=lambda:None)
    except ValueError:check(not called and not (td/'invalid').exists(),'resume source choice refuses before transport or files')
    else:raise AssertionError('implicit current resume accepted')
    # Session failure is sticky at the host parser, even with an otherwise
    # canonical response and increasing own counter.
    def failed(frame):return session.HEADER+bytes((1,0,10))+capture.u32(1)+capture.u32(0)+capture.u32(0)+capture.u32(0)+b'\xf7'
    try:session.Session(failed,io.StringIO()).ask(1,b'',begin=True)
    except ValueError:check(True,'failed phase never accepted as completion')
    else:raise AssertionError('FAILED accepted')
print(f'Connected instrument collector/session: {checks} checks; actual private C bridge; no hardware qualification')
