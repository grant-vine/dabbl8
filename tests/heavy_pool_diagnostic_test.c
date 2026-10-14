/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
static void hp_observe_hook(void);
static void hp_start_hook(void);
#define D8_HEAVY_POOL_DIAGNOSTIC_CENSUS_HOOK() hp_observe_hook()
#define D8_HEAVY_POOL_DIAGNOSTIC_START_HOOK() hp_start_hook()
#define main hostsim_main
#include "hostsim.c"
#undef main
#if NPART < NVOICE
#include "../firmware/src/heavy_pool_diagnostic.h"
#endif
#if D8_HEAVY_POOL_DIAGNOSTIC
static unsigned hp_checks, hp_nested, hp_start_nested;
#define HP_CHECK(x) do { hp_checks++; if (!(x)) { fprintf(stderr, "FAIL line %u: %s\n", __LINE__, #x); return 1; } } while (0)
static void hp_observe_hook(void)
{
    if (hp_nested) { hp_nested = 0; (void)heavy_get(12u, 0, 0); }
}
static void hp_start_hook(void)
{
    if (hp_start_nested) { hp_start_nested = 0; (void)heavy_get(12u, 0, 0); }
}
static void hp_fresh(void)
{
    memset(trk, 0, sizeof trk); memset(&song, 0, sizeof song);
    memset(heavy_owner, 0, sizeof heavy_owner); memset(heavy_pool, 0, sizeof heavy_pool);
}
static void hp_live(unsigned engine, unsigned part, unsigned voice)
{
    trk[part].engine = engine; trk[part].v[voice].active = 1;
    if (engine == 13u) trk[part].v[voice].s[4] |= 128u;
}
int main(void)
{
    struct d8_heavy_pool_report report, unchanged;
    memset(&report, 0x5a, sizeof report); unchanged = report;
    hp_fresh(); HP_CHECK(!d8hp_snapshot(&report) && !memcmp(&report, &unchanged, sizeof report));
    HP_CHECK(d8hp_start(77) && d8hp.report.generation == 1 && d8hp.report.boots == 77);
    HP_CHECK(d8hp.report.censuses == 1 && d8hp.report.capacity_slots == NVOICE && d8hp.report.body_bytes == sizeof(heavy_state_t) && d8hp.report.reserved_bytes == sizeof heavy_pool);
    HP_CHECK(!d8hp_start(88) && d8hp.report.boots == 77 && d8hp.report.generation == 1);
    HP_CHECK(!d8hp_snapshot(&report) && !memcmp(&report, &unchanged, sizeof report));
    HP_CHECK(!heavy_get(0,0,0) && !heavy_get(11,0,0) && !heavy_get(14,0,0));
    HP_CHECK(!heavy_get(12,NPART,0) && !heavy_get(12,0,NVOICE) && !heavy_get(12,0,0));
    hp_live(10,0,0); HP_CHECK(!heavy_get(10,0,1));
    trk[0].v[0].active = 0; HP_CHECK(d8hp.report.requests == 0);
    for (unsigned i = 0; i < NVOICE; i++) { hp_live(12,0,i); HP_CHECK(heavy_get(12,0,i)); }
    HP_CHECK(d8hp.report.requests == NVOICE && d8hp.report.allocations == NVOICE);
    HP_CHECK(d8hp.report.last_live_owned_slots == NVOICE && d8hp.report.peak_live_owned_slots == NVOICE);
    HP_CHECK(d8hp.report.last_engine[12] == NVOICE && d8hp.report.peak_engine[12] == NVOICE);
    hp_live(12,1,0); HP_CHECK(!heavy_get(12,1,0) && d8hp.report.failures == 1);
    uint16_t owners[NVOICE]; memcpy(owners, heavy_owner, sizeof owners);
    HP_CHECK(heavy_get(12,0,0) && heavy_get(12,0,0));
    HP_CHECK(d8hp.report.reuses == 2 && !memcmp(owners, heavy_owner, sizeof owners));
    HP_CHECK(d8hp_stop() && !d8hp_stop() && !d8hp_snapshot(NULL));
    HP_CHECK(d8hp_snapshot(&report) && !report.invalid && report.requests == NVOICE + 3u);
    HP_CHECK(report.generation == 1 && report.boots == 77 && report.peak_live_owned_slots == NVOICE);
    for (unsigned i = 0; i < NVOICE; i++) trk[0].v[i].active = 0;
    HP_CHECK(d8hp_snapshot(&report) && report.last_live_owned_slots == NVOICE);
    hp_live(12,1,0); HP_CHECK(heavy_get(12,1,0));
    HP_CHECK(d8hp_snapshot(&report) && report.requests == NVOICE + 3u);
    HP_CHECK(d8hp_start(77) && d8hp.report.generation == 2);
    HP_CHECK(heavy_get(12,1,0) && d8hp.report.last_live_owned_slots == 1 && d8hp.report.peak_live_owned_slots == 1);
    HP_CHECK(d8hp.report.last_engine[12] == 1 && d8hp.report.peak_engine[12] == 1);
    HP_CHECK(d8hp_stop()); hp_fresh(); HP_CHECK(d8hp_start(77));
    hp_live(10,0,0); HP_CHECK(heavy_get(10,0,0));
    hp_live(12,1,0); HP_CHECK(heavy_get(12,1,0));
    HP_CHECK(d8hp.report.last_live_owned_slots == 2 && d8hp.report.last_engine[10] == 1 && d8hp.report.last_engine[12] == 1);
    trk[0].v[0].active = 0; HP_CHECK(heavy_get(12,1,0));
    HP_CHECK(d8hp.report.last_live_owned_slots == 1 && !d8hp.report.last_engine[10] && d8hp.report.peak_engine[10] == 1);
    HP_CHECK(d8hp_stop()); hp_fresh(); HP_CHECK(d8hp_start(77));
    for (unsigned e = 7; e <= 13; e++) {
        if (e == 11 || !D8BUILD_HAS(e)) continue;
        hp_live(e,0,0); HP_CHECK(heavy_get(e,0,0));
        HP_CHECK(d8hp.report.last_live_owned_slots == 1 && d8hp.report.last_engine[e] == 1);
        trk[0].v[0].active = 0;
    }
    unsigned independent_peaks = 0;
    for (unsigned e = 0; e < D8HP_ENGINES; e++) independent_peaks += d8hp.report.peak_engine[e];
    HP_CHECK(independent_peaks > d8hp.report.peak_live_owned_slots && d8hp.report.peak_live_owned_slots == 1);
    HP_CHECK(d8hp_stop()); hp_fresh(); hp_live(12,0,0); HP_CHECK(heavy_get(12,0,0));
    HP_CHECK(d8hp_start(77) == 1 && d8hp.report.last_live_owned_slots == 1 && d8hp.report.requests == 0 && d8hp.report.censuses == 1);
    HP_CHECK(d8hp_stop()); hp_start_nested = 1;
    HP_CHECK(d8hp_start(77) == 2 && !d8hp.enabled && d8hp_snapshot(&report) == 2 && report.invalid == D8HP_REENTRY);
    hp_nested = 1;
    HP_CHECK(d8hp_start(77) == 2 && !d8hp.enabled && d8hp_snapshot(&report) == 2 && report.invalid == D8HP_REENTRY);
    HP_CHECK(d8hp_start(77) == 1);
    d8hp.report.requests = UINT32_MAX;
    HP_CHECK(heavy_get(12,0,0) && !d8hp.enabled);
    HP_CHECK(d8hp_snapshot(&report) == 2 && report.invalid == D8HP_OVERFLOW && report.requests == UINT32_MAX);
    HP_CHECK(d8hp_start(77)); d8hp.report.reuses = UINT32_MAX;
    HP_CHECK(heavy_get(12,0,0) && d8hp_snapshot(&report) == 2 && report.invalid == D8HP_OVERFLOW);
    HP_CHECK(d8hp_start(77)); hp_nested = 1;
    HP_CHECK(heavy_get(12,0,0) && d8hp_snapshot(&report) == 2 && report.invalid == D8HP_REENTRY);
    HP_CHECK(!d8hp.busy && !d8hp.enabled);
    hp_fresh(); hp_live(12,0,0); HP_CHECK(heavy_get(12,0,0));
    heavy_owner[NVOICE-1] = heavy_owner[0];
    HP_CHECK(d8hp_start(77) == 2 && d8hp_snapshot(&report) == 2 && report.invalid == D8HP_OWNER);
    hp_fresh(); hp_live(12,0,0); HP_CHECK(heavy_get(12,0,0));
    HP_CHECK(d8hp_start(77) == 1); heavy_owner[NVOICE-1] = heavy_owner[0];
    HP_CHECK(heavy_get(12,0,0) && d8hp_snapshot(&report) == 2 && report.invalid == D8HP_OWNER);
    trk[0].v[0].active = 0;
    HP_CHECK(d8hp_start(77) == 1 && d8hp.report.last_live_owned_slots == 0);
    hp_live(12,1,0); HP_CHECK(heavy_get(12,1,0) && !d8hp.report.invalid && d8hp.report.last_live_owned_slots == 1);
    d8hp_observe(D8HP_FAIL + 1u);
    HP_CHECK(d8hp_snapshot(&report) == 2 && report.invalid == D8HP_REQUEST);
    hp_fresh(); hp_live(12,0,0); HP_CHECK(heavy_get(12,0,0));
    HP_CHECK(d8hp_start(77)); heavy_owner[NVOICE-1] = UINT16_MAX;
    HP_CHECK(heavy_get(12,0,0) && d8hp_snapshot(&report) == 2 && report.invalid == D8HP_OWNER);
    d8hp.report.generation = UINT32_MAX; unchanged = report;
    HP_CHECK(!d8hp_start(77) && d8hp.report.generation == UINT32_MAX);
    printf("Heavy-pool diagnostic: %u checks PASS; tracks=%u profile=%u; bounded live-owner observations, paused report only\n", hp_checks, NPART, D8BUILD_PROFILE);
    return 0;
}
#else
static void hp_observe_hook(void) {}
static void hp_start_hook(void) {}
int main(void)
{
    printf("Heavy-pool diagnostic: disabled; tracks=%u profile=%u; no measurement capability\n", NPART, D8BUILD_PROFILE);
    return 0;
}
#endif
