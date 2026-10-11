// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Dabbl8 contributors
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {spawnSync} from "node:child_process";
import {parseTrackInfo} from "./d8tracks.js";
import {d8EditorTitle,d8PortName} from "./d8info.js";
let n=0;
for (const [v,title] of [["DABBL8 v0.1.0-dev","Dabbl8 Editor"],["DABBL8 TEST","Dabbl8 Editor"],["FELUCCA v1.1.5.1","Felucca Editor"],["FELUCCA 0.4 BETA (MOCK)","Felucca Editor"],["OTHER DABBL8 v1","Felucca / Dabbl8 Editor"],["", "Felucca / Dabbl8 Editor"],[null,"Felucca / Dabbl8 Editor"]]) {assert.equal(d8EditorTitle(v),title);n++;}
for (const v of ["Dabbl8","Dabbl8 MIDI","Felucca","Felucca Update"]) {assert(d8PortName(v));n++;}
for (const v of ["",null,"IAC Driver","Other synth"]) {assert(!d8PortName(v));n++;}
const html=readFileSync(new URL("editor.html",import.meta.url),"utf8");
assert(html.includes('id="editorTitle"'));n++;
if (process.argv[2]) {
  const p=spawnSync(process.argv[2],["--bridge"],{input:"f0 7d 46 4c 01 f7\n#stats\n",encoding:"utf8"});
  assert.equal(p.status,0,p.stderr);n++;
  const lines=p.stdout.trim().split(/\r?\n/),frame=lines[0].split(/\s+/).map(x=>parseInt(x,16));
  assert.deepEqual(frame.slice(0,5),[240,125,70,76,1]);assert.equal(frame.at(-1),247);n++;
  const info=parseTrackInfo(frame.slice(5,-1));
  assert.equal(d8EditorTitle(info.version),process.argv[3]);n++;
  const s=JSON.parse(lines[1]);assert.equal(s.writes,0);assert.equal(s.unchanged,true);n++;
  assert.equal(info.discovery,1);n++;
}
console.log(`Dabbl8 editor identity: ${n} checks passed; cosmetic only, no capabilities or device qualification`);
