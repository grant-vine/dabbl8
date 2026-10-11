/* SPDX-License-Identifier: GPL-3.0-only
 * Felucca-derived metadata/state, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hügelton Instruments. Stable unavailable-engine metadata; no sounding implementation. */
/* Stable ID7, exact structural fields/presets retained; no renderer. */

static const char *const N_DRW_REG[] = {"FLUTE", "MELLO", "HOLLW", "SMOOT", "3BAR", "BLUES", "GOSPL", "ROCK",
                                        "TOPS", "CLARI", "REED", "STRNG", "CHAPL", "BRITE", "BASS", "FULL"};
static const char *const N_DRW_PERC[] = {"OFF", "2ND", "3RD", "2SOFT", "3SOFT", "2SLOW", "3SLOW"};
static const char *const N_DRW_ROTR[] = {"OFF", "SLOW", "FAST"};
static const preset_t WHEEL_PRESETS[] = {
    /* name, {REG, SUB, BODY, TOP, PERC, CLICK, DRIVE, ROTR}, {A D S R}, fenv, mono */
    {"FULL ORGAN", {15, 0, 0, 0, 0, 30, 20, 1}, {0, 64, 127, 45}, 0, 0, FX(0, 0, 10, 35), PAT(5)},
    {"JAZZ PERC", {4, 0, 0, 0, 2, 50, 8, 1}, {0, 64, 127, 40}, 0, 0, FX(0, 0, 0, 25), PAT(3)},
    {"GOSPEL", {6, 0, 0, 0, 1, 60, 40, 2}, {0, 64, 127, 45}, 0, 0, FX(0, 0, 0, 40), PAT(6)},
    {"SOFT FLUTE", {1, 0, -2, 0, 0, 10, 0, 1}, {0, 64, 127, 55}, 0, 0, FX(0, 20, 15, 55), PAT(5)},
    {"ROCK DRIVE", {7, 0, 0, 0, 0, 70, 100, 2}, {0, 64, 127, 40}, 0, 0, FX(35, 0, 10, 25), PAT(4)},
};
static const engine_t ENG_WHEEL = {
    .name = "WHEEL",
    .page_title = {"BARS", "TONE"},
    .edit = {
        {"REG", F_ENUM, 0, 15, 4, N_DRW_REG, 0},
        {"SUB", F_INT, -8, 8, 0, 0, 0},
        {"BODY", F_INT, -8, 8, 0, 0, 0},
        {"TOP", F_INT, -8, 8, 0, 0, 0},
        {"PERC", F_ENUM, 0, 6, 0, N_DRW_PERC, 0},
        {"CLICK", F_PCT, 0, 127, 40, 0, 0},
        {"DRV", F_PCT, 0, 127, 0, 0, 0},
        {"ROTR", F_ENUM, 0, 2, 1, N_DRW_ROTR, 0},
    },
    .presets = WHEEL_PRESETS,
    .npresets = NELEM(WHEEL_PRESETS),
    .note_on = d8_unavailable_note_on,
    .render = d8_unavailable_render,
    .knob = {P_E0, P_E4, P_E6, P_E7},
    .block = 0,
};

#include "../../firmware/src/wheel_limits.h"
static const char DRW_REG[16][DRW_NP + 1] = {
    "008000000", /* FLUTE  one soft flute */
    "008400000", /* MELLO  flute and octave */
    "800800000", /* HOLLW  sub and octave, hollow */
    "838000000", /* SMOOT  smooth comping */
    "888000000", /* 3BAR   the first three bars out */
    "888800000", /* BLUES  three bars and the octave */
    "888600006", /* GOSPL  body with a little shimmer */
    "888000888", /* ROCK   body and the top three */
    "800000888", /* TOPS   sub and the top only */
    "008080800", /* CLARI  odd harmonics, reedy-hollow */
    "008765432", /* REED   a falling slope, brassy */
    "006876540", /* STRNG  bright and thin */
    "004432000", /* CHAPL  soft, churchy */
    "008888000", /* BRITE  even bars, bright */
    "858000000", /* BASS   for the left hand */
    "888888888", /* FULL   everything */
};
static int32_t drw_level(const int16_t *p, uint32_t k)
{
    int32_t d = DRW_REG[(uint32_t)p[P_E0] & 15u][k] - '0';
    d += k < 2u ? p[P_E1] : k < 4u ? p[P_E2] : p[P_E3];
    return clamp(d, 0, 8);
}
