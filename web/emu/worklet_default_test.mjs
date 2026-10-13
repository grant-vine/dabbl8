// SPDX-License-Identifier: GPL-3.0-only
// The upstream four-track worklet keeps its original panel/storage behavior.
import fs from "node:fs";
const [wasmPath, output] = process.argv.slice(2);
if (!wasmPath || !output) throw new Error("Four-track WASM and EVIDENCE_JSON are required");
let Processor;
globalThis.AudioWorkletProcessor = class {
  constructor() { this.messages = [];this.port = { postMessage: (m, transfer = []) => this.messages.push(structuredClone(m, { transfer })) }; }
};
globalThis.registerProcessor = (_name, klass) => { Processor = klass; };
globalThis.currentTime = 0;
await import("./worklet.js");
const bytes = fs.readFileSync(wasmPath);
async function processor(sectors) {
  const p = new Processor();await p.onMessage({ type: "load", wasm: bytes, sectors, master: 700 });
  let frames = 0;const out = [[new Float32Array(128), new Float32Array(128)]];
  p.render = (ms) => { for (let n = 0; n < Math.round(ms * 44.1); n += 128) { frames += 128;globalThis.currentTime = frames / 44100;p.process([], out); } };
  p.tap = async (bit) => { await p.onMessage({ type: "buttons", mask: 1 << bit });await p.onMessage({ type: "buttons", mask: 0 });p.render(200); };
  p.render(1000);return p;
}
const p = await processor({});
const cases = [{ scenario: "upstream four-track worklet does not emit first-use progress", observable: { tracks: p.ex.web_qa_tracks(), phases: p.messages.filter((m) => m.firstUsePhase !== undefined).length }, passed: p.ex.web_qa_tracks() === 4 && !p.messages.some((m) => m.firstUsePhase !== undefined) }];
await p.tap(7);await p.tap(13);await p.tap(13);
const sectors = p.messages.filter((m) => m.sectors).at(-1)?.sectors || {}, reboot = await processor(sectors);
cases.push({ scenario: "upstream queued panel preset SAVE retains across real sector boot", observable: { used: p.ex.web_test_user_preset(0), restored: reboot.ex.web_test_user_preset(0), sectors: Object.keys(sectors).length }, passed: p.ex.web_test_user_preset(0) === 1 && reboot.ex.web_test_user_preset(0) === 1 && Object.keys(sectors).length > 0 });
fs.writeFileSync(output, JSON.stringify({ invocation: process.argv, cases }, null, 2));
for (const c of cases) console.log(`upstream-worklet: ${c.scenario}: ${c.passed ? "PASS" : "FAIL"}`);
process.exitCode = cases.some((c) => !c.passed) ? 1 : 0;
