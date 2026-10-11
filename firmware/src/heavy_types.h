/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
#ifndef DABBL8_HEAVY_TYPES_H
#define DABBL8_HEAVY_TYPES_H
/* Typed shared pool layout; requires the PHYS/DRUM/FM6 support types, independent of engine renderers. */
#if D8BUILD_POOL_HAS(7)
#include "wheel_state.h"
#endif
#if D8BUILD_POOL_HAS(8)
#include "grain_state.h"
#endif
#if D8BUILD_POOL_HAS(9)
typedef struct {
    uint8_t model;               /* PM_* the state belongs to */
    union {
        px_modal_t m;            /* MODAL, MEMB */
        px_string_t s;
        px_symp_t y;
    } u;
} phys_slot_t;
#endif

#if D8BUILD_POOL_HAS(10)
typedef struct {
    dv_param_t key;              /* the parameters the coefficients were set up with */
    dv_coef_t c;
    dv_voice_t v;
    dv_metal_t mb;
    uint8_t owner;               /* the voice playing the lane: index + 1, 0 = none */
    uint8_t role;                /* the drum struck (DVT_*) */
    int8_t st;                   /* its semitones from the designed pitch (the GM map) */
    uint8_t pad;
} drum_lane_t;
#endif

/* SLICE is included later; keep its unchanged window size checked there. */
#define HEAVY_SLC_RB 64u
/* A typed union preserves alignment and avoids byte-buffer aliasing. */
typedef union {
#if D8BUILD_POOL_HAS(7)
    drw_vc_t wheel;
#endif
#if D8BUILD_POOL_HAS(12)
    fm6_note_t fm6;
#endif
#if D8BUILD_POOL_HAS(13)
    int16_t slice[HEAVY_SLC_RB];
#endif
#if D8BUILD_POOL_HAS(8)
    gr_part_t grain;
#endif
#if D8BUILD_POOL_HAS(9)
    phys_slot_t phys;
#endif
#if D8BUILD_POOL_HAS(10)
    drum_lane_t drum[DV_NLANE];
#endif
} heavy_state_t;
#endif
