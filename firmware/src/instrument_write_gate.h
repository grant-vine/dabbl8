/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_INSTRUMENT_WRITE_GATE_H
#define DABBL8_INSTRUMENT_WRITE_GATE_H
#include "track_limits.h"
#if NTRK > 4
#define D8_INSTRUMENT_QUARANTINED (-10)
/* The general legacy/raw/settings/sample write route remains closed.
 * Clean first-use formatting and own-format local project/preset operations
 * use separate, narrowly validated transaction tickets and RAM driver paths.
 * Recognition, a caller Boolean or a remote packet never grants this route. */
static int d8_instrument_write_allowed(void) { return 0; }
#endif
#endif
