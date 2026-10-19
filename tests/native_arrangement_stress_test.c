/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual audio ISR, native arrangement, FX and UI; clocks/DMA/NOR are simulated.
 * This is not target timing, physical MIDI routing or stack qualification. */
#define D8ARR_TEST_NO_MAIN 1
#include "native_arrangement_test.c"
#include "native_queue_audio_fixture.h"
#include <assert.h>

static const struct scenario {
    const char *name;
    unsigned clock, effects, spring, interactive, loss;
} scenarios[] = {
    {"internal-dry", 0, 0, 0, 0, 0},
    {"internal-room", 0, 1, 0, 1, 0},
    {"usb-spring", 1, 1, 1, 0, 0},
    {"trs-loss-wrap", 2, 1, 0, 0, 1}
};
static const struct scenario *scenario;
static uint64_t samples;
static unsigned epoch, next_clock, held, live_key, next_note, frames_drawn;
static unsigned peak_voices, scene_mask, sounded_tracks, audible, wet_audio, sends[3];
static unsigned fill_queued, fill_ran, fill_returned, row_queued, row_applied, clock_lost;
static unsigned live_inputs, live_observed, lcd_started;
static uint8_t initial_selection;
static uint8_t initial[D8P1_LIMIT], final[D8P1_LIMIT];
static size_t initial_size, final_size;

static void realtime(unsigned status)
{
    if (scenario->clock == 2) um_byte((uint8_t)status);
    else midi_in_event(15u | status << 8);
}

static void live_note(unsigned velocity)
{
    unsigned status = (velocity ? 0x90u : 0x80u) | 7u;
    if (scenario->clock == 2) {
        um_byte((uint8_t)status); um_byte((uint8_t)live_key); um_byte((uint8_t)velocity);
    } else midi_in_event((status >> 4) | status << 8 | live_key << 16 | velocity << 24);
    held = velocity != 0;
    if (held) live_inputs++;
}

static void setup(void)
{
    seed();
    static const unsigned engines[8] = {12,12,12,12,12,12,10,7};
    for (unsigned t = 0; t < 8; t++) {
        proof(!set_engine_of(&trk[t], engines[t]), "supported stress destination");
        apply_preset_to(&trk[t], 0);
        trk[t].p[P_VOICE] = V_MONO; trk[t].p[P_REL] = 0;
        trk[t].p[P_AMODE] = trk[t].p[P_AHOLD] = trk[t].p[P_CHRD] = 0;
        trk[t].p[P_LEVEL] = 48; trk[t].p[P_MUTE] = 0;
        trk[t].p[P_DIST] = scenario->effects ? 32 : 0;
        trk[t].p[P_CHOR] = trk[t].p[P_DLY] = trk[t].p[P_REV] = scenario->effects ? 48 : 0;
    }
    song.g[G_ROUTE] = 0; song.g[G_CLOCK] = (int16_t)scenario->clock;
    song.g[G_BPM] = scenario->clock ? 120 : 240;
    song.g[G_RTYPE] = (int16_t)scenario->spring;
    song.g[G_RSIZE] = 64; song.g[G_DFDBK] = 48;
    song.master_q12 = 2048; cin_bars = 0; click_mode = CLICK_ON;
    epoch = scenario->loss ? UINT32_MAX - 2000u : 1000u;
    fm1_ms = epoch; host_ticks = epoch * 1000u;
    mix_block(audio, CTL); /* settle requested engines and external clock mode */
    d8p1_arrangement *a = &d8p1_runtime_cache.arrangement;
    a->banks = 4; a->scenes = 16; a->rows = 16; a->bank[3] = a->bank[0];
    for (unsigned s = 0; s < 16; s++) {
        a->scene[s] = a->scene[s % 3];
        a->scene[s].bank = (uint8_t)(s % 4); a->scene[s].apply = 7;
        a->scene[s].mute = 0;
        for (unsigned t = 0; t < 8; t++) {
            a->scene[s].level[t] = 48;
            a->scene[s].pan[t] = (int8_t)(t * 16 - 56);
            a->scene[s].transpose[t] = (int8_t)((int)(s % 3) * 2 - 2);
        }
        a->row[s].scene = (uint8_t)s; a->row[s].repeat = 1;
    }
    initial_selection = song.sel;
    lcd_started = dma_consumed;
    proof(!d8p1_capture_runtime(initial, sizeof initial, &initial_size), "retain editable stress setup");
    memcpy(baseline, nor, sizeof nor); reset();
    proof(!chain_prepare() && chain.armed, "prepare all banks and scenes read-only");
    queue_audio_dbg.boots = 17;
    proof(d8hp_start(17) == 1, "start real heavy ownership observer");
    proof(audio_timing_start(), "start actual ISR observer with simulated timer");
    if (scenario->clock) realtime(0xFAu); else transport_req = 1;
}

