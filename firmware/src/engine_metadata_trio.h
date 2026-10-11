/* SPDX-License-Identifier: GPL-3.0-only
 * Felucca-derived metadata/state, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hügelton Instruments. Stable unavailable-engine metadata; no sounding implementation. */
/* Stable ID6, exact structural fields/presets retained; no renderer. */

static const char *const N_TRIO_WAVE[] = {"SAW3", "PLS3", "PPT", "SST", "TRI3", "S&T", "P&S", "P+N",
                                          "NOIS", "SYNC", "SYNCP", "SYNC3", "RING", "RING3", "R+S", "R+SP"};
static const char *const N_TRIO_MODE[] = {"LP", "BP", "HP", "NOT"};
static const preset_t TRIO_PRESETS[] = {
    /* name, {WAVE, INT2, INT3, DTN, MODE, CUT, RES, PW}, {A D S R}, fenv, mono */
    {"FAT BASS", {0, 0, -12, 9, 0, 62, 45, 64}, {0, 62, 60, 25}, 40, 1, FX(10, 0, 10, 10), PAT(2)},
    {"ARP LEAD", {2, 12, 0, 4, 0, 88, 25, 32}, {0, 60, 90, 30}, 8, 0, FX(0, 0, 35, 20), PAT(13)},
    {"SYNC LEAD", {9, 9, 0, 0, 0, 82, 30, 64}, {2, 70, 100, 40}, 18, 1, FX(0, 10, 40, 25), PAT(4)},
    {"RING BELL", {13, 0, 18, 6, 1, 96, 30, 64}, {0, 92, 0, 80}, 0, 0, FX(0, 20, 35, 55), PAT(7)},
    {"CHIP CHOIR", {1, 0, 12, 7, 1, 62, 95, 40}, {70, 90, 110, 85}, 30, 0, FX(0, 50, 20, 65), PAT(5)},
};
static const engine_t ENG_TRIO = {
    .name = "TRIO",
    .page_title = {"OSC", "TONE"},
    .edit = {
        {"WAVE", F_ENUM, 0, 15, 0, N_TRIO_WAVE, 0},
        {"INT2", F_SEMI, -24, 24, 0, 0, 0},
        {"INT3", F_SEMI, -24, 24, -12, 0, 0},
        {"DTN", F_INT, 0, 50, 6, 0, "ct"},
        {"MODE", F_ENUM, 0, 3, 0, N_TRIO_MODE, 0},
        {"CUT", F_CUTOFF, 0, 127, 80, 0, 0},
        {"RES", F_PCT, 0, 127, 40, 0, 0},
        {"PW", F_PCT, 0, 127, 64, 0, 0},
    },
    .presets = TRIO_PRESETS,
    .npresets = NELEM(TRIO_PRESETS),
    .note_on = d8_unavailable_note_on,
    .render = d8_unavailable_render,
    .knob = {P_E5, P_E6, P_E7, P_E1},
};
