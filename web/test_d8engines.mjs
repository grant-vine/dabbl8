// SPDX-License-Identifier: GPL-3.0-only
// Actual C parser/handler bridge plus exact malformed-reply mocks. No physical MIDI.
import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { createInterface } from "node:readline";
import { D8Engines, parseEngineAvailability } from "./d8engines.js";
let checks=0; const ok=(x)=>{assert.ok(x);checks++;};
class Bridge {
  constructor(path) {
    this.calls=[];this.pending=[];this.child=spawn(path,[],{stdio:["pipe","pipe","inherit"]});
    this.exit=new Promise(resolve=>this.child.on("close",code=>{this.pending.splice(0).forEach(p=>p.reject(Error("Bridge closed")));resolve(code);}));
    this.child.on("error",e=>this.pending.splice(0).forEach(p=>p.reject(e)));
    createInterface({input:this.child.stdout}).on("line",line=>{
      const p=this.pending.shift();if(!p)throw Error("Unexpected reply");
      try {const r=JSON.parse(line);assert.equal(r.flash_writes,0);assert.equal(r.flash_erases,0);p.resolve(r);}catch(e){p.reject(e);}
    });
  }
  request=async ([cmd,args])=>{
    this.calls.push([cmd,[...args]]);
    const r=await new Promise((resolve,reject)=>{this.pending.push({resolve,reject});this.child.stdin.write([cmd,...args].join(" ")+"\n");});
    if(r.cmd===76)throw Error("Firmware refusal "+r.args.join(","));
    assert.equal(r.cmd,cmd);return r.args;
  };
  async close(){this.child.stdin.end();assert.equal(await this.exit,0);}
}
for(const path of process.argv.slice(2)) {
  const b=new Bridge(path);
  try {
    const c=await D8Engines.connect(b.request);ok(c.available);
    assert.deepEqual(b.calls,[[1,[]],[75,[]],[75,[2]]]);checks++;
    const raw=await b.request([75,[2]]), caps=c.caps;
    assert.deepEqual(caps,parseEngineAvailability(raw));checks++;
    for(let id=0;id<caps.slots;id++)ok(c.has(id)===!!(caps.mask&(1<<id)));
    ok(Object.isFrozen(caps)&&Object.isFrozen(caps.ids));
    const info=await b.request([1,[]]), schema1=await b.request([75,[]]);ok(schema1.length===15);
    for(const [index,value] of [[0,0],[1,0],[2,1],[3,2],[4,12],[5,0],[6,0],[7,1]]) {
      const bad=[...raw];bad[index]=value;
      if(index===5||index===6){bad[5]=0;bad[6]=0;}
      assert.throws(()=>parseEngineAvailability(bad));checks++;
    }
    for(const bad of [raw.slice(1),[...raw,0],raw.map((v,i)=>i===5?128:v),[68,56,2,1,13,0,64,0]]) {
      assert.throws(()=>parseEngineAvailability(bad));checks++;
    }
    for(let i=0;i<raw.length;i++) {
      const sparse=[...raw];delete sparse[i];assert.throws(()=>parseEngineAvailability(sparse));checks++;
      const invalid=await D8Engines.connect(async ([cmd,args])=>cmd===1?info:args.length?sparse:schema1);ok(!invalid.available);
    }
    for(let i=0;i<info.length;i++) {
      const sparse=[...info];delete sparse[i];let calls=0;
      const invalid=await D8Engines.connect(async ([cmd])=>{calls++;assert.equal(cmd,1);return sparse;});ok(!invalid.available&&calls===1);
    }
    for(let i=0;i<schema1.length;i++) {
      const sparse=[...schema1];delete sparse[i];let calls=0;
      const invalid=await D8Engines.connect(async ([cmd])=>{calls++;return cmd===1?info:sparse;});ok(!invalid.available&&calls===2);
    }
    for(const failure of [Error("Timeout"),[68,56,3,1,14,125,127,0]]) {
      const calls=[];
      const old=await D8Engines.connect(async ([cmd,args])=>{calls.push([cmd,args]);if(cmd===1)return info;if(!args.length)return schema1;if(failure instanceof Error)throw failure;return failure;});
      ok(!old.available && old.caps===null && !old.has(0));assert.deepEqual(calls,[[1,[]],[75,[]],[75,[2]]]);checks++;
    }
    const old=await D8Engines.connect(async ([cmd])=>{assert.equal(cmd,1);return info.slice(0,-3);});ok(!old.available);
    const mismatch=[...schema1];mismatch[9]=mismatch[9]===14?13:14;let calls=0;
    const bad=await D8Engines.connect(async ([cmd])=>{calls++;return cmd===1?info:mismatch;});ok(!bad.available&&calls===2);
    c.close();ok(!c.available && c.caps===null && !c.has(0));
  } finally {await b.close();}
}
console.log(`engine capability browser module: ${checks} checks PASS; actual host C bridge and malformed mocks only`);
