// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { spawn } from 'node:child_process';
import { createInterface } from 'node:readline';
import { D8Tracks, TrackProtocolError } from './d8tracks.js';
import { CompanionModel, parameterDescriptor, parameterName, noteName } from './d8companion-model.js';
import { descriptors } from './d8descriptors.js';
let checks=0;function ok(value,label){assert.ok(value,label);checks++;console.log('Companion model: '+label+' ok');}
const exported=JSON.parse(readFileSync('build/host/desc.json','utf8'));
assert.deepEqual(descriptors.common,exported.TP);assert.deepEqual(descriptors.engine,exported.ENG.map(({name,titles,edit})=>({name,titles,edit})));
ok(descriptors.params===exported.P_COUNT&&descriptors.engineStart===exported.P_E0&&descriptors.engines===exported.ENG.length,'musical schema equals actual pinned descriptor export');
for(let e=0;e<14;e++)for(let id=0;id<99;id++){const d=parameterDescriptor(e,id);assert.ok(d&&typeof d.label==='string');assert.ok(Number.isInteger(d.min)&&Number.isInteger(d.max));assert.ok(parameterName(e,id).includes(' · '));}
ok(true,'all 1386 engine/parameter combinations have musical labels and ranges');
ok(parameterName(0,39)==='Voice · Pan'&&parameterName(0,91)==='ANALOG · Waveform'&&parameterName(10,91)==='DRUM · Kit','engine controls use correct engine-specific descriptors');
ok(noteName(60)==='C4'&&noteName(0)==='C-1'&&noteName(127)==='G9','musical note names span the full MIDI range');
class Bridge{
 constructor(path){this.calls=[];this.pending=[];this.child=spawn(path);this.exit=new Promise((resolve,reject)=>{this.child.on('close',resolve);this.child.on('error',reject);});createInterface({input:this.child.stdout}).on('line',(line)=>{const p=this.pending.shift();assert.ok(p);const r=JSON.parse(line);assert.equal(r.flash_writes,0);assert.equal(r.flash_erases,0);p(r);});}
 request=async([cmd,args])=>{this.calls.push([cmd,[...args]]);const r=await new Promise((resolve)=>{this.pending.push(resolve);this.child.stdin.write([cmd,...args].join(' ')+'\n');});if(r.cmd===76&&r.args.length===3&&r.args[0]===1&&r.args[1]===cmd)throw new TrackProtocolError(r.args[2]);assert.equal(r.cmd,cmd);return r.args;};
 async close(){this.child.stdin.end();assert.equal(await this.exit,0);}
}
const bridge=new Bridge(process.argv[2]),four=new Bridge(process.argv[3]);
try{
 const client=await D8Tracks.connect(bridge.request);let changes=0,closed=0;
 const model=new CompanionModel({tracks:client,close:()=>{closed++;client.close();}},()=>changes++);
 await model.refresh();ok(model.state.metadata.tracks.length===8&&model.state.dump.params.length===99&&model.state.step.index===0,'complete initial eight-track view from actual handlers');
 for(let track=0;track<8;track++){
  await model.select(track);ok(model.state.selected===track&&model.state.dump.track===track,'visible selection loads matching sound and step '+(track+1));
  await model.mix(track,{level:30+track,mute:track%2===1});ok(model.state.metadata.tracks[track].level===30+track&&model.state.metadata.tracks[track].mute===(track%2===1),'visible mixer reflects actual values '+(track+1));
  await model.parameter(39,-32+track);ok(model.state.dump.params[39]===-32+track,'visible signed pan reaches actual track '+(track+1));
 }
 await model.readStep(63);
 const update={n:4,notes:[60,64,67,72],time:0,flags:3,vel:100,hit:255,acc:128,chance:42,ratchet:4};
 const applying=model.applyStep(update);update.notes[0]=1;update.hit=0;await applying;
 ok(model.state.step.index===63&&model.state.step.notes[0]===60&&model.state.step.hit===255&&model.state.step.acc===128&&model.state.step.chance===42&&model.state.step.ratchet===4,'last step preserves captured notes and all drum/chance/ratchet fields');
 const calls=bridge.calls.length;for(const value of [NaN,64,-65,1.5,'0'])await assert.rejects(model.parameter(39,value));
 ok(bridge.calls.length===calls&&model.editable,'invalid musical range/value sends no requests');
 for(let track=0;track<8;track++)ok((await client.parameter(track,39)).value===-32+track,'pan values remain isolated '+(track+1));
 let alternate=null;
 const facade={get editable(){return client.editable;},get reason(){return client.reason;},readTracks:(...a)=>client.readTracks(...a),dump:async(...a)=>{const d=await client.dump(...a);return alternate===null?d:{...d,engine:alternate};},step:(...a)=>client.step(...a),parameter:(...a)=>client.parameter(...a),select:(...a)=>client.select(...a),mix:(...a)=>client.mix(...a)};
 const guarded=new CompanionModel({tracks:facade,close:()=>{}});await guarded.refresh();alternate=2;
 const before=bridge.calls.filter(([cmd,a])=>cmd===77&&a[1]===31&&a.length===6).length;
 await assert.rejects(guarded.parameter(91,0));ok(guarded.state.reviewRequired&&guarded.state.step===null,'changed sound discards cached step and requires review');
 await assert.rejects(guarded.parameter(91,0));
 ok(bridge.calls.filter(([cmd,a])=>cmd===77&&a[1]===31&&a.length===6).length===before,'changed-sound preflight and review gate send no parameter writes');
 alternate=null;await guarded.refresh();ok(!guarded.state.reviewRequired&&guarded.state.step!==null,'explicit refresh clears changed-sound review gate');
 let release;const gate=new Promise((r)=>{release=r;});
 const slow={...facade,dump:async(...a)=>{await gate;return client.dump(...a);}};
 const stale=new CompanionModel({tracks:slow,close:()=>{}});const waiting=stale.refresh();
 await assert.rejects(stale.select(1));ok(true,'pending operation blocks concurrent UI actions');
 const stepsBefore=bridge.calls.filter(([cmd,a])=>cmd===77&&a[1]===30).length;stale.close();release();await assert.rejects(waiting);ok(bridge.calls.filter(([cmd,a])=>cmd===77&&a[1]===30).length===stepsBefore,'close stops follow-up step reads after delayed dump');ok(stale.state.dump===null,'closed model rejects pending result without applying stale state');
 model.close();const count=changes;await assert.rejects(model.refresh());ok(closed===1&&changes===count,'closed view cannot issue new traffic or callbacks');
 const legacy=await D8Tracks.connect(four.request);assert.throws(()=>new CompanionModel({tracks:legacy}));ok(true,'four-track firmware cannot create an editing model');legacy.close();
 ok(bridge.calls.every(([cmd,a])=>[1,75,77].includes(cmd)&&(cmd!==77||[27,28,29,30,31].includes(a[1]))),'view model never sends legacy globals project preset sample or backup commands');
}finally{await bridge.close();await four.close();}
console.log(`Companion model: ${checks} checks passed; actual C host handlers, physical hardware unqualified`);
