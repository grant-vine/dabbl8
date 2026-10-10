/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual UI input, layer ownership and instrumented screen-render checks. */
#define UI_RENDER_NO_MAIN 1
#include "ui_render.c"
_Static_assert(NTRK == 8 && NVOICE == 8, "eight tracks retain eight shared voices");

static int bank_gestures(void)
{
    int bad = 0, ok;
    cur_name = "eight-track gestures";
    ui_power_on();
    for (uint32_t i = 1; i < 8u; i++) turn(EN_ALGO, 1);
    bad += check("ALGORITHM reaches track eight and selects bank 5-8", song.sel == 7u && mixer_bank() == 4u);
    turn(EN_ALGO, 1); bad += check("selection cannot exceed track eight", song.sel == 7u);
    for (uint32_t i = 0; i < 8u; i++) {
        ui_power_on(); track_select(i); press(B_REC);
        bad += check("REC arms its exact selected destination in either bank", song.rec == (1u << i));
    }
    for (uint32_t bank = 0; bank < 8u; bank += 4u) {
        ui_power_on(); track_select(bank);
        for (uint32_t i = 0; i < NTRK; i++) { trk[i].p[P_LEVEL] = 60; trk[i].p[P_MUTE] = (int16_t)(i & 1u); }
        song.g[G_BPM] = 124; btn_down(B_GLO);
        for (uint32_t k = 0; k < 4u; k++) turn(EN_K1 + k, 3);
        ok = ui.layer == LAYER_GLO && layer_open() == LAYER_GLO;
        for (uint32_t i = 0; i < NTRK; i++) ok &= trk[i].p[P_LEVEL] == ((i >= bank && i < bank + 4u) ? 63 : 60);
        bad += check("four global knobs edit only the selected four-track bank", ok);
        for (uint32_t i = 0; i < NTRK; i++) { trk[i].p[P_LEVEL] = (int16_t)(90 + i); trk[i].p[P_MUTE] ^= 1; }
        song.g[G_BPM] = 177; press(B_OCTDN); ok = song.g[G_BPM] == 124;
        for (uint32_t i = 0; i < NTRK; i++) ok &= trk[i].p[P_LEVEL] == 60 && trk[i].p[P_MUTE] == (int16_t)(i & 1u);
        bad += check("global undo restores all eight levels/mutes and BPM without overlap", ok);
        btn_up(B_GLO); frame();
        for (uint32_t p = 0; p < 4u; p++) {
            ui_power_on(); track_select(bank); btn_down(B_GLO); key_down(key_at(1, p)); frame();
            ok = trk[bank + p].p[P_MUTE] == 1;
            for (uint32_t i = 0; i < NTRK; i++) if (i != bank + p) ok &= !trk[i].p[P_MUTE];
            bad += check("global black-key mute has one exact bank destination", ok);
            key_up(key_at(1, p)); btn_up(B_GLO); frame();
        }
    }
    ui_power_on(); track_select(4); btn_down(B_GLO); key_down(white(0)); frame();
    ok = perf_solo == 0x10u; track_select(0); frame(); ok &= perf_solo == 0x10u;
    key_up(white(0)); frame();
    bad += check("held global solo retains its key-down bank through selection changes", ok && !perf_solo);
    btn_up(B_GLO); frame();
    ui_power_on(); track_select(7); for (uint32_t i = 0; i < NTRK; i++) trk[i].p[P_MUTE] = 1;
    btn_down(B_GLO); key_down(white(4)); frame(); ok = !perf_solo;
    for (uint32_t i = 0; i < NTRK; i++) ok &= !trk[i].p[P_MUTE];
    bad += check("C4 unmute-all keeps its role in the upper bank", ok);
    key_up(white(4)); key_down(white(7)); frame();
    bad += check("F4 remains tap tempo rather than track-eight solo", lys.ntap == 1u && !perf_solo);
    key_up(white(7)); btn_up(B_GLO); frame();
    for (uint32_t p = 0; p < NTRK; p++) {
        ui_power_on(); track_select(7); btn_down(B_FX); key_down(key_at(1,p)); frame();
        bad += check("FX black keys retain eight independent mute owners", perf_held == PF_BIT(PF_M1 + p));
        key_up(key_at(1,p)); btn_up(B_FX); frame();
    }
    ui_power_on(); track_select(7); btn_down(B_FX); key_down(white(4)); frame();
    bad += check("FX white-key LPF role is unchanged on track eight", perf_held == PF_BIT(PF_LPF) && !gates());
    key_up(white(4)); btn_up(B_FX); frame();
    return bad;
}

