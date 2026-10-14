/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * State types retain their upstream engine fields and dependency notices. */
#ifndef DABBL8_HEAVY_STATE_H
#define DABBL8_HEAVY_STATE_H
/* Expanded shared pool and bounded ownership accessors. The full build keeps
 * its historical inclusion point; types and limits are independent headers. */
#if D8BUILD_POOL_HAS(9)
#include "phys_dsp.c"
#endif
#if D8BUILD_POOL_HAS(9)
#include "phys_symp.c"
#endif
#if D8BUILD_POOL_HAS(10)
#include "drum_voice.c"
#endif
#if D8BUILD_POOL_HAS(12)
#include "fm6_core.c"
#endif
#include "heavy_types.h"
static heavy_state_t heavy_pool[NVOICE] __attribute__((section(".pool")));
static uint16_t heavy_owner[NVOICE]; /* engine/track/voice key + 1, zero = free */

static int heavy_live(uint32_t engine, uint32_t part, uint32_t voice)
{
    uint32_t i;
#if D8BUILD_PROFILE != 0
    if (engine >= NENGINES || !D8BUILD_HAS(engine)) return 0;
#endif
    if (part >= NPART || voice >= NVOICE || trk[part].engine != engine)
        return 0;
    if (engine == 13u)
        return trk[part].v[voice].active && ((uint32_t)trk[part].v[voice].s[4] & 128u); /* reverse only */
    if (engine == 7u || engine == 9u || engine == 12u)                                  /* WHEEL/PHYS/FM6: one body per sounding voice */
        return trk[part].v[voice].active != 0;
    if (engine != 8u && engine != 10u)
        return 0;
    for (i = 0; i < NVOICE; i++)                        /* GRAIN/DRUM: one complete state per active track */
        if (trk[part].v[i].active)
            return 1;
    return 0;
}
#include "heavy_pool_diagnostic.h"
static heavy_state_t *heavy_get(uint32_t engine, uint32_t part, uint32_t voice)
{
    uint32_t key, k;
#if D8BUILD_PROFILE != 0
    if (engine >= NENGINES || !D8BUILD_HAS(engine)) return 0;
#endif
    if (engine < 7u || engine > 13u || engine == 11u || (engine != 7u && engine != 9u && engine != 12u && engine != 13u && voice != 0u) || !heavy_live(engine, part, voice))
        return 0;                                     /* idle block callbacks must not reserve memory */
    key = ((engine - 7u) * NPART + part) * NVOICE + voice + 1u;
    for (k = 0; k < NVOICE; k++)
        if (heavy_owner[k] == key)
#if D8_HEAVY_POOL_DIAGNOSTIC
        {
            d8hp_observe(D8HP_REUSE);
            return &heavy_pool[k];
        }
#else
            return &heavy_pool[k];                     /* retrigger retains the active engine's body */
#endif
    for (k = 0; k < NVOICE; k++) {
        uint32_t old = heavy_owner[k], index = old ? old - 1u : 0;
        if (!old || !heavy_live(7u + index / (NPART * NVOICE), (index / NVOICE) % NPART, index % NVOICE)) {
            heavy_owner[k] = (uint16_t)key;
#if D8BUILD_PROFILE != 0
            switch (engine) {
#if D8BUILD_POOL_HAS(7)
            case 7u: memset(&heavy_pool[k].wheel,0,sizeof heavy_pool[k].wheel); break;
#endif
#if D8BUILD_POOL_HAS(8)
            case 8u: memset(&heavy_pool[k].grain,0,sizeof heavy_pool[k].grain); break;
#endif
#if D8BUILD_POOL_HAS(9)
            case 9u: memset(&heavy_pool[k].phys,0,sizeof heavy_pool[k].phys); break;
#endif
#if D8BUILD_POOL_HAS(10)
            case 10u: memset(&heavy_pool[k].drum,0,sizeof heavy_pool[k].drum); break;
#endif
#if D8BUILD_POOL_HAS(12)
            case 12u: memset(&heavy_pool[k].fm6,0,sizeof heavy_pool[k].fm6); break;
#endif
#if D8BUILD_POOL_HAS(13)
            case 13u: memset(&heavy_pool[k].slice,0,sizeof heavy_pool[k].slice); trk[part].v[voice].s[0]=0x7fffffff; break;
#endif
            default: heavy_owner[k]=0; return 0;
            }
#else
            if (engine == 7u) memset(&heavy_pool[k].wheel, 0, sizeof heavy_pool[k].wheel);
            else if (engine == 8u) memset(&heavy_pool[k].grain, 0, sizeof heavy_pool[k].grain);
            else if (engine == 9u) memset(&heavy_pool[k].phys, 0, sizeof heavy_pool[k].phys);
            else if (engine == 12u) memset(&heavy_pool[k].fm6, 0, sizeof heavy_pool[k].fm6);
            else if (engine == 13u) {
                memset(heavy_pool[k].slice, 0, sizeof heavy_pool[k].slice);
                trk[part].v[voice].s[0] = 0x7FFFFFFF; /* new window: force decode before reading */
            }
            else memset(heavy_pool[k].drum, 0, sizeof heavy_pool[k].drum);
#endif
#if D8_HEAVY_POOL_DIAGNOSTIC
            d8hp_observe(D8HP_ALLOCATE);
#endif
            return &heavy_pool[k];                     /* another engine cannot inherit stale state */
        }
    }
#if D8_HEAVY_POOL_DIAGNOSTIC
    d8hp_observe(D8HP_FAIL);
#endif
    return 0;                                         /* no admitted slot: fail closed */
}
#if D8BUILD_POOL_HAS(7)
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
#endif
