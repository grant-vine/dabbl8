#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Injected MIDI ports plus actual C capture bridge; never opens native endpoints."""
import io
import hashlib
from contextlib import nullcontext
import json
from pathlib import Path
import sys
import tempfile
import threading
from types import SimpleNamespace
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import dabbl8_readonly_midi as transport
import dabbl8_instrument_capture as capture
checks = 0


def check(value, label):
    global checks
    checks += 1
    if not value:
        raise AssertionError(label)


def reply(frame):
    op=frame[6]
    payload={0:bytes(6),1:b'',2:bytes(6),3:bytes(31),5:b'',6:b''}.get(op)
    if op==4:
        count=frame[18]|(frame[19]<<7);payload=bytes(8+count+(count+6)//7)
    return capture.HEADER + bytes((1, op, 0)) + capture.u32(1) + bytes((0,)) + payload + b'\xf7'


class Port:
    def __init__(self, backend, output):
        self.backend, self.output = backend, output
        self.closed = False; self.closes = 0
    def send(self, message):
        frame = bytes((240, *message.data, 247));self.backend.sent.append(frame)
        if self.backend.action:
            self.backend.action(frame)
    def close(self):
        self.closed = True; self.closes += 1
        if self.backend.close_error:
            raise ValueError('injected close failure')


class Backend:
    def __init__(self):
        self.sent = []; self.names = ['selected']; self.action = lambda f: self.emit(reply(f))
        self.callback = None;self.ports = [];self.fail_output = False;self.close_error = False
    def get_input_names(self):return self.names
    def get_output_names(self):return self.names
    def open_input(self, name, callback):
        check(name == 'selected', 'exact selected input');self.callback = callback
        p=Port(self,False);self.ports.append(p);return p
    def open_output(self, name):
        check(name == 'selected', 'exact selected output')
        if self.fail_output:raise ValueError('output refused')
        p=Port(self,True);self.ports.append(p);return p
    def emit(self, frame):self.callback(SimpleNamespace(type='sysex',data=tuple(frame[1:-1])))


def message(kind, data):return SimpleNamespace(type=kind,data=data)
def link(b):return transport.ReadOnlyMidi(b,message,'selected','selected',timeout=0.01,settle=0)
def request(op=0,suffix=b''):
    return capture.HEADER+bytes((1,op))+(b'' if op<2 else capture.u32(1))+suffix+b'\xf7'
def refuses(call,label):
    try:call()
    except ValueError:check(True,label)
    else:raise AssertionError(label)


valid=[request(),request(1,b'\0'),request(1,b'\1'),request(2),request(3,b'\x10'),request(4,b'\0'+capture.u32(0)+bytes((0,2))),request(5),request(6)]
for frame in valid:
    b=Backend();l=link(b);check(l.exchange(frame)==reply(frame),'actual frame roundtrip');check(b.sent==[frame],'one exact send');l.close();check(all(p.closes==1 for p in b.ports),'close once')
invalid=[b'',valid[0][:-1],valid[0]+b'\0',bytes((240,125,70,76,79,1,0,247)),bytes((240,125,70,76,0,247)),request(7),request(1,b'\2'),request(3,b'\x11'),request(4,b'\0'+capture.u32(0)+b'\0\0'),request(4,b'\0'+capture.u32(0)+b'\1\2'),request(2,b'\0'),capture.HEADER+b'\2\0\xf7',capture.HEADER+b'\1\2\0\0\0\0\x10\xf7',request(4,b'\0'+capture.u32(81920)+b'\1\0')]
for frame in invalid:
    b=Backend();l=link(b);refuses(lambda:l.exchange(frame),'unsafe request refused');check(not b.sent and l.closed,'no unsafe send and poisoned link')
for names in ([],['selected','selected'],['other']):
    b=Backend();b.names=names;refuses(lambda:link(b),'absent/ambiguous explicit names refused');check(not b.ports,'no default opening')
b=Backend();b.fail_output=True;refuses(lambda:link(b),'partial open fails');check(b.ports[0].closed,'partial input cleaned')
variants=[('wrong command',lambda f:bytes((240,125,70,76,79))+reply(f)[5:]),('wrong operation',lambda f:reply(request(2))),('truncated',lambda f:b'\xf0\x7d\x46\x4c\x4e\xf7'),('noncanonical token',lambda f:reply(f)[:12]+b'\x10'+reply(f)[13:]),('oversize',lambda f:capture.HEADER+bytes(600)+b'\xf7')]
for label,alter in variants:
    b=Backend();b.action=lambda f:b.emit(alter(f));l=link(b);refuses(lambda:l.exchange(request()),label);check(l.closed,'malformed reply poisons');refuses(lambda:l.exchange(request()),'no retry');check(len(b.sent)==1,'one send only after poison')
b=Backend();b.action=lambda f:b.emit(reply(f)[:8]+capture.u32(2)+reply(f)[13:]);l=link(b);refuses(lambda:l.exchange(request(2)),'stale token');check(l.closed,'stale token poisons')
b=Backend();b.action=lambda f:b.emit(reply(f)[:-2]+b'\xf7');l=link(b);refuses(lambda:l.exchange(request()),'wrong success payload size')
b=Backend();b.action=lambda f:b.emit(reply(f)[:13]+b'\x05'+reply(f)[14:]);l=link(b);refuses(lambda:l.exchange(request()),'unknown phase')
b=Backend();l=transport.ReadOnlyMidi(b,message,'selected','selected',settle=0,record=lambda frame:(_ for _ in ()).throw(OSError('journal failed')));refuses(lambda:l.exchange(request()),'receive journal failure poisons')
b=Backend();b.action=lambda f:(b.emit(reply(f)),b.emit(reply(f)));l=link(b);refuses(lambda:l.exchange(request()),'duplicate queued response');check(l.closed,'overflow closes')
b=Backend();l=link(b);b.emit(reply(request()));refuses(lambda:l.exchange(request()),'idle unsolicited reply');check(not b.sent,'unsolicited cannot acknowledge later request')
b=Backend();b.action=None;l=link(b);refuses(lambda:l.exchange(request()),'timeout');b.emit(reply(request()));refuses(lambda:l.exchange(request()),'late reply cannot retry');check(len(b.sent)==1,'no timeout retry')
b=Backend();l=link(b);b.ports[0].closed=True;refuses(lambda:l.exchange(request()),'closed endpoint');check(not b.sent,'closed endpoint no send')
b=Backend();l=link(b);b.names=[];refuses(lambda:l.exchange(request()),'disconnected endpoint');check(not b.sent and l.closed,'disconnected endpoint no send or reopen')
b=Backend();b.action=lambda f:setattr(b.ports[0],'closed',True);l=link(b);refuses(lambda:l.exchange(request()),'disconnect while waiting times out');check(l.closed and len(b.sent)==1,'waiting disconnect fail closed no retry')
b=Backend();b.action=lambda f:(_ for _ in ()).throw(ValueError('send failed'));l=link(b);refuses(lambda:l.exchange(request()),'send failure');check(l.closed,'send failure closed')
b=Backend();l=link(b);b.callback(SimpleNamespace(type='note_on'));check(l.exchange(request())==reply(request()),'ordinary notes ignored');l.close()
b=Backend();l=link(b);errors=[]
def other_thread():
    try:l.exchange(request())
    except ValueError as error:errors.append(str(error))
t=threading.Thread(target=other_thread);t.start();t.join();check(errors and not b.sent,'second thread cannot send');l.close()
b=Backend();l=link(b);b.close_error=True;refuses(l.close,'cleanup failures observable');check(all(p.closes==1 for p in b.ports),'both ports attempted despite close failure')
for text in ('#consent\n','f0\n','f0','ff '*700+'\n','f0  7d\n'):
    b=Backend();l=link(b);refuses(lambda:transport.bridge(l,io.StringIO(text),io.StringIO()),'hostile bridge input');check(not b.sent,'hostile text no send');l.close()
b=Backend();l=link(b);stream=io.StringIO();transport.bridge(l,io.StringIO(request().hex(' ')+'\n'),stream);check(stream.getvalue()==reply(request()).hex(' ')+'\n','hex bridge exact framing');l.close()

if len(sys.argv) in (2,3):
    executable=Path(sys.argv[1]).resolve()
    context = tempfile.TemporaryDirectory() if len(sys.argv)==2 else nullcontext(sys.argv[2])
    if len(sys.argv)==3:
        Path(sys.argv[2]).mkdir(parents=True,exist_ok=False)
    with context as temp:
        for mode in ('full','raw-store'):
            with capture.Bridge(executable) as actual:
                b=Backend();b.action=lambda f:b.emit(actual.exchange(f));l=link(b)
                result=capture.collect(l.exchange,Path(temp)/mode,mode,'offline injected MIDI ports / actual C virtual NOR')
                stats=json.loads(actual.line('#stats'))
                check(result['accepted'],result.get('reason','actual capture accepted'))
                check(stats['writes']==0 and stats['unchanged'],'all NOR equality and zero write callbacks')
                check(len(result['objects'])==(17 if mode=='full' else 12),'complete roles preserved')
                check(not any(result[k] for k in ('hardware_qualified','restore_qualified','migration_authorized','ownership_authorized','device_writes')),'no authority granted')
                check(b.sent[-1][6]==6,'ordinary collector cleanup retained');l.close()
                (Path(temp)/mode/'nor-stats.json').write_text(json.dumps(stats,indent=2))
        for name,index in (('wrong-read-role',14),('wrong-read-offset',15),('wrong-read-crc',23)):
            with capture.Bridge(executable) as actual:
                b=Backend();changed=[False]
                def corrupt(frame):
                    raw=actual.exchange(frame)
                    if frame[6]==4 and not changed[0]:
                        data=bytearray(raw);data[index]^=1;raw=bytes(data);changed[0]=True
                    b.emit(raw)
                b.action=corrupt;l=link(b)
                dest=Path(temp)/name;result=capture.collect(l.exchange,dest,'full','offline same-op/token hostile reply')
                stats=json.loads(actual.line('#stats'));l.close()
                check(changed[0] and not result['accepted'],'same-op/token hostile READ refused by actual collector')
                check(not (dest/'verified').exists(),'hostile READ publishes no verified snapshot')
                transcript=(dest/'received-frames.jsonl').read_text()
                check('response_hex' in transcript and result['transcript_sha256']==hashlib.sha256((dest/'received-frames.jsonl').read_bytes()).hexdigest(),'exact hostile private transcript retained')
                check(stats['writes']==0 and stats['unchanged'],'hostile READ whole NOR unchanged')
                check(result['reason'].startswith('Original role checksum mismatch' if name=='wrong-read-crc' else 'Wrong chunk role/offset/count echo'),'isolated collector rejection reason')
                (dest/'nor-stats.json').write_text(json.dumps(stats,indent=2))
else:
    print('SKIP actual C bridge: pass an explicitly compiled capture fixture')
print('readonly MIDI adapter:',checks,'checks, PASS; native MIDI never opened')
