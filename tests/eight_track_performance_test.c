/* SPDX-License-Identifier: GPL-3.0-only */
/* Real mixer dry/send buses: solo ownership is not enough to prove muting. */
#define NPART 8
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static __typeof__(pf) cold;
static unsigned checks,failures,cases;
#define CHECK(x) do {checks++;if(!(x)){failures++;fprintf(stderr,"line %d: %s\n",__LINE__,#x);}}while(0)
static void setup(unsigned candidate)
{
    ui_power_on();pf=cold;kb_prev=kb_lock=0;host_notes=host_pressed=0;
    for(unsigned t=0;t<8;t++) {
        host_preset(&trk[t],0,5);
        trk[t].p[P_ATK]=trk[t].p[P_REL]=0;trk[t].p[P_SUS]=127;
        trk[t].p[P_DIST]=trk[t].p[P_SLCR]=trk[t].p[P_MUTE]=0;
        trk[t].p[P_CHOR]=trk[t].p[P_DLY]=trk[t].p[P_REV]=64;
        trk[t].p[P_AMODE]=0;trk[t].p[P_LEVEL]=100;trk[t].p[P_PAN]=0;
    }
    memset(dly_buf,0,sizeof dly_buf);memset(cho_buf,0,sizeof cho_buf);rev_clear();
#ifdef FT16
    fx_tail.delay=fx_tail.chorus=0; /* match the synthetic buffer reset */
#endif
    dc_l=dc_r=dce_l=dce_r=lc_l1=lc_l2=lc_r1=lc_r2=0;
    memset(lce,0,sizeof lce);lim_env=LIM_T;
    perf_held=perf_latched=perf_act=perf_solo=perf_kill=0;
    for(unsigned k=0;k<4;k++)perf_k[k]=0;
    song.master_q12=4096;trk_note_on(&trk[candidate],60,100);
}
/* Observe the actual production stages before wet tails/master processing:
 * muting stops new dry/send input, not already-resident effects tails. */
static void bus_step(void)
{
    for(unsigned i=0;i<CTL;i++)send_c[i]=send_d[i]=send_r[i]=mix_l[i]=mix_r[i]=0;
    events_block(CTL);int perf=perf_begin(CTL);
    for(unsigned t=0;t<NTRK;t++)mix_part(&trk[t],CTL);
    if(perf)perf_pre(mix_l,mix_r,send_d,send_r,CTL);
}
static int32_t peak(const int32_t *b)
{
    int32_t p=0;for(unsigned i=0;i<CTL;i++) {int32_t a=b[i]<0?-b[i]:b[i];if(a>p)p=a;}return p;
}
static void probe(unsigned solo,unsigned mute,unsigned candidate,int panel)
{
    setup(candidate);
    if(panel) {
        frame(); /* released panel scan clears the previous UI key edge */
        song.sel=solo<16?0:4;
        unsigned selected=0;while(!(solo&(1u<<selected)))selected++;
        lay_combo(B_GLO,white(selected%4));
        CHECK(perf_solo==solo);
    } else perf_solo=(uint8_t)solo;
    perf_held=mute<<PF_M1;
    int32_t p[5]={0};
    for(unsigned b=0;b<24;b++) {
        bus_step();if(b<8)continue;
        const int32_t *bus[]={mix_l,mix_r,send_c,send_d,send_r};
        for(unsigned k=0;k<5;k++){int32_t a=peak(bus[k]);if(a>p[k])p[k]=a;}
    }
    unsigned audible=(!solo||((solo>>candidate)&1u))&&!((mute>>candidate)&1u);
    for(unsigned k=0;k<5;k++) {
        checks++;if((p[k]>0)!=audible) {
            failures++;fprintf(stderr,"solo %02x mute %02x track %u bus %u peak %d expected %u\n",solo,mute,candidate+1,k,p[k],audible);
        }
    }
    cases++;
}
int main(void)
{
    cold=pf;for(unsigned t=0;t<8;t++)CHECK(cold.mg[t]==32768);
    /* First activation must not ramp unrelated tracks from an implicit zero.
     * Compare actual first-block dry/send samples with the untouched path. */
    for(unsigned t=0;t<8;t++) {
        int32_t reference[5][CTL];setup(t);bus_step();
        const int32_t *bus[]={mix_l,mix_r,send_c,send_d,send_r};
        for(unsigned k=0;k<5;k++)memcpy(reference[k],bus[k],sizeof reference[k]);
        setup(t);perf_held=PF_BIT(PF_M1+(t+1)%8);bus_step();
        for(unsigned k=0;k<5;k++)CHECK(!memcmp(reference[k],bus[k],sizeof reference[k]));
    }
    /* Actual banked panel gesture and audio for all64 solo/candidate pairs. */
    for(unsigned s=0;s<8;s++)for(unsigned t=0;t<8;t++)probe(1u<<s,0,t,1);
    /* All logical solo masks: no solo/all solo, mixed banks and complements. */
    for(unsigned mask=0;mask<256;mask++)for(unsigned t=0;t<8;t++)probe(mask,0,t,0);
    /* Direct mutes and their union with solo, including selected-track mute. */
    for(unsigned m=0;m<8;m++)for(unsigned t=0;t<8;t++) {
        probe(0,1u<<m,t,0);probe(1u<<t,1u<<m,t,0);
    }
    /* Releasing solo must ramp previously muted tracks back into every bus. */
    for(unsigned t=0;t<8;t++) {
        setup(t);perf_solo=(uint8_t)(1u<<((t+1)%8));
        for(unsigned b=0;b<8;b++)bus_step();
        CHECK(!peak(mix_l)&&!peak(mix_r)&&!peak(send_c)&&!peak(send_d)&&!peak(send_r));
        perf_solo=0;for(unsigned b=0;b<8;b++)bus_step();
        CHECK(peak(mix_l)&&peak(mix_r)&&peak(send_c)&&peak(send_d)&&peak(send_r));
    }
    printf("Eight-track performance audio: %u checks, %u failures; %u dry/stereo/send cases, cold first-block parity and solo release; host mixer only\n",checks,failures,cases);
    return failures!=0;
}
