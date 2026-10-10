/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
_Static_assert(NTRK == 8, "expanded quick-layer conflicts");
/* Exhaust every source/destination pair across the physical bank boundary.
 * A release belongs to the key-down track, even after ALGORITHM changes selection. */
static int held_owner_transitions(void)
{
    int bad = 0;
    for (uint32_t source = 0; source < NTRK; source++)
    for (uint32_t destination = 0; destination < NTRK; destination++) {
        ui_power_on(); track_select(source);
        host_preset(TSEL, 0, 5);
        TSEL->p[P_CHRD] = CH_MAJ; TSEL->p[P_VOICE] = V_POLY;
        key_down(white(0)); frame();
        int ok = gated_notes(&trk[source]) == 3u;
        track_select(destination); frame();
        ok &= song.sel == destination && kb_trk[white(0)] == source;
        for (uint32_t i = 0; i < NTRK; i++)
            ok &= gated_notes(&trk[i]) == (i == source ? 3u : 0u);
        key_up(white(0)); frame();
        for (uint32_t i = 0; i < NTRK; i++) ok &= !gated_notes(&trk[i]);
        bad += check("held chord releases its original track after every selection transition", ok);

        ui_power_on(); track_select(source); btn_down(B_GLO);
        uint32_t solo_key = white(source & 3u);
        key_down(solo_key); frame(); ok = perf_solo == (1u << source);
        track_select(destination); frame();
        ok &= perf_solo == (1u << source);
        key_up(solo_key); frame();
        bad += check("held solo crosses banks without moving or leaving a stale solo", ok && !perf_solo);
        btn_up(B_GLO); frame();

        ui_power_on(); track_select(source); btn_down(B_FX);
        key_down(key_at(1, source)); frame();
        ok = perf_held == PF_BIT(PF_M1 + source);
        track_select(destination); frame();
        ok &= perf_held == PF_BIT(PF_M1 + source);
        key_up(key_at(1, source)); frame();
        bad += check("held FX mute survives selection and releases only its original owner", ok && !perf_held && !gates());
        btn_up(B_FX); frame();
    }
    for (uint32_t selected = 0; selected < NTRK; selected++)
    for (uint32_t lane = 0; lane < NLANE; lane++) {
        ui_power_on(); track_select(selected);
        set_engine_of(TSEL, ENGI_DRUM); TSEL->engine = TSEL->eng_req;
        open_family(FAM_SEQ); frame();
        key_down(key_at(1, lane)); frame();
        int ok = grid_on() && ui.lane == lane && song.sel == selected;
        key_up(key_at(1, lane)); frame(); key_down(white(0)); frame();
        for (uint32_t i = 0; i < NTRK; i++) {
            ok &= trk[i].step[0].hit == (i == selected ? (1u << lane) : 0u);
            ok &= !trk[i].p[P_MUTE];
        }
        key_up(white(0)); frame();
        bad += check("every drum lane edits only its selected track without FX mute aliasing", ok);
    }
    printf("held owner transitions: 64 track pairs, 192 chord/solo/FX cases; 64 drum lane cases\n");
    return bad;
}

int main(void)
{
    int bad=held_owner_transitions(),ok=1;
    ui_power_on();track_select(7);uint32_t root[8];
    for(uint32_t i=0;i<8u;i++)root[i]=trk[i].p[P_ROOT];
    btn_down(B_SCL);key_down(white(7));frame();
    ok=ui.layer==LAYER_SCL && trk[7].p[P_ROOT]==(int16_t)((white(7)+5u)%12u) && song.sel==7u && !gates();
    for(uint32_t i=0;i<7u;i++)ok&=trk[i].p[P_ROOT]==root[i];
    bad+=check("SCL root key owns only track eight and does not become a bank selector",ok);
    key_up(white(7));btn_up(B_SCL);frame();
    ui_power_on();track_select(7);set_engine_of(TSEL,ENGI_DRUM);TSEL->engine=TSEL->eng_req;
    open_family(FAM_SEQ);frame();key_down(key_at(1,7));frame();
    ok=grid_on()&&ui.lane==7u&&song.sel==7u;
    for(uint32_t i=0;i<8u;i++)ok&=!trk[i].p[P_MUTE];
    bad+=check("DRUM black key eight selects lane eight without muting any track",ok);
    key_up(key_at(1,7));frame();key_down(white(0));frame();
    ok=(trk[7].step[0].hit&0x80u)!=0;
    for(uint32_t i=0;i<7u;i++)ok&=!trk[i].step[0].hit;
    bad+=check("track-eight DRUM step key edits its lane without lower-track aliasing",ok);
    key_up(white(0));frame();
    ui_power_on();track_select(7);TSEL->p[P_CHRD]=CH_MAJ;TSEL->p[P_VOICE]=V_POLY;
    key_down(white(0));frame();ok=gated_notes(&trk[7])==3u && kb_chn[white(0)]==3u&&song.sel==7u;
    for(uint32_t i=0;i<7u;i++)ok&=!gated_notes(&trk[i]);
    bad+=check("melodic chord key plays its chord on track eight without selecting a bank",ok);
    key_up(white(0));frame();ok=!gated_notes(&trk[7]);
    bad+=check("track-eight chord release leaves no held notes",ok);
    for(uint32_t selected=4;selected<8u;selected++) {
        ui_power_on();track_select(selected);song.rec=0xF0u;song.playing=1;
        btn_down(B_GLO);for(uint32_t k=0;k<4u;k++)turn(EN_K1+k,1);
        ok=motion.count==1u;
        for(uint32_t i=0;i<8u;i++)ok&=motion_count(&trk[i])==(i==selected?1u:0u);
        bad+=check("global automation retains its selected armed upper-bank destination",ok);
        btn_up(B_GLO);frame();
    }
    printf("quick conflicts: %d failures; host evidence only\n",bad);return !!bad;
}
