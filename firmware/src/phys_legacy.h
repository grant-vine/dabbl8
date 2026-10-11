/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
#ifndef DABBL8_PHYS_LEGACY_H
#define DABBL8_PHYS_LEGACY_H
/* Pure historical parameter migration; no physical-model DSP or state allocation. */
#include "phys_models.h"
/* a project or user preset saved before 1.0 (MODEL 2 was DUST): its E values as today's */
static void phys_legacy(int16_t *e)
{
    if (e[0] == 2) {                                     /* DUST -> MODAL bowed (random hits into resonators) */
        e[0] = PM_MODAL;
        e[6] = 64;
    }
}
#endif
