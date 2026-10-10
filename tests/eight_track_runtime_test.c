/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Actual eight-track recording/playback and shared USB/TRS routing paths.
 * Host checks do not qualify physical MIDI, audio deadlines or an eight-track release. */
#define EDITOR_TEST_NO_MAIN 1
#include "editor_test.c"
_Static_assert(NPART == 8 && NVOICE == 8, "eight tracks share eight sounding voices");

static void runtime_block(void)
{
    int32_t out[2 * CTL];
    mix_block(out, CTL);
}

static void runtime_reset(void)
{
    reset();
    memset(midi_ch, 0, sizeof midi_ch);
    memset(midi_sel_on, 0, sizeof midi_sel_on);
    memset(midi_owners, 0, sizeof midi_owners);
    memset(mchord, 0, sizeof mchord);
    memset(midi_bend_q8, 0, sizeof midi_bend_q8);
    memset(midi_bend_target, 0, sizeof midi_bend_target);
    memset(kb_chord, 0, sizeof kb_chord);
    memset(kb_chn, 0, sizeof kb_chn);
    memset(&midi_clock, 0, sizeof midi_clock);
    memset(&pf, 0, sizeof pf);
    fm1_in.notes = fm1_in.buttons = 0;
    midi_route = midi_hint = 0;
    cin_left = cin_total = cin_flush = cin_bars = 0;
    mi_r = mi_w = mo_r = mo_w = 0;
    song.master_q12 = 4096;
    for (uint32_t i = 0; i < NTRK; i++) {
        host_preset(&trk[i], 0, 5);
        trk[i].p[P_ATK] = trk[i].p[P_REL] = 0;
        trk[i].p[P_SUS] = 127;
        trk[i].p[P_VOICE] = V_POLY;
        trk[i].p[P_DIST] = trk[i].p[P_CHOR] = trk[i].p[P_DLY] = trk[i].p[P_REV] = 0;
    }
    runtime_block();
}

/* Each entry reaches the real port parser before the shared queue consumer. */
static void port_event(uint32_t trs, uint32_t status, uint32_t a, uint32_t b)
{
    if (trs) {
        um_byte(status); um_byte(a); um_byte(b);
    } else {
        midi_in_event((status >> 4) | status << 8 | a << 16 | b << 24);
    }
}

static uint32_t held_mask(void)
{
    uint32_t mask = 0;
    for (uint32_t i = 0; i < NTRK; i++)
        for (uint32_t v = 0; v < NVOICE; v++)
            if (trk[i].v[v].active && trk[i].v[v].gate) mask |= 1u << i;
    return mask;
}

static uint32_t active_voices(void)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < NTRK; i++)
        for (uint32_t v = 0; v < NVOICE; v++) n += trk[i].v[v].active != 0;
    return n;
}

static int channel_routes(void)
{
    int bad = 0;
    for (uint32_t trs = 0; trs < 2u; trs++) {
        runtime_reset();
        for (uint32_t ch = 0; ch < 8u; ch++) port_event(trs, 0x90u | ch, 48u + ch, 100);
        runtime_block();
        int owners = held_mask() == 0xFFu && active_voices() == NVOICE;
        for (uint32_t ch = 0; ch < 8u; ch++)
            owners &= midi_sel_on[ch][48u + ch] == ch + 1u && midi_owners[ch] == 1u;
        bad += check(trs ? "TRS channels 1..8 hold eight distinct tracks" : "USB channels 1..8 hold eight distinct tracks", owners);
        for (uint32_t ch = 0; ch < 8u; ch++) port_event(!trs, 0x80u | ch, 48u + ch, 0);
        runtime_block();
        bad += check("other port releases the same eight channel owners", !held_mask());
        runtime_reset();
        for (uint32_t ch = 8; ch < 16u; ch++) {
            port_event(trs, 0x90u | ch, 48u + ch, 100);
            port_event(trs, 0xB0u | ch, 7, 0);
            port_event(trs, 0xE0u | ch, 0, 127);
        }
        runtime_block();
        int ignored = !held_mask();
        for (uint32_t i = 0; i < NTRK; i++) ignored &= !midi_owners[i] && !midi_bend_target[i] && trk[i].p[P_LEVEL] != 0;
        bad += check(trs ? "TRS channels 9..16 leave notes/controllers untouched" : "USB channels 9..16 leave notes/controllers untouched", ignored);
    }
    runtime_reset();
    port_event(0, 0x97, 60, 100); port_event(1, 0xB7, 64, 127); runtime_block();
    port_event(1, 0x97, 60, 0); runtime_block();
    bad += check("track eight pedal ownership uses bit seven without alias", midi_sel_on[7][60] == (MIDI_PEDAL_NOTE | 8u) && midi_ch[7].targets == 0x80u);
    song.g[G_ROUTE] = 1; song.sel = 6; runtime_block();
    port_event(0, 0x9F, 72, 100); runtime_block();
    song.sel = 0;
    port_event(1, 0x8F, 72, 0); runtime_block();
    bad += check("SEL note-off follows track seven after selection changes", !midi_owners[6] && midi_owners[7] == 1u);
    song.sel = 5; port_event(0, 0x9E, 74, 100); port_event(0, 0xBE, 64, 127); runtime_block();
    port_event(0, 0x8E, 74, 0); runtime_block();
    song.g[G_ROUTE] = 0; runtime_block();
    bad += check("return to channel mode releases high channels but retains channel eight pedal", !midi_owners[5] && !midi_ch[14].pedal && midi_owners[7] == 1u);
    port_event(0, 0xB7, 64, 0); runtime_block();
    bad += check("channel eight pedal-up clears its final owner", !midi_owners[7] && !midi_ch[7].targets);
    return bad;
}

