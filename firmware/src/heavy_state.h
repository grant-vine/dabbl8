/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * State types retain their upstream engine fields and dependency notices. */
#ifndef DABBL8_HEAVY_STATE_H
#define DABBL8_HEAVY_STATE_H
/* Included by GRAIN after SAMPLE and the GR_* limits, only in expanded builds. */
#include "phys_dsp.c"
#include "phys_symp.c"
#include "drum_voice.c"
typedef struct {
    const smp_zone_t *z;
    uint32_t pos;                /* forward: the next sample to decode; reverse: the index of a */
    int32_t frac;                /* Q16 between a and b */
    uint32_t step;               /* Q16 source samples per output sample */
    uint32_t wph, winc;          /* window phase: one turn = the grain */
    uint32_t left;               /* output samples to go */
    int32_t a, b;                /* the two source samples around the read position */
    int32_t pred, idx;           /* ADPCM state (forward) */
    uint32_t rlo;                /* reverse: the source index of rb[0] */
    int32_t gain;                /* Q15 */
    uint8_t owner;               /* voice index + 1, 0 = free */
    uint8_t rev;
    uint8_t zl;                  /* its zone in the set */
} gr_grain_t;

typedef struct {
    gr_grain_t g[GR_NG];
    int16_t rb[GR_NG][GR_RB];    /* reverse windows */
    int16_t ipred[GR_NIDX];      /* seek index: the state before sample k * GR_SEG of a zone */
    uint8_t iidx[GR_NIDX];
    uint16_t zbase[GR_MAXZ], zcnt[GR_MAXZ], zdone[GR_MAXZ];   /* entries of a zone: first, all, built */
    uint32_t stamp;              /* what the index was built for (user slots: smp_user_gen) */
    int32_t rng;
    uint8_t src;                 /* SRC + 1 the index holds, 0 = none */
    uint8_t nz;
} gr_part_t;
typedef struct {
    uint8_t model;               /* PM_* the state belongs to */
    union {
        px_modal_t m;            /* MODAL, MEMB */
        px_string_t s;
        px_symp_t y;
    } u;
} phys_slot_t;

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

/* A typed union preserves alignment and avoids byte-buffer aliasing. */
typedef union {
    drw_vc_t wheel;
    gr_part_t grain;
    phys_slot_t phys;
    drum_lane_t drum[DV_NLANE];
} heavy_state_t;
static heavy_state_t heavy_pool[NVOICE] __attribute__((section(".pool")));
static uint16_t heavy_owner[NVOICE]; /* engine/track/voice key + 1, zero = free */

static int heavy_live(uint32_t engine, uint32_t part, uint32_t voice)
{
    uint32_t i;
    if (part >= NPART || voice >= NVOICE || trk[part].engine != engine)
        return 0;
    if (engine == 7u || engine == 9u)                                  /* WHEEL/PHYS: one body per sounding voice */
        return trk[part].v[voice].active != 0;
    if (engine != 8u && engine != 10u)
        return 0;
    for (i = 0; i < NVOICE; i++)                        /* GRAIN/DRUM: one complete state per active track */
        if (trk[part].v[i].active)
            return 1;
    return 0;
}
static heavy_state_t *heavy_get(uint32_t engine, uint32_t part, uint32_t voice)
{
    uint32_t key, k;
    if (engine < 7u || engine > 10u || (engine != 7u && engine != 9u && voice != 0u) || !heavy_live(engine, part, voice))
        return 0;                                     /* idle block callbacks must not reserve memory */
    key = ((engine - 7u) * NPART + part) * NVOICE + voice + 1u;
    for (k = 0; k < NVOICE; k++)
        if (heavy_owner[k] == key)
            return &heavy_pool[k];                     /* retrigger retains the active engine's body */
    for (k = 0; k < NVOICE; k++) {
        uint32_t old = heavy_owner[k], index = old ? old - 1u : 0;
        if (!old || !heavy_live(7u + index / (NPART * NVOICE), (index / NVOICE) % NPART, index % NVOICE)) {
            heavy_owner[k] = (uint16_t)key;
            if (engine == 7u) memset(&heavy_pool[k].wheel, 0, sizeof heavy_pool[k].wheel);
            else if (engine == 8u) memset(&heavy_pool[k].grain, 0, sizeof heavy_pool[k].grain);
            else if (engine == 9u) memset(&heavy_pool[k].phys, 0, sizeof heavy_pool[k].phys);
            else memset(heavy_pool[k].drum, 0, sizeof heavy_pool[k].drum);
            return &heavy_pool[k];                     /* another engine cannot inherit stale state */
        }
    }
    return 0;                                         /* no admitted slot: fail closed */
}
/* WHEEL is compiled before the shared types; its typed declaration lives there. */
static drw_vc_t *wheel_state_of(track_t *t, voice_t *v)
{
    uint32_t i;
    heavy_state_t *state;
    if (t < &trk[0] || t >= &trk[NPART]) return 0;
    i = (uint32_t)(v - t->v);
    if (i >= NVOICE) return 0;
    state = heavy_get(7u, (uint32_t)(t - trk), i);
    return state ? &state->wheel : 0;
}
#endif
