/* SPDX-License-Identifier: GPL-3.0-only
 * Real eight-part reverse-window isolation and exact decoder checks. */
#if NPART != 8
#error Build with -DNPART=8
#endif
#define main hostsim_main
#include "hostsim.c"
#undef main
static int bad;
static void check(const char *name, int ok)
{
    printf("SLICE pool: %s %s\n", name, ok ? "PASS" : "FAIL"); bad += !ok;
}
static void fresh(uint32_t reverse, uint32_t src, uint32_t distinct)
{
    memset(trk, 0, sizeof trk); memset(&song, 0, sizeof song);
    memset(slc_owner, 0, sizeof slc_owner); memset(slc_rbuf, 0, sizeof slc_rbuf);
    host_tracks_init();
    for (uint32_t p = 0; p < NPART; p++) {
        host_preset(&trk[p], ENGI_SLICE, 0); trk[p].p[P_VOICE] = V_POLY;
        trk[p].p[P_E0] = src; trk[p].p[P_E5] = reverse;
        trk[p].p[P_E4] = SLC_LOOP;
        trk_note_on(&trk[p], SLC_BASE + (distinct ? p : 0), 100);
    }
}
static int ownership(void)
{
    int16_t *used[NVOICE]; uint32_t n = 0;
    for (uint32_t p = 0; p < NPART; p++)
        for (uint32_t i = 0; i < NVOICE; i++) {
            voice_t *v = &trk[p].v[i];
            if (!v->active || trk[p].engine != ENGI_SLICE || !((uint32_t)v->s[4] & 128u)) continue;
            int16_t *window = slc_rb(&trk[p], v);
            if (!window || n == NVOICE) return 0;
            for (uint32_t k = 0; k < n; k++) if (used[k] == window) return 0;
            used[n++] = window;
        }
    return 1;
}
static int exact_reverse(void)
{
    static int32_t fw[NPART][65536]; uint32_t n[NPART], longest = 0;
    const slc_src_t *s = slc_get(0);
    fresh(1, 0, 1);
    for (uint32_t p = 0; p < NPART; p++) {
        uint32_t a, b, st; slc_dec_t d;
        slc_bounds_v(s, &trk[p].v[0], &a, &b, &st); n[p] = b - a;
        if (n[p] > 65536) return 0;
        if (n[p] > longest) longest = n[p];
        slc_dec_at(&d, a, st);
        for (uint32_t i = 0; i < n[p]; i++) fw[p][i] = slc_dec_next(s, &d);
    }
    /* Interleave every sample to expose cross-track overwrites at window boundaries. */
    for (uint32_t i = 0; i < longest; i++)
        for (uint32_t p = 0; p < NPART; p++) if (i < n[p]) {
            int32_t x;
            if (!slc_rev(s, &trk[p].v[0], slc_rb(&trk[p], &trk[p].v[0]), 0, &x) || x != fw[p][n[p] - i - 1]) return 0;
        }
    for (uint32_t p = 0; p < NPART; p++) {
        int32_t x; if (slc_rev(s, &trk[p].v[0], slc_rb(&trk[p], &trk[p].v[0]), 0, &x)) return 0;
    }
    return 1;
}
static int render_isolation(uint32_t reverse, uint32_t src)
{
    int32_t reference[CTL], out[CTL]; uint32_t nonzero = 0;
    fresh(reverse, src, 0);
    for (uint32_t b = 0; b < 100; b++) {
        track_render(&trk[0], reference, CTL);
        for (uint32_t i = 0; i < CTL; i++) nonzero += reference[i] != 0;
        for (uint32_t p = 1; p < NPART; p++) {
            track_render(&trk[p], out, CTL);
            if (memcmp(reference, out, sizeof out)) return 0;
        }
        if (voices_busy() > 8) return 0;
    }
    if (!reverse) for (uint32_t k = 0; k < NVOICE; k++) if (slc_owner[k]) return 0;
    return nonzero != 0;
}
int main(void)
{
    fresh(1, 0, 1);
    check("eight reverse notes own distinct windows", ownership() && voices_busy() == 8);
    check("window storage is eight bodies", sizeof slc_rbuf == NVOICE * SLC_RB * sizeof(int16_t));
    check("interleaved reverse samples equal forward decode reversed", exact_reverse());
    check("eight reverse renderers produce isolated identical BREAK loops", render_isolation(1, 0));
    check("forward BREAK render keeps its predictor and claims no reverse window", render_isolation(0, 0));
    check("forward PIANO render keeps its predictor and claims no reverse window", render_isolation(0, SLC_SRC_PIANO));
    check("reverse PIANO rendering has independent windows", render_isolation(1, SLC_SRC_PIANO));

    fresh(1, 0, 1); int16_t snapshot[SLC_RB]; int32_t x;
    slc_rev(slc_get(0), &trk[0].v[0], slc_rb(&trk[0], &trk[0].v[0]), 0, &x);
    memcpy(snapshot, slc_rb(&trk[0], &trk[0].v[0]), sizeof snapshot);
    trk_note_on(&trk[7], SLC_BASE + 7, 100);
    check("another track's retrigger cannot overwrite a cached window", !memcmp(snapshot, slc_rb(&trk[0], &trk[0].v[0]), sizeof snapshot));
    trk_note_on(&trk[7], SLC_BASE + 10, 100);
    check("ninth admitted note reuses retired state without aliasing", ownership() && voices_busy() == 8);
    fresh(1, 0, 1); trk[0].engine = trk[0].eng_req = 0;
    voice_t *v = &trk[7].v[1]; v->active = 1; v->s[0] = 23;
    check("changed-engine reclamation invalidates the prior decoded window", slc_rb(&trk[7], v) && v->s[0] == 0x7FFFFFFF);
    v->active = 0; trk_all_off(&trk[0]); engine_block(&trk[0]);
    host_preset(&trk[0], ENGI_SLICE, 0); trk[0].p[P_E5] = 1; trk_note_on(&trk[0], SLC_BASE, 100);
    check("return to SLICE cannot alias a reassigned body", ownership());
    fresh(1, 0, 1); trk[0].v[1].active = 1;
    check("unadmitted ninth window is refused", slc_rb(&trk[0], &trk[0].v[1]) == 0);
    trk[0].v[1].active = 0;
    uint32_t seed = 5, errors = 0; int32_t out[CTL];
    for (uint32_t k = 0; k < 5000; k++) {
        seed = seed * 1664525u + 1013904223u; uint32_t p = seed >> 29;
        trk_note_on(&trk[p], SLC_BASE + seed % 16, 100);
        uint32_t rendered = 0;
        for (uint32_t j = 0; j < NPART; j++) rendered += track_render(&trk[j], out, CTL);
        if (!ownership() || voices_busy() > 8 || rendered > 8) { errors++; break; }
    }
    check("5000 reverse-loop steals preserve bounded unique windows", !errors);
    for (uint32_t p = 0; p < NPART; p++) trk_all_off(&trk[p]);
    for (uint32_t b = 0; b < 5000 && voices_busy(); b++) for (uint32_t p = 0; p < NPART; p++) track_render(&trk[p], out, CTL);
    check("all releases finish", !voices_busy());
    printf("SLICE window pool: %d failures, %zu bytes for eight windows\n", bad, sizeof slc_rbuf);
    return !!bad;
}
