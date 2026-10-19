/* SPDX-License-Identifier: GPL-3.0-only */
/* Selective upstream regressions against the real Dabbl8 UI, no device. */
#include <stdint.h>
#include <string.h>
static unsigned faces[4], face_count, watch_fill, rule_clears;
static uint32_t rule_x, rule_y;
static void observe_face(const char *text, int why)
{
    (void)text;
    if (face_count < 4) faces[face_count++] = why == 0 ? 0 : why == 3 ? 2 : 1;
}
static void observe_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h)
{
    if (watch_fill && x <= rule_x && rule_x < x+w && y <= rule_y && rule_y < y+h && h > 1)
        rule_clears++;
}
#define LARGE_HOOK(s,w) observe_face(s,w)
#define UI_FILL_HOOK(x,y,w,h) observe_fill(x,y,w,h)
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bipolar_endpoints(void)
{
    char text[16]; const char *unit;
    param_format(&TP[P_PAN],63,text,&unit);
    int ok = !strcmp(text,"+100");
    param_format(&TP[P_PAN],-64,text,&unit); ok &= !strcmp(text,"-100");
    param_format(&TP[P_PAN],0,text,&unit); ok &= !strcmp(text,"0");
    return check("bipolar percentages reach both 100-percent endpoints",ok);
}
static int stable_rows(void)
{
    int ok=1;
    for (unsigned track=0;track<NTRK;track++) {
        ui_power_on(); track_select(track); track_t *t=TSEL;
        motion_set_lock(t,0,P_ATK,10); motion_set_lock(t,0,P_REL,20);
        uint8_t rows[MOTION_MAX]; ev_rows(rows); unsigned second=rows[1];
        ui.ev_row=1; ev_knob(2,-8); ev_rows(rows);
        ok &= ui.ev_row==1 && rows[1]==second;
        ok &= MOTION_ID(&motion.event[second]) < P_ATK;
        ev_knob(2,8); ev_rows(rows); ok &= ui.ev_row==1 && rows[1]==second;
        for(unsigned i=0;i<motion.count;i++) ok &= (motion.event[i].place>>6)==track;
    }
    return check("automation parameter edits retain row identity on every track",ok);
}
static int stable_large_values(void)
{
    ui_power_on(); select_engine(0); ui_prefs|=PREF_LARGE;
    int ok=1; unsigned first=3;
    for(int value=0;value<=127;value++) {
        TSEL->p[P_E4]=(int16_t)value; ui.force=1; face_count=0; draw_columns();
        if(first==3) first=faces[0];
        ok &= face_count==4 && faces[0]==first;
    }
    return check("LARGE CUT retains one font across its entire knob range",ok);
}
static int steady_rules(void)
{
    ui_power_on(); ux.style=ST_LINE; draw_frame();
    rule_x=CARD_X(0)+3; rule_y=(H_HEAD+Y_LABEL)/2;
    rule_clears=0; watch_fill=1; draw_frame(); watch_fill=0;
    return check("LINE redraw never blanks a divider before repainting it",rule_clears==0);
}
int main(void)
{
    int bad=bipolar_endpoints()+stable_rows()+stable_large_values()+steady_rules();
    return bad!=0;
}
