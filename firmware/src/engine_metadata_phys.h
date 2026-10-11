/* SPDX-License-Identifier: GPL-3.0-only
 * Felucca-derived metadata/state, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hügelton Instruments. Stable unavailable-engine metadata; no sounding implementation. */
/* Stable ID9, exact structural fields/presets retained; no renderer. */
#include "../../firmware/src/phys_models.h"
#define PHYS_POLY 3
static const char *const N_PHYS_MODEL[] = {"MODAL", "STRNG", "MEMB", "SYMP"};
static const preset_t PHYS_PRESETS[] = {
    /* name, {MODEL, STRC, BRIT, DAMP, POS, ACC, BOW, EXC}, {A D S R}, fenv, mono */
    {"BELL TREE", {PM_MODAL, 70, 100, 106, 12, 100, 0, 6}, {0, 100, 127, 96}, 0, 0, FX(0, 20, 30, 60), PAT(7)},
    {"MARIMBA", {PM_MODAL, 116, 38, 40, 24, 90, 0, 30}, {0, 90, 127, 52}, 0, 0, FX(0, 0, 15, 30), PAT(6)},
    {"PLUCK", {PM_STRING, 56, 72, 84, 40, 96, 0, 0}, {0, 100, 127, 58}, 0, 0, FX(0, 10, 30, 30), PAT(3)},
    {"BOWED METAL", {PM_MODAL, 98, 70, 112, 10, 80, 90, 0}, {60, 90, 127, 90}, 0, 0, FX(0, 40, 20, 70), PAT(5)},
    {"KALIMBA", {PM_MODAL, 34, 16, 56, 100, 90, 0, 48}, {0, 100, 127, 60}, 0, 0, FX(0, 10, 20, 40), PAT(7)},
    /* MEMB: {MODEL, HARM, BRIT, DAMP, POS, ACC, BEND, EXC} */
    {"HAND DRUM", {PM_MEMB, 112, 76, 56, 44, 100, 14, 24}, {0, 100, 127, 64}, 0, 0, FX(0, 0, 10, 30), PAT(6)},
    {"TOMS", {PM_MEMB, 0, 52, 40, 30, 100, 40, 30}, {0, 100, 127, 70}, 0, 0, FX(0, 0, 0, 40), PAT(2)},
    /* SYMP: {MODEL, CHRD, BRIT, DAMP, SYMP, ACC, BUZZ, EXC} */
    {"DRONE STRING", {PM_SYMP, 127, 80, 88, 64, 100, 92, 0}, {0, 100, 127, 90}, 0, 0, FX(0, 10, 20, 50), PAT(3)},
    {"HARP", {PM_SYMP, 8, 62, 74, 60, 90, 0, 0}, {0, 100, 127, 80}, 0, 0, FX(0, 20, 20, 60), PAT(3)},
};
static const engine_t ENG_PHYS = {
    .name = "PHYS",
    .page_title = {"BODY", "EXCT"},
    .edit = {
        {"MODEL", F_ENUM, 0, PM_COUNT - 1, 0, N_PHYS_MODEL, 0},
        {"STRC", F_PCT, 0, 127, 64, 0, 0},
        {"BRIT", F_PCT, 0, 127, 80, 0, 0},
        {"DAMP", F_PCT, 0, 127, 80, 0, 0},
        {"POS", F_PCT, 0, 127, 20, 0, 0},
        {"ACC", F_PCT, 0, 127, 90, 0, 0},
        {"BOW", F_PCT, 0, 127, 0, 0, 0},
        {"EXC", F_PCT, 0, 127, 0, 0, 0},
    },
    .presets = PHYS_PRESETS,
    .npresets = NELEM(PHYS_PRESETS),
    .note_on = d8_unavailable_note_on,
    .render = d8_unavailable_render,
    .knob = {P_E1, P_E2, P_E3, P_E4},
    .poly = PHYS_POLY,
    .desc = 0,
};
