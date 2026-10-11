/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
#ifndef DABBL8_WHEEL_STATE_H
#define DABBL8_WHEEL_STATE_H
/* Shared voice state only; no WHEEL renderer or track allocation. */
#include "wheel_limits.h"
typedef struct {                 /* per voice */
    uint32_t ph[DRW_NP];
    int32_t gp[DRW_NP];          /* each partial's gain at the end of the last block (the ramp start) */
    int32_t perc;                /* percussion envelope, Q15 */
    int32_t clk;                 /* click envelope, Q15 */
    int32_t nz, hp;              /* click noise state, its high-pass state */
    uint8_t gate, init;
} drw_vc_t;
#endif
