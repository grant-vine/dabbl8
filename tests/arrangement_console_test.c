/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual native arrangement transitions and formatter; fake guard/CDC/time. */
#define D8ARR_TEST_NO_MAIN 1
#include "native_arrangement_test.c"
#include <assert.h>
static struct { unsigned boots; } arrangement_dbg = {17};
static struct { unsigned stalled; } con;
static unsigned locked, saved_key = 0x1234u, lock_calls, print_calls;
static unsigned fm1__lock(void) { assert(!locked); locked = 1; lock_calls++; return saved_key; }
static void fm1__unlock(unsigned key) { assert(locked && key == saved_key); locked = 0; }
static void con_putc(char c) { assert(!locked); print_calls++; putchar(c); }
static void con_puts(const char *s) { while (*s) con_putc(*s++); }
static void con_hex(unsigned v, unsigned digits) {
    while (digits--) con_putc("0123456789ABCDEF"[(v >> (digits * 4u)) & 15u]);
}
static void con_timing_crc_word(uint32_t *crc, uint32_t value) {
    for (unsigned b = 0; b < 4; b++) {
        *crc ^= (value >> (b * 8u)) & 255u;
        for (unsigned bit = 0; bit < 8; bit++) *crc = (*crc >> 1) ^ ((*crc & 1u) ? 0xEDB88320u : 0u);
    }
}
static void con_timing_word(uint32_t *crc, uint32_t value) {
    con_putc(' '); con_hex(value, 8); con_timing_crc_word(crc, value);
}
#define felucca_dbg arrangement_dbg
#include "../firmware/src/arrangement_diagnostic_console.h"
#undef felucca_dbg

int main(void)
{
#if DABBL8_TIMING_DIAGNOSTIC
    seed();
    chain.native_mode = 0; con_arrangement_status();
    chain.native_mode = 2; con_arrangement_status();
    chain.native_mode = 0;
    prepare_start();
    for (unsigned i = 0; i < 4; i++) {
        snapshot();
        con_arrangement_status();
        proof(unchanged(), "status snapshot preserves all music/cache/ownership state");
        boundary(0);
    }
    proof(!song.playing && !chain.running, "actual native run reaches stopped suspension");
    snapshot(); con_arrangement_status();
    proof(unchanged() && !locked && lock_calls == 7 && print_calls, "guard restored; printing outside; no state mutation");
    con.stalled = 1; con_arrangement_status(); /* Incomplete frame, never CRC-closed. */
    proof(!locked && lock_calls == 8, "stalled output also restores guard");
#else
    assert(!lock_calls && !print_calls);
#endif
    fprintf(stderr, "arrangement status checks=%u failures=%u simulated_only=1\n", checks, failures);
    return !!failures;
}
