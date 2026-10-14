/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual forward/reverse render source admission; independent allocated slots. */
#define SAMPLE_SOURCE_RANGE_NO_MAIN 1
#include "sample_source_range_test.c"
int main(int argc, char **argv)
{
    FILE *pcm = argc == 2 || argc == 3 ? fopen(argv[1], "wb") : 0;
    FILE *state = argc == 3 ? fopen(argv[2], "wb") : 0;
    if (!pcm || (argc == 3 && !state)) return 2;
    for (unsigned k = 0; k < SMP_USER_SLOTS; k++) {
        range_slots[k] = malloc(SMP_USER_SIZE);
        if (!range_slots[k]) return 2;
        range_case(k, 0);
        smp_user_hdr_t *h = (smp_user_hdr_t *)range_slots[k];
        h->zone[0].off = 17;
        range_allow(range_slots[k] + SMP_USER_DATA, h->data_len);
        smp_user_scan(k);
        range_expect(usr_nz[k] == 2);
        uint32_t start = 0;
        range_expect(slc_man_restore(k, 1, 0, &start) == 0);
    }
    for (unsigned src = 0; src <= SLC_SRC_PIANO; src++) for (unsigned rev = 0; rev < 2; rev++) {
        ui_power_on();
        track_t *t = &trk[0]; host_preset(t, 13, 0);
        t->p[P_E0] = src; t->p[P_E1] = src >= 1 && src <= 3 ? SLC_DIV_MAN : 0; t->p[P_E4] = SLC_LOOP; t->p[P_E5] = rev;
        if (src >= 1 && src <= 3) {
            smp_user_hdr_t *h = (smp_user_hdr_t *)range_slots[src - 1];
            range_allow(range_slots[src - 1] + SMP_USER_DATA, h->data_len);
        } else range_allow(SMP_DATA, sizeof SMP_DATA);
        trk_note_on(t, SLC_BASE, 100);
        range_expect(t->v[0].active && ((unsigned)t->v[0].s[4] & 7u) == src);
        int32_t out[CTL];
        uint32_t boundary = src >= 1 && src <= 3 ? slc_usr[src - 1].seg[1].at : 0;
        unsigned saw_first = 0, saw_second = 0;
        for (unsigned block = 0; block < 80; block++) {
            track_render(t, out, CTL);
            saw_first |= t->v[0].ph[0] < boundary;
            saw_second |= t->v[0].ph[0] >= boundary;
            if (fwrite(out, sizeof out, 1, pcm) != 1) return 2;
            if (state) {
                const voice_t *v = &t->v[0];
                uint32_t flags[] = {v->active, v->gate, v->stage};
                if (fwrite(v->ph, sizeof v->ph, 1, state) != 1 ||
                    fwrite(v->s, sizeof v->s, 1, state) != 1 ||
                    fwrite(flags, sizeof flags, 1, state) != 1) return 2;
            }
        }
        range_expect(!range_invalid && range_reads);
        range_expect(!boundary || (saw_first && saw_second));
    }
    /* Unsupported packed voice source5/6 ends before reading any sample. */
    for (unsigned src = 5; src <= 6; src++) {
        ui_power_on(); track_t *t = &trk[0]; host_preset(t, 13, 0);
        trk_note_on(t, SLC_BASE, 100); t->v[0].s[4] = src; t->v[0].s[6] = 0;
        range_allow(SMP_DATA, sizeof SMP_DATA); int32_t out[CTL];
        track_render(t, out, CTL);
        range_expect(!range_reads && !t->v[0].active);
    }
    for (unsigned k = 0; k < SMP_USER_SLOTS; k++) free(range_slots[k]);
    fclose(pcm);
    if (state) fclose(state);
    printf("SLICE actual source renders: %u checks, %u failures; 0/4 builtin, 1..3 disjoint user, forward/reverse, 5/6 refuse; tracks=%u\n",range_checks,range_bad,NTRK);
    return range_bad != 0;
}
