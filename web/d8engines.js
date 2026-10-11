// SPDX-License-Identifier: GPL-3.0-only
// Separate, explicitly requested read-only engine observation. No write API.
import { parseTrackInfo } from "./d8tracks.js";
function fail(s) { throw Error(s); }
function wire(a) {
  if (!Array.isArray(a)) fail("Invalid seven-bit capability reply");
  for (const v of a) if (!Number.isInteger(v) || v < 0 || v > 127) fail("Invalid seven-bit capability reply");
  return a;
}
export function parseEngineAvailability(a) {
  wire(a);
  if (a.length !== 8 || a[0] !== 68 || a[1] !== 56 || a[2] !== 2 || a[3] !== 1 ||
      ![13, 14].includes(a[4]) || a[7] !== 0) fail("Unsupported engine availability schema");
  const mask = a[5] | (a[6] << 7), slots = a[4];
  if (!mask || mask >>> slots) fail("Invalid engine availability mask");
  return Object.freeze({ schema: 2, engineSchema: 1, slots, mask,
    ids: Object.freeze(Array.from({ length: slots }, (_, i) => i).filter(i => mask & (1 << i))) });
}
export function validateEngineHandshake(info, reply) {
  const a = wire(reply);
  if (info.discovery !== 1) fail("Unsupported Dabbl8 discovery");
  if (a.length !== 15 || a[0] !== 68 || a[1] !== 56 || a[2] !== 1 || a[3] !== info.tracks ||
      a[3] < 1 || a[3] > 8 || a[4] !== 64 || a[5] !== 8 || a[6] !== 64 || a[7] !== 99 ||
      a[8] !== 27 || ![13,14].includes(a[9]) || a[10] !== 1 || a[11] !== 1 || a[12] !== 1 || a[13] !== 9 ||
      a[4] !== info.steps || a[7] !== info.params || a[8] !== info.globals || a[9] !== info.engines || info.engineParam !== 91)
    fail("INFO/schema-1 mismatch");
}
export class D8Engines {
  #request; #open = true; #caps = null; #reason = "Not negotiated";
  constructor(request) { if (typeof request !== "function") fail("Missing transport"); this.#request = request; }
  static async connect(request) {
    const c = new D8Engines(request);
    try {
      const info = parseTrackInfo(wire(await c.#ask([], 1)));
      if (info.discovery !== 1) fail("Unsupported Dabbl8 discovery");
      validateEngineHandshake(info, await c.#ask([]));
      const caps = parseEngineAvailability(await c.#ask([2]));
      if (caps.slots !== info.engines) fail("Engine slot count mismatch");
      c.#caps = caps; c.#reason = "";
    } catch (e) { c.#reason = e.message; }
    return c;
  }
  async #ask(args, cmd = 75) {
    if (!this.#open) fail("Disconnected");
    const reply = await this.#request([cmd, args]);
    if (!this.#open) fail("Disconnected");
    return reply;
  }
  get available() { return this.#open && this.#caps !== null; }
  get caps() { return this.available ? this.#caps : null; }
  get reason() { return this.#open ? this.#reason : "Disconnected"; }
  has(id) {
    if (!Number.isInteger(id) || id < 0 || id >= (this.#caps?.slots ?? 14)) fail("Invalid stable engine ID");
    return this.available && !!(this.#caps.mask & (1 << id));
  }
  close() { this.#open = false; this.#caps = null; }
}
