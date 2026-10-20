/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* FM-1 time base: TIMER4 free-running from the 24 MHz crystal.
 * Measured on hardware: under mask-ROM UBOOT the crystal runs and TIMER4 counts
 * 24 MHz, matching the host's measurement of the same interval to 0.3 %.
 * TIMER4 is not used by the ROM or the SPL. No interrupts, no ROM calls.
 *
 * JL_TIMER4 = 0x10800: CON [1:0]=1 run, [3:2]=2 OSC source, [7:4]=0 /1,
 * bit14 = clear pending; CNT +4; PRD +8 (32-bit, set to 0xFFFFFFFF).
 * The counter wraps every 178.9 s; use differences (unsigned arithmetic).
 */
#pragma once
#include <stdint.h>

#ifndef FM1_TIME_REG32
#define FM1_TIME_REG32(a) (*(volatile uint32_t *)(a))
#endif
#define FM1_T4_CON FM1_TIME_REG32(0x10800u)
#define FM1_T4_CNT FM1_TIME_REG32(0x10804u)
#define FM1_T4_PRD FM1_TIME_REG32(0x10808u)
#define FM1_TICKS_PER_US 24u

/* Preserve continuity only for an inherited clock matching our time base.
 * The sealed updater retains its historical handover policy. */
static inline void fm1_time_init(void)
{
#if defined(FELUCCA_LOADER) && FELUCCA_LOADER
    if ((FM1_T4_CON & 0xFu) == ((2u << 2) | 1u))
#else
    if ((FM1_T4_CON & 0xFFu) == ((2u << 2) | 1u) && FM1_T4_PRD == UINT32_MAX)
#endif
        return;
    FM1_T4_CON = 0;
    FM1_T4_PRD = 0xFFFFFFFFu;
    FM1_T4_CNT = 0;
    FM1_T4_CON = (2u << 2) | 1u | (1u << 14);
}

static inline uint32_t fm1_ticks(void) { return FM1_T4_CNT; }
static inline uint32_t fm1_time_config(void) { return FM1_T4_CON; }
static inline uint32_t fm1_time_period(void) { return FM1_T4_PRD; }

/* Microseconds, wrapping every ~178.9 s. */
static inline uint32_t fm1_micros(void) { return FM1_T4_CNT / FM1_TICKS_PER_US; }

static inline void fm1_delay_us(uint32_t us)
{
    uint32_t start = FM1_T4_CNT, span = us * FM1_TICKS_PER_US;
    while ((uint32_t)(FM1_T4_CNT - start) < span)
        ;
}

static inline void fm1_delay_ms(uint32_t ms)
{
    while (ms--)
        fm1_delay_us(1000u);
}
