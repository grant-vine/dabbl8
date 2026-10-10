/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_INSTRUMENT_WRITE_GATE_H
#define DABBL8_INSTRUMENT_WRITE_GATE_H
#include "track_limits.h"
#if NTRK > 4
#define D8_INSTRUMENT_QUARANTINED (-10)
/* Pre-migration protection only. There is deliberately no grant, setter,
 * retained permission or protocol authorization. Native recognition/binding
 * cannot prove originals retention, completed migration or RAM adoption.
 * A future separately reviewed trusted coordinator must replace this policy;
 * constant closure is not a completed persistence implementation. */
static int d8_instrument_write_allowed(void) { return 0; }
#endif
#endif
