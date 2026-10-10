/* SPDX-License-Identifier: GPL-3.0-only
 * Actual eight-part allocation/render paths, without storage/UI enlargement. */
#if NPART != 8
#error Build this test with -DNPART=8
#endif
#define main hostsim_main
#include "hostsim.c"
#undef main
static int bad;
static void check(const char *name, int ok)
{
    printf("eight voices: %s %s\n", name, ok ? "PASS" : "FAIL");
    bad += !ok;
}
static void fresh(void)
{
    memset(trk, 0, sizeof trk); memset(&song, 0, sizeof song);
    memset(midi_ch, 0, sizeof midi_ch); memset(midi_notes, 0, sizeof midi_notes);
    memset(midi_owners, 0, sizeof midi_owners); memset(mchord, 0, sizeof mchord);
    host_tracks_init(); vage = voice_kills = 0;
    for (uint32_t p = 0; p < NPART; p++) {
        host_preset(&trk[p], 0, 0);
        trk[p].p[P_VOICE] = V_POLY; trk[p].p[P_SUS] = 127;
    }
}
static uint32_t active(void)
{
    uint32_t n = 0;
    for (uint32_t p = 0; p < NPART; p++)
        for (uint32_t i = 0; i < NVOICE; i++) n += !!trk[p].v[i].active;
    return n;
}
static int held(uint32_t p, uint32_t note)
{
    for (uint32_t i = 0; i < NVOICE; i++)
        if (trk[p].v[i].active && trk[p].v[i].gate && trk[p].v[i].note == note) return 1;
    return 0;
}
static uint32_t render(void)
{
    int32_t out[CTL]; uint32_t n = 0;
    for (uint32_t p = 0; p < NPART; p++) n += track_render(&trk[p], out, CTL);
    return n;
}
static void fill(void)
{
    for (uint32_t p = 0; p < NPART; p++) trk_note_on(&trk[p], 40 + p, 100);
}
int main(void)
{
    fresh(); fill();
    check("eight protected held notes use eight slots", active() == 8 && voices_busy() == 8);
    check("eight held notes render exactly eight engine voices", render() == 8);
    trk_note_on(&trk[7], 80, 100);
    check("ninth note replaces oldest protected bass with no ninth tail", !held(0, 40) && held(7, 80) && active() == 8 && render() <= 8);
    trk_note_on(&trk[0], 30, 100);
    check("cross-track chord extra yields before another protected bass", held(1, 41) && !held(7, 80) && held(0, 30) && active() == 8);

    fresh(); fill(); trk_note_off(&trk[5], 45); trk_note_on(&trk[0], 70, 100);
    check("release tail yields before protected lead/bass", !held(5, 45) && !trk[5].v[0].active && held(1, 41) && active() == 8 && render() <= 8);
    fresh(); trk[0].p[P_VOICE] = V_UNISON; trk_note_on(&trk[0], 50, 100);
    trk_note_on(&trk[1], 60, 100);
    check("extra unison yields before its lead", held(0, 50) && trk[0].v[0].active && held(1, 60) && active() == 8 && render() <= 8);
    fresh(); fill(); trk[7].p[P_VOICE] = V_UNISON; trk_note_on(&trk[7], 70, 100);
    check("soft unison extras cannot steal eight protected leads", active() == 8 && held(0, 40) && held(7, 70));
    fresh(); for (uint32_t i = 0; i < 4; i++) trk_note_on(&trk[0], 36 + i * 4, 100);
    for (uint32_t p = 1; p < 5; p++) trk_note_on(&trk[p], 60 + p, 100);
    trk_note_on(&trk[5], 80, 100);
    check("chord non-bass yields before lowest chord note", held(0, 36) && !held(0, 40) && active() == 8 && render() <= 8);
    fresh(); host_preset(&trk[0], ENGI_DRUM, 0); fill(); trk_note_on(&trk[7], 80, 100);
    check("protected drum has no unlimited premium over synth leads", active() <= 8 && render() <= 8 && held(7, 80));

    fresh(); song.sel = 0; midi_note_event(0, 60, 100); midi_channel(0)->pedal = 1;
    midi_note_event(0, 60, 0);
    for (uint32_t p = 1; p < NPART; p++) trk_note_on(&trk[p], 40 + p, 100);
    check("real MIDI pedal retains ownership and held allocation", held(0, 60) && midi_notes[0][60] == (1 | MIDI_PEDAL_NOTE) && active() == 8);
    trk_note_on(&trk[7], 90, 100);
    check("pedal-held protected note may yield to ninth event", !held(0, 60) && active() == 8 && render() <= 8);
    midi_pedal_up(0);
    check("pedal-up of stolen note clears ownership without releasing another track", !midi_notes[0][60] && held(7, 90) && active() == 8);
    for (uint32_t p = 0; p < NPART; p++) trk_all_off(&trk[p]);
    for (uint32_t b = 0; b < 30000 && active(); b++) render();
    check("all release tails end without hanging voices", active() == 0);

    fresh(); fill(); uint32_t seed = 13, peak = 0, burst_bad = 0;
    for (uint32_t k = 0; k < 10000; k++) {
        seed = seed * 1664525u + 1013904223u;
        uint32_t p = seed >> 29, note = 36 + (seed >> 16) % 60;
        if (k % 7 == 0) trk_note_off(&trk[p], note);
        else trk_note_on(&trk[p], note, 100);
        uint32_t n = active(); if (n > peak) peak = n;
        if (n > 8 || render() > 8) { burst_bad++; break; }
    }
    check("10,000 burst events never exceed eight active/rendered engines", peak == 8 && !burst_bad);
    uint32_t engine_bad = 0;
    for (uint32_t e = 0; e < NENGINES; e++) {
        if (!eng_ok(e)) continue;
        for (uint32_t mode = V_POLY; mode <= V_UNISON; mode++) {
            fresh();
            for (uint32_t p = 0; p < NPART; p++) {
                host_preset(&trk[p], e, 0); trk[p].p[P_VOICE] = mode;
                trk_note_on(&trk[p], 60 + p, 100);
                engine_bad += active() > 8 || render() > 8;
            }
            for (uint32_t k = 0; k < 16; k++) {
                trk_note_on(&trk[k % NPART], 48 + k, 100);
                engine_bad += active() > 8 || render() > 8;
            }
        }
    }
    check("every enabled engine and voice mode bounds eight-part bursts", !engine_bad);
    printf("result: %d failures; target still four tracks; no deadline/click claim\n", bad);
    return !!bad;
}
