/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
#ifndef DABBL8_GRAIN_LIMITS_H
#define DABBL8_GRAIN_LIMITS_H
/* Unchanged GRAIN capacity and decode limits; shared state does not own a renderer. */
#define GR_POLY 3                /* voices per part */
#define GR_NG 12                 /* grains per part */
#define GR_SEG_LOG2 8
#define GR_SEG (1u << GR_SEG_LOG2)   /* seek index interval, source samples */
#define GR_NIDX 1024             /* seek index entries per part (sized for the retired PERC: 1001; it keeps user slots room) */
#define GR_MAXZ 32               /* zones per set (a user slot <= 16; the retired PERC had 24) */
#define GR_RB 128                /* reverse window, source samples */
#define GR_STEP_MAX (2u << 16)   /* grain rate cap: 2 source samples per output sample */
#define GR_LOAD_MAX (8u << 16)   /* decodes per output sample of all a part's grains (a reverse grain counts twice) */
#endif
