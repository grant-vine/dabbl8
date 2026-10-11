/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual double-tap/input dispatch; host panel and display stubs only. */
#define UI_RENDER_NO_MAIN 1
#include "ui_render.c"
static unsigned cases;
static int locked(unsigned track) {
    ui_power_on(); track_select(track); go_title("ENV"); frame();
    press(B_GLO); frames(64); press(B_GLO); frames(500);
    cases++;
    return check("actual double tap locks GLO after button release", ui.lock == LAYER_GLO && layer_open() == LAYER_GLO && !fm1_in.buttons);
}
int main(void) {
    int bad=0;rep=tmpfile();if(!rep)return 2;cur_name="GLO transport";
    for(unsigned t=0;t<NTRK;t++) {
        bad+=locked(t);song.playing=1;transport_req=0;press(B_PLAY);cases++;
        bad+=check("unheld locked GLO PLAY requests STOP",transport_req==2u);
        events_block(CTL);cases++;bad+=check("actual events apply STOP",!song.playing&&!transport_req);
        bad+=locked(t);song.playing=0;transport_req=0;press(B_PLAY);cases++;
        bad+=check("unheld locked GLO stopped PLAY requests START",transport_req==1u);
        bad+=locked(t);song.playing=1;song.g[G_CLOCK]=1;transport_req=0;press(B_PLAY);cases++;
        bad+=check("unheld locked GLO external-clock PLAY still requests STOP",transport_req==2u);
        bad+=locked(t);song.playing=0;cin_left=cin_total=4;transport_req=0;press(B_PLAY);cases++;
        bad+=check("unheld locked GLO count-in PLAY requests STOP",transport_req==2u);
        bad+=locked(t);song.playing=1;transport_req=0;btn_down(B_GLO);frame();press(B_PLAY);cases++;
        bad+=check("physically held locked GLO PLAY retains RESTART",transport_req==3u);
        bad+=locked(t);song.playing=1;song.g[G_CLOCK]=1;transport_req=0;btn_down(B_GLO);frame();press(B_PLAY);cases++;
        bad+=check("physically held GLO external restart remains refused",!transport_req&&msg_is("RESTART: CLK IS EXT"));
        bad+=locked(t);song.playing=0;transport_req=0;btn_down(B_GLO);frame();press(B_PLAY);cases++;
        bad+=check("physically held GLO stopped PLAY retains START",transport_req==1u);
        bad+=locked(t);song.playing=1;chain.running=1;transport_req=0;press(B_PLAY);cases++;
        bad+=check("locked GLO running chain still routes STOP",transport_req==2u);
    }
    fclose(rep);printf("GLO transport: %u cases, %d failures, %u tracks; host input only\n",cases,bad,(unsigned)NTRK);return bad!=0;
}
