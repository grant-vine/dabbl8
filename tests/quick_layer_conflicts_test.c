/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
_Static_assert(NTRK == 8, "expanded quick-layer conflicts");
int main(void)
{
    int bad=0,ok=1;
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
