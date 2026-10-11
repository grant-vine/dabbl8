/* SPDX-License-Identifier: GPL-3.0-only */
/* Optional diagnostic only. Single ALNK0 writer; start/stop/report belong to the
 * non-reentrant main-loop console. Every field is volatile and naturally aligned.
 * A paused writer cannot resume until that main-loop command completes. No IRQ
 * masking, allocation, storage writes or printing from the interrupt. Target
 * publication and overhead still require linked-code and hardware review. */
#ifndef D8_AUDIO_TIMING_H
#define D8_AUDIO_TIMING_H
#define D8_TIMING_BINS 128u
#define D8_TIMING_SHIFT 10u
struct d8_timing_histogram {
    uint32_t bins[D8_TIMING_BINS];
    uint32_t count, max_ticks;
};
static volatile struct {
    struct d8_timing_histogram render, inclusive;
    uint32_t enabled, generation, saturated, invalid;
    uint32_t first_half, last_half;
} audio_timing;

static int audio_timing_start(void)
{
    uint32_t i;
    if (audio_timing.enabled || audio_timing.generation == UINT32_MAX)
        return 0;
    /* Collection is already stopped. The ISR never reads these bins while off. */
    for (i = 0; i < D8_TIMING_BINS; i++) {
        audio_timing.render.bins[i] = 0;
        audio_timing.inclusive.bins[i] = 0;
    }
    audio_timing.render.count = audio_timing.render.max_ticks = 0;
    audio_timing.inclusive.count = audio_timing.inclusive.max_ticks = 0;
    audio_timing.saturated = audio_timing.invalid = 0;
    audio_timing.first_half = audio_timing.last_half = 0;
    audio_timing.generation++;
    audio_timing.enabled = 1; /* publish after every volatile reset */
    return 1;
}

static int audio_timing_stop(void)
{
    if (!audio_timing.enabled)
        return 0;
    audio_timing.enabled = 0;
    return 1;
}

static void audio_timing_bin(volatile struct d8_timing_histogram *h, uint32_t ticks)
{
    uint32_t bin = ticks >> D8_TIMING_SHIFT;
    if (bin >= D8_TIMING_BINS)
        bin = D8_TIMING_BINS - 1u;
    h->bins[bin]++;
    h->count++;
    if (ticks > h->max_ticks)
        h->max_ticks = ticks;
}

static void audio_timing_record(uint32_t render, uint32_t inclusive, uint32_t half)
{
    if (!audio_timing.enabled)
        return;
    if (audio_timing.render.count == UINT32_MAX) {
        audio_timing.saturated = 1;
        audio_timing.enabled = 0;
        return;
    }
    if (render > inclusive) {
        if (audio_timing.invalid != UINT32_MAX)
            audio_timing.invalid++;
        else {
            audio_timing.saturated = 1;
            audio_timing.enabled = 0;
        }
        return;
    }
    if (!audio_timing.render.count)
        audio_timing.first_half = half;
    audio_timing.last_half = half;
#ifdef D8_TIMING_RECORD_TEST_HOOK
    D8_TIMING_RECORD_TEST_HOOK();
#endif
    audio_timing_bin(&audio_timing.render, render);
    audio_timing_bin(&audio_timing.inclusive, inclusive);
#ifdef D8_TIMING_RECORD_TEST_HOOK
    D8_TIMING_RECORD_TEST_HOOK();
#endif
}
#endif
