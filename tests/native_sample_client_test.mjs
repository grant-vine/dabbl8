// SPDX-License-Identifier: GPL-3.0-only
// Executes the actual browser negotiation/upload helpers with bounded fake replies.
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const html=readFileSync(new URL('../web/editor.html',import.meta.url),'utf8');
const segment=(a,b)=>html.slice(html.indexOf(a),html.indexOf(b,html.indexOf(a)));
const code=segment('const CMD = {','/* frames the device sends')+segment('function pack7(b)', 'function unpack7(a)')+segment('/* Versioned own-sample transaction.', 'async function smpUpload(k, prog)');
const context=vm.createContext({setTimeout,Uint8Array});vm.runInContext(code+'\nglobalThis.api={CMD,d8Negotiate,d8Request,d8SampleUpload,d8SampleReply,d8SampleU32};',context);
const E=context.api;let checks=0;const check=(x)=>{checks++;assert.ok(x);};
const caps=[68,56,1,8,64,8,64,99,27,14,1,1,1,9,13];const info={d8Schema:1,ntrk:8,nstep:64,pcount:99,gcount:27,nengines:14};
const mode=await E.d8Negotiate(info,async()=>caps);check(!mode.writable&&mode.sampleWritable);
let sends=0;await assert.rejects(E.d8Request(mode,async()=>{sends++;},[11,[0]]));check(!sends);
await E.d8Request(mode,async()=>{sends++;return [];},[80,[1,2,1,0,0,0,0]]);check(sends===1);
check(!(await E.d8Negotiate({...info,ntrk:4},async()=>caps)).sampleWritable);
check(!(await E.d8Negotiate(info,async()=>caps.map((v,i)=>i===14?5:v))).sampleWritable);
const data=new Uint8Array(4097).map((_,i)=>i&255),header=new Uint8Array(480);const before=Array.from(data);let offset=512,polls=0,writes=0,commit=0,cancel=0;
const reply=(op,phase=3,token=7)=>[1,op,0,...E.d8SampleU32(token),phase,...E.d8SampleU32(offset)];
const request=async([cmd,a])=>{check(cmd===80&&a[0]===1);if(a[1]===1)return reply(1,1);if(a[1]===2)return reply(2,++polls>1?3:1);if(a[1]===3){check(a.slice(7,12).every((v,i)=>v===E.d8SampleU32(offset)[i]));const encoded=a.length-12;offset+=encoded-Math.ceil(encoded/8);writes++;return reply(3);}if(a[1]===4){commit++;return reply(4,0);}if(a[1]===5){cancel++;return reply(5,0);}throw Error('unexpected');};
await E.d8SampleUpload(request,2,header,data,()=>{},async()=>{});check(offset===4609&&writes===17&&commit===1&&!cancel);check(JSON.stringify(before)===JSON.stringify(Array.from(data)));
await assert.rejects(E.d8SampleUpload(async([,a])=>a[1]===1?reply(1,1):reply(a[1],3,8),0,header,data,()=>{},async()=>{}));check(true);
assert.throws(()=>E.d8SampleReply([1,2,0,...E.d8SampleU32(7),3,0,0,0,0,16],2,7));check(true);
console.log(`Native sample browser helpers: ${checks} checks PASS; fake replies only`);
