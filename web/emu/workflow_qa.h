/* SPDX-License-Identifier: GPL-3.0-only */
/* Read-only observations for panel-driven emulator regression tests. */
EXPORT uint32_t web_qa_tracks(void) { return NTRK; }
EXPORT uint32_t web_qa_selected(void) { return song.sel; }
EXPORT int32_t web_qa_level(uint32_t p) { return p < NTRK ? trk[p].p[P_LEVEL] : -1; }
EXPORT uint32_t web_qa_track_signature(uint32_t p)
{
    if (p >= NTRK) return 0;
    uint32_t h = as_mix(2166136261u, trk[p].p, sizeof trk[p].p);
    h = as_mix(h, trk[p].step, sizeof trk[p].step);
    return as_mix(h, &trk[p].eng_req, sizeof trk[p].eng_req);
}
EXPORT uint32_t web_qa_steps(uint32_t p)
{
    uint32_t n = 0;
    if (p < NTRK) for (uint32_t i = 0; i < NSTEP; i++) n += step_on(&trk[p].step[i]);
    return n;
}
EXPORT uint32_t web_qa_step_note(uint32_t p, uint32_t s)
{ return p < NTRK && s < NSTEP ? trk[p].step[s].note[0] : 0; }
EXPORT uint32_t web_qa_active_tracks(void)
{
    uint32_t mask = 0;
    for (uint32_t p = 0; p < NTRK; p++) for (uint32_t i = 0; i < NVOICE; i++)
        if (trk[p].v[i].active) mask |= 1u << p;
    return mask;
}
EXPORT uint32_t web_qa_active_voices(void)
{
    uint32_t n = 0;
    for (uint32_t p = 0; p < NTRK; p++) for (uint32_t i = 0; i < NVOICE; i++) n += trk[p].v[i].active != 0;
    return n;
}
EXPORT const char *web_qa_message(void) { return ui.msg; }
EXPORT uint32_t web_qa_slot_selection(void) { return song.g[G_SLOT]; }
EXPORT uint32_t web_qa_graph(void) { return ui.home ? 0 : cur_page()->graph; }
EXPORT uint32_t web_qa_name_open(void) { return name_on(); }
EXPORT uint32_t web_qa_confirmation(void) { return ui.confirm != 0; }
EXPORT const char *web_qa_current_name(void) { return proj_name; }
EXPORT const char *web_qa_project_name(uint32_t p)
{
    static char name[13];
    memset(name, 0, sizeof name);
    if (p < PROJECT_UI_SLOTS) project_name(p, name);
    return name;
}
EXPORT uint32_t web_qa_project_slot(void) { return proj_cur; }
EXPORT int32_t web_qa_bpm(void) { return song.g[G_BPM]; }
EXPORT uint32_t web_qa_palette(void) { return settings.palette; }
#if NTRK == 8
EXPORT uint32_t web_qa_quiet(void) { return ls_quiet(); }
EXPORT const int16_t *web_qa_fx_ap(void) { return rev_ap; }
EXPORT const int16_t *web_qa_fx_comb(void) { return rev_comb; }
EXPORT const int32_t *web_qa_fx_state(void)
{
    static int32_t out[30];
    int32_t state[30] = {dc_l, dc_r, fx.dly_lp, fx.sp_lp, fx.sp_hp,
        fx.comb_lp[0], fx.comb_lp[1], fx.comb_lp[2], fx.comb_lp[3],
        fx_tail.delay, fx_tail.chorus, fx_tail.comb, fx_tail.ap,
        fx_rev_wait.comb, fx_rev_wait.ap, fx_rev_wait.model,
        fx_uniform_wait.delay, fx_uniform_wait.comb, fx_uniform_wait.ap,
        fx_uniform_wait.ap_fixed, fx_uniform_wait.delay_span, delay_samples(), pf.src, PF_N, dce_l, dce_r, song.g[G_RDAMP], song.g[G_RSIZE], song.master_q12, song.g[G_DMIX]};
    memcpy(out, state, sizeof out); return out;
}
EXPORT uint32_t web_qa_fx_blockers(void)
{
    uint32_t mask = ((uint32_t)fx.sp_he >= 64u || (uint32_t)dce_l >= 4096u || (uint32_t)dce_r >= 4096u);
    mask |= (!!(lc_l1 || lc_l2 || lc_r1 || lc_r2)) << 1;
    mask |= (!!(sb_lp1 || sb_lp2 || sb_lp3 || sb_lp4 || sb_env || sb_h1 || sb_h2 || sb_hl)) << 2;
    mask |= (!!(click_req || clk.env)) << 3;
    for (unsigned i = 0; i < 4; i++) if ((uint32_t)lce[i] >= 32u) mask |= 16;
    for (unsigned p = 0; p < NTRK; p++) if (trk[p].tail && (trk[p].dist_hp || trk[p].dist_lp1 || trk[p].dist_lp2)) mask |= 32;
    if (!fx_stationary_quiet()) mask |= 64;
    return mask;
}
EXPORT uint32_t web_qa_quiet_flags(void) { return (!autosave_quiet()) | (!fx_resident_quiet() << 1) | (!audio_output_quiet() << 2) | (!mc_inputs_quiet() << 3); }
EXPORT uint32_t web_first_use_phase(void) { return d8fi.phase; }
EXPORT uint32_t web_qa_first_use(void) { return d8fi.phase; }
EXPORT uint32_t web_qa_storage_status(void) { return project_native_status(); }
EXPORT uint32_t web_qa_intent(void) { return d8ls.intent; }
EXPORT uint32_t web_qa_consent(void) { return d8ls.phase; }
EXPORT uint32_t web_qa_autosave_writes(void) { return native_as.writes; }
/* Manufactured audio load only; panel/storage workflows never call this helper. */
EXPORT void web_test_eight_active(void)
{
    for (uint32_t p = 0; p < NTRK; p++) {
        track_t *t = &trk[p];
        host_preset(t, 0, 0);
        t->p[P_VOICE] = V_POLY;
        t->p[P_SUS] = 127;
        for (uint32_t i = 0; i < 16; i++) {
            uint8_t note[4] = {(uint8_t)(48 + p + i / 4), 0, 0, 0};
            put_step(t, i, 1, note, ST_NOTE, 0);
        }
    }
    ui.force = 1;
}
#endif
