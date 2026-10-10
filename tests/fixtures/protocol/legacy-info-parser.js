/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Reader and INFO parser extracted unchanged from audited Felucca v1.1.5
 * 276f72a4e6ea8a12499a7a6819aadf3165126755, web/editor.html. */
const CMD = {INFO:1};
class Reader {
  constructor(a) { this.a = a; this.i = 0; }
  b() { if (this.i >= this.a.length) throw new Error("short reply"); return this.a[this.i++]; }
  v() { const lo = this.b(); return v14dec(lo, this.b()); }
  s() { let s = ""; for (;;) { const c = this.b(); if (!c) return s; s += String.fromCharCode(c); } }
}

const parse = {
  [CMD.INFO](a) {
    const r = new Reader(a);
    const o = { version: r.s(), nengines: r.b(), pcount: r.b(), gcount: r.b(), nstep: r.b(), pe0: r.b(), engines: [] };
    for (let i = 0; i < o.nengines; i++) o.engines.push(r.s());
    o.chainRows = 0;
    o.ntrk = r.i < a.length ? r.b() : 0;             /* v3: tracks (0 = older firmware, one instrument) */
    o.uiCaps = 0;
    const trailer = a.slice(r.i);
    if (trailer[0] === 16) o.chainRows = 16;       /* existing v6 SONG capability */
    if (trailer.length >= 4 && trailer[0] === 16 && trailer[1] === 0x55 && trailer[2] === 1)
      o.uiCaps = trailer[3] & 15;                 /* tagged preferences; never interpret old PR bytes as SONG */
    o.motionMax = 0; o.chance = false; o.backupCaps = 0;
    if (o.uiCaps && trailer[4] === 0x4d && trailer[5] === 1 && trailer[6] === 64) {
      o.motionMax = 64; o.chance = trailer[7] === 1;
      if (trailer[8] === 0x42 && trailer[9] === 1) o.backupCaps = trailer[10] & 3;
    }
    o.fm6 = null;                                 /* FM6 patches (cmds 68..71): the factory and bank slot counts */
    o.syncCaps = 0;                               /* live sync: bit 0 WATCH while on keeps pending pushes, bit 1 no RELOAD */
    o.ratchet = 0;                                /* a step's ratchet (STEP_SET's byte after the chance), 0 = none */
    for (const p of [8, 11]) if (o.motionMax && trailer[p] === 0x46 && trailer[p + 1] === 1) {   /* for the editor's */
      o.fm6 = { factory: trailer[p + 2], bank: trailer[p + 3], caps: 0 };                         /* PRESET / G_ENGSEL */
      if (trailer[p + 4] === 0x53 && trailer[p + 5] === 1) o.syncCaps = trailer[p + 6] & 3;
      /* FM6 v2 (1.0.3): bit 0 no bank (SLOT F1..F8, 8 = OWN), bit 1 the user presets carry their patch (target 3) */
      if (trailer[p + 7] === 0x50 && trailer[p + 8] === 1) o.fm6.caps = trailer[p + 9] & 3;
      let r = trailer[p + 7] === 0x50 ? p + 10 : p + 7;                                   /* RATCH after FM6 v2 */
      if (trailer[r] === 0x4E && trailer[r + 1] === 1) r += 3;                            /* and the MENU settings (1.0.4) */
      if (o.syncCaps && trailer[r] === 0x52 && trailer[r + 1] === 1) o.ratchet = trailer[r + 2];   /* RATCH: max hits */
    }
    return o;
  },
};
