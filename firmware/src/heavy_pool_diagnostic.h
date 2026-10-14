/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_HEAVY_POOL_DIAGNOSTIC_H
#define DABBL8_HEAVY_POOL_DIAGNOSTIC_H
#ifndef D8_HEAVY_POOL_DIAGNOSTIC
#define D8_HEAVY_POOL_DIAGNOSTIC 0
#endif
#define D8HP_FLAG_0_END() 1
#define D8HP_FLAG_1_END() 1
#define D8HP_FLAG_TOKEN_(value) D8HP_FLAG_##value##_END
#define D8HP_FLAG_TOKEN(value) D8HP_FLAG_TOKEN_(value)
#if !D8HP_FLAG_TOKEN(D8_HEAVY_POOL_DIAGNOSTIC)()
#error "Heavy-pool diagnostic must be literal 0 or 1"
#endif
#undef D8HP_FLAG_TOKEN
#undef D8HP_FLAG_TOKEN_
#undef D8HP_FLAG_1_END
#undef D8HP_FLAG_0_END
#if D8_HEAVY_POOL_DIAGNOSTIC
#if NPART < NVOICE
#error "Heavy-pool diagnostic needs the eight-track shared pool"
#endif
/* Included after heavy_live. Start must exclude owner/voice mutation; main-loop
 * start/stop/snapshot calls are serialized. No IRQ masking, storage or printing.
 * Canonical request reentry invalidates, including while start resets a paused
 * run. Other mutation sources still require the caller's start exclusion. */
enum { D8HP_REUSE, D8HP_ALLOCATE, D8HP_FAIL };
enum { D8HP_REENTRY = 1u, D8HP_OVERFLOW = 2u, D8HP_OWNER = 4u, D8HP_REQUEST = 8u };
#define D8HP_ENGINES 14u
struct d8_heavy_pool_report {
    uint32_t schema, generation, boots, invalid;
    uint32_t requests, allocations, reuses, failures, censuses;
    uint32_t last_live_owned_slots, peak_live_owned_slots;
    uint32_t capacity_slots, body_bytes, reserved_bytes;
    uint8_t last_engine[D8HP_ENGINES], peak_engine[D8HP_ENGINES];
};
static volatile struct {
    struct d8_heavy_pool_report report;
    uint32_t enabled, busy, reentered;
} d8hp;
static int d8hp_increment(volatile uint32_t *value)
{
    if (*value == UINT32_MAX) {
        d8hp.report.invalid |= D8HP_OVERFLOW;
        d8hp.enabled = 0;
        return 0;
    }
    (*value)++;
    return 1;
}
static void d8hp_census(void)
{
    uint8_t counts[D8HP_ENGINES] = {0};
    uint32_t live = 0;
#ifdef D8_HEAVY_POOL_DIAGNOSTIC_CENSUS_HOOK
    D8_HEAVY_POOL_DIAGNOSTIC_CENSUS_HOOK();
#endif
    for (unsigned i = 0; i < NVOICE; i++) {
        uint32_t owner = heavy_owner[i];
        if (!owner) continue;
        uint32_t index = owner - 1u;
        uint32_t engine = 7u + index / (NPART * NVOICE);
        uint32_t part = (index / NVOICE) % NPART, voice = index % NVOICE;
        if (engine > 13u || engine == 11u ||
            ((engine == 8u || engine == 10u) && voice)) {
            d8hp.report.invalid |= D8HP_OWNER;
            d8hp.enabled = 0;
            break;
        }
        if (heavy_live(engine, part, voice)) {
            unsigned duplicate = 0;
            for (unsigned j = 0; j < i; j++) if (heavy_owner[j] == owner) duplicate = 1;
            if (duplicate) {
                d8hp.report.invalid |= D8HP_OWNER;
                d8hp.enabled = 0;
                break;
            }
            counts[engine]++; live++;
        }
    }
    if (d8hp.reentered) d8hp.report.invalid |= D8HP_REENTRY;
    if (d8hp.report.invalid || !d8hp_increment(&d8hp.report.censuses)) return;
    d8hp.report.last_live_owned_slots = live;
    if (live > d8hp.report.peak_live_owned_slots) d8hp.report.peak_live_owned_slots = live;
    for (unsigned i = 0; i < D8HP_ENGINES; i++) {
        d8hp.report.last_engine[i] = counts[i];
        if (counts[i] > d8hp.report.peak_engine[i])
            d8hp.report.peak_engine[i] = counts[i];
    }
}
/* 0 refuses without reset; 1 starts; 2 retains an invalid, paused new run. */
static int d8hp_start(uint32_t boots)
{
    if (d8hp.enabled || d8hp.busy || d8hp.report.generation == UINT32_MAX)
        return 0;
    uint32_t generation = d8hp.report.generation + 1u;
    d8hp.reentered = 0; d8hp.busy = 1u;
#ifdef D8_HEAVY_POOL_DIAGNOSTIC_START_HOOK
    D8_HEAVY_POOL_DIAGNOSTIC_START_HOOK();
#endif
    struct d8_heavy_pool_report next = {0};
    next.schema = 1u; next.generation = generation; next.boots = boots;
    next.capacity_slots = NVOICE; next.body_bytes = sizeof(heavy_state_t);
    next.reserved_bytes = sizeof heavy_pool;
    d8hp.report = next;
    d8hp_census();
    d8hp.busy = 0;
    if (d8hp.report.invalid) return 2;
    d8hp.enabled = 1u;
    return 1;
}
static int d8hp_stop(void)
{
    if (!d8hp.enabled || d8hp.busy) return 0;
    d8hp.enabled = 0;
    return 1;
}
/* 0 unavailable/running (output unchanged); 1 valid scope; 2 invalid snapshot. */
static int d8hp_snapshot(struct d8_heavy_pool_report *out)
{
    if (!out || d8hp.enabled || d8hp.busy || !d8hp.report.generation)
        return 0;
    *out = d8hp.report;
    return out->invalid ? 2 : 1;
}
static void d8hp_observe(unsigned outcome)
{
    if (d8hp.busy) {
        d8hp.reentered = 1u;
        d8hp.report.invalid |= D8HP_REENTRY;
        d8hp.enabled = 0;
        return;
    }
    if (!d8hp.enabled) return;
    if (outcome > D8HP_FAIL) {
        d8hp.report.invalid |= D8HP_REQUEST;
        d8hp.enabled = 0;
        return;
    }
    d8hp.busy = 1u; d8hp.reentered = 0;
    d8hp_census();
    if (d8hp.enabled && !d8hp.report.invalid &&
        d8hp_increment(&d8hp.report.requests))
        d8hp_increment(outcome == D8HP_ALLOCATE ? &d8hp.report.allocations :
                       outcome == D8HP_REUSE ? &d8hp.report.reuses : &d8hp.report.failures);
    d8hp.busy = 0;
}
#endif
#endif
