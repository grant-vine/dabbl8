/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual shared historical UI fixture copy and real importer; no device I/O. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static unsigned checks, failures;
static void expect(int ok) { checks++; failures += !ok; }
int main(void)
{
    project_t input, before, output;
    struct { uint32_t pre[8]; project_v6_t v; uint32_t post[8]; } guarded;
    ui_power_on();
    for (uint32_t t = 0; t < NTRK; t++) {
        set_engine_of(&trk[t], 0); apply_preset_to(&trk[t], 0);
        my_steps(&trk[t]); trk[t].p[P_LEVEL] = 51 + t; trk[t].p[P_REV] = 21 + t;
    }
    project_capture(&input); before = input;
    memset(&guarded, 0, sizeof guarded);
    for (unsigned k = 0; k < 8; k++) guarded.pre[k] = guarded.post[k] = 0xa5c33ca5u;
    guarded.v.magic = PROJ_MAGIC_V6; guarded.v.size = sizeof guarded.v;
    guarded.v.parts = PROJ_LEGACY_NTRK; guarded.v.phys = PROJ_PHYS;
    memcpy(guarded.v.g, input.g, sizeof guarded.v.g);
    ui_legacy_v6_tracks(&guarded.v, &input);
    expect(sizeof guarded.v.t / sizeof guarded.v.t[0] == 4u);
    expect(PROJ_LEGACY_NTRK == 4u);
    for (unsigned k = 0; k < 8; k++) expect(guarded.pre[k] == 0xa5c33ca5u && guarded.post[k] == 0xa5c33ca5u);
    expect(!memcmp(&before, &input, sizeof input));
    for (unsigned t = 0; t < 4; t++) {
        expect(!memcmp(guarded.v.t[t].p, input.t[t].p, 61u * sizeof(int16_t)));
        expect(!memcmp(guarded.v.t[t].p + 61, input.t[t].p + P_E0, 8u * sizeof(int16_t)));
        for (unsigned s = 0; s < NSTEP; s++) expect(!memcmp(&guarded.v.t[t].step[s], &input.t[t].step[s], sizeof(step10_t)));
    }
    guarded.v.sum = proj_hash(&guarded.v, offsetof(project_v6_t, sum));
    memset(&output, 0x6d, sizeof output);
    expect(proj_import(&output, &guarded.v, sizeof guarded.v));
    for (unsigned t = 0; t < 4; t++) {
        expect(output.t[t].engine == 0 && output.t[t].preset == input.t[t].preset);
        expect(output.t[t].p[P_LEVEL] == 51 + (int)t && output.t[t].p[P_REV] == 21 + (int)t);
        for (unsigned s = 0; s < NSTEP; s++) {
            expect(!memcmp(output.t[t].step[s].note, input.t[t].step[s].note, 4u));
            expect(output.t[t].step[s].flags == (input.t[t].step[s].flags & 3u));
        }
    }
    /* Import does not invent historical tracks 5..8; FM6 bank defaults are separate. */
    for (unsigned t = 4; t < NTRK; t++) {
        proj_trk_t zero = {0};
        expect(!memcmp(&output.t[t], &zero, sizeof zero));
        expect(!memcmp(output.fm6[t], FM6_INIT, FM6_PACKED));
    }
    expect(!memcmp(&before, &input, sizeof input));
    printf("UI legacy FUN6 fixture: %u checks, %u failures; tracks=%u, historical=4, extra canonical tracks zero\n",checks,failures,NTRK);
    return failures != 0;
}
