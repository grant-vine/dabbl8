/* SPDX-License-Identifier: GPL-3.0-only
 * Felucca-derived metadata/state, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hügelton Instruments. Stable unavailable-engine metadata; no sounding implementation. */
/* Stable ID5, exact structural fields/presets retained; no renderer. */

static const preset_t FORMANT_PRESETS[] = {
    /* VOWEL VOWL2 TALK SHIFT | BUZZ BREATH Q RAND */
    {"CHOIR AAH", {0, 0, 0, 0, 40, 22, 60, 0}, {85, 90, 115, 95}, 0, 0, FX(0, 70, 15, 90), PAT(5)},
    {"VOX LEAD", {32, 0, 0, 2, 90, 8, 72, 0}, {6, 70, 105, 55}, 0, 1, FX(0, 20, 45, 40), PAT(4)},
    {"WOW BASS", {95, 0, 68, 0, 100, 0, 80, 0}, {0, 70, 70, 30}, 0, 1, FX(10, 0, 10, 10), PAT(8)},
    {"WHISPER", {0, 95, 88, 3, 50, 120, 50, 0}, {50, 90, 110, 90}, 0, 0, FX(0, 40, 30, 70), PAT(5)},
};
static const engine_t ENG_FORMANT = {
    .name = "VOICE",
    .page_title = {"VOWL", "TONE"},
    .edit = {
        {"VOWL", F_INT, 0, 127, 0, 0, 0},
        {"VOWL2", F_INT, 0, 127, 64, 0, 0},
        {"TALK", F_TIME, 0, 127, 0, 0, 0},
        {"SHIFT", F_SEMI, -12, 12, 0, 0, 0},
        {"BUZZ", F_PCT, 0, 127, 64, 0, 0},
        {"BRTH", F_PCT, 0, 127, 10, 0, 0},
        {"Q", F_PCT, 0, 127, 64, 0, 0},
        {"RAND", F_PCT, 0, 127, 0, 0, 0},
    },
    .presets = FORMANT_PRESETS,
    .npresets = NELEM(FORMANT_PRESETS),
    .note_on = d8_unavailable_note_on,
    .render = d8_unavailable_render,
    .knob = {P_E0, P_E4, P_E5, P_E2},
    .poly = 4,
};
