// SPDX-License-Identifier: GPL-3.0-only
// Production modules, simulated owned ports/DOM, and optional actual C bridges.
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {createInterface} from 'node:readline';
import {D8MidiSession} from './d8midi.js';
import {mountCompanion} from './d8companion.js';
let checks=0;const ok=(x,label)=>{assert.ok(x,label);checks++;};
const frame=(cmd,args=[])=>[240,125,70,76,cmd,...args,247];
const info=(tracks=4,slots=14)=>[...Buffer.from('test'),0,slots,99,27,64,91,...Array(slots).fill(0),tracks,16,85,1,15,77,1,64,1,66,1,0,70,1,8,0,83,1,3,80,1,3,78,1,0,82,1,4,76,1,1,68,56,1];
const caps=(tracks=4,slots=14)=>[68,56,1,tracks,64,8,64,99,27,slots,1,1,1,9,5];
const observation=(slots=14,mask=0x3ffd)=>[68,56,2,1,slots,mask&127,mask>>7,0];
class Port extends EventTarget{
 constructor(type){super();this.type=type;this.id=type;this.state='connected';this.connection='closed';this.sent=[];this.listeners=0;this.opens=0;this.closes=0;}
 addEventListener(type,fn,opts){super.addEventListener(type,fn,opts);if(type==='midimessage')this.listeners++;}
 removeEventListener(type,fn,opts){super.removeEventListener(type,fn,opts);if(type==='midimessage')this.listeners--;}
 async open(){this.opens++;this.connection='open';return this;}
 async close(){this.closes++;this.connection='closed';}
 send(data){this.sent.push([...data]);this.respond?.(data);}
 receive(data){const e=new Event('midimessage');e.data=data;this.dispatchEvent(e);}
}
function pair({tracks=4,slots=14,reply=observation(slots,slots===13?0x1ffd:0x3ffd)}={}){
 const input=new Port('input'),output=new Port('output');output.respond=(r)=>queueMicrotask(()=>{
  if(r[4]===1)input.receive(frame(1,info(tracks,slots)));
  else if(r[4]===75&&r.length===6)input.receive(frame(75,caps(tracks,slots)));
  else if(r[4]===75&&reply!==null)input.receive(frame(75,reply));
 });return {input,output};
}
for(const tracks of [4,8])for(const slots of [13,14]){
 const p=pair({tracks,slots}),s=await D8MidiSession.connect(p.input,p.output,{engineAvailability:true});
 ok(s.engineAvailability.caps.slots===slots&&!(s.engineAvailability.caps.mask&2),'observed stable mask independently of track writes');
 assert.deepEqual(p.output.sent,[frame(1),frame(75),frame(75,[2])]);ok(p.input.listeners===1&&p.input.opens===1,'single transport ownership');
 await s.close();ok(s.engineAvailability.caps===null&&p.input.listeners===0,'close invalidates observation');
}
for(const reply of [[68,56,1,1,14,125,127,0],[68,56,2,1,14,0,0,0],[68,56,2,1,14,125,127,1],[68,56,2,1,13,125,63,0],observation().slice(1),[...observation(),0]]){
 const p=pair({tracks:8,reply}),s=await D8MidiSession.connect(p.input,p.output,{engineAvailability:true});
 ok(!s.engineAvailability.caps&&s.tracks.editable&&s.connected,'malformed observation never grants or revokes independent track contract');await s.close();
}
for(const reason of [1,2,3,4]){
 const p=pair({tracks:8});const respond=p.output.respond;p.output.respond=(r)=>r.length===7?queueMicrotask(()=>p.input.receive(frame(76,[1,75,reason]))):respond(r);
 const s=await D8MidiSession.connect(p.input,p.output,{engineAvailability:true});ok(!s.engineAvailability.caps&&s.connected&&s.tracks.editable,'explicit optional refusal stays unknown');await s.close();
}
{
 const p=pair({tracks:8,reply:null}),s=await D8MidiSession.connect(p.input,p.output,{engineAvailability:true,timeout:15});
 ok(!s.engineAvailability.caps&&s.connected&&s.tracks.editable,'optional timeout preserves independent client');
 let settled=false;p.output.respond=()=>{};const pending=s.tracks.mix(7,{level:64,mute:true}).then(r=>{settled=true;return r;});
 p.input.receive(frame(75,observation()));p.input.receive(frame(76,[1,75,2]));await Promise.resolve();ok(!settled&&s.connected,'late optional replies cannot acknowledge track edit');
 p.input.receive(frame(77,[1,28,7,64,64,1]));ok((await pending).mute,'correct track ack alone resolves');await s.close();
}
for(const bad of [frame(76,[1,75,5]),frame(77,[]),frame(75,[128])]){
 const p=pair();const respond=p.output.respond;p.output.respond=(r)=>r.length===7?queueMicrotask(()=>p.input.receive(bad)):respond(r);
 await assert.rejects(D8MidiSession.connect(p.input,p.output,{engineAvailability:true}));ok(p.input.listeners===0&&p.input.closes===1,'hostile framing closes and releases owner');
}
{
 const p=pair({reply:null}),a=new AbortController();let query;
 const q=new Promise(r=>query=r),respond=p.output.respond;p.output.respond=(r)=>{respond(r);if(r.length===7)query();};
 const pending=D8MidiSession.connect(p.input,p.output,{engineAvailability:true,signal:a.signal});await q;a.abort();await assert.rejects(pending);ok(p.input.listeners===0,'abort during discovery removes sole listener');
}
{
 const p=pair();p.output.respond=()=>queueMicrotask(()=>p.input.receive(frame(1,[...info().slice(0,-3)])));
 const s=await D8MidiSession.connect(p.input,p.output,{engineAvailability:true});
 ok(s.connected&&!s.engineAvailability.caps&&!s.tracks.editable&&p.output.sent.length===1,'legacy discovery stays connected with unknown availability and no schema guessing');await s.close();
}
{
 const p=pair();await assert.rejects(D8MidiSession.connect(p.input,p.output,{engineAvailability:'yes'}));
 ok(p.input.opens===0&&p.input.listeners===0,'invalid opt-in rejected before opening or ownership');
}
// Minimal DOM exercises actual mount and handlers; not a browser rendering claim.
class Element extends EventTarget{
 constructor(tag,doc,text=''){super();this.tagName=tag.toUpperCase();this.ownerDocument=doc;this.children=[];this.attrs={};this._text=text;this.value='';}
 setAttribute(k,v){this.attrs[k]=String(v);if(k==='value')this.value=String(v);}
 append(...nodes){this.children.push(...nodes);}
 replaceChildren(...nodes){this._text='';this.children=[...nodes];}
 set textContent(v){this._text=String(v);this.children=[];}
 get textContent(){return this._text+this.children.map(n=>n.textContent).join(' ');}
 querySelectorAll(tags){const names=tags.toUpperCase().split(',');return this.children.flatMap(n=>[...(names.includes(n.tagName)?[n]:[]),...n.querySelectorAll(tags)]);}
}
function dom(){const view=new EventTarget(),doc={defaultView:view,createElement(tag){return new Element(tag,this);},createTextNode(s){return new Element('#text',this,s);}};return new Element('main',doc);}
const tick=()=>new Promise(r=>setTimeout(r,0));
async function click(root,label){const b=root.querySelectorAll('button').find(n=>n.textContent===label);assert.ok(b);b.dispatchEvent(new Event('click'));await tick();}
for(const reply of [observation(),observation(14,0x3fff),observation(13,0x1ffd),null,[68,56,2,1,14,0,0,0]]){
 const p=pair({reply,slots:reply?.[4]===13?13:14}),root=dom();const mount=mountCompanion(root,{getAccess:async()=>({sysexEnabled:true,inputs:new Map([[p.input.id,p.input]]),outputs:new Map([[p.output.id,p.output]])}),connect:(i,o,options)=>D8MidiSession.connect(i,o,{...options,timeout:15})});
 ok(root.textContent.includes('Unknown'),'disconnected UI unknown');await click(root,'Find MIDI ports');
 const selects=root.querySelectorAll('select');selects[0].value=p.input.id;selects[1].value=p.output.id;
 await click(root,'Connect');await new Promise(r=>setTimeout(r,25));
 if(reply===null||reply[5]===0)ok(root.textContent.includes('Unknown')&&!root.textContent.includes('ID 0'),'failed negotiation UI never guesses mask');
 else ok(root.textContent.includes('ID 0 · ANALOG · Available')&&root.textContent.includes('ID 1 · DIGITAL (historical) · '+((reply[5]&2)?'Available':'Unavailable')),'actual companion visible stable engine state');
 ok(p.input.listeners===1&&p.output.sent.length===3,'UI owns one listener with three read-only handshake frames');
 await click(root,'Disconnect');ok(root.textContent.includes('Unknown')&&!root.textContent.includes('ID 0'),'UI clears previous device availability');await mount.destroy();ok(p.input.listeners===0,'destroy clears transport');
}
{
 const p=pair({reply:null}),root=dom();let queried;const query=new Promise(r=>queried=r),respond=p.output.respond;
 p.output.respond=r=>{respond(r);if(r.length===7)queried();};
 const mount=mountCompanion(root,{getAccess:async()=>({sysexEnabled:true,inputs:new Map([[p.input.id,p.input]]),outputs:new Map([[p.output.id,p.output]])})});
 await click(root,'Find MIDI ports');const selects=root.querySelectorAll('select');selects[0].value=p.input.id;selects[1].value=p.output.id;
 await click(root,'Connect');await query;await click(root,'Disconnect');p.input.receive(frame(75,observation()));await tick();
 ok(p.input.listeners===0&&root.textContent.includes('Unknown')&&!root.textContent.includes('ID 0'),'cancelled UI discovery and late reply cannot publish prior device state');await mount.destroy();
}
for(const path of process.argv.slice(2)){
 const child=spawn(path,[],{stdio:['pipe','pipe','inherit']}),exit=new Promise((resolve,reject)=>{child.on('close',resolve);child.on('error',reject);}),lines=createInterface({input:child.stdout});
 const p={input:new Port('input'),output:new Port('output')};
 lines.on('line',line=>{const r=JSON.parse(line);ok(r.flash_writes===0&&r.flash_erases===0,'actual C no NOR writes');p.input.receive(frame(r.cmd,r.args));});
 p.output.respond=(r)=>child.stdin.write([r[4],...r.slice(5,-1)].join(' ')+'\n');
 const s=await D8MidiSession.connect(p.input,p.output,{engineAvailability:true});
 ok(s.engineAvailability.caps!==null&&p.output.sent.length===3&&p.input.listeners===1,'actual C availability through owned transport');
 const expected=s.engineAvailability.caps;await s.close();
 const root=dom();let opened;
 const mount=mountCompanion(root,{getAccess:async()=>({sysexEnabled:true,inputs:new Map([[p.input.id,p.input]]),outputs:new Map([[p.output.id,p.output]])}),connect:async(...args)=>{opened=await D8MidiSession.connect(...args);return opened;}});
 await click(root,'Find MIDI ports');const selects=root.querySelectorAll('select');selects[0].value=p.input.id;selects[1].value=p.output.id;await click(root,'Connect');
 for(let i=0;i<1000&&!root.textContent.includes('ID 0');i++)await tick();
 ok(opened&&root.textContent.includes('ID 0 · ANALOG · Available')&&p.input.listeners===1,'actual C visible companion negotiation retains sole listener');
 ok(root.textContent.includes('ID 1 · DIGITAL (historical) · '+((expected.mask&2)?'Available':'Unavailable')),'actual C visible historical FM4 bit');
 if(expected.slots===13)ok(root.textContent.includes('ID 13 · SLICE · Unavailable (outside compiled slots)'),'actual C visible SLICE-off slot');
 if(opened.tracks.editable){for(let i=0;i<1000&&!root.textContent.includes('Refreshed from device.');i++)await tick();ok(root.textContent.includes('Refreshed from device.'),'availability display coexists with actual eight-track refresh');}
 await mount.destroy();ok(p.input.listeners===0&&!root.textContent.includes('ID 0'),'actual C companion teardown clears observation');
 child.stdin.end();assert.equal(await exit,0);
}
console.log(`companion engine integration: ${checks} checks PASS; simulated ports/DOM and supplied host C only`);
