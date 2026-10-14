/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once

#ifndef D8_STACK_WATERMARK_DIAGNOSTIC
#define D8_STACK_WATERMARK_DIAGNOSTIC 0
#endif
#define FM1_STACK_FLAG_0_END() 1
#define FM1_STACK_FLAG_1_END() 1
#define FM1_STACK_FLAG_TOKEN_(value) FM1_STACK_FLAG_##value##_END
#define FM1_STACK_FLAG_TOKEN(value) FM1_STACK_FLAG_TOKEN_(value)
#if !FM1_STACK_FLAG_TOKEN(D8_STACK_WATERMARK_DIAGNOSTIC)()
#error D8_STACK_WATERMARK_DIAGNOSTIC must be 0 or 1
#endif
#undef FM1_STACK_FLAG_TOKEN
#undef FM1_STACK_FLAG_TOKEN_
#undef FM1_STACK_FLAG_0_END
#undef FM1_STACK_FLAG_1_END
#if D8_STACK_WATERMARK_DIAGNOSTIC && defined(FELUCCA_LOADER) && FELUCCA_LOADER
#error Stack watermark diagnostics are application-only
#endif

/* Shared with crt0.S; no C suffix or declarations in assembler input. */
#define FM1_STACK_WATERMARK_PATTERN 0xA55A3CC3

#ifndef __ASSEMBLER__
#include <stdint.h>

enum { FM1_STACK_WATERMARK_UNAVAILABLE, FM1_STACK_WATERMARK_OBSERVED,
       FM1_STACK_WATERMARK_INVALID };
typedef struct { uintptr_t low, top; } fm1_stack_window_t;
typedef struct {
    uintptr_t low, top;
    uint32_t status, capacity_bytes, untouched_bytes, observed_bytes;
} fm1_stack_watermark_t;
typedef struct {
    uint32_t enabled;
    fm1_stack_watermark_t user, supervisor;
} fm1_stack_watermarks_t;

#if D8_STACK_WATERMARK_DIAGNOSTIC
/* This cap bounds diagnostic work, independently of the linker memory map. */
#define FM1_STACK_WATERMARK_MAX_BYTES 65536u
static inline int fm1_stack_window_valid(fm1_stack_window_t window)
{
    return window.low && !((window.low | window.top) & 3u) &&
           window.top > window.low &&
           window.top - window.low <= FM1_STACK_WATERMARK_MAX_BYTES;
}

/* A low-to-high scan observes surviving writes on a downward-growing stack.
 * Matching writes and unwritten reserved frames can hide deeper use. */
static inline fm1_stack_watermark_t fm1_stack_watermark_scan(fm1_stack_window_t window)
{
    fm1_stack_watermark_t result = {window.low, window.top,
        FM1_STACK_WATERMARK_INVALID, 0, 0, 0};
    if (!fm1_stack_window_valid(window)) return result;
    uintptr_t cursor = window.low;
    while (cursor < window.top &&
           *(const volatile uint32_t *)cursor == FM1_STACK_WATERMARK_PATTERN)
        cursor += sizeof(uint32_t);
    result.status = FM1_STACK_WATERMARK_OBSERVED;
    result.capacity_bytes = (uint32_t)(window.top - window.low);
    result.untouched_bytes = (uint32_t)(cursor - window.low);
    result.observed_bytes = (uint32_t)(window.top - cursor);
    return result;
}
#endif

/* Validate both windows before reading either. Call only in a paused diagnostic
 * context: interrupts remain enabled and the snapshot is not atomic. */
static inline fm1_stack_watermarks_t fm1_stack_watermark_read_windows(
    fm1_stack_window_t user, fm1_stack_window_t supervisor)
{
    fm1_stack_watermarks_t result = {D8_STACK_WATERMARK_DIAGNOSTIC,
        {user.low, user.top, FM1_STACK_WATERMARK_UNAVAILABLE, 0, 0, 0},
        {supervisor.low, supervisor.top, FM1_STACK_WATERMARK_UNAVAILABLE, 0, 0, 0}};
#if D8_STACK_WATERMARK_DIAGNOSTIC
    result.user.status = result.supervisor.status = FM1_STACK_WATERMARK_INVALID;
    if (!fm1_stack_window_valid(user) || !fm1_stack_window_valid(supervisor) ||
        (user.low < supervisor.top && supervisor.low < user.top))
        return result;
    result.user = fm1_stack_watermark_scan(user);
    result.supervisor = fm1_stack_watermark_scan(supervisor);
#endif
    return result;
}

static inline fm1_stack_watermarks_t fm1_stack_watermark_read(void)
{
#if D8_STACK_WATERMARK_DIAGNOSTIC
    extern char d8_stack_watermark_startup_enabled[];
    extern char _ustack_lo[], _ustack_top[], _sstack_lo[], _sstack_top[];
    fm1_stack_window_t user = {(uintptr_t)_ustack_lo, (uintptr_t)_ustack_top};
    fm1_stack_window_t supervisor = {(uintptr_t)_sstack_lo, (uintptr_t)_sstack_top};
    /* An enabled C readout cannot link against an uncolored startup. The
     * absolute symbol is emitted only by the enabled assembly branch. */
    if ((uintptr_t)d8_stack_watermark_startup_enabled != 1u) {
        fm1_stack_watermarks_t invalid = {1,
            {user.low, user.top, FM1_STACK_WATERMARK_INVALID, 0, 0, 0},
            {supervisor.low, supervisor.top, FM1_STACK_WATERMARK_INVALID, 0, 0, 0}};
        return invalid;
    }
#else
    fm1_stack_window_t user = {0, 0}, supervisor = {0, 0};
#endif
    return fm1_stack_watermark_read_windows(user, supervisor);
}
#endif
