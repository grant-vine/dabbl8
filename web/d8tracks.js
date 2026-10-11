// SPDX-License-Identifier: GPL-3.0-only
// Dabbl8 companion's bounded in-memory track client. No project/flash operations.
import { d8InfoDiscovery } from "./d8info.js";
const CMD = Object.freeze({ INFO: 1, CAPS: 75, TRACK: 77 });
export const TRACK_OP = Object.freeze({ SELECT: 27, MIX: 28, DUMP: 29, STEP: 30, PARAM: 31 });
const fail = (message) => { throw new Error(message); };
const integer = (x, lo, hi, name) => Number.isInteger(x) && x >= lo && x <= hi ? x : fail(`Invalid ${name}`);
function wire(a) {
  if (!Array.isArray(a) || a.length > 594 || a.some((v) => !Number.isInteger(v) || v < 0 || v > 127)) fail("Invalid seven-bit reply");
  return [...a];
}
const v14 = (value) => { const n = integer(value, -8192, 8191, "value") + 8192; return [n & 127, n >> 7]; };
const valueOf = (a, i) => (a[i] | a[i + 1] << 7) - 8192;
export function parseTrackInfo(a) {
  wire(a); let i = 0;
  const byte = () => i < a.length ? a[i++] : fail("Short INFO reply");
  const string = () => { let s = "", v; while ((v = byte()) !== 0) s += String.fromCharCode(v); return s; };
  const version = string(), engines = byte(), params = byte(), globals = byte(), steps = byte(), engineParam = byte();
  const names = []; for (let n = 0; n < engines; n++) names.push(string());
  const tracks = i < a.length ? byte() : 0;
  return Object.freeze({ version, engines, params, globals, steps, engineParam, names: Object.freeze(names), tracks, discovery: d8InfoDiscovery(a.slice(i)) });
}
export function parseTrackCaps(a) {
  wire(a);
  if (a.length !== 15 || a[0] !== 68 || a[1] !== 56 || a[2] !== 1) fail("Unsupported capability schema");
  integer(a[3], 1, 8, "track count");
  if (a[4] !== 64 || a[5] !== 8 || a[6] !== 64 || a[7] !== 99 || a[8] !== 27 || a[9] !== 14 ||
      a[10] !== 1 || a[11] !== 1 || a[12] !== 1 || a[13] !== 9) fail("Unsupported firmware schema");
  return Object.freeze({ tracks: a[3], steps: a[4], voices: a[5], motion: a[6], params: a[7], globals: a[8], engines: a[9], features: a[14] });
}
export class TrackProtocolError extends Error {
  constructor(reason) {
    integer(reason, 1, 4, "protocol error");
    super(reason === 4 ? "Song playback is busy; this edit was refused." : `Track request refused (${reason}).`);
    this.name = "TrackProtocolError"; this.reason = reason;
  }
}
export class D8Tracks {
  #request; #open = true; #tail = Promise.resolve(); #editable = false; #info; #caps = null; #reason = "Not connected";
  constructor(request) { if (typeof request !== "function") fail("Missing request transport"); this.#request = request; }
  static async connect(request) {
    const client = new D8Tracks(request);
    client.#info = parseTrackInfo(await client.#ask(CMD.INFO, []));
    if (client.#info.discovery !== 1) { client.#reason = "This firmware has no supported Dabbl8 handshake."; return client; }
    try {
      const caps = parseTrackCaps(await client.#ask(CMD.CAPS, [])), info = client.#info;
      if (caps.tracks !== info.tracks || caps.steps !== info.steps || caps.params !== info.params ||
          caps.globals !== info.globals || caps.engines !== info.engines || info.engineParam !== 91) fail("INFO/capability mismatch");
      client.#caps = caps;
      if (caps.tracks <= 4 || (caps.features & 5) !== 5 || (caps.features & 2)) {
        client.#reason = "This firmware does not advertise expanded track editing."; return client;
      }
      client.#editable = true; client.#reason = "";
    } catch (e) { client.#reason = e.message; }
    return client;
  }
  get info() { return this.#info; }
  get caps() { return this.#caps; }
  get editable() { return this.#open && this.#editable; }
  get reason() { return this.#open ? this.#reason : "Disconnected"; }
  close() { this.#open = false; this.#editable = false; }
  async #ask(cmd, args) {
    if (!this.#open) fail("Disconnected");
    const reply = await this.#request([cmd, [...args]]);
    if (!this.#open) fail("Disconnected"); // do not apply stale replies after close
    return wire(reply);
  }
  #transaction(operation) {
    this.#ready();
    const job = this.#tail.then(async () => {
      this.#ready();
      try { return await operation(); }
      catch (e) {
        if (!(e instanceof TrackProtocolError && e.reason === 4)) { this.#editable = false; this.#reason = e.message; }
        throw e;
      }
    });
    this.#tail = job.catch(() => {}); return job;
  }
  #ready() { if (!this.editable) fail(this.reason || "Track editing is unavailable"); }
  #track(track) { this.#ready(); return integer(track, 0, this.#caps.tracks - 1, "track"); }
  #step(step) { return integer(step, 0, this.#caps.steps - 1, "step"); }
  async #envelope(op, args) {
    this.#ready(); const a = await this.#ask(CMD.TRACK, [1, op, ...args]);
    if (a.length < 2 || a[0] !== 1 || a[1] !== op) fail("Mismatched track reply schema/operation");
    return a.slice(2);
  }
  #metadata(a) {
    if (a.length !== 2 + this.#caps.tracks * 6 || a[1] !== this.#caps.tracks) fail("Invalid track metadata count");
    integer(a[0], 0, this.#caps.tracks - 1, "selected track");
    const tracks = Array.from({ length: a[1] }, (_, track) => {
      const i = 2 + track * 6;
      return { track, engine: integer(a[i], 0, this.#caps.engines - 1, "engine"), preset: a[i + 1],
        level: integer(valueOf(a, i + 2), 0, 127, "level"), mute: Boolean(integer(a[i + 4], 0, 1, "mute")),
        armed: Boolean(integer(a[i + 5], 0, 1, "armed")) };
    });
    return { selected: a[0], tracks };
  }
  async readTracks() { return this.#transaction(async () => this.#metadata(await this.#envelope(TRACK_OP.SELECT, []))); }
  async select(track) {
    this.#track(track); return this.#transaction(async () => {
      const result = this.#metadata(await this.#envelope(TRACK_OP.SELECT, [track]));
      if (result.selected !== track) fail("Selection acknowledgment targets another track"); return result;
    });
  }
  async mix(track, update) {
    this.#track(track); const args = [track];
    if (update !== undefined) { integer(update.level, 0, 127, "level"); if (typeof update.mute !== "boolean") fail("Invalid mute"); args.push(...v14(update.level), Number(update.mute)); }
    return this.#transaction(async () => {
      const a = await this.#envelope(TRACK_OP.MIX, args);
      if (a.length !== 4 || a[0] !== track) fail("Invalid mixer acknowledgment");
      return { track, level: integer(valueOf(a, 1), 0, 127, "level"), mute: Boolean(integer(a[3], 0, 1, "mute")) };
    });
  }
  async parameter(track, id, value) {
    this.#track(track); integer(id, 0, this.#caps.params - 1, "parameter");
    const args = [track, id, ...(value === undefined ? [] : v14(value))];
    return this.#transaction(async () => {
      const a = await this.#envelope(TRACK_OP.PARAM, args);
      if (a.length !== 4 || a[0] !== track || a[1] !== id) fail("Invalid parameter acknowledgment");
      return { track, id, value: valueOf(a, 2) };
    });
  }
  async dump(track) {
    this.#track(track); return this.#transaction(async () => {
      const a = await this.#envelope(TRACK_OP.DUMP, [track]);
      if (a.length !== 3 + this.#caps.params * 2 || a[0] !== track) fail("Invalid parameter dump");
      integer(a[1], 0, this.#caps.engines - 1, "engine");
      return { track, engine: a[1], preset: a[2], params: Array.from({ length: this.#caps.params }, (_, i) => valueOf(a, 3 + i * 2)) };
    });
  }
  async step(track, index, update) {
    this.#track(track); this.#step(index); const args = [track, index];
    if (update !== undefined) {
      const s = update; integer(s.n, 0, 4, "note count");
      if (!Array.isArray(s.notes) || s.notes.length !== 4) fail("Invalid notes"); s.notes.forEach((v) => integer(v, 0, 127, "note"));
      integer(s.time, 0, 2, "step type"); integer(s.flags, 0, 3, "step flags"); integer(s.vel, 0, 127, "velocity");
      integer(s.hit, 0, 255, "drum hits"); integer(s.acc, 0, 255, "drum accents"); if (s.acc & ~s.hit) fail("Accent without drum hit");
      integer(s.chance, 0, 100, "chance"); integer(s.ratchet, 1, 4, "ratchet");
      args.push(s.n, ...s.notes, s.time, s.flags, s.vel, s.hit & 127, s.acc & 127, (s.hit >> 7) | (s.acc >> 7) << 1, s.chance, s.ratchet);
    }
    return this.#transaction(async () => {
      const a = await this.#envelope(TRACK_OP.STEP, args);
      if (a.length !== 15 || a[0] !== track || a[1] !== index) fail("Invalid step acknowledgment");
      const s = { track, index, n: integer(a[2], 0, 4, "note count"), notes: a.slice(3, 7), time: integer(a[7], 0, 2, "step type"),
      flags: integer(a[8], 0, 3, "step flags"), vel: a[9], hit: a[10] | (a[12] & 1) << 7,
      acc: a[11] | (a[12] & 2) << 6, chance: integer(a[13], 0, 100, "chance"), ratchet: integer(a[14], 1, 4, "ratchet") };
      integer(a[12], 0, 3, "drum high bits"); if (s.acc & ~s.hit) fail("Invalid drum accents"); return s;
    });
  }
}
