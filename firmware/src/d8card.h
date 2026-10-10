/* SPDX-License-Identifier: GPL-3.0-only
 * Engine identities derive from Felucca, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hügelton Instruments. */
#ifndef DABBL8_CARD_H
#define DABBL8_CARD_H
#include "d8p1.h"
#define D8CARD_ALL 0x3ffdu
#define D8CARD_ENGINES 13u
enum { D8CARD_SAMPLE_RUNTIME=1, D8CARD_FM6_PATCH=2, D8CARD_HEAVY_POOL=4 };
enum { D8CARD_OK=0, D8CARD_INVALID_PROFILE=1, D8CARD_MISSING_COMPONENT=2,
       D8CARD_INVALID_PROJECT=3, D8CARD_UNSUPPORTED_PROJECT=4, D8CARD_MISSING_ENGINE=5 };
typedef struct { unsigned id,components; const char *name; } d8card_engine;
typedef struct { unsigned required,missing; } d8card_report;
const d8card_engine *d8card_engine_by_id(unsigned id);
int d8card_validate(unsigned engine_mask,unsigned components);
/* Borrowed input. No native adoption, audio, allocation, erase or write.
 * Report published only for OK/MISSING_ENGINE. Other failures preserve it.
 * Retired DIGITAL refuses here; archive conversion must precede preflight. */
int d8card_preflight(d8card_report *out,const void *project,size_t length,
                     unsigned engine_mask,unsigned components);
#endif
