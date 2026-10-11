/* SPDX-License-Identifier: GPL-3.0-only */
/* Stage-2 policy preparation only. Not a firmware card or capability grant. */
#ifndef DABBL8_ENGINE_POLICY_H
#define DABBL8_ENGINE_POLICY_H
#include "d8card.h"
/* Stable stored/wire ID space, including retired DIGITAL. Never subset size. */
#define D8ENGINE_ID_LIMIT 14u
#define D8ENGINE_DIGITAL 1u
#define D8ENGINE_SAMPLE 4u
#define D8ENGINE_DRUM 10u
#define D8ENGINE_FM6 12u
#define D8ENGINE_PERC_PRESET 4u

enum { D8ENGINE_OK=0, D8ENGINE_INVALID_MASK=1, D8ENGINE_INVALID_REQUEST=2,
       D8ENGINE_UNAVAILABLE=3 };
enum { D8ENGINE_CANONICAL=0, D8ENGINE_LEGACY_SELECT=1, D8ENGINE_LEGACY_FACTORY=2 };
typedef struct { unsigned engine,preset; } d8engine_sound;

/* Pure helpers: no runtime, undo, patch, cache, storage or transport access.
 * A mask must be nonempty and contain only current stable selectable IDs.
 * Direct component validation remains d8card_validate's separate obligation.
 * Outputs publish only on OK; all refusals preserve their previous bytes.
 * Factory preset validity/modulo is still the caller's existing policy.
 * New UI/editor exact-ID requests use CANONICAL: retired DIGITAL refuses.
 * LEGACY_SELECT converts DIGITAL to FM6 only for historical sound requests.
 * LEGACY_FACTORY additionally converts SAMPLE/PERC
 * to DRUM. CANONICAL rejects retired IDs and never performs migrations:
 * callers must perform historical PHYS/PERC/DIGITAL normalization first. */
int d8engine_target(unsigned *out,unsigned mask,unsigned requested,unsigned preset,unsigned source);
unsigned d8engine_count(unsigned mask); /* zero for an invalid mask */
int d8engine_visible(unsigned *out,unsigned mask,unsigned position); /* exact index, no modulo */
/* dir is -1 or +1. An unavailable current stable ID starts at the first
 * available display entry; invalid/reserved IDs refuse instead of aliasing. */
int d8engine_step(unsigned *out,unsigned mask,unsigned current,int dir);
/* Preserve a supported historical factory sound; otherwise choose the first
 * available display engine's preset 0. For startup/default insertion only:
 * never substitute an unavailable sound during adoption. Invalid requests
 * do not become defaults.
 * DIGITAL factory preset numbers require migration by the adoption caller. */
int d8engine_default(d8engine_sound *out,unsigned mask,unsigned requested,unsigned preset);
#endif
