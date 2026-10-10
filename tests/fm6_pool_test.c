/* SPDX-License-Identifier: GPL-3.0-only
 * Real eight-part FM6 operator-state lifetime and bounded ownership. */
#if NPART != 8
#error Build with -DNPART=8
#endif
#define main hostsim_main
#include "hostsim.c"
#undef main
static int bad;
static void check(const char *name, int ok)
{
    printf("FM6 pool: %s %s\n", name, ok ? "PASS" : "FAIL"); bad += !ok;
}
static void fresh(void)
{
    memset(trk, 0, sizeof trk); memset(&song, 0, sizeof song);
    memset(heavy_owner, 0, sizeof heavy_owner); memset(heavy_pool, 0, sizeof heavy_pool);
    host_tracks_init();
    for (uint32_t p = 0; p < NPART; p++) {
        host_preset(&trk[p], ENGI_FM6, p % ENGINES[ENGI_FM6]->npresets);
        trk[p].p[P_VOICE] = V_POLY; trk_note_on(&trk[p], 48 + p, 100);
    }
}
static int ownership(void)
{
    fm6_note_t *used[NVOICE]; uint32_t n = 0;
    for (uint32_t p = 0; p < NPART; p++)
        for (uint32_t i = 0; i < NVOICE; i++) {
            voice_t *v = &trk[p].v[i];
            if (!v->active || trk[p].engine != ENGI_FM6) continue;
            fm6_note_t *state = fm6_note_of(&trk[p], v);
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
    check("eight tracks own distinct operator states", ownership() && voices_busy() == 8 && render() == 8);
    check("state bound is eight bodies, independent of six-voice track cap", sizeof heavy_pool / sizeof heavy_pool[0] == NVOICE && sizeof heavy_pool[0].fm6 == sizeof(fm6_note_t));
    fm6_note_t *first = fm6_note_of(&trk[0], &trk[0].v[0]);
    fm6_note_t snapshot = *first;
    trk_note_on(&trk[7], 55, 100);
    check("another track's retrigger preserves the first operator body", memcmp(first, &snapshot, sizeof snapshot) == 0);
    fm6_note_t *again = fm6_note_of(&trk[7], &trk[7].v[0]);
    int32_t phase = again->op[0].phase;
    trk_note_on(&trk[7], 55, 100);
    check("same destination retrigger retains nonzero operator phase", phase && again == fm6_note_of(&trk[7], &trk[7].v[0]) && phase == again->op[0].phase);
    trk_note_on(&trk[7], 80, 100);
    check("ninth note reuses retired state without aliasing live notes", ownership() && voices_busy() == 8 && render() <= 8);

    fresh();
    trk[0].engine = trk[0].eng_req = 0;
    voice_t *v = &trk[7].v[1]; v->active = 1;
    fm6_note_t *reclaimed = fm6_note_of(&trk[7], v), zero = {0};
    check("changed-engine ownership is reclaimed with all operator state cleared", reclaimed && memcmp(reclaimed, &zero, sizeof zero) == 0);
    v->active = 0; trk_all_off(&trk[0]); engine_block(&trk[0]);
    host_preset(&trk[0], ENGI_FM6, 0); trk_note_on(&trk[0], 36, 100);
    check("returning to FM6 cannot alias the reassigned body", ownership() && render() <= 8);

    fresh(); trk[0].v[1].active = 1;
    check("unadmitted ninth state refuses instead of overwriting another track", fm6_note_of(&trk[0], &trk[0].v[1]) == 0);
    trk[0].v[1].active = 0;
    uint32_t seed = 9, errors = 0;
    for (uint32_t k = 0; k < 5000; k++) {
        seed = seed * 1664525u + 1013904223u;
        uint32_t p = seed >> 29;
        if (k % 23 == 0) host_preset(&trk[p], ENGI_FM6, (seed >> 16) % ENGINES[ENGI_FM6]->npresets);
        trk_note_on(&trk[p], 36 + seed % 60, 100);
        if (!ownership() || voices_busy() > 8 || render() > 8) { errors++; break; }
    }
    check("5000 mixed-patch steals retain unique bounded state", !errors);
    for (uint32_t p = 0; p < NPART; p++) trk_all_off(&trk[p]);
    for (uint32_t b = 0; b < 30000 && voices_busy(); b++) render();
    check("operator-envelope release ends all voices", voices_busy() == 0);
    fresh(); check("reset recreates eight independent operator bodies", ownership() && render() == 8);
    memset(trk, 0, sizeof trk); host_tracks_init(); host_preset(&trk[0], ENGI_FM6, 0); trk[0].p[P_VOICE] = V_POLY;
    for (uint32_t i = 0; i < 20; i++) trk_note_on(&trk[0], 36 + i, 100);
    check("one track retains its original six-voice cap", voices_busy() == FM6_POLY && ownership() && render() == FM6_POLY);
    printf("FM6 state pool: %d failures, %zu shared heavy-pool host bytes\n", bad, sizeof heavy_pool);
    return !!bad;
}