static int record_and_play(void)
{
    int bad = 0;
    runtime_reset(); song.rec = 0xFFu; transport_req = 1; runtime_block();
    for (uint32_t i = 0; i < NTRK; i++) port_event(i & 1u, 0x90u | i, 48u + i, 100);
    runtime_block();
    int separate = 1;
    for (uint32_t i = 0; i < NTRK; i++) separate &= trk[i].step[0].n == 1u && trk[i].step[0].note[0] == 48u + i && trk[i].step[0].time == ST_NOTE;
    bad += check("all eight armed tracks record independent notes concurrently", separate && song.rec == 0xFFu);
    for (uint32_t i = 0; i < NTRK; i++) port_event(!(i & 1u), 0x80u | i, 48u + i, 0);
    runtime_block(); separate = 1;
    for (uint32_t i = 0; i < NTRK; i++) separate &= !trk[i].rh_n && !midi_owners[i];
    bad += check("recording releases all eight destinations without stale holds", separate);
    for (uint32_t selected = 4; selected < 8u; selected++) {
        runtime_reset(); song.rec = (uint8_t)(1u << selected); transport_req = 1; runtime_block();
        for (uint32_t i = 0; i < NTRK; i++) port_event(i & 1u, 0x90u | i, 60u + i, 100);
        runtime_block(); separate = 1;
        for (uint32_t i = 0; i < NTRK; i++) separate &= trk[i].step[0].n == (i == selected ? 1u : 0u);
        bad += check("an upper-bank record bit never writes another track", separate);
    }
    for (uint32_t muted = 0; muted < NTRK; muted++) {
        runtime_reset();
        for (uint32_t i = 0; i < NTRK; i++) {
            trk[i].p[P_SLEN] = 2;
            trk[i].step[0] = (step_t){{(uint8_t)(48u + i)}, 1, ST_NOTE, 0, 100, 0, 0};
        }
        trk[muted].p[P_MUTE] = 1; transport_req = 1; runtime_block();
        separate = held_mask() == (0xFFu ^ (1u << muted));
        for (uint32_t i = 0; i < NTRK; i++) if (i != muted) separate &= trs_held(&trk[i], 48u + i);
        bad += check("each playback mute suppresses only its own sequenced track", separate && active_voices() <= NVOICE);
    }
    return bad;
}

static int selection_and_bounds(void)
{
    int bad = 0; runtime_reset();
    for (uint32_t i = 0; i < NTRK; i++) track_select(i);
    bad += check("selection reaches track eight", song.sel == 7u && TSEL == &trk[7]);
    track_select(8); track_select(255);
    bad += check("out-of-range selection cannot alias a valid track", song.sel == 7u);
    request(ED_TRACK, 0, 0);
    bad += check("read-only track discovery reports eight complete records", host_wire[4] == ED_TRACK && host_wire[5] == 7u && host_wire[6] == 8u && host_wire_n == 7u + 8u * 6u + 1u);
    int16_t before = trk[7].p[P_LEVEL];
    request(ED_TRACK_MIX, (const uint8_t[]){7, 0, 0, 1}, 4);
    bad += check("legacy editor writes remain refused on the expanded model", host_wire[4] == ED_D8_ERROR && trk[7].p[P_LEVEL] == before);
    uint8_t sum = midi_owners[7];
    request(ED_TRACK_DUMP, (const uint8_t[]){8}, 1);
    bad += check("invalid track reads leave runtime ownership unchanged", midi_owners[7] == sum && song.sel == 7u);
    bad += check("channel route label matches eight-track mode", str_eq(N_ROUTE[0], "CH1-8"));
    bad += check("four project slots and four note positions stay independent of track count", GP[G_SLOT].max == 4 && sizeof proj_slot / sizeof proj_slot[0] == 4u && sizeof trk[7].step[0].note == 4u);
    return bad;
}

int main(void)
{
    int bad = channel_routes() + record_and_play() + selection_and_bounds();
    printf("eight-track runtime: %d failures; host evidence only, default build remains four tracks\n", bad);
    return !!bad;
}
