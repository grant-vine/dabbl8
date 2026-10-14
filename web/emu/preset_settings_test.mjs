// SPDX-License-Identifier: GPL-3.0-only
// Real eight-track preset/setting panel workflows, booted from actual first-use sectors.
import fs from "node:fs";
import path from "node:path";
import { device, B, EN } from "./workflow_device.mjs";
import { storeSectors } from "./worklet.js";
const [wasmPath, initializedNor, evidence] = process.argv.slice(2);
if (!wasmPath || !initializedNor || !evidence) throw new Error("WASM, initialized NOR and EVIDENCE_DIR are required");
const bytes = fs.readFileSync(wasmPath), sectors = storeSectors(fs.readFileSync(initializedNor)), cases = [];
function check(scenario, observable, passed) { cases.push({ scenario, observable, passed });console.log(`eight-ancillary: ${scenario}: ${passed ? "PASS" : "FAIL"} ${JSON.stringify(observable)}`); }
const d = await device(bytes, sectors);
d.tap(B.HOME);d.select(7);d.turn(EN.K1, 3);
const signature = d.ex.web_qa_track_signature(7), writes = d.ex.web_flash_writes_count();
d.tap(B.SAVE);d.tap(B.OCTUP);d.tap(B.OCTUP);d.render(300);
check("T8 user preset saves through actual NAME confirmation", { used: d.ex.web_test_user_preset(0), writes: d.ex.web_flash_writes_count() }, d.ex.web_test_user_preset(0) === 1 && d.ex.web_flash_writes_count() > writes);
d.tap(B.HOME);d.turn(EN.K1, 5);d.tap(B.SAVE);d.turn(EN.K2, 1);d.tap(B.OCTUP);d.render(300);
check("actual USER LOAD restores saved T8 sound", { signature: d.ex.web_qa_track_signature(7), expected: signature }, d.ex.web_qa_track_signature(7) === signature);
const kept = d.sectors(), restored = await device(bytes, kept), absent = await device(bytes, sectors);
check("user preset survives sector retention in fresh WASM", { restored: restored.ex.web_test_user_preset(0), absent: absent.ex.web_test_user_preset(0) }, restored.ex.web_test_user_preset(0) === 1 && absent.ex.web_test_user_preset(0) === 0);
const previousPalette = d.ex.web_qa_palette();
d.tap(B.HOME);d.ex.web_buttons(1 << B.HOME);d.render(800);d.ex.web_buttons(0);d.render(100);d.turn(EN.K1, 1);d.tap(B.HOME);d.render(2000);
const palette = d.ex.web_qa_palette(), settingsRestored = await device(bytes, d.sectors());
check("actual MENU palette choice persists across fresh WASM", { before: previousPalette, chosen: palette, restored: settingsRestored.ex.web_qa_palette() }, palette !== previousPalette && settingsRestored.ex.web_qa_palette() === palette);
fs.writeFileSync(path.join(evidence, "preset-settings-nor.bin"), d.nor());
fs.writeFileSync(path.join(evidence, "preset-settings.json"), JSON.stringify({ invocation: process.argv, cases, failures: cases.filter((c) => !c.passed).length }, null, 2));
process.exitCode = cases.some((c) => !c.passed) ? 1 : 0;
