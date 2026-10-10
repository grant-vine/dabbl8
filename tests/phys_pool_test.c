/* SPDX-License-Identifier: GPL-3.0-only
 * Real eight-part PHYS ownership, reuse and cross-engine lifetime checks. */
#if NPART != 8
#error Build with -DNPART=8
#endif
#define main hostsim_main
#include "hostsim.c"
#undef main
static int bad;
static void check(const char *name, int ok)
{
    printf("PHYS pool: %s %s\n", name, ok ? "PASS" : "FAIL"); bad += !ok;
}
static void fresh(void)
{
    memset(trk, 0, sizeof trk); memset(&song, 0, sizeof song);
    memset(phys_owner, 0, sizeof phys_owner); memset(phys_slot, 0, sizeof phys_slot);
    host_tracks_init();
    for (uint32_t p = 0; p < NPART; p++) {
        host_preset(&trk[p], ENGI_PHYS, 0); trk[p].p[P_VOICE] = V_POLY;
        trk[p].p[P_E0] = p % PM_COUNT; trk_note_on(&trk[p], 48 + p, 100);
    }
}
static int ownership(void)
{
    phys_slot_t *used[NVOICE]; uint32_t n = 0;
    for (uint32_t p = 0; p < NPART; p++)
        for (uint32_t i = 0; i < NVOICE; i++) {
            voice_t *v = &trk[p].v[i];
            if (!v->active || trk[p].engine != ENGI_PHYS) continue;
            phys_slot_t *state = phys_slot_of(&trk[p], v);
            if (!state || n == NVOICE) return 0;
            for (uint32_t k = 0; k < n; k++) if (used[k] == state) return 0;
            used[n++] = state;
        }
    return 1;
}
static uint32_t render(void)
{
    int32_t out[CTL]; uint32_t count = 0;
    for (uint32_t p = 0; p < NPART; p++) count += track_render(&trk[p], out, CTL);
    return count;
}
int main(void)
{
    fresh();
    check("eight independent track notes own distinct state slots", ownership() && voices_busy() == 8);
    check("eight sounding states render within the shared budget", render() == 8);
    check("buffer bound is eight states, not eight times the track cap", sizeof phys_slot == NVOICE * sizeof(phys_slot_t));
    phys_slot_t *first = phys_slot_of(&trk[0], &trk[0].v[0]);
    phys_slot_t snapshot = *first;
    trk_note_on(&trk[7], 55, 100); /* same note retrigger */
    check("another track's retrigger cannot overwrite the first body", memcmp(first, &snapshot, sizeof snapshot) == 0);
    phys_slot_t *again = phys_slot_of(&trk[7], &trk[7].v[0]);
    trk_note_on(&trk[7], 55, 100);
    check("same destination retrigger retains its body", again == phys_slot_of(&trk[7], &trk[7].v[0]));
    trk_note_on(&trk[7], 80, 100);
    check("ninth note reclaims retired state without aliasing live bodies", ownership() && voices_busy() == 8 && render() <= 8);

    fresh();
    trk[0].engine = trk[0].eng_req = 0; /* old owner active on another engine */
    trk_note_off(&trk[4], 52); trk_note_on(&trk[7], 80, 100);
    check("stale ownership on another engine is reusable", ownership() && render() <= 8);
    trk_all_off(&trk[0]); engine_block(&trk[0]);
    host_preset(&trk[0], ENGI_PHYS, 0); trk_note_on(&trk[0], 36, 100);
    check("returning to PHYS cannot alias the reassigned owner", ownership() && render() <= 8);

    fresh();
    trk[0].v[1].active = 1; /* invalid caller tries a ninth physical state */
    check("full state pool fails closed for an unadmitted ninth voice", phys_slot_of(&trk[0], &trk[0].v[1]) == 0);
    trk[0].v[1].active = 0;
    uint32_t errors = 0, seed = 7;
    for (uint32_t k = 0; k < 5000; k++) {
        seed = seed * 1664525u + 1013904223u;
        uint32_t p = seed >> 29;
        trk[p].p[P_E0] = (seed >> 16) % PM_COUNT;
        trk_note_on(&trk[p], 36 + seed % 60, 100);
        if (!ownership() || voices_busy() > 8 || render() > 8) { errors++; break; }
    }
    check("5000 mixed-model steals retain unique bounded ownership", !errors);
    for (uint32_t p = 0; p < NPART; p++) trk_all_off(&trk[p]);
    for (uint32_t b = 0; b < 5000 && voices_busy(); b++) render();
    check("release completes and all states can be reused", voices_busy() == 0);
    fresh(); check("reset after a session recreates eight independent bodies", ownership() && render() == 8);
    printf("PHYS state pool: %d failures, %zu bytes for eight states\n", bad, sizeof phys_slot);
    return !!bad;
}
