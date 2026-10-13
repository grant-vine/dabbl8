// SPDX-License-Identifier: GPL-3.0-only
// Byte comparison of real native and WASM interleaved float32 outputs.
import fs from "node:fs";
import crypto from "node:crypto";
const [wasmPath, nativePath, output] = process.argv.slice(2);
if (!wasmPath || !nativePath || !output) throw new Error("WASM audio, native audio and EVIDENCE_JSON are required");
const wasm = fs.readFileSync(wasmPath), native = fs.readFileSync(nativePath);
const sha = (bytes) => crypto.createHash("sha256").update(bytes).digest("hex");
const passed = wasm.length === 88200 * 8 && wasm.equals(native);
const report = { invocation: process.argv, scenario: "native/WASM eight-active audio parity", frames: wasm.length / 8,
  nativeSha256: sha(native), wasmSha256: sha(wasm), passed };
fs.writeFileSync(output, JSON.stringify(report, null, 2));
console.log(`eight-emu: ${report.scenario}: ${passed ? "PASS" : "FAIL"} ${report.frames} frames`);
process.exitCode = passed ? 0 : 1;