static void inputs(void)
{
    uint32_t ms = (uint32_t)(samples * 1000u / FS);
    fm1_ms = epoch + ms; host_ticks = epoch * 1000u + (uint32_t)(samples * 1000000u / FS);
    if (scenario->clock) {
        while ((uint64_t)next_clock * 500u / 24u <= ms) {
            if (!scenario->loss || ms < 8000u) {
                fm1_ms = epoch + next_clock * 500u / 24u;
                realtime(0xF8u);
            } else clock_lost = 1;
            next_clock++;
        }
        fm1_ms = epoch + ms;
    }
    if (ms >= next_note) {
        if (held) { live_note(0); next_note += 400u; }
        else {
            live_key = trk[7].seq_n ? trk[7].seq_notes[0] : 72u;
            live_note(64); next_note += 100u;
        }
    }
}

static void half(void)
{
    unsigned reads_before = reads, writes_before = writes;
    fm1_alnk0_irq();
    proof(reads == reads_before && writes == writes_before, "audio ISR never accesses source NOR");
    proof(!queue_audio_dbg.in_audio && !queue_audio_dbg.late, "simulated half returns cleanly");
    if (held) {
        proof(midi_sel_on[7][live_key] == 8u, "parser delivered live note to track eight");
        live_observed++;
    }
    unsigned busy = voices_busy(); assert(busy <= 8);
    if (busy > peak_voices) peak_voices = busy;
    for (unsigned t = 0; t < 8; t++) for (unsigned v = 0; v < NVOICE; v++)
        if (trk[t].v[v].gate) sounded_tracks |= 1u << t;
    for (unsigned i = 0; i < HALF_WORDS; i++) audible |= abuf[qa_half * HALF_WORDS + i] != 0;
    for (unsigned i = 0; i < CTL; i++) {
        wet_audio |= wet[i] != 0;
        sends[0] |= send_c[i] != 0; sends[1] |= send_d[i] != 0; sends[2] |= send_r[i] != 0;
    }
    if (d8arr_running()) scene_mask |= 1u << chain.native.policy.scene;
    if (scenario->interactive) {
        if (chain.native.policy.fill_active && chain.row == 12u) fill_ran = 1;
        if (fill_ran && !chain.native.policy.fill_active && chain.row == 4u) fill_returned = 1;
        if (row_queued && chain.row == 10u) row_applied = 1;
    }
    qa_half ^= 1u; samples += HALF_FRAMES;
}

