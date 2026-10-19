/* SPDX-License-Identifier: GPL-3.0-only */
/* Synthetic C-only source references for a future separately authorized test.
 * All time, DMA, input and NOR are simulated. No device adapter exists here. */
#define D8ARR_TEST_NO_MAIN 1
#include "native_arrangement_test.c"
#include "native_queue_audio_fixture.h"

static uint8_t proposal[D8P1_LIMIT], after[D8P1_LIMIT];
static size_t proposal_n, after_n;
static uint64_t samples;
static unsigned epoch, scenes_seen, peak_voice, all_gates, output_seen, ui_frames;

static void render_half(void)
{
    fm1_ms = epoch + (uint32_t)(samples * 1000u / FS);
    host_ticks = (uint32_t)(samples * 1000000u / FS);
    unsigned r = reads, w = writes;
    fm1_alnk0_irq();
    proof(reads == r && writes == w, "no source NOR access inside audio interrupt");
    unsigned voices = voices_busy();
    if (voices > peak_voice) peak_voice = voices;
    for (unsigned t = 0; t < 8; t++) for (unsigned v = 0; v < NVOICE; v++)
        if (trk[t].v[v].gate) all_gates |= 1u << t;
    for (unsigned i = 0; i < HALF_WORDS; i++) output_seen |= abuf[qa_half * HALF_WORDS + i] != 0;
    if (d8arr_running()) scenes_seen |= 1u << chain.native.policy.scene;
    qa_half ^= 1u; samples += HALF_FRAMES;
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    seed(); seq_stop();
    static const unsigned engines[8] = {12,12,12,12,12,12,10,7};
    for (unsigned t = 0; t < 8; t++) {
        proof(!set_engine_of(&trk[t], engines[t]), "supported proposal engine");
        apply_preset_to(&trk[t], 0);
        trk[t].p[P_VOICE] = V_MONO; trk[t].p[P_REL] = 0;
        trk[t].p[P_AMODE] = trk[t].p[P_AHOLD] = trk[t].p[P_CHRD] = 0;
        trk[t].p[P_LEVEL] = 48; trk[t].p[P_MUTE] = 0;
        trk[t].p[P_DIST] = 32;
        trk[t].p[P_CHOR] = trk[t].p[P_DLY] = trk[t].p[P_REV] = 48;
        trk[t].p[P_SLEN] = 16; trk[t].p[P_SDIV] = 2;
        memset(trk[t].step, 0, sizeof trk[t].step);
        for (unsigned k = 0; k < NSTEP; k++) {
            step_t *s = &trk[t].step[k];
            s->time = ST_NOTE; s->n = 1; s->vel = 64;
            s->note[0] = (uint8_t)(t == 6 ? (k & 1u ? 42 : 36) : 48 + t);
        }
    }
    memset(&motion, 0, sizeof motion);
    song.sel = 0; song.rec = 0; song.g[G_BPM] = 120;
    song.g[G_ROUTE] = song.g[G_CLOCK] = song.g[G_RTYPE] = 0;
    song.g[G_RSIZE] = 64; song.g[G_DFDBK] = 48; song.g[G_SLOT] = 3;
    song.master_q12 = 2048; cin_bars = 0; click_mode = CLICK_ON;
    memset(proj_name, 0, sizeof proj_name); memcpy(proj_name, "RC ARRANGE", 10);
    mix_block(audio, CTL);
    d8p1_arrangement *a = &d8p1_runtime_cache.arrangement;
    memset(a, 0, sizeof *a); a->banks = 4; a->scenes = a->rows = 16;
    for (unsigned b = 0; b < 4; b++) for (unsigned t = 0; t < 8; t++) {
        a->bank[b].project[t] = 2; a->bank[b].track[t] = (uint8_t)t;
    }
    for (unsigned s = 0; s < 16; s++) {
        a->scene[s].bank = (uint8_t)(s % 4); a->scene[s].apply = 7;
        for (unsigned t = 0; t < 8; t++) {
            a->scene[s].level[t] = 48;
            a->scene[s].pan[t] = (int8_t)((int)t * 16 - 56);
            a->scene[s].transpose[t] = t == 6 ? 0 : (int8_t)((int)(s % 3) * 2 - 2);
        }
        a->row[s].scene = (uint8_t)s; a->row[s].repeat = 1;
    }
    proof(!d8p1_capture_runtime(proposal, sizeof proposal, &proposal_n), "capture synthetic proposal");
    d8p1_view view;
    proof(d8p1_read(&view, proposal, proposal_n) == 1 && d8p1_refs_available(&view, 4) &&
          !d8p1_refs_available(&view, 3), "only saved C is required, never A or B");
    proof(!d8pool_save(&pool, 2, proposal, proposal_n, 4), "seed exact proposal into virtual C");
    proof(!d8p1_load_pool(&arr_pool, 0), "load another native project before the C recall");
    mix_block(audio, CTL);
    proof(!d8p1_load_pool(&arr_pool, 2), "actual native C recall adopts arrangement and sounds");
    mix_block(audio, CTL);
    memcpy(baseline, nor, sizeof nor); reset();
    queue_audio_dbg.boots = 17;
    proof(d8hp_start(17) == 1 && audio_timing_start(), "start simulated ownership and ISR observers");
    ui.home = 1; hold(B_SEQ);
    proof(!ui.home && cur_page()->graph == GR_SONG, "holding SEQ opens SONG without editing music");
    press(B_PLAY);
    proof(chain.armed && transport_req == 1 && !song.playing, "actual SONG PLAY prepares sources before audio starts");
    epoch = fm1_ms;
    do {
        render_half();
        if (qa_acks % 6u == 0) { ui.force = 1; ui_draw(); ui_frames++; }
    } while (song.playing && samples < (uint64_t)FS * 36u && !failures);
    proof(!song.playing && !chain.running && scenes_seen == 65535u, "all16scenes stop naturally without more owner actions");
    unsigned quiet = 0;
    for (unsigned i = 0; i < FS * 12u / HALF_FRAMES; i++) {
        render_half();
        if (!voices_busy() && audio_output_quiet()) { quiet = 1; break; }
    }
    lcd_sync();
    proof(quiet && !dma_errors && ui_frames > 1000u, "bounded silence and actual asynchronous drawing");
    proof(peak_voice == 8 && all_gates == 255u && output_seen, "eight track gates and audible mixed output");
    proof(d8hp.report.peak_live_owned_slots == 8 && !d8hp.report.failures && !d8hp.report.invalid,
          "eight heavy slots without allocation failure");
    proof(!queue_audio_dbg.late && !audio_timing.invalid && !audio_timing.taint,
          "simulated ISR returns cleanly with valid observation");
    proof(!d8p1_capture_runtime(after, sizeof after, &after_n) && after_n == proposal_n &&
          !memcmp(after, proposal, proposal_n), "editable project is exact after Stop");
    proof(!writes && !memcmp(baseline, nor, sizeof nor), "virtual source set unchanged");
    proof(audio_timing_stop() && d8hp_stop(), "observers closed after bounded cleanup");
    if (failures) return 1;
    FILE *f = fopen(argv[1], "wbx");
    if (!f) return 2;
    int failed = fwrite(proposal, 1, proposal_n, f) != proposal_n;
    if (fclose(f)) failed = 1;
    printf("arrangement-proposal checks=%u failures=%u bytes=%zu halves=%u voices=%u pool=%u ui=%u simulated_only=1\n",
           checks, failures, proposal_n, qa_acks, peak_voice, d8hp.report.peak_live_owned_slots, ui_frames);
    return failed ? 2 : 0;
}
