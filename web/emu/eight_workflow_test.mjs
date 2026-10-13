// SPDX-License-Identifier: GPL-3.0-only
// node web/emu/eight_workflow_test.mjs WASM EVIDENCE_DIR [NATIVE.f32]
import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";
import { device, B, EN } from "./workflow_device.mjs";
import { projectPage, proposeSave, saveProject, loadProject, setName } from "./project_workflow.mjs";
const [wasmPath, evidence, nativePath] = process.argv.slice(2);
if (!wasmPath || !evidence) throw new Error("WASM and EVIDENCE_DIR are required");
fs.mkdirSync(evidence, { recursive: true });
const bytes = fs.readFileSync(wasmPath), cases = [];
const sha = (value) => crypto.createHash("sha256").update(value).digest("hex");
const equal = (a, b) => JSON.stringify(a) === JSON.stringify(b);
function check(scenario, observable, passed) {
  cases.push({ scenario, observable, passed });
  console.log(`eight-emu: ${scenario}: ${passed ? "PASS" : "FAIL"} ${JSON.stringify(observable)}`);
}
const d = await device(bytes);
check("fresh boot awaits real first-use SAVE without writes", { tracks: d.ex.web_qa_tracks(), phase: d.ex.web_qa_first_use(), writes: d.ex.web_flash_writes_count() },
  d.ex.web_qa_tracks() === 8 && d.ex.web_qa_first_use() === 1 && d.ex.web_flash_writes_count() === 0);
d.screen(path.join(evidence, "first-use.rgb565"));
d.tap(B.SAVE);
d.render(30000);
check("press and release SAVE completes first-use", { phase: d.ex.web_qa_first_use(), status: d.ex.web_qa_storage_status(), writes: d.ex.web_flash_writes_count() },
  d.ex.web_qa_first_use() === 0 && d.ex.web_qa_storage_status() === 1 && d.ex.web_flash_writes_count() > 0);
const initialized = d.sectors();
fs.writeFileSync(path.join(evidence, "initialized-nor.bin"), d.nor());
for (let p = 0; p < 8; p++) {
  // Given a selected track; When its real EDIT/GLO/SEQ/REC/key gestures run.
  d.tap(B.HOME); d.select(p);
  d.ex.web_buttons(1 << B.EDIT); d.render(400); d.key(0); d.ex.web_buttons(0); d.render(400);
  d.ex.web_buttons(1 << B.GLO); d.render(400); d.turn(EN.K1 + p % 4, -(p + 1)); d.ex.web_buttons(0); d.render(400);
  d.tap(B.SEQ); d.tap(B.REC);
  check(`T${p + 1} STEP recording remains stopped`, { selected: d.ex.web_qa_selected(), playing: d.ex.web_playing() }, d.ex.web_qa_selected() === p && d.ex.web_playing() === 0);
  d.key(7 + p); d.tap(B.REC);
  // Then the selected bank's own level and note differ on all eight tracks.
  check(`T${p + 1} panel edits its own level and recorded note`, { level: d.ex.web_qa_level(p), steps: d.ex.web_qa_steps(p), note: d.ex.web_qa_step_note(p, 0) },
    d.ex.web_qa_level(p) === 103 - p && d.ex.web_qa_steps(p) === 1 && d.ex.web_qa_step_note(p, 0) === 60 + p);
  d.screen(path.join(evidence, `track-${p + 1}.rgb565`));
}
d.tap(B.HOME);d.peak = 0;d.activeTracks = 0;d.tap(B.PLAY);d.render(2000);
check("panel-recorded song plays on all eight tracks", { playing: d.ex.web_playing(), trackMask: d.activeTracks, peak: d.peak, voices: d.maxVoices },
  d.ex.web_playing() === 1 && d.activeTracks === 255 && d.peak > 0.01 && d.maxVoices <= 8);
