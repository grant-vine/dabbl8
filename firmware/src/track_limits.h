/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
#ifndef FELUCCA_TRACK_LIMITS_H
#define FELUCCA_TRACK_LIMITS_H
#define NVOICE 8                 /* voices per part, and the budget shared by all parts */
#ifndef NPART
#define NPART 4                  /* target stays four until runtime/memory gates pass */
#endif
#define NTRK NPART               /* tracks (the formats and the protocol count these): every track is a part */
#endif
