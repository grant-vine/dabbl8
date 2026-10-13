// SPDX-License-Identifier: GPL-3.0-only
// Generated build configuration is the browser's mode-selection boundary.
// node web/emu/build_mode_test.mjs UPSTREAM_DIR DABBL8_DIR EVIDENCE_JSON
import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";
const [fourDir, eightDir, output] = process.argv.slice(2);
if (!fourDir || !eightDir || !output) throw new Error("Two build directories and EVIDENCE_JSON are required");
async function readBuild(dir) {
  const html = fs.readFileSync(path.join(dir, "index.html"), "utf8");
  const wasm = fs.readFileSync(path.join(dir, "felucca.wasm"));
  const { instance } = await WebAssembly.instantiate(wasm, {});
  instance.exports._initialize();
  return { title: html.match(/<title>(.*?)<\/title>/)[1], version: html.match(/class="version">(.*?)<\/small>/)[1],
    store: html.match(/const STORE = "(.*?)"/)[1], midi: html.match(/const MIDI_ENABLED = (true|false);/)[1] === "true",
    installer: html.includes("../installer/"), tracks: instance.exports.web_qa_tracks(),
    unresolved: /@(APP|VERSION|PREVIEW|STORE|MIDI|NAV)@/.test(html),
    wasmSha256: crypto.createHash("sha256").update(wasm).digest("hex") };
}
const upstream = await readBuild(fourDir), dabbl8 = await readBuild(eightDir);
const cases = [
  { scenario: "default build retains upstream four-track identity/storage/MIDI/navigation", passed: upstream.title === "Felucca" && upstream.tracks === 4 && upstream.store === "felucca-emu-flash" && upstream.midi && upstream.installer && !upstream.unresolved },
  { scenario: "explicit Dabbl8 build uses eight tracks and current experimental identity", passed: dabbl8.title === "Dabbl8" && dabbl8.tracks === 8 && dabbl8.version === "v0.1.1 (experimental preview)" && !dabbl8.unresolved },
  { scenario: "Dabbl8 saves are isolated from upstream storage", passed: dabbl8.store === "dabbl8-emu-flash-v1" && dabbl8.store !== upstream.store },
  { scenario: "Dabbl8 build does not enable automatic MIDI or absent installer navigation", passed: !dabbl8.midi && !dabbl8.installer }
];
fs.writeFileSync(output, JSON.stringify({ invocation: process.argv, upstream, dabbl8, cases }, null, 2));
for (const c of cases) console.log(`emu-build: ${c.scenario}: ${c.passed ? "PASS" : "FAIL"}`);
process.exitCode = cases.some((c) => !c.passed) ? 1 : 0;
