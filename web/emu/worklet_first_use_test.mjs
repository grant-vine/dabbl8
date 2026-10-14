// SPDX-License-Identifier: GPL-3.0-only
// Actual worklet module/render queue + real WASM; only the browser message port is collected.
import fs from "node:fs";
import path from "node:path";
const [wasmPath, evidence] = process.argv.slice(2);
if (!wasmPath || !evidence) throw new Error("WASM and EVIDENCE_DIR are required");
let Processor;
globalThis.AudioWorkletProcessor = class {
  constructor() {
    this.messages = [];
    this.port = { postMessage: (message, transfer = []) => this.messages.push(structuredClone(message, { transfer })) };
  }
};
globalThis.registerProcessor = (_name, klass) => { Processor = klass; };
globalThis.currentTime = 0;
await import("./worklet.js");
const bytes = fs.readFileSync(wasmPath), cases = [];
function check(scenario, observable, passed) { cases.push({ scenario, observable, passed });console.log(`worklet-first-use: ${scenario}: ${passed ? "PASS" : "FAIL"} ${JSON.stringify(observable)}`); }
async function processor(sectors) {
  const p = new Processor();
  await p.onMessage({ type: "load", wasm: bytes, sectors, master: 700 });
  let frames = 0;
  const outputs = [[new Float32Array(128), new Float32Array(128)]];
  p.render = (ms) => {
    for (let n = 0; n < Math.round(ms * 44.1); n += 128) {
      frames += 128;globalThis.currentTime = frames / 44100;p.process([], outputs);
    }
  };
  p.tap = async (mask) => { await p.onMessage({ type: "buttons", mask });await p.onMessage({ type: "buttons", mask: 0 }); };
  p.saved = () => p.messages.filter((m) => m.sectors).at(-1)?.sectors || {};
  await p.onMessage({ type: "buttons", mask: 0 });await p.onMessage({ type: "keys", mask: 0 });p.render(1000);
  return p;
}
const p = await processor({});
await p.tap(128);p.render(1000);
check("queued real SAVE publishes first-use execution progress", { phase: p.ex.web_first_use_phase(), published: p.messages.some((m) => m.firstUsePhase === 5) }, p.ex.web_first_use_phase() === 5 && p.messages.some((m) => m.firstUsePhase === 5));
let completionMs = 1000;
while (p.ex.web_first_use_phase() === 5 && completionMs < 30000) { p.render(250);completionMs += 250; }
p.render(50);
check("queued first-use completes and publishes controls-ready phase", { completionMs, phase: p.ex.web_first_use_phase(), writes: p.ex.web_flash_writes_count(), publishedReady: p.messages.some((m) => m.firstUsePhase === 0) }, p.ex.web_first_use_phase() === 0 && p.ex.web_flash_writes_count() === 159 && p.messages.some((m) => m.firstUsePhase === 0));
const reboot = await processor(p.saved());
check("actual published worklet sectors survive fresh WASM boot", { phase: reboot.ex.web_first_use_phase(), status: reboot.ex.web_qa_storage_status(), writes: reboot.ex.web_flash_writes_count() }, reboot.ex.web_first_use_phase() === 0 && reboot.ex.web_qa_storage_status() === 1 && reboot.ex.web_flash_writes_count() === 0);
const held = await processor({});
await held.onMessage({ type: "buttons", mask: 128 });held.render(5000);
check("latched SAVE waits for an explicit release without writes", { phase: held.ex.web_first_use_phase(), writes: held.ex.web_flash_writes_count(), publishedRelease: held.messages.some((m) => m.firstUsePhase === 4) }, held.ex.web_first_use_phase() === 4 && held.ex.web_flash_writes_count() === 0 && held.messages.some((m) => m.firstUsePhase === 4));
await held.onMessage({ type: "buttons", mask: 0 });held.render(30000);
check("explicit latched SAVE release completes first-use", { phase: held.ex.web_first_use_phase(), writes: held.ex.web_flash_writes_count() }, held.ex.web_first_use_phase() === 0 && held.ex.web_flash_writes_count() === 159);
const interrupted = await processor({});await interrupted.tap(128);interrupted.render(10000);await interrupted.tap(64);interrupted.render(1000);
const interruptedWrites = interrupted.ex.web_flash_writes_count();interrupted.render(1000);
check("competing raw input still fails closed in native first-use", { phase: interrupted.ex.web_first_use_phase(), writes: interrupted.ex.web_flash_writes_count(), preservedWrites: interruptedWrites, publishedFailure: interrupted.messages.some((m) => m.firstUsePhase === 6) }, interrupted.ex.web_first_use_phase() === 6 && interrupted.ex.web_flash_writes_count() === interruptedWrites && interrupted.messages.some((m) => m.firstUsePhase === 6));
fs.writeFileSync(path.join(evidence, "worklet-interrupted-nor.bin"), new Uint8Array(interrupted.ex.memory.buffer, interrupted.ex.web_nor(), interrupted.ex.web_nor_size()));
const retry = await processor(interrupted.saved());await retry.tap(128);retry.render(30000);
check("retained interrupted first-use can restart through real SAVE", { phase: retry.ex.web_first_use_phase(), status: retry.ex.web_qa_storage_status(), writes: retry.ex.web_flash_writes_count() }, retry.ex.web_first_use_phase() === 0 && retry.ex.web_qa_storage_status() === 1 && retry.ex.web_flash_writes_count() === 159);
const report = { invocation: process.argv, cases, failures: cases.filter((c) => !c.passed).length, completionMs,
  phaseMessages: p.messages.filter((m) => m.firstUsePhase !== undefined).map((m) => m.firstUsePhase),
  limitations: ["Worklet quanta run deterministically in Node; real browser wall time and the progress overlay are verified separately."] };
fs.writeFileSync(path.join(evidence, "worklet-first-use.json"), JSON.stringify(report, null, 2));
process.exitCode = report.failures ? 1 : 0;
