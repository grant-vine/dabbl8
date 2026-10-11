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
/* Counter values must remain unchanged around the caller's timer read. A
 * bounded unstable snapshot taints the run; it is never retried in the IRQ. */
struct d8_timing_snapshot { uint32_t ticks, boots, late, under, over, unstable; };
#define D8_TIMING_SNAPSHOT 1u
#define D8_TIMING_JUMP 2u
#define D8_TIMING_BOOT 4u
#define D8_TIMING_OVERFLOW 8u
static volatile struct {
    struct d8_timing_histogram render, inclusive;
    uint32_t enabled, generation, saturated, invalid;
    uint32_t first_half, last_half;
    uint32_t window_started, first_ticks, last_ticks, elapsed_lo, elapsed_hi;
    uint32_t window_boots, previous_late, previous_under, previous_over;
    uint32_t run_late, run_under, run_over, live_aux[3], run_aux[3], taint;
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
    audio_timing.window_started = audio_timing.first_ticks = audio_timing.last_ticks = 0;
    audio_timing.elapsed_lo = audio_timing.elapsed_hi = audio_timing.window_boots = 0;
    audio_timing.previous_late = audio_timing.previous_under = audio_timing.previous_over = 0;
    audio_timing.run_late = audio_timing.run_under = audio_timing.run_over = audio_timing.taint = 0;
    for (i = 0; i < 3u; i++) audio_timing.live_aux[i] = audio_timing.run_aux[i] = 0;
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
/* ALNK0-only while collecting; main-only reset/report while stopped. Source
 * counters wrap rather than saturate. Unseen full wraps or external resets that
 * alias wraps cannot be excluded from values alone. */
static void audio_timing_add(volatile uint32_t *value, uint32_t delta)
{
    if (delta > UINT32_MAX - *value) {
        audio_timing.taint |= D8_TIMING_OVERFLOW;
        audio_timing.saturated = 1;
        audio_timing.enabled = 0;
    } else *value += delta;
}
static void audio_timing_aux(uint32_t pending)
{
    uint32_t i;
    if (!audio_timing.enabled || !audio_timing.window_started) return;
    for (i = 0; i < 3u; i++)
        if (pending & (0x10u << i)) audio_timing_add(&audio_timing.live_aux[i], 1);
}
static void audio_timing_record_window(uint32_t render, uint32_t half,
        const struct d8_timing_snapshot *start, const struct d8_timing_snapshot *end,
        uint32_t pending)
{
    uint32_t dt, lo, late, under, over, i;
    if (!audio_timing.enabled) return;
    audio_timing_record(render, end->ticks - start->ticks, half);
    if (render > end->ticks - start->ticks || audio_timing.saturated) return;
    if (start->unstable || end->unstable) audio_timing.taint |= D8_TIMING_SNAPSHOT;
    if (!audio_timing.window_started) {
        audio_timing.first_ticks = audio_timing.last_ticks = start->ticks;
        audio_timing.window_boots = start->boots;
        audio_timing.previous_late = start->late;
        audio_timing.previous_under = start->under;
        audio_timing.previous_over = start->over;
        audio_timing.window_started = 1;
    }
    if (start->boots != audio_timing.window_boots || end->boots != audio_timing.window_boots)
        audio_timing.taint |= D8_TIMING_BOOT;
    dt = end->ticks - audio_timing.last_ticks;
    late = end->late - audio_timing.previous_late;
    under = end->under - audio_timing.previous_under;
    over = end->over - audio_timing.previous_over;
    /* Reject ambiguous/reset-sized jumps. Unseen whole wraps can alias small
     * deltas, so retained external elapsed and reset evidence is still required. */
    if (late > 1u || ((dt | late | under | over) & 0x80000000u)) audio_timing.taint |= D8_TIMING_JUMP;
    lo = audio_timing.elapsed_lo;
    audio_timing.elapsed_lo += dt;
    if (audio_timing.elapsed_lo < lo) audio_timing_add(&audio_timing.elapsed_hi, 1);
    audio_timing_add(&audio_timing.run_late, late);
    audio_timing_add(&audio_timing.run_under, under);
    audio_timing_add(&audio_timing.run_over, over);
    audio_timing_aux(pending);
    for (i = 0; i < 3u; i++) audio_timing.run_aux[i] = audio_timing.live_aux[i];
    audio_timing.last_ticks = end->ticks;
    audio_timing.previous_late = end->late;
    audio_timing.previous_under = end->under;
    audio_timing.previous_over = end->over;
}
#endif