d.tap(B.PLAY);d.render(2000);
let settledMs = 0;
while (!d.ex.web_qa_quiet() && settledMs < 20000) { d.render(250);settledMs += 250; }
check("stopped song reaches actual DSP write-quiet within 20 seconds", { settledMs, quiet: d.ex.web_qa_quiet(), flags: d.ex.web_qa_quiet_flags(), fxBlockers: d.ex.web_qa_fx_blockers() }, d.ex.web_qa_quiet() === 1);
const names = ["ALPHA", "BRAVO", "CHARLIE"], signatures = [], tempos = [];
for (let slot = 0; slot < 3; slot++) {
  d.tap(B.HOME);d.select(7);d.turn(EN.K1, 1);d.turn(EN.SELECT, 1);
  signatures.push(d.signatures());tempos.push(d.ex.web_qa_bpm());
  saveProject(d, slot, names[slot]);
  check(`named project ${slot + 1} commits through NAME`, { slot: d.ex.web_qa_project_slot(), name: d.text(d.ex.web_qa_project_name(slot)) },
    d.ex.web_qa_project_slot() === slot && d.text(d.ex.web_qa_project_name(slot)) === names[slot]);
}
const named = d.sectors();
fs.writeFileSync(path.join(evidence, "named-nor.bin"), d.nor());
for (let slot = 0; slot < 3; slot++) {
  loadProject(d, slot);
  check(`project ${slot + 1} loads all eight independent tracks`, { slot: d.ex.web_qa_project_slot(), signatures: d.signatures(), bpm: d.ex.web_qa_bpm() },
    d.ex.web_qa_project_slot() === slot && equal(d.signatures(), signatures[slot]) && d.ex.web_qa_bpm() === tempos[slot]);
}
projectPage(d, 0);d.turn(EN.K4, 1);
const cancelHash = sha(d.nor()), cancelWrites = d.ex.web_flash_writes_count();d.tap(B.OCTUP);
check("existing project opens overwrite confirmation", { confirmation: d.ex.web_qa_confirmation() }, d.ex.web_qa_confirmation() === 1);
d.tap(B.OCTDN);
check("overwrite cancellation preserves NOR", { digest: sha(d.nor()), writes: d.ex.web_flash_writes_count() }, sha(d.nor()) === cancelHash && d.ex.web_flash_writes_count() === cancelWrites);
projectPage(d, 0);d.turn(EN.K4, 1);d.tap(B.OCTUP);d.tap(B.OCTUP);
const nameHash = sha(d.nor());setName(d, "CANCEL");d.tap(B.OCTDN);
check("NAME cancellation preserves project and NOR", { name: d.text(d.ex.web_qa_project_name(0)), digest: sha(d.nor()) },
  d.text(d.ex.web_qa_project_name(0)) === "ALPHA" && sha(d.nor()) === nameHash);
projectPage(d, 0);d.turn(EN.K4, 1);d.tap(B.PLAY);
const playingHash = sha(d.nor()), playingWrites = d.ex.web_flash_writes_count();d.tap(B.OCTUP);
check("project SAVE is refused during playback", { playing: d.ex.web_playing(), nameOpen: d.ex.web_qa_name_open(), confirmation: d.ex.web_qa_confirmation(), writes: d.ex.web_flash_writes_count() },
  d.ex.web_playing() === 1 && d.ex.web_qa_name_open() === 0 && d.ex.web_qa_confirmation() === 0 && sha(d.nor()) === playingHash && d.ex.web_flash_writes_count() === playingWrites);
d.tap(B.PLAY);d.render(2000);d.settle();
loadProject(d, 0);d.tap(B.HOME);d.select(0);d.turn(EN.K1, 2);
const overwritten = d.signatures();saveProject(d, 0, "REPLACED");
check("confirmed overwrite replaces project contents", { name: d.text(d.ex.web_qa_project_name(0)) }, d.text(d.ex.web_qa_project_name(0)) === "REPLACED");
loadProject(d, 1);loadProject(d, 0);
check("overwritten project reloads exact eight-track state", { signatures: d.signatures() }, equal(d.signatures(), overwritten));
const b = await device(bytes, named);
check("fresh WASM restores named sector catalog without write intent", { names: names.map((_, p) => b.text(b.ex.web_qa_project_name(p))), intent: b.ex.web_qa_intent(), writes: b.ex.web_flash_writes_count() },
  names.every((name, p) => b.text(b.ex.web_qa_project_name(p)) === name) && b.ex.web_qa_intent() === 0 && b.ex.web_flash_writes_count() === 0);
