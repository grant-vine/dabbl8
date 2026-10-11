/* SPDX-License-Identifier: GPL-3.0-only
 * Felucca-derived metadata/state, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hügelton Instruments. Stable unavailable-engine metadata; no sounding implementation. */
#include "../../firmware/src/grain_limits.h"
/* Stable ID8, exact structural fields/presets retained; no renderer. */

static const preset_t GRAIN_PRESETS[] = {
    /* name, {SRC, POS, SIZE, DENS, PTCH, SPRD, RAND, TONE}, {A D S R}, fenv, mono */
    {"CLOUD PAD", {2, 50, 92, 88, 0, 40, 14, 100}, {70, 90, 120, 90}, 0, 0, FX(0, 40, 25, 85), PAT(5)},
    {"GLITCH", {3, 64, 24, 112, 0, 100, 90, 127}, {0, 70, 100, 30}, 0, 0, FX(10, 0, 50, 25), PAT(4)},
    {"FROZEN", {0, 40, 108, 72, 0, 0, 10, 92}, {50, 100, 127, 100}, 0, 0, FX(0, 30, 20, 95), PAT(5)},
    {"SHIMMER", {0, 30, 70, 100, 12, 30, 24, 110}, {30, 90, 110, 90}, 0, 0, FX(0, 50, 40, 90), PAT(7)},
};
static const engine_t ENG_GRAIN = {
    .name = "GRAIN",
    .page_title = {"GRAN", "SPRY"},
    .edit = {
        {"SRC", F_ENUM, 0, SMP_NALL - 1, 0, SMP_ALL_NAMES, 0},
        {"POS", F_PCT, 0, 127, 32, 0, 0},
        {"SIZE", F_PCT, 0, 127, 80, 0, 0},
        {"DENS", F_PCT, 0, 127, 80, 0, 0},
        {"PTCH", F_SEMI, -24, 24, 0, 0, 0},
        {"SPRD", F_PCT, 0, 127, 30, 0, 0},
        {"RAND", F_PCT, 0, 127, 10, 0, 0},
        {"TONE", F_PCT, 0, 127, 127, 0, 0},
    },
    .presets = GRAIN_PRESETS,
    .npresets = NELEM(GRAIN_PRESETS),
    .note_on = d8_unavailable_note_on,
    .render = d8_unavailable_render,
    .knob = {P_E1, P_E2, P_E3, P_E5},
    .poly = GR_POLY,
    .block = 0,
};
