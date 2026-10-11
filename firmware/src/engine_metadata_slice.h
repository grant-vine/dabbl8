/* SPDX-License-Identifier: GPL-3.0-only
 * Felucca-derived metadata/state, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hügelton Instruments. Stable unavailable-engine metadata; no sounding implementation. */
enum { SLC_ONE, SLC_GATE, SLC_LOOP };
/* Stable ID13, exact structural fields/presets retained; no renderer. */
#define SLC_NSRC 5
static const char *const N_SLC_SRC[] = {"BREAK", "USR1", "USR2", "USR3", "PIANO"};
static const char *const N_SLC_DIV[] = {"4", "8", "16", "32", "AUTO", "MAN"};
static const char *const N_SLC_MODE[] = {"ONE", "GATE", "LOOP"};
static const char *const N_SLC_REV[] = {"OFF", "ON"};
static const preset_t SLICE_PRESETS[] = {
    {"CHOP", {0, 2, 0, 0, SLC_ONE, 0, 127, 127}, {0, 127, 127, 30}, 0, 0, FX(0, 0, 0, 12), PAT(9)},
    {"STUTTER", {0, 1, 0, 0, SLC_GATE, 0, 90, 110}, {0, 127, 127, 12}, 0, 0, FX(10, 0, 20, 10), PAT(10)},
};
static const engine_t ENG_SLICE = {
    .name = "SLICE",
    .page_title = {"SLCE", "PLAY"},
    .edit = {
        {"SRC", F_ENUM, 0, SLC_NSRC - 1, 0, N_SLC_SRC, 0},
        {"DIV", F_ENUM, 0, 5, 2, N_SLC_DIV, 0},
        {"START", F_INT, 0, 31, 0, 0, 0},
        {"PTCH", F_SEMI, -24, 24, 0, 0, 0},
        {"MODE", F_ENUM, 0, 2, 0, N_SLC_MODE, 0},
        {"REV", F_ENUM, 0, 1, 0, N_SLC_REV, 0},
        {"DCAY", F_TIME, 0, 127, 127, 0, 0},
        {"TONE", F_INT, 0, 127, 127, 0, 0},
    },
    .presets = SLICE_PRESETS,
    .npresets = NELEM(SLICE_PRESETS),
    .note_on = d8_unavailable_note_on,
    .render = d8_unavailable_render,
    .knob = {P_E0, P_E1, P_E3, P_E6},
    .sampled = 1,
    .keys = 0,
    .amp = 0,
};
