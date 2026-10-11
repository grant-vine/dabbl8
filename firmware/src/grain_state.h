/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
#ifndef DABBL8_GRAIN_STATE_H
#define DABBL8_GRAIN_STATE_H
/* Requires the retained SAMPLE zone type; no allocation or synthesis implementation. */
#include "grain_limits.h"
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
#endif
