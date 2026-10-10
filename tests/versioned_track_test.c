/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual USB parser/editor handlers. No hardware or flash writes. */
#define EDITOR_TEST_NO_MAIN 1
#include "editor_test.c"
_Static_assert(NTRK==8 && NVOICE==8,"eight tracks share eight voices");
static unsigned assertions;
static int proof(const char *label,int value) { assertions++; return check(label,value); }
static int rejected(const uint8_t *a,unsigned n,unsigned rc) {
    project_t before,after; project_capture(&before);
    unsigned size=request(ED_D8_TRACK,a,n); project_capture(&after);
    return size==9 && host_wire[4]==ED_D8_ERROR && host_wire[5]==1 &&
        host_wire[6]==ED_D8_TRACK && host_wire[7]==rc && !memcmp(&before,&after,sizeof before) && !host_writes && !host_erases;
}
static int level_scope(unsigned owner,const int16_t *before,int16_t value) {
    for(unsigned i=0;i<8;i++) if(trk[i].p[P_LEVEL]!=(i==owner?value:before[i])) return 0;
    return 1;
}
static int step_scope(unsigned owner,const step_t *before) {
    for(unsigned i=0;i<8;i++) if(i!=owner && memcmp(&trk[i].step[63],&before[i],sizeof(step_t))) return 0;
    return 1;
}
int main(void) {
    int bad=0;reset();uint8_t a[32]={1};unsigned n=request(ED_D8_CAPS,a,0);
    bad+=proof("expanded capabilities advertise track envelope, refuse legacy writes",n==21 && host_wire[8]==8 && host_wire[19]==5);
    for(unsigned t=0;t<8;t++) {
        a[0]=1;a[1]=ED_TRACK;a[2]=t;n=request(ED_D8_TRACK,a,3);
        bad+=proof("versioned selection reaches the exact track and returns all eight",n==58 && song.sel==t && host_wire[4]==77 && host_wire[5]==1 && host_wire[6]==ED_TRACK && host_wire[7]==t && host_wire[8]==8);
        int16_t levels[8]; for(unsigned i=0;i<8;i++) levels[i]=trk[i].p[P_LEVEL];
        a[1]=ED_TRACK_PARAM;a[3]=P_LEVEL;a[4]=16+t;a[5]=64;n=request(ED_D8_TRACK,a,6);
        bad+=proof("versioned parameter write changes only its addressed level",n==12 && host_wire[7]==t && host_wire[8]==P_LEVEL && trk[t].p[P_LEVEL]==16+(int)t && level_scope(t,levels,16+(int)t));
        n=request(ED_D8_TRACK,a,4);
        bad+=proof("versioned parameter read retains selection and signed framing",n==12 && ed_rv(host_wire+9)==16+(int)t && song.sel==t);
        a[1]=ED_TRACK_DUMP;n=request(ED_D8_TRACK,a,3);
        bad+=proof("versioned dump reports complete descriptor-sized parameter array",n==209 && host_wire[7]==t && ed_rv(host_wire+10+2*P_LEVEL)==16+(int)t);
        step_t previous[8]; for(unsigned i=0;i<8;i++) previous[i]=trk[i].step[63];
        a[1]=ED_TRACK_STEP;a[3]=63;a[4]=1;a[5]=60+t;a[6]=a[7]=a[8]=0;a[9]=ST_NOTE;a[10]=3;a[11]=100;a[12]=127;a[13]=127;a[14]=3;a[15]=42;a[16]=4;
        n=request(ED_D8_TRACK,a,17);
        bad+=proof("versioned last step preserves notes, full drum bits, chance and ratchet",n==23 && host_wire[7]==t && host_wire[8]==63 && trk[t].step[63].n==1 && trk[t].step[63].note[0]==60+t && trk[t].step[63].hit==255 && trk[t].step[63].acc==255 && step_chance(&trk[t].step[63])==42 && step_ratchet(&trk[t].step[63])==4 && step_scope(t,previous));
        for(unsigned i=0;i<8;i++) levels[i]=trk[i].p[P_LEVEL];
        a[1]=ED_TRACK_MIX;a[3]=127;a[4]=127;a[5]=1;n=request(ED_D8_TRACK,a,6);
        bad+=proof("versioned mixer clamps level and mute without changing selection",n==12 && trk[t].p[P_LEVEL]==127 && trk[t].p[P_MUTE]==1 && song.sel==t && level_scope(t,levels,127));
    }
    memset(a,0,sizeof a);a[0]=2;bad+=proof("unknown envelope schema refuses before mutation",rejected(a,2,2));
    a[0]=1;bad+=proof("empty envelope refuses before mutation",rejected(a,0,2));
    bad+=proof("missing operation refuses before mutation",rejected(a,1,3));
    for(unsigned op=0;op<128;op++) if(op!=ED_TRACK && op!=ED_TRACK_MIX && op!=ED_TRACK_DUMP && op!=ED_TRACK_STEP && op!=ED_TRACK_PARAM) {
        a[1]=op;bad+=proof("envelope cannot forward an unsupported operation or flash write",rejected(a,6,3));
    }
    a[1]=ED_TRACK;a[2]=8;bad+=proof("track eight index is refused without aliasing track zero",rejected(a,3,3));
    a[1]=ED_TRACK_PARAM;a[2]=7;a[3]=P_COUNT;bad+=proof("unknown parameter refused before mutation",rejected(a,6,3));
    a[1]=ED_TRACK_STEP;a[3]=64;bad+=proof("step 64 index is refused without overflow",rejected(a,17,3));
    a[3]=63;a[15]=101;a[16]=4;bad+=proof("invalid step chance refused before mutation",rejected(a,17,3));
    a[15]=100;a[16]=0;bad+=proof("invalid step ratchet refused before mutation",rejected(a,17,3));
    a[16]=4;bad+=proof("trailing step bytes refused before mutation",rejected(a,18,3));
    chain.armed=1;
    bad+=proof("busy song refuses step mutation explicitly",rejected(a,17,4) && chain.armed==1);
    a[1]=ED_TRACK_PARAM;a[3]=P_SLEN;a[4]=2;a[5]=64;
    bad+=proof("busy song refuses timing mutation explicitly",rejected(a,6,4) && chain.armed==1);
    a[3]=P_LEVEL;n=request(ED_D8_TRACK,a,6);
    bad+=proof("busy song still allows live mix parameters",n==12 && trk[7].p[P_LEVEL]==2 && chain.armed==1);
    chain.armed=0;
    project_t before,after;project_capture(&before);
    uint8_t old[4]={7,P_LEVEL,10,64};n=request(ED_TRACK_PARAM,old,4);project_capture(&after);
    bad+=proof("legacy parameter write remains denied on expanded firmware",n==9 && host_wire[7]==1 && !memcmp(&before,&after,sizeof before));
    bad+=proof("versioned editing made no flash erase or write",host_writes==0 && host_erases==0);
    printf("versioned tracks: %u assertions, %d failures; host evidence only\n",assertions,bad);
    return bad?1:0;
}