for (let slot = 0; slot < 3; slot++) {
  const restoredProject = await device(bytes, named);
  loadProject(restoredProject, slot);
  check(`fresh WASM loads project ${slot + 1}`, { signatures: restoredProject.signatures(), bpm: restoredProject.ex.web_qa_bpm() }, equal(restoredProject.signatures(), signatures[slot]) && restoredProject.ex.web_qa_bpm() === tempos[slot]);
}
loadProject(b, 2);
const consentHash = sha(b.nor());proposeSave(b, 2, "REFUSED");b.screen(path.join(evidence, "save-consent.rgb565"));
check("reboot save requires visible session consent", { phase: b.ex.web_qa_consent(), writes: b.ex.web_flash_writes_count() }, b.ex.web_qa_consent() === 1 && sha(b.nor()) === consentHash);
b.tap(B.HOME);
check("other input cancels session consent with no NOR write", { phase: b.ex.web_qa_consent(), intent: b.ex.web_qa_intent(), digest: sha(b.nor()) }, b.ex.web_qa_consent() === 0 && b.ex.web_qa_intent() === 0 && sha(b.nor()) === consentHash);
saveProject(b, 2, "RENEWED");
check("real SAVE renews session and commits after reboot", { intent: b.ex.web_qa_intent(), name: b.text(b.ex.web_qa_project_name(2)) }, b.ex.web_qa_intent() === 1 && b.text(b.ex.web_qa_project_name(2)) === "RENEWED");
b.tap(B.HOME);b.select(7);b.turn(EN.K1, 4);b.turn(EN.SELECT, 2);
const autoState = b.signatures(), autoBpm = b.ex.web_qa_bpm(), autoWrites = b.ex.web_qa_autosave_writes();
b.render(12000);
check("actual stopped scheduler autosaves a panel mutation", { before: autoWrites, after: b.ex.web_qa_autosave_writes() }, b.ex.web_qa_autosave_writes() === autoWrites + 1);
const autoSectors = b.sectors();fs.writeFileSync(path.join(evidence, "autosave-nor.bin"), b.nor());
const c = await device(bytes, autoSectors);
check("fresh WASM restores autosaved eight-track state and tempo", { signatures: c.signatures(), bpm: c.ex.web_qa_bpm(), writes: c.ex.web_flash_writes_count() }, equal(c.signatures(), autoState) && c.ex.web_qa_bpm() === autoBpm && c.ex.web_flash_writes_count() === 0);
// Given a real initialized NOR; When the separately labelled manufactured audio fixture plays.
async function audioFixture() {
  const s = await device(bytes, initialized);s.render(500);s.ex.web_test_eight_active();s.tap(B.PLAY);s.render(2000, true);return s;
}
const audio = await audioFixture(), again = await audioFixture();
const samples = Buffer.from(new Float32Array(audio.samples).buffer), repeated = Buffer.from(new Float32Array(again.samples).buffer);
fs.writeFileSync(path.join(evidence, "eight-active.f32"), samples);
check("manufactured eight-active fixture renders deterministically within shared voice cap", { frames: samples.length / 8, peak: audio.peak, allActive: audio.allActive, maxVoices: audio.maxVoices, sha256: sha(samples) },
  audio.peak > 0.01 && audio.allActive && audio.maxVoices <= 8 && samples.equals(repeated));
if (nativePath) {
  const native = fs.readFileSync(nativePath);
  check("native and WASM eight-active fixture are bit-for-bit equal", { bytes: native.length, sha256: sha(native) }, native.length > 0 && samples.equals(native));
}
const report = { format: "dabbl8-emulator-eight-workflows-v1", invocation: process.argv, wasm: { path: wasmPath, sha256: sha(bytes) }, cases, failures: cases.filter((c) => !c.passed).length,
  limitations: ["Audio fixture directly constructs sequencer steps; track editing/project/autosave cases use actual panel gestures.", "Host/WASM semantics do not establish FM-1 timing, memory, flash recovery or hardware safety.", "Original four-track emulator assertions are unchanged."] };
fs.writeFileSync(path.join(evidence, "eight-workflows.json"), JSON.stringify(report, null, 2));
console.log(`eight-emu: ${cases.length} cases, ${report.failures} failures`);
process.exitCode = report.failures ? 1 : 0;
