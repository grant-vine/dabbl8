/* SPDX-License-Identifier: GPL-3.0-only
 * Felucca-derived metadata/state, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hügelton Instruments. Stable unavailable-engine metadata; no sounding implementation. */
/* Stable ID2, exact structural fields/presets retained; no renderer. */

static const char *const N_PD_WAVE[] = {"SAW", "SQR", "PLS", "DSIN", "SPLS", "RSAW", "RTRI", "RTRP"};
static const char *const N_PD_WAVE2[] = {"-", "SAW", "SQR", "PLS", "DSIN", "SPLS", "RSAW", "RTRI", "RTRP"};
static const char *const N_PD_LINE[] = {"MIX", "RING"};
static const preset_t PHASE_PRESETS[] = {
    /* name, {WAVE, WAVE2, DCW, ENV, DTN, LINE, SUB, -}, {A D S R}, fenv, mono */
    {"BRASS", {0, 0, 30, 90, 0, 0, 0, 0}, {8, 70, 90, 40}, 0, 0, FX(0, 20, 20, 40), PAT(6)},
    {"ORGAN", {3, 0, 40, 0, 0, 0, 0, 0}, {0, 127, 127, 30}, 0, 0, FX(0, 40, 0, 30), PAT(6)},
    {"STRING", {0, 4, 50, 40, 12, 0, 0, 0}, {40, 90, 100, 70}, 0, 0, FX(0, 50, 20, 60), PAT(5)},
    {"RESO", {5, 0, 60, 60, 0, 0, 0, 0}, {0, 70, 30, 60}, 0, 0, FX(0, 0, 40, 40), PAT(1)},
    {"BELL", {6, 0, 80, 50, 0, 0, 0, 0}, {0, 95, 0, 90}, 0, 0, FX(0, 0, 30, 70), PAT(7)},
    {"WIRE", {4, 7, 70, 40, 7, 0, 0, 0}, {10, 80, 80, 60}, 0, 0, FX(15, 30, 30, 40), PAT(4)},
};
static const engine_t ENG_PHASE = {
    .name = "PHASE",
    .page_title = {"PHS", "LINE"},
    .edit = {
        {"WAVE", F_ENUM, 0, 7, 0, N_PD_WAVE, 0},
        {"WAVE2", F_ENUM, 0, 8, 0, N_PD_WAVE2, 0},
        {"DCW", F_PCT, 0, 127, 60, 0, 0},
        {"ENV", F_PCT, 0, 127, 64, 0, 0},
        {"DTN", F_INT, 0, 127, 0, 0, "ct"},
        {"LINE", F_ENUM, 0, 1, 0, N_PD_LINE, 0},
        {"SUB", F_PCT, 0, 127, 0, 0, 0},
        {"-", F_INT, 0, 0, 0, 0, 0},
    },
    .presets = PHASE_PRESETS,
    .npresets = NELEM(PHASE_PRESETS),
    .note_on = d8_unavailable_note_on,
    .render = d8_unavailable_render,
    .knob = {P_E2, P_E3, P_E4, P_REL},
};
