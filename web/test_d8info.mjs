// SPDX-License-Identifier: GPL-3.0-only
// Actual capture C handler plus inherited INFO parser; no physical MIDI/device.
import assert from "node:assert/strict";
import { spawn, execFileSync } from "node:child_process";
import { createInterface } from "node:readline";
import { readFileSync, mkdtempSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import vm from "node:vm";
import { D8Tracks, parseTrackInfo, TrackProtocolError } from "./d8tracks.js";
import { d8InfoDiscovery } from "./d8info.js";
let checks = 0;
const ok = (v, label) => { assert.ok(v, label); checks++; };
const html = readFileSync(new URL("editor.html", import.meta.url), "utf8");
const proto = html.slice(html.indexOf("/*PROTO-BEGIN*/"), html.indexOf("/*PROTO-END*/"));
const editor = vm.runInNewContext(proto + ";({parse,CMD,d8Negotiate})", {d8InfoDiscovery,setTimeout,clearTimeout,setInterval,clearInterval,console});
class Bridge {
  constructor(path, capture) {
    this.calls = []; this.pending = []; this.capture = capture;
    this.child = spawn(path, capture ? ["--bridge"] : [], {stdio:["pipe","pipe","inherit"]});
    this.exit = new Promise(resolve => this.child.on("close", code => {
      this.pending.splice(0).forEach(p => p.reject(Error("C bridge exited"))); resolve(code);
    }));
    createInterface({input:this.child.stdout}).on("line", line => {
      const p = this.pending.shift(); if(!p) throw Error("Unexpected C reply");
      try { p.resolve(line); } catch(e) { p.reject(e); }
    });
    this.child.on("error", e => this.pending.splice(0).forEach(p => p.reject(e)));
  }
  line(line) { return new Promise((resolve,reject)=>{this.pending.push({resolve,reject});this.child.stdin.write(line+"\n");}); }
  request = async ([cmd,args]) => {
    this.calls.push([cmd,[...args]]);
    let r;
    if(this.capture) {
      const frame=[240,125,70,76,cmd,...args,247];
      const reply=(await this.line(frame.map(v=>v.toString(16).padStart(2,"0")).join(" "))).trim().split(/\s+/).map(v=>parseInt(v,16));
      assert.deepEqual(reply.slice(0,4),[240,125,70,76]); assert.equal(reply.at(-1),247);
      r={cmd:reply[4],args:reply.slice(5,-1)};
    } else {
      r=JSON.parse(await this.line([cmd,...args].join(" ")));
      assert.equal(r.flash_writes,0);assert.equal(r.flash_erases,0);
    }
    if(r.cmd===76&&r.args[0]===1&&r.args[1]===cmd)throw new TrackProtocolError(r.args[2]);
    assert.equal(r.cmd,cmd);return r.args;
  };
  async close() {this.child.stdin.end();assert.equal(await this.exit,0);}
}
function trailerOffset(a) {
  let i=a.indexOf(0)+1,engines=a[i++]; i+=4;
  for(let n=0;n<engines;n++)i=a.indexOf(0,i)+1;
  return i+1;
}
const capture=new Bridge(process.argv[2],true),four=new Bridge(process.argv[3],false);
try {
  const info=await capture.request([1,[]]),off=trailerOffset(info),tail=info.slice(off);
  ok(JSON.stringify(tail.slice(-7))===JSON.stringify([68,56,1,67,56,1,78]),"actual C capture tag stays last after D8");
  const client=await D8Tracks.connect(capture.request);
  ok(client.editable&&client.info.discovery===1&&client.caps.tracks===8&&client.caps.voices===8,"actual INFO→CAPS companion handshake");
  ok((await client.select(7)).selected===7,"actual track8 select");
  ok((await client.parameter(7,0,29)).value===29&&(await client.parameter(7,0)).value===29,"actual track8 write/read");
  const legacy=editor.parse[editor.CMD.INFO](info);
  ok(legacy.d8Schema===1&&legacy.ntrk===8&&legacy.backupCaps===0,"inherited parser recognizes actual capture INFO without backup grant");
  const negotiated=await editor.d8Negotiate(legacy,capture.request);
  ok(!negotiated.writable&&/does not support/.test(negotiated.reason),"inherited four-track editor remains read-only on eight tracks after actual CAPS");
  const oldInfo=await four.request([1,[]]),old=await D8Tracks.connect(four.request);
  ok(!old.editable&&old.info.tracks===4,"actual four-track companion stays read-only");
  const oldLegacy=editor.parse[editor.CMD.INFO](oldInfo),oldNegotiated=await editor.d8Negotiate(oldLegacy,four.request);
  ok(oldLegacy.d8Schema===1&&oldNegotiated.writable,"actual four-track inherited negotiation unchanged");
  const withoutCapture=[...info.slice(0,off),...tail.slice(0,33)];
  ok(parseTrackInfo(withoutCapture).discovery===1&&editor.parse[editor.CMD.INFO](withoutCapture).d8Schema===1,"previous D8-only suffix recognized");
  const historical=[...oldInfo.slice(0,trailerOffset(oldInfo)),...oldInfo.slice(trailerOffset(oldInfo),-3)];
  ok(parseTrackInfo(historical).discovery===0&&editor.parse[editor.CMD.INFO](historical).d8Schema===0,"older four-track INFO without D8 remains identity-only");
  const mutations=[];
  for(let cut=1;cut<=3;cut++)mutations.push(["partial capture extension "+cut,tail.slice(0,-cut)]);
  for(let cut=1;cut<=2;cut++)mutations.push(["partial D8 extension "+cut,tail.slice(0,33-cut)]);
  mutations.push(["duplicated D8",[...tail.slice(0,33),68,56,1]], ["duplicated D8 before C8",[...tail.slice(0,33),68,56,1,...tail.slice(33)]],
    ["duplicated C8",[...tail,...tail.slice(33)]], ["unknown extension after D8",[...tail.slice(0,33),88,1,1]],
    ["C8 without D8",[...tail.slice(0,30),...tail.slice(33)]], ["arbitrary prefix with valid suffix",[...Array(30).fill(0),...tail.slice(30)]]);
  for(const [index,value] of [[0,15],[1,0],[27,0],[32,2],[33,88],[34,57],[35,2],[36,79],[36,128]]) {
    const changed=[...tail];changed[index]=value;mutations.push(["unknown prefix/suffix "+index,changed]);
  }
  for(const [label,t] of mutations) {
    const changed=[...info.slice(0,off),...t];
    ok(d8InfoDiscovery(t)===0,label+" bounded helper refuses");
    ok(editor.parse[editor.CMD.INFO](changed).d8Schema===0,label+" inherited discovery refuses");
    if(changed.some(v=>v>127)) {assert.throws(()=>parseTrackInfo(changed));checks++;continue;}
    ok(parseTrackInfo(changed).discovery===0,label+" companion discovery refuses");
    const sent=[];const refused=await D8Tracks.connect(async r=>{sent.push(r[0]);return changed;});
    ok(!refused.editable&&sent.length===1&&sent[0]===1,label+" no CAPS/track request by guess");
  }
  const caps=await capture.request([75,[]]);
  for(const index of [2,3,14]) {
    const corrupt=[...caps];corrupt[index]=index===14?0:corrupt[index]+1;
    const sent=[];const refused=await D8Tracks.connect(async ([cmd])=>{sent.push(cmd);return cmd===1?info:corrupt;});
    ok(!refused.editable&&sent.every(v=>v===1||v===75),"capture suffix does not relax capability validation "+index);
  }
  const stats=JSON.parse(await capture.line("#stats"));
  ok(stats.writes===0&&stats.unchanged,"actual C RAM edits never write NOR or mint authority");
  if(process.argv[4]) {
    const out=mkdtempSync(join(tmpdir(),"dabbl8-discovery-site-"));
    execFileSync("python3",["web/make_site.py",process.argv[4],"test",out],{stdio:"pipe"});
    const site=join(out,"webapp/editor"),copied=readFileSync(join(site,"d8info.js"));
    ok(copied.equals(readFileSync(new URL("d8info.js",import.meta.url))),"actual site builder copies exact shared helper");
    ok(readFileSync(join(site,"index.html"),"utf8").includes('import { d8InfoDiscovery } from "./d8info.js";'),"copied editor imports relative helper");
    console.log("Local built editor preview: "+site);
  } else throw Error("Package argument required for actual site copy coverage");
  console.log(`D8 INFO discovery: ${checks} checks passed; actual capture C and legacy four-track handlers, no physical MIDI/device qualification`);
} finally {await capture.close();await four.close();}