static void display(void)
{
    song.sel = (uint8_t)((frames_drawn / 8u) % 8u);
    switch (frames_drawn % 4u) {
    case 0: ui.home = 1; break;
    case 1: go_page(GR_SONG); break;
    case 2: go_page(GR_ROLL); break;
    case 3: go_page(GR_TRK); break;
    }
    ui.force = 1; ui_draw(); frames_drawn++;
    if (scenario->interactive && chain.row == 2 && !fill_queued) {
        go_page(GR_SONG); ui.song_row = 12; edit_param(2, 1); fill_queued = 1;
        proof(chain.native.policy.fill_pending == 12, "actual SONG knob queues fill during audio");
    }
    if (scenario->interactive && chain.row == 8 && !row_queued) {
        go_page(GR_SONG); ui.song_row = 10; edit_param(3, 1); row_queued = 1;
        proof(chain.native.policy.pending == 10, "actual SONG knob queues row during audio");
    }
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    for (unsigned i = 0; i < sizeof scenarios / sizeof scenarios[0]; i++)
        if (!strcmp(argv[1], scenarios[i].name)) scenario = &scenarios[i];
    assert(scenario); setup();
    do {
        inputs(); half();
        if (qa_acks % 6u == 0) display();
    } while (song.playing && samples < (uint64_t)FS * 45u && !failures);
    proof(!song.playing && !chain.running, "bounded arrangement reaches Stop");
    uint64_t stopped_at = samples;
    if (held) live_note(0);
    seq_stop();
    unsigned drained = 0;
    for (unsigned i = 0; i < FS * 12u / HALF_FRAMES; i++) {
        fm1_ms = epoch + (uint32_t)(samples * 1000u / FS);
        host_ticks = epoch * 1000u + (uint32_t)(samples * 1000000u / FS);
        half();
        if (!voices_busy() && audio_output_quiet()) { drained = 1; break; }
    }
    lcd_sync();
    proof(drained && !voices_busy() && audio_output_quiet(), "bounded note and effect cleanup reaches DAC silence");
    proof(!dma_errors && dma_consumed - lcd_started >= frames_drawn, "actual UI DMA completes without source overwrite");
    for (unsigned c = 0; c < 16; c++) for (unsigned n = 0; n < 128; n++)
        proof(!midi_sel_on[c][n], "all live MIDI notes released");
    proof(peak_voices == 8 && sounded_tracks == 255u && audible, "all eight tracks gate with audible ISR output");
    proof(live_inputs >= 16u && live_observed >= live_inputs, "live input is observed throughout playback");
    proof(d8hp.report.peak_live_owned_slots == 8 && !d8hp.report.failures && !d8hp.report.invalid,
          "combined playback fills all eight heavy slots without allocation failure");
    if (scenario->effects) proof(wet_audio && sends[0] && sends[1] && sends[2], "all requested buses produce wet audio");
    if (!scenario->interactive && !scenario->loss) proof(scene_mask == 65535u, "all sixteen scenes render naturally");
    if (scenario->interactive) proof(fill_queued && fill_ran && fill_returned && row_queued && row_applied,
                                     "queued fill runs and returns before queued row is applied");
    if (scenario->loss) proof(clock_lost && stopped_at < (uint64_t)FS * 10u, "TRS clock loss stops across timestamp wrap");
    /* Undo only the selection explicitly changed by display(); preserve every
     * other live byte as an independent oracle for musical-state cleanup. */
    song.sel = initial_selection;
    int capture_rc = d8p1_capture_runtime(final, sizeof final, &final_size);
    proof(!capture_rc && final_size == initial_size && !memcmp(initial, final, initial_size),
          "editable project restored exactly after playback");
    proof(!memcmp(baseline, nor, sizeof nor) && !writes, "all stored sources unchanged");
    proof(audio_timing_stop() && audio_timing.render.count == qa_acks && !audio_timing.invalid && !audio_timing.taint,
          "every simulated half accounted once without observer invalidity");
    proof(d8hp_stop(), "heavy ownership observer closes after cleanup");
    printf("arrangement-stress %s checks=%u failures=%u halves=%u scenes=%04X voices=%u pool=%u ui=%u live_notes=%u quiet=%u simulated_only=1\n",
           scenario->name, checks, failures, qa_acks, scene_mask, peak_voices,
           d8hp.report.peak_live_owned_slots, frames_drawn, live_inputs, drained);
    return !!failures;
}
