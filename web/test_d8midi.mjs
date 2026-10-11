// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { createInterface } from 'node:readline';
import { D8MidiSession } from './d8midi.js';
import { TrackProtocolError } from './d8tracks.js';
let checks = 0;
function ok(value, label) { assert.ok(value, label); checks++; console.log('MIDI session: ' + label + ' ok'); }
const frame = (cmd, args = []) => [240, 125, 70, 76, cmd, ...args, 247];
// Known modern INFO prefix before D8, as emitted by the actual C handler below.
const info = [...Buffer.from('test'), 0, 14, 99, 27, 64, 91, ...Array(14).fill(0), 8,
  16, 0x55, 1, 15, 0x4d, 1, 64, 1, 0x42, 1, 0, 0x46, 1, 8, 0,
  0x53, 1, 3, 0x50, 1, 3, 0x4e, 1, 0, 0x52, 1, 4, 0x4c, 1, 1, 68, 56, 1];
const caps = [68, 56, 1, 8, 64, 8, 64, 99, 27, 14, 1, 1, 1, 9, 5];
class Port extends EventTarget {
  constructor(type) { super(); this.type = type; this.state = 'connected'; this.connection = 'closed'; this.opens = 0; this.closes = 0; this.sent = []; }
  async open() { this.opens++; if (this.failOpen) throw Error('open failed'); if (this.openWait) await this.openWait; this.connection = 'open'; return this; }
  async close() { this.closes++; this.connection = 'closed'; if (this.failClose) throw Error('close failed'); return this; }
  send(bytes) { this.sent.push([...bytes]); if (this.failSend) throw Error('send failed'); this.respond?.(bytes); }
  receive(data) { const e = new Event('midimessage'); e.data = Uint8Array.from(data); this.dispatchEvent(e); }
  stateChange(state = 'disconnected', connection = 'pending') { this.state = state; this.connection = connection; this.dispatchEvent(new Event('statechange')); }
}
function pair() {
  const input = new Port('input'), output = new Port('output');
  output.respond = (bytes) => queueMicrotask(() => input.receive(frame(bytes[4], bytes[4] === 1 ? info : caps)));
  return { input, output };
}
async function refuses(promise, label) { await assert.rejects(promise); ok(true, label); }
const p = pair(); const session = await D8MidiSession.connect(p.input, p.output);
ok(session.connected && session.tracks.editable, 'framed INFO/caps handshake');
ok(JSON.stringify(p.output.sent) === JSON.stringify([frame(1), frame(75)]), 'only bounded handshake frames sent');
await refuses(D8MidiSession.connect(p.input, p.output), 'owned ports reject concurrent connection');
const client = session.tracks;
p.output.respond = () => {
  p.input.receive([144, 60, 100]); p.input.receive([240, 1, 2, 3, 247]);
  p.input.receive(frame(32, [0])); p.input.receive(frame(77, [1, 28, 7, 64, 64, 1]));
};
const mixed = await client.mix(7, { level: 64, mute: true });
ok(mixed.level === 64 && mixed.mute, 'notes alien SysEx and WATCH push do not acknowledge request');
p.output.respond = () => p.input.receive(frame(76, [1, 77, 4]));
await assert.rejects(client.step(7, 0), (e) => e instanceof TrackProtocolError && e.reason === 4);
ok(client.editable && session.connected, 'valid busy refusal remains recoverable');
p.output.respond = () => p.input.receive(frame(77, [1, 31, 7, 5, 64, 63]));
ok((await client.parameter(7, 5, -64)).value === -64, 'signed parameter framing and acknowledgment');
const closed = session.close(); ok(closed === session.close(), 'close is idempotent'); await closed;
ok(!session.connected && !client.editable && p.input.closes === 1 && p.output.closes === 1, 'close invalidates client and releases both ports');
const sent = p.output.sent.length; p.input.receive(frame(77)); await refuses(client.mix(7), 'closed client never sends'); ok(p.output.sent.length === sent, 'late old-session frame cannot restart traffic');
const fresh = await D8MidiSession.connect(...Object.values(pair())); await fresh.close();
ok(true, 'fresh instance negotiates independently');
for (const [label, bad] of [
  ['wrong command', frame(75, caps)], ['unknown refusal', frame(76, [1, 77, 5])],
  ['wrong refusal command', frame(76, [1, 75, 4])], ['short refusal', frame(76, [1, 77])],
  ['wrong refusal schema', frame(76, [2, 77, 4])], ['non-seven-bit payload', frame(77, [128])],
  ['missing terminator', [240, 125, 70, 76, 77, 1]], ['oversized reply', frame(77, Array(595).fill(0))]
]) {
  const q = pair(), s = await D8MidiSession.connect(q.input, q.output); q.output.respond = () => q.input.receive(bad);
  await refuses(s.tracks.mix(0), label + ' rejects');
  await s.close(); ok(!s.connected && !s.tracks.editable && q.input.closes === 1, label + ' closes session');
}
for (const reason of [1, 2, 3]) {
  const q = pair(), s = await D8MidiSession.connect(q.input, q.output); q.output.respond = () => q.input.receive(frame(76, [1, 77, reason]));
  await assert.rejects(s.tracks.mix(0), (e) => e instanceof TrackProtocolError && e.reason === reason);
  ok(!s.tracks.editable, 'firmware refusal ' + reason + ' disables editing'); await s.close();
}
{
  const q = pair(), s = await D8MidiSession.connect(q.input, q.output, { timeout: 10 }); q.output.respond = () => {};
  const one = s.tracks.parameter(7, 0, 10), two = s.tracks.parameter(6, 0, 11);
  await Promise.all([refuses(one, 'missing reply rejects in-flight edit'), refuses(two, 'timeout blocks queued edit')]);
  await s.close(); ok(q.output.sent.length === 3 && !s.connected, 'timeout never retries uncertain mutation');
}
for (const target of ['input', 'output']) {
  const q = pair(), s = await D8MidiSession.connect(q.input, q.output); q.output.respond = () => {};
  const waiting = s.tracks.mix(0); q[target].stateChange(); await refuses(waiting, target + ' disconnect rejects pending request');
  await s.close(); ok(!s.connected && !s.tracks.editable, target + ' disconnect invalidates client');
}
{
  const q = pair(), s = await D8MidiSession.connect(q.input, q.output); q.output.failSend = true;
  await refuses(s.tracks.mix(0), 'send exception rejects'); await s.close(); ok(!s.connected, 'send exception closes session');
}
{
  const q = pair(); q.output.failOpen = true;
  await refuses(D8MidiSession.connect(q.input, q.output), 'partial open failure rejects');
  ok(q.input.closes === 1 && q.output.closes === 1 && q.output.sent.length === 0, 'partial open cleans both ports before any send');
  q.output.failOpen = false; await (await D8MidiSession.connect(q.input, q.output)).close(); ok(true, 'failed open releases ownership');
}
{
  const q = pair(), abort = new AbortController(); let release; q.output.openWait = new Promise((r) => { release = r; });
  const waiting = D8MidiSession.connect(q.input, q.output, { signal: abort.signal }); abort.abort(); release();
  await refuses(waiting, 'abort during open rejects'); ok(q.output.sent.length === 0 && q.input.closes === 1 && q.output.closes === 1, 'cancelled opening sends nothing and closes after open settles');
}
{
  const q = pair(), abort = new AbortController(), s = await D8MidiSession.connect(q.input, q.output, { signal: abort.signal });
  abort.abort(); await s.close(); ok(!s.connected && !s.tracks.editable, 'abort after connection invalidates session');
}
{
  const q = pair(), s = await D8MidiSession.connect(q.input, q.output); q.input.failClose = true;
  const results = await s.close(); ok(results[0].status === 'rejected' && results[1].status === 'fulfilled', 'browser close failure is observable without preventing peer cleanup');
}
{
  const q = pair(), s = await D8MidiSession.connect(q.input, q.output); q.input.receive(frame(77)); await s.close();
  ok(!s.connected, 'unsolicited own reply invalidates idle session');
}
{
  const q = pair(), abort = new AbortController(); abort.abort();
  await refuses(D8MidiSession.connect(q.input, q.output, { signal: abort.signal }), 'pre-aborted connect sends nothing');
  await refuses(D8MidiSession.connect(q.input, q.output, { signal: {} }), 'invalid signal rejected before ownership');
  ok(q.input.opens === 0 && q.output.sent.length === 0, 'invalid or cancelled options never open ports');
  await (await D8MidiSession.connect(q.input, q.output)).close(); ok(true, 'invalid options do not retain ownership');
}
{
  const q = pair(), abort = new AbortController(); let first;
  q.output.respond = () => { first?.(); };
  const sent = new Promise((r) => { first = r; });
  const connecting = D8MidiSession.connect(q.input, q.output, { signal: abort.signal });
  await sent; abort.abort(); await refuses(connecting, 'abort during handshake rejects and releases ports');
  ok(q.input.closes === 1 && q.output.closes === 1 && q.output.sent.length === 1, 'cancelled handshake never continues with CAPS');
}
{
  const q = pair(); let external = 0; q.input.addEventListener('midimessage', () => { external++; });
  const s = await D8MidiSession.connect(q.input, q.output); await s.close(); q.input.receive([144, 60, 0]);
  ok(external === 3, 'cleanup preserves unrelated MIDI listeners');
}
{
  const q = pair(), s = await D8MidiSession.connect(q.input, q.output); q.output.respond = () => {};
  const pending = s.tracks.mix(0); q.output.stateChange('connected', 'closed');
  await refuses(pending, 'externally closed port rejects pending request'); await s.close();
  ok(!s.connected, 'connected device with closed port requires fresh connection');
}
// Real parser/handlers over a line bridge, using the production MIDI transport.
class Bridge {
  constructor(path) {
    this.child = spawn(path); this.pending = []; this.stderr = '';
    this.child.stderr.on('data', (d) => { this.stderr += d; });
    createInterface({ input: this.child.stdout }).on('line', (line) => {
      const p = this.pending.shift(); assert.ok(p); const r = JSON.parse(line);
      assert.equal(r.flash_writes, 0); assert.equal(r.flash_erases, 0); p(r);
    });
    this.exit = new Promise((resolve, reject) => { this.child.on('error', reject); this.child.on('close', resolve); });
  }
  request(cmd, args) { return new Promise((resolve) => { this.pending.push(resolve); this.child.stdin.write([cmd, ...args].join(' ') + '\n'); }); }
  async close() { this.child.stdin.end(); assert.equal(await this.exit, 0, this.stderr); }
}
for (const [path, expanded] of [[process.argv[2], true], [process.argv[3], false]]) {
  assert.ok(path, 'Pass compiled eight/four-track bridge paths.'); const bridge = new Bridge(path), q = pair();
  q.output.respond = async (bytes) => { const r = await bridge.request(bytes[4], bytes.slice(5, -1)); q.input.receive(frame(r.cmd, r.args)); };
  const s = await D8MidiSession.connect(q.input, q.output);
  try {
    ok(s.tracks.editable === expanded, 'actual ' + (expanded ? 'eight' : 'four') + '-track negotiation through framed transport');
    if (expanded) {
      for (let track = 0; track < 8; track++) {
        ok((await s.tracks.select(track)).selected === track, 'actual selection track ' + (track + 1));
        const result = await s.tracks.mix(track, { level: 64 + track, mute: true });
        ok(result.level === 64 + track && result.mute, 'actual framed mixer write track ' + (track + 1));
        ok((await s.tracks.mix(track)).level === 64 + track, 'actual mixer readback track ' + (track + 1));
      }
      const result = await s.tracks.parameter(7, 5, -64);
      ok(result.value === -64 && (await s.tracks.parameter(7, 5)).value === -64, 'actual signed parameter round trip');
    } else {
      const count = q.output.sent.length; await refuses(s.tracks.mix(0), 'actual four-track edit refused locally');
      ok(q.output.sent.length === count, 'actual four-track handshake sends no track edits');
    }
  } finally { await s.close(); await bridge.close(); }
}
console.log(`MIDI session: ${checks} checks passed; no real MIDI access or hardware qualification`);