static int bank_renders(const char *out)
{
    int scenes[S_COUNT]; for (uint32_t i = 0; i < S_COUNT; i++) scenes[i] = (int)i;
    uint32_t frames_checked = 0, bank_labels_checked = 0, bank_labels_bad = 0;
    nfind = nink = mono_bad = al_fail = al_lost = al_n = al_count = 0;
    for (uint32_t large = 0; large < 2u; large++) for (uint32_t style = 0; style < 2u; style++)
    for (uint32_t p = 0; p < NPALETTES; p++) for (uint32_t tr = 0; tr < NTRK; tr++)
    for (uint32_t j = 0; j < sizeof scenes/sizeof scenes[0]; j++) {
        if (!FELUCCA_FM4 && (scenes[j] == S_OP_ENV || (scenes[j] >= S_ALG1 && scenes[j] <= S_OP_LEVEL))) continue;
        char name[128];
        snprintf(name,sizeof name,"%s/large%u/style%u/T%u/%s",UI_PALETTES[p].name,large,style,tr+1,S_NAME[scenes[j]]);
        cur_name = name; large_on = large; setup(scenes[j]);
        uint32_t source_tr = song.sel;
        if (source_tr != tr) {
            const track_t *source = &trk[source_tr];
            host_preset(&trk[tr], source->eng_req, source->preset);
            memcpy(trk[tr].p, source->p, sizeof trk[tr].p);
            memcpy(trk[tr].step, source->step, sizeof trk[tr].step);
            trk[tr].user = source->user;
            trk[tr].seq_idx = source->seq_idx;
            memcpy(fm6_patch[tr], fm6_patch[source_tr], sizeof fm6_patch[tr]);
            fm6_slot[tr] = fm6_slot[source_tr]; fm6_pgen[tr]++;
        }
        song.sel = (uint8_t)tr;
        pal(p); ui_style = (uint8_t)style; style_apply();
        if (scenes[j] == S_MIXER || scenes[j] == S_MIXER_PAN) {
            song.rec = 0xA5u;
            for (uint32_t i = 0; i < NTRK; i++) { trk[i].p[P_MUTE] = (int16_t)(i & 1u); trk[i].peak = (int32_t)(i+1)*3500; }
        }
        draw(scenes[j]); lint(); mono_check(); frames_checked++;
        if (scenes[j] == S_MIXER || scenes[j] == S_MIXER_PAN || scenes[j] == S_GLO_ACTIVE || scenes[j] == S_FX_HELD) {
            const char *expected = tr < 4u ? "1-4" : "5-8";
            const char *wrong = tr < 4u ? "5-8" : "1-4";
            uint32_t found = 0, stale = 0;
            for (uint32_t i = 0; i < nscr; i++) {
                found += !strcmp(scr[i].s, expected);
                stale += !strcmp(scr[i].s, wrong);
            }
            bank_labels_checked++;
            if (found != 1u || stale) {
                fprintf(rep, "bank label mismatch %s: expected %s once, found %u, stale %u\n", name, expected, found, stale);
                bank_labels_bad++;
            }
        }
        if (out && p == UI_GREY_INDEX && tr == 7u && style == 0 && (scenes[j] == S_MIXER || scenes[j] == S_GLO_ACTIVE || scenes[j] == S_FX_HELD)) {
            char tag[24]; snprintf(tag,sizeof tag,"large%u",large); write_ppm(out,tag,S_NAME[scenes[j]]);
        }
    }
    printf("banked mixer renders: %u frames, %u lint findings, %u ink errors, %u neutral-color errors, %u alignment failures, %u lost declarations\n",
           frames_checked,nfind,nink,mono_bad,al_fail,al_lost);
    printf("bank labels: %u rendered checks, %u failures\n", bank_labels_checked, bank_labels_bad);
    return !!(nfind || nink || mono_bad || al_fail || al_lost || bank_labels_bad);
}

int main(int argc,char **argv)
{
    const char *out = argc > 1 ? argv[1] : 0;
    char path[1024];
    if (out) { snprintf(path,sizeof path,"%s/render-report.txt",out); rep=fopen(path,"w"); }
    else rep=tmpfile();
    if (!rep) return 2;
    int bad = bank_gestures();
    bad += bank_renders(out);
    fclose(rep);
    printf("banked mixer: %d failures; prototype, shipping remains four tracks\n",bad);
    return !!bad;
}
