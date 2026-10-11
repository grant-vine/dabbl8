/* SPDX-License-Identifier: GPL-3.0-only
 * Felucca-derived metadata/state, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hügelton Instruments. Stable unavailable-engine metadata; no sounding implementation. */
enum { NZ_ANLG, NZ_DUST, NZ_LFSR, NZ_META };
/* Stable ID11, exact structural fields/presets retained; no renderer. */

static const char *const N_NOISE_MODE[] = {"ANLG", "DUST", "LFSR", "META"};
static const preset_t NOISE_PRESETS[] = {
    {"WIND", {NZ_ANLG, 64, 70, 100, 127, 0, 70, 0}, {90, 90, 110, 90}, 0, 0, FX(0, 30, 20, 80), PAT(5)},
    {"RAIN", {NZ_DUST, 30, 88, 80, 64, 105, 40, 0}, {10, 90, 127, 70}, 0, 0, FX(0, 0, 20, 70), PAT(5)},
    /* a 7-bit register (127 steps) clocked 84 semitones (7 octaves) above the key: one period per key period,
     * a pitched chip-style buzz */
    {"ARCADE", {NZ_LFSR, 0, 127, 0, 127, 112, 0, 84}, {0, 80, 0, 50}, 0, 0, FX(0, 0, 10, 20), PAT(3)},
    {"METAL", {NZ_META, 0, 100, 40, 100, 32, 0, 0}, {0, 75, 40, 60}, 10, 0, FX(0, 20, 30, 45), PAT(7)},
};
static const engine_t ENG_NOISE = {
    .name = "NOISE",
    .page_title = {"SRC", "MOVE"},
    .edit = {
        {"MODE", F_ENUM, 0, 3, 0, N_NOISE_MODE, 0},
        {"COLR", F_PCT, 0, 127, 64, 0, 0},
        {"FREQ", F_CUTOFF, 0, 127, 80, 0, 0},
        {"RES", F_PCT, 0, 127, 30, 0, 0},
        {"TRK", F_PCT, 0, 127, 64, 0, 0},
        {"DENS", F_PCT, 0, 127, 0, 0, 0},           /* LFSR, META: the register (noise_desc) */
        {"DRFT", F_PCT, 0, 127, 0, 0, 0},
        {"CRSH", F_PCT, 0, 127, 0, 0, 0},           /* LFSR, META: the clock above the key */
    },
    .presets = NOISE_PRESETS,
    .npresets = NELEM(NOISE_PRESETS),
    .note_on = d8_unavailable_note_on,
    .render = d8_unavailable_render,
    .knob = {P_E2, P_E3, P_E1, P_REL},
    .keep = 0xF3,                /* the filter, the colour poles */
    .desc = 0,
};

static const param_desc_t NOISE_CLK = {"CLK", F_INT, 0, 127, 0, 0, "st"};           /* = edit[7]: above the key */
