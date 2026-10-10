// SPDX-License-Identifier: GPL-3.0-only
// Companion + actual C parser/handlers; mocks only for malformed/reconnect cases.
import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { createInterface } from "node:readline";
import { D8Tracks, TRACK_OP, TrackProtocolError, parseTrackInfo, parseTrackCaps } from "./d8tracks.js";
let checks = 0;
function ok(value, label) { assert.ok(value, label); checks++; console.log(`tracks: ${label} ok`); }
async function refuses(operation, label) { await assert.rejects(operation); checks++; console.log(`tracks: ${label} ok`); }
class Bridge {
  constructor(path) {
    this.calls = []; this.pending = []; this.child = spawn(path, [], { stdio: ["pipe", "pipe", "inherit"] });
    this.exit = new Promise((resolve) => this.child.on("close", (code) => { this.pending.splice(0).forEach((p) => p.reject(Error("Bridge exited"))); resolve(code); }));
    createInterface({ input: this.child.stdout }).on("line", (line) => {
      const p = this.pending.shift(); if (!p) throw Error("Unexpected bridge reply");
      try { const r = JSON.parse(line); assert.equal(r.flash_writes, 0); assert.equal(r.flash_erases, 0); p.resolve(r); } catch (e) { p.reject(e); }
    });
    this.child.on("error", (e) => this.pending.splice(0).forEach((p) => p.reject(e)));
  }
  line(line) { return new Promise((resolve, reject) => { this.pending.push({ resolve, reject }); this.child.stdin.write(line + "\n"); }); }
  request = async ([cmd, args]) => {
    this.calls.push([cmd, [...args]]); const r = await this.line([cmd, ...args].join(" "));
    if (r.cmd === 76 && r.args.length === 3 && r.args[0] === 1 && r.args[1] === cmd) throw new TrackProtocolError(r.args[2]);
    if (r.cmd !== cmd) throw Error("Mismatched firmware command"); return r.args;
  };
  async close() { this.child.stdin.end(); assert.equal(await this.exit, 0); }
}
const eight = new Bridge(process.argv[2]), four = new Bridge(process.argv[3]);
try {
  const client = await D8Tracks.connect(eight.request);
  ok(client.editable && client.caps.tracks === 8 && client.caps.voices === 8, "actual expanded capabilities grant only track editing");
  const info = await eight.request([1, []]), caps = await eight.request([75, []]);
  let meta = await client.readTracks(); ok(meta.tracks.length === 8, "metadata contains all eight actual tracks");
  for (let track = 0; track < 8; track++) {
    meta = await client.select(track); ok(meta.selected === track, "selection reaches track " + (track + 1));
    const previous = (await client.readTracks()).tracks.map((t) => t.level);
    const changed = await client.parameter(track, 0, 20 + track);
    ok(changed.value === 20 + track && (await client.parameter(track, 0)).value === changed.value, "parameter write/read reaches track " + (track + 1));
    meta = await client.readTracks(); ok(meta.tracks.every((t) => t.level === (t.track === track ? 20 + track : previous[t.track])), "parameter write isolates track " + (track + 1));
    const dump = await client.dump(track); ok(dump.params.length === 99 && dump.params[0] === 20 + track, "complete dump reaches track " + (track + 1));
    const step = { n: 2, notes: [60 + track, 72 + track, 0, 0], time: 0, flags: 3, vel: 100, hit: 255, acc: 128, chance: 42, ratchet: 4 };
    const written = await client.step(track, 63, step), read = await client.step(track, 63);
    ok(Object.entries(step).every(([k, v]) => JSON.stringify(written[k]) === JSON.stringify(v)) && JSON.stringify(read) === JSON.stringify(written), "full step/drum/chance/ratchet round trip reaches track " + (track + 1));
    const mixed = await client.mix(track, { level: 60 + track, mute: true });
    ok(mixed.level === 60 + track && mixed.mute && (await client.mix(track)).mute, "mix write/read reaches track " + (track + 1));
  }
  const count = eight.calls.length;
  for (const operation of [() => client.select(8), () => client.select(-1), () => client.parameter(7, 99), () => client.parameter(7, 0, 8192),
    () => client.parameter(7, 0, "42"), () => client.step(7, 64), () => client.mix(7, { level: 128, mute: true }),
    () => client.mix(7, { level: 50, mute: 1 }), () => client.step(7, 0, { n: 5 }),
    () => client.step(7, 0, { n: 1, notes: [60, 0, 0, 0], time: 0, flags: 0, vel: 100, hit: 0, acc: 1, chance: 100, ratchet: 1 })]) {
    await refuses(operation, "invalid local index/value refuses before transport");
  }
  ok(eight.calls.length === count && client.editable, "local input refusal sends nothing and preserves the session");
  await eight.line("BUSY 1");
  await assert.rejects(() => client.step(7, 63, { n: 1, notes: [60, 0, 0, 0], time: 0, flags: 0, vel: 90, hit: 0, acc: 0, chance: 100, ratchet: 1 }), (e) => e instanceof TrackProtocolError && e.reason === 4); checks++;
  ok(client.editable && (await client.mix(7, { level: 19, mute: false })).level === 19, "actual busy refusal is recoverable and live mix remains available");
  await eight.line("BUSY 0");
  const old = await D8Tracks.connect(four.request);
  ok(!old.editable && old.info.tracks === 4, "actual default four-track firmware remains read-only in companion");
  const oldCalls = four.calls.length; await refuses(() => old.select(0), "read-only client refuses track mutation");
  ok(four.calls.length === oldCalls && !four.calls.some(([cmd]) => cmd === 77), "no unsupported track command sent to four-track firmware");
  const noTag = info.slice(0, -3), calls = [];
  const legacy = await D8Tracks.connect(async ([cmd, args]) => { calls.push([cmd, args]); return noTag; });
  ok(!legacy.editable && calls.length === 1 && calls[0][0] === 1, "untagged firmware receives identity read only");
  for (const [index, value] of [[0, 0], [2, 2], [3, 9], [4, 63], [5, 7], [6, 63], [7, 100], [8, 28], [9, 15], [10, 2], [11, 2], [12, 2], [13, 10], [14, 1], [14, 7]]) {
    const bad = [...caps]; bad[index] = value; const sent = [];
    const wrong = await D8Tracks.connect(async ([cmd, a]) => { sent.push([cmd, a]); return cmd === 1 ? info : bad; });
    ok(!wrong.editable, "unsupported/mismatched capability " + index + "=" + value + " stays read-only");
    await refuses(() => wrong.mix(7, { level: 50, mute: false }), "mismatched capability refuses before write");
    ok(sent.every(([cmd]) => cmd === 1 || cmd === 75), "mismatched capability sends no edit");
  }
  for (const count of [5, 6, 7]) {
    const dynamicInfo = [...info], parsed = parseTrackInfo(info); let i = info.indexOf(0) + 1 + 5;
    for (let n = 0; n < parsed.engines; n++) i = info.indexOf(0, i) + 1;
    dynamicInfo[i] = count; const dynamicCaps = [...caps]; dynamicCaps[3] = count;
    const dynamic = await D8Tracks.connect(async ([cmd, a]) => {
      if (cmd === 1) return dynamicInfo; if (cmd === 75) return dynamicCaps;
      return [1, TRACK_OP.SELECT, 0, count, ...Array.from({ length: count }, () => [0, 0, 50, 64, 0, 0]).flat()];
    });
    ok(dynamic.editable && (await dynamic.readTracks()).tracks.length === count, "arrays follow negotiated count " + count + " rather than a fixed eight");
  }
  const mutationCases = [
    [TRACK_OP.SELECT, (a) => a.slice(0, -1)], [TRACK_OP.SELECT, (a) => { a[3] = 7; return a; }],
    [TRACK_OP.PARAM, (a) => { a[0] = 2; return a; }], [TRACK_OP.PARAM, (a) => { a[1] = 28; return a; }],
    [TRACK_OP.PARAM, (a) => { a[2] = 6; return a; }], [TRACK_OP.PARAM, (a) => { a[3] = 1; return a; }],
    [TRACK_OP.PARAM, (a) => { a[4] = 128; return a; }], [TRACK_OP.DUMP, (a) => a.slice(0, -1)],
    [TRACK_OP.STEP, (a) => { a[3] = 62; return a; }], [TRACK_OP.STEP, (a) => { a[16] = 0; return a; }],
  ];
  for (const [op, corrupt] of mutationCases) {
    const broken = await D8Tracks.connect(async (r) => { const a = await eight.request(r); return r[0] === 77 ? corrupt([...a]) : a; });
    const action = op === TRACK_OP.SELECT ? () => broken.readTracks() : op === TRACK_OP.PARAM ? () => broken.parameter(7, 0) : op === TRACK_OP.DUMP ? () => broken.dump(7) : () => broken.step(7, 63);
    await refuses(action, "malformed acknowledgment refused for operation " + op);
    ok(!broken.editable, "malformed acknowledgment disables further editing");
  }
  let release, started; const start = new Promise((r) => { started = r; }); let edits = 0;
  const queued = await D8Tracks.connect(async ([cmd]) => {
    if (cmd === 1) return info; if (cmd === 75) return caps;
    edits++; started(); return new Promise((r) => { release = r; });
  });
  const first = queued.parameter(7, 0, 20), second = queued.parameter(6, 0, 21);
  const results = Promise.allSettled([first, second]); await start; queued.close(); release([1, 31, 7, 0, 20, 64]);
  ok((await results).every((r) => r.status === "rejected") && edits === 1 && !queued.editable, "disconnect ignores pending reply and cancels queued mutation before transport");
  let resolveBad, sent = 0; const corruptQueue = await D8Tracks.connect(async ([cmd]) => {
    if (cmd === 1) return info; if (cmd === 75) return caps;
    sent++; return new Promise((r) => { resolveBad = r; });
  });
  const badFirst = corruptQueue.parameter(7, 0, 20), badSecond = corruptQueue.parameter(6, 0, 21);
  const badResults = Promise.allSettled([badFirst, badSecond]); await new Promise((r) => setImmediate(r)); resolveBad([1, 31, 6, 0, 20, 64]);
  ok((await badResults).every((r) => r.status === "rejected") && sent === 1, "full reply validation finishes before a queued edit can transmit");
  client.close(); const reconnect = await D8Tracks.connect(eight.request);
  ok(reconnect.editable && (await reconnect.readTracks()).tracks.length === 8, "fresh handshake reconnects without reviving the closed client");
  ok(!client.editable, "old closed client stays disabled after reconnect");
  ok(eight.calls.every(([cmd]) => cmd === 1 || cmd === 75 || (cmd === 77)) && eight.calls.filter(([cmd]) => cmd === 77).every(([, a]) => Object.values(TRACK_OP).includes(a[1])), "client exposes no legacy/project/sample/backup write family");
  assert.throws(() => parseTrackInfo([])); checks++;
  assert.throws(() => parseTrackCaps([...caps, 0])); checks++;
  assert.throws(() => { reconnect.caps.tracks = 1; }); checks++;
  ok(reconnect.caps.tracks === 8, "negotiated bounds cannot be mutated by callers");
  console.log(`companion tracks: ${checks} checks passed; actual C four/eight-track handlers, no physical device`);
} finally { await eight.close(); await four.close(); }
