// SPDX-License-Identifier: GPL-3.0-only
// Actual C command handler and WASM codec; private virtual NOR, never a device.
import assert from 'node:assert/strict';
import {spawn,execFileSync} from 'node:child_process';
import {createInterface} from 'node:readline';
import {readFileSync,writeFileSync,mkdtempSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join,resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {mock} from 'node:test';
import {D8MidiSession} from '../web/d8midi.js';
import {D8Projects,ProjectSessionError,projectReply,pack7,unpack7,u32,crc32} from '../web/d8projects.js';
import {nativeArchiveCodec,createOwnArchive,parseOwnArchive,validateOwnArchive,strictArchiveJSON} from '../web/d8archive.js';
let checks=0;const check=(v)=>{assert.ok(v);checks++;};
const [bridgePath,adapterPath,wasmPath]=process.argv.slice(2);assert.ok(bridgePath&&adapterPath&&wasmPath);
const {default:factory}=await import(pathToFileURL(resolve(wasmPath)));const codec=await nativeArchiveCodec(factory);
class Bridge{
 constructor(){this.child=spawn(bridgePath);this.pending=[];this.calls=[];this.physical=0;this.stderr='';this.child.stderr.on('data',d=>this.stderr+=d);this.exit=new Promise((res,rej)=>{this.child.on('error',rej);this.child.on('close',res);});createInterface({input:this.child.stdout}).on('line',line=>{const result=JSON.parse(line);assert.equal(result.failures??0,0);this.physical=result.physical_calls??this.physical;this.pending.shift()(result);});}
 line(text){return new Promise(res=>{this.pending.push(res);this.child.stdin.write(text+'\n');});}
 async request([cmd,args]){this.calls.push([cmd,args]);return this.line([cmd,...args].join(' '));}
 async close(){this.child.stdin.end();assert.equal(await this.exit,0,this.stderr);}
}
class Port extends EventTarget{
 constructor(type){super();this.type=type;this.state='connected';this.connection='closed';}
 async open(){this.connection='open';return this;}async close(){this.connection='closed';return this;}
 receive(data){const e=new Event('midimessage');e.data=new Uint8Array(data);this.dispatchEvent(e);}
}
const bridge=new Bridge(),input=new Port('input'),output=new Port('output');
let holdCommitReply=null;
output.send=bytes=>{void bridge.request([bytes[4],bytes.slice(5,-1)]).then(async r=>{
 if(bytes[4]===81&&bytes[6]===7&&holdCommitReply)await holdCommitReply();
 input.receive([240,125,70,76,r.cmd,...r.args,247]);
});};
let session;const td=mkdtempSync(join(tmpdir(),'d8-client-'));
try{
 session=await D8MidiSession.connect(input,output,{ownProjects:true});check(session.projects.available&&session.tracks.editable);
 const initial=await session.projects.exportSet();check(initial.present===15&&initial.objects.length===4&&initial.live.length>0&&!session.projectBusy&&bridge.physical===0);
 check(!initial.live.every((v,i)=>v===initial.objects[3][i]));
 const archive=await createOwnArchive(initial,codec),retained=archive.slice();if(process.argv[5])writeFileSync(process.argv[5],archive);const parsed=await validateOwnArchive(archive,codec);check(parsed.present===15);assert.deepEqual(parsed.pool,initial.pool);checks++;
 // Exact raw C/WASM agreement, all roles including binary stdout and repeated calls.
 for(let i=0;i<4;i++){assert.deepEqual(codec('read',initial.pool,i),initial.objects[i]);checks++;}
 const corrupt=parsed.live.slice();corrupt[40]^=1;assert.throws(()=>codec('check',corrupt,7));checks++;check(codec('check',parsed.live,7).bytes===parsed.live.length);
 for(const bad of ['{"a":1,"a":2}','{"a":1,"\\u0061":2}','{"a":1.0}','{"a":1e0}']){assert.throws(()=>strictArchiveJSON(bad));checks++;}
 let altered=JSON.parse(new TextDecoder().decode(archive));altered.objects[0].blob.sha256='0'.repeat(64);await assert.rejects(parseOwnArchive(new TextEncoder().encode(JSON.stringify(altered))));checks++;
 await session.projects.stage(parsed);check(session.projectBusy&&session.projects.phase===3&&bridge.physical===0);
 const calls=bridge.calls.length;await assert.rejects(session.tracks.select(1));check(bridge.calls.length===calls);
 await assert.rejects(session.projects.commit());check(bridge.physical===0);check((await session.projects.status()).phase===3);
 await session.projects.cancel();check(!session.projectBusy);assert.deepEqual(archive,retained);checks++;
 await session.projects.stage(parsed);check((await bridge.line('APPROVE')).phase===5);check((await session.projects.status()).phase===5);
 // Whole-set NOR commit can finish after the normal interactive reply deadline.
 let releaseReply,replyReady;
 const held=new Promise(resolve=>{releaseReply=resolve;}),ready=new Promise(resolve=>{replyReady=resolve;});
 holdCommitReply=()=>{replyReady();return held;};
 mock.timers.enable({apis:['setTimeout']});
 try{
  const committing=session.projects.commit();void committing.catch(()=>{});
  await ready;mock.timers.tick(2000);
  assert.equal(session.connected,true,'A completed whole-set commit must survive a reply delayed beyond 1500ms');checks++;
  releaseReply();const restored=await committing;check(restored.present===15&&!session.projectBusy);assert.deepEqual(restored.live,initial.live);checks++;
 }finally{releaseReply();holdCommitReply=null;mock.timers.reset();}
 // Empty roles are restored exactly, including another restore after a real fixture reboot.
 const emptyPath=join(td,'empty.bin');execFileSync(adapterPath,['build',emptyPath,'0','-','-','-','-']);
 const empty={present:0,pool:new Uint8Array(readFileSync(emptyPath)),live:initial.live,objects:Array.from({length:4},()=>new Uint8Array())};
 const emptyArchive=await createOwnArchive(empty,codec);await validateOwnArchive(emptyArchive,codec);checks++;
 await session.projects.stage(empty);await bridge.line('APPROVE');await session.projects.status();const emptyRestored=await session.projects.commit();check(emptyRestored.present===0);await validateOwnArchive(await createOwnArchive(emptyRestored,codec),codec);checks++;
 await bridge.line('REBOOT');const before=bridge.physical;await session.projects.stage(empty);await bridge.line('APPROVE');await session.projects.status();check((await session.projects.commit()).present===0&&bridge.physical===before);
 // A physical outcome error closes the host owner; retain archive for fresh confirmation.
 await session.projects.stage(parsed);await bridge.line('APPROVE');await session.projects.status();await bridge.line('CUT 2');
 await assert.rejects(session.projects.commit());check(!session.connected);assert.deepEqual(archive,retained);checks++;
 await session.close();await bridge.line('CUT -1');await bridge.line('REBOOT');
 session=await D8MidiSession.connect(input,output,{ownProjects:true,timeout:5000});await session.projects.stage(parsed);check(session.projects.phase===3);await bridge.line('APPROVE');await session.projects.status();check((await session.projects.commit()).present===15);
 for(const bad of [[1,0,0],[1,0,0,...Array(22).fill(0),128]]){assert.throws(()=>projectReply(bad,0));checks++;}
 for(let n=0;n<257;n++){const raw=Uint8Array.from({length:n},(_,i)=>(i*131+n)&255);assert.deepEqual(unpack7(pack7(raw),n),raw);checks++;}
 // Malformed acknowledgments revoke the owner before any further write.
 const reply=(op,{token=0,phase=0,total=0,crc=0,extra=[]}={})=>[1,op,0,...u32(token),phase,0,...u32(0),...u32(total),...u32(crc),...extra];
 const caps=reply(0,{total:40960,extra:[15,3,3,...u32(40960),...u32(7936),...u32(256),2]});
 for(const badCRC of [true,false]){
  let faulted=0,writes=0,locked=0;
  const malformed=await D8Projects.connect(async([,a])=>{
   if(a[1]===0)return caps;
   if(a[1]===2)return reply(2,{token:7,phase:2,total:40960,crc:(crc32(initial.pool)+(badCRC?1:0))>>>0});
   if(a[1]===4){writes++;return reply(4,{token:8,phase:2});}
   if(a[1]===8)return reply(8,{token:7});throw Error('Unexpected test request');
  },()=>{locked++;return ()=>locked--;},()=>faulted++);
  await assert.rejects(malformed.stage(initial));check(faulted===1&&locked===0&&writes===(badCRC?0:1));
 }
 await assert.rejects(session.projects.stage(null));checks++;
 // A genuinely missing commit acknowledgement stays bounded and is never retried.
 await session.projects.stage(parsed);await bridge.line('APPROVE');await session.projects.status();
 const lost=new Promise(resolve=>{releaseReply=resolve;}),lostReady=new Promise(resolve=>{replyReady=resolve;});
 holdCommitReply=()=>{replyReady();return lost;};
 const beforeCommit=bridge.calls.length;
 mock.timers.enable({apis:['setTimeout']});
 try{
  const committing=session.projects.commit();void committing.catch(()=>{});await lostReady;
  mock.timers.tick(29999);check(session.connected);
  mock.timers.tick(1);await assert.rejects(committing,/command 81\/7.*timed out.*uncertain/);checks++;
  await session.close();check(!session.connected&&input.connection==='closed'&&output.connection==='closed');
  assert.deepEqual(bridge.calls.slice(beforeCommit).map(([cmd,args])=>[cmd,args[1]]),[[81,7]]);checks++;
  releaseReply();await Promise.resolve();check(!session.connected);
 }finally{releaseReply();holdCommitReply=null;mock.timers.reset();}
 // A real C COMMIT can succeed before a distinct EXPORT-copy failure.
 const readbackBridge=new Bridge();
 try{
  let locked=0,faulted=0;
  const client=await D8Projects.connect(async request=>(await readbackBridge.request(request)).args,()=>{locked++;return()=>locked--;},()=>faulted++);
  await client.stage(empty);await readbackBridge.line('APPROVE');await client.status();await readbackBridge.line('EXPORT_COPY_FAIL');
  const start=readbackBridge.calls.length;
  await assert.rejects(client.commit(),e=>e instanceof ProjectSessionError&&e.status===4&&e.op===1&&e.commitAcknowledged===true&&/81\/1/.test(e.message));checks++;
  check(!locked&&faulted===1);
  check(readbackBridge.calls.slice(start).filter(([,a])=>a[1]===7).length===1);
  const writes=readbackBridge.physical;await readbackBridge.line('CLEAR_READ_FAULT');
  const actual=await client.exportSet();check(actual.present===0&&readbackBridge.physical===writes);
  assert.deepEqual(actual.live,empty.live);assert.deepEqual(actual.objects,empty.objects);checks+=2;
 }finally{await readbackBridge.close();}
 // Cleanup transport failure must not replace the real native COMMIT error.
 const cleanupBridge=new Bridge();
 try{
  let failClose=false,locked=0,faulted=0;
  const client=await D8Projects.connect(async request=>{
   if(failClose&&request[1][1]===8)throw Error('cleanup transport probe');
   return (await cleanupBridge.request(request)).args;
  },()=>{locked++;return()=>locked--;},()=>faulted++);
  await client.stage(parsed);await cleanupBridge.line('APPROVE');await client.status();await cleanupBridge.line('CUT 2');failClose=true;
  const start=cleanupBridge.calls.length;
  await assert.rejects(client.commit(),e=>e instanceof ProjectSessionError&&e.status===4&&e.op===7&&e.commitAcknowledged===false&&e.cleanupError?.message==='cleanup transport probe');checks++;
  check(!locked&&faulted===1);
  check(cleanupBridge.calls.slice(start).filter(([,a])=>a[1]===7).length===1);
 }finally{await cleanupBridge.close();}
 // Missing advertised capability safely leaves the existing track workflow available.
 const missing=new D8Projects(async()=>{throw Error('Disconnected');});check(!missing.available);
 console.log(`Own-project browser/C/WASM interoperability: ${checks} checks PASS; simulated storage and MIDI ports only`);
}finally{await session?.close();await bridge.close();rmSync(td,{recursive:true,force:true});}
