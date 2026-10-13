// SPDX-License-Identifier: GPL-3.0-only
// Source/compiler identity for the local cached-input emulator checks.
import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";
import { execFileSync } from "node:child_process";
const [out] = process.argv.slice(2);
if (!out) throw new Error("Build directory is required");
const command = (binary, args) => execFileSync(binary, args, { encoding: "utf8" }).trim();
const hash = (file) => crypto.createHash("sha256").update(fs.readFileSync(file)).digest("hex");
const sourceFiles = command("rg", ["--files", "firmware/src", "tests/hostsim.c", "web/emu", "build/gen"]).split("\n")
  .filter((file) => /\.(c|h|mjs|js|sh|html|md)$/.test(file));
const report = { invocation: process.argv, head: command("git", ["rev-parse", "HEAD"]), branch: command("git", ["branch", "--show-current"]),
  tools: { node: process.version, emcc: command("emcc", ["--version"]), cc: command("cc", ["--version"]) },
  configuration: { NPART: 8, DABBL8_IDENTITY: 1, FELUCCA_FM4: 0, FELUCCA_SLICE: 1, FELUCCA_VERSION: "v0.1.1", deviceWrites: false },
  sourceHashes: Object.fromEntries(sourceFiles.map((file) => [file, hash(file)])),
  binaryHashes: Object.fromEntries(["felucca.wasm", "native_eight_check"].map((file) => [file, hash(path.join(out, file))])) };
fs.writeFileSync(path.join(out, "evidence/source-receipt.json"), JSON.stringify(report, null, 2));
