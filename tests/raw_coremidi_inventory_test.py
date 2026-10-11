#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Raw inventory/duplicate refusal tests; never imports or opens native MIDI."""
from pathlib import Path
import sys,threading,json,tempfile
from types import SimpleNamespace
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import dabbl8_readonly_midi as t
import dabbl8_instrument_capture as capture
checks=0

def check(ok,label):
 global checks;checks+=1
 if not ok:raise AssertionError(label)
def refuses(call,label):
 try:call()
 except (ValueError,RuntimeError,ImportError):check(True,label)
 else:raise AssertionError(label)
class Native:
 API_MACOSX_CORE=1
 def __init__(self):
  self.inputs=['selected'];self.outputs=['selected'];self.clients=[];self.api=1;self.compiled=[1];self.fail_scan=False;self.fail_delete=False;self.fail_create=False;self.fail_api=False;self.fail_module=False;self.input_scans=0;self.disappear=False
 def get_compiled_api(self):return self.compiled
 def MidiIn(self,rtapi):return self.create('input',rtapi)
 def MidiOut(self,rtapi):return self.create('output',rtapi)
 def create(self,kind,rtapi):
  check(rtapi==1,'exact native requested API')
  if self.fail_create:raise RuntimeError('create failed')
  n=self
  class Client:
   deleted=0
   def get_current_api(self):
    if n.fail_api:raise RuntimeError('API query failed')
    return n.api
   def get_ports(self):
    if n.fail_scan:raise RuntimeError('scan failed')
    if kind=='input':
     n.input_scans+=1
     if n.disappear and n.input_scans==2:return []
    return n.inputs if kind=='input' else n.outputs
   def delete(self):
    self.deleted+=1
    if n.fail_delete:raise RuntimeError('delete failed')
  c=Client();self.clients.append(c);return c
class Backend:
 name='mido.backends.rtmidi';api='MACOSX_CORE'
 def __init__(self,n):self.n=n;self.loads=0;self.opened=[];self.sent=[];self.exchange=None
 @property
 def module(self):
  self.loads+=1
  if self.n.fail_module:raise ImportError('native module unavailable')
  return SimpleNamespace(rtmidi=self.n)
 def get_input_names(self):return list(dict.fromkeys(self.n.inputs))
 def get_output_names(self):return list(dict.fromkeys(self.n.outputs))
 def open_input(self,name,callback):self.opened.append('input');self.callback=callback;return self.port()
 def open_output(self,name):self.opened.append('output');return self.port()
 def port(self):
  b=self
  class Port:
   closed=False
   def close(self):self.closed=True
   def send(self,msg):
    b.sent.append(msg)
    data=(125,70,76,78,1,0,0,1,0,0,0,0,0,*bytes(6))
    if b.exchange is not None:data=tuple(b.exchange(bytes((240,*msg.data,247)))[1:-1])
    b.callback(SimpleNamespace(type='sysex',data=data))
  return Port()
def msg(kind,data):return SimpleNamespace(type=kind,data=data)
def link(raw):return t.ReadOnlyMidi(raw,msg,'selected','selected',settle=0,timeout=.01)
for direction in ('inputs','outputs'):
 n=Native();setattr(n,direction,['selected','selected']);b=Backend(n);raw=t.StrictCoreMidiBackend(b)
 check(b.loads==0,'wrapper constructor lazy');check(getattr(b,'get_'+direction[:-1]+'_names')()==['selected'],'simulated Mido coalesces duplicate')
 refuses(lambda:link(raw),'raw duplicate cannot be hidden');check(not b.opened and not b.sent,'duplicate refuses before open/send');check(all(c.deleted==1 for c in n.clients),'duplicate inventories deleted once')
n=Native();n.inputs=['x','selected','selected'];b=Backend(n);raw=t.StrictCoreMidiBackend(b);check(raw.get_input_names()==n.inputs,'order+duplicates retained');check(n.clients[-1].deleted==1,'success deletes exactly once')
for attr,value in [('api',2),('api',True),('inputs',['']),('inputs',[None]),('inputs',['a'*1025]),('inputs',['a\0b']),('inputs',['a']*257),('inputs',('selected',)),('fail_scan',True),('fail_delete',True),('fail_create',True),('fail_api',True),('fail_module',True),('disappear',True)]:
 n=Native();setattr(n,attr,value);b=Backend(n);raw=t.StrictCoreMidiBackend(b);refuses(lambda:link(raw),'API/count/name/create/scan/delete failure refuses');check(not b.opened and not b.sent,'failure no fallback/open/send');check(all(c.deleted==1 for c in n.clients),'failure cleanup attempted once')
for compiled in ([],None,(1,),[True],[1]*33):
 n=Native();n.compiled=compiled;b=Backend(n);raw=t.StrictCoreMidiBackend(b);refuses(raw.get_input_names,'uncompiled/malformed/oversize API list');check(not n.clients and not b.opened,'no fallback API')
b=Backend(Native());b.api=None;refuses(lambda:t.StrictCoreMidiBackend(b),'implicit API refused')
n=Native();b=Backend(n);raw=t.StrictCoreMidiBackend(b);l=link(raw);check(b.opened==['input','output'],'unique explicit endpoints open fake only');check(all(c.deleted==1 for c in n.clients),'all initial scan clients deleted');n.inputs=['selected','selected'];refuses(lambda:l.exchange(t.HEADER+b'\1\0\xf7'),'new duplicate before next exchange');check(not b.sent and l.closed,'new duplicate no send or reopen')
n=Native();b=Backend(n);raw=t.StrictCoreMidiBackend(b);errors=[]
def other():
 try:raw.get_input_names()
 except ValueError:errors.append(1)
th=threading.Thread(target=other);th.start();th.join();check(errors and not n.clients,'other thread cannot allocate/cleanup native clients')
n=Native();b=Backend(n);raw=t.StrictCoreMidiBackend(b);n.outputs=['selected','selected'];refuses(lambda:raw.open_output('selected'),'direct wrapper open refuses raw duplicate');check(not b.opened,'no direct delegated open')
if len(sys.argv)==2:
 with tempfile.TemporaryDirectory() as temp:
  for mode in ('full','raw-store'):
   with capture.Bridge(Path(sys.argv[1]).resolve()) as actual:
    n=Native();b=Backend(n);b.exchange=actual.exchange;raw=t.StrictCoreMidiBackend(b);l=link(raw)
    result=capture.collect(l.exchange,Path(temp)/mode,mode,'offline raw CoreMIDI inventory wrapper / actual C virtual NOR')
    stats=json.loads(actual.line('#stats'));l.close()
    check(result['accepted'],result.get('reason','actual collector accepted through strict wrapper'))
    check(stats['writes']==0 and stats['unchanged'],'actual whole NOR unchanged / zero writes')
    check(len(result['objects'])==(17 if mode=='full' else 12),'complete capture through raw inventory')
    check(all(c.deleted==1 for c in n.clients),'all raw scan clients cleaned exactly once through fullcapture')
    check(not result['hardware_qualified'] and not result['migration_authorized'],'no physical or migration authority')
else:
 print('SKIP actual C capture wrapper: pass compiled capture fixture')
print('raw CoreMIDI inventory:',checks,'checks PASS; native MIDI not imported')
