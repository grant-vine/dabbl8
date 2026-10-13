// SPDX-License-Identifier: GPL-3.0-only
// Real emulator exports and browser sector persistence, without DOM/audio mocks.
import fs from "node:fs";
import { storeSectors, restoreSectors } from "./worklet.js";
export const B = { EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, SEQ: 9, PLAY: 10, REC: 11, OCTDN: 12, OCTUP: 13 };
export const EN = { SELECT: 0, ALGO: 1, PRESETS: 2, K1: 3, K2: 4, K3: 5, K4: 6 };
export async function device(bytes, saved) {
  const { instance } = await WebAssembly.instantiate(bytes, {});
  const ex = instance.exports;
  ex._initialize();
  ex.web_nor_erase();
  restoreSectors(new Uint8Array(ex.memory.buffer, ex.web_nor(), ex.web_nor_size()), saved);
  ex.web_boot();
  const d = { ex, peak: 0, maxVoices: 0, activeTracks: 0, allActive: false, samples: [] };
  d.nor = () => new Uint8Array(ex.memory.buffer, ex.web_nor(), ex.web_nor_size());
  d.sectors = () => {
    const kept = JSON.parse(JSON.stringify(Object.fromEntries(Object.entries(storeSectors(d.nor())).map(
      ([k, v]) => [k, Buffer.from(v).toString("base64")]))));
    return Object.fromEntries(Object.entries(kept).map(([k, v]) => [k, new Uint8Array(Buffer.from(v, "base64"))]));
  };
  d.render = (ms, capture = false) => {
    const frames = Math.round(ms * 44.1);
    for (let k = 0; k < frames; k += 128) {
      const n = Math.min(128, frames - k);
      ex.web_render(n);
      const l = new Float32Array(ex.memory.buffer, ex.web_out_l(), n);
      const r = new Float32Array(ex.memory.buffer, ex.web_out_r(), n);
      d.maxVoices = Math.max(d.maxVoices, ex.web_qa_active_voices());
      d.activeTracks |= ex.web_qa_active_tracks();
      d.allActive ||= ex.web_qa_active_tracks() === 255;
      for (let i = 0; i < n; i++) {
        d.peak = Math.max(d.peak, Math.abs(l[i]), Math.abs(r[i]));
        if (capture) d.samples.push(l[i], r[i]);
      }
    }
  };
  d.tap = (button) => { ex.web_buttons(1 << button); d.render(60); ex.web_buttons(0); d.render(60); };
  d.key = (key) => { ex.web_keys(1 << key); d.render(60); ex.web_keys(0); d.render(60); };
  d.turn = (role, steps) => {
    for (let i = 0; i < Math.abs(steps); i++) { ex.web_enc(role, Math.sign(steps)); d.render(40); }
  };
  d.text = (ptr) => {
    const value = new Uint8Array(ex.memory.buffer, ptr, 13);
    return Buffer.from(value.subarray(0, value.indexOf(0) < 0 ? 13 : value.indexOf(0))).toString();
  };
  d.signatures = () => Array.from({ length: 8 }, (_, p) => ex.web_qa_track_signature(p));
  d.select = (p) => { d.turn(EN.ALGO, p - ex.web_qa_selected()); d.render(300); };
  d.settle = () => {
    for (let ms = 0; ms <= 20000; ms += 250) {
      if (ex.web_qa_quiet()) return ms;
      d.render(250);
    }
    throw new Error(`Stopped DSP did not become write-quiet within 20 s (flags ${ex.web_qa_quiet_flags()})`);
  };
  d.screen = (path) => fs.writeFileSync(path, new Uint8Array(ex.memory.buffer, ex.web_screen(), 240 * 240 * 2));
  d.render(1000);
  return d;
}
