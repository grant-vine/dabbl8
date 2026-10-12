// SPDX-License-Identifier: GPL-3.0-only
// Actual C command handler and WASM codec; private virtual NOR, never a device.
import assert from 'node:assert/strict';
import {spawn,execFileSync} from 'node:child_process';
import {createInterface} from 'node:readline';
import {readFileSync,writeFileSync,mkdtempSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join,resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {D8MidiSession} from '../web/d8midi.js';
import {D8Projects,projectReply,pack7,unpack7,u32,crc32} from '../web/d8projects.js';
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
output.send=bytes=>{void bridge.request([bytes[4],bytes.slice(5,-1)]).then(r=>input.receive([240,125,70,76,r.cmd,...r.args,247]));};
let session;const td=mkdtempSync(join(tmpdir(),'d8-client-'));
try{
 session=await D8MidiSession.connect(input,output,{ownProjects:true,timeout:5000});check(session.projects.available&&session.tracks.editable);
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
 const restored=await session.projects.commit();check(restored.present===15&&!session.projectBusy);assert.deepEqual(restored.live,initial.live);checks++;
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
 // Missing advertised capability safely leaves the existing track workflow available.
 const missing=new D8Projects(async()=>{throw Error('Disconnected');});check(!missing.available);
 console.log(`Own-project browser/C/WASM interoperability: ${checks} checks PASS; simulated storage and MIDI ports only`);
}finally{await session?.close();await bridge.close();rmSync(td,{recursive:true,force:true});}
