/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual audio ISR with simulated timer/DMA. No physical deadline or click claim. */
#define AUDIO_TEST_NO_MAIN 1
#include "audio_test.c"
#include <assert.h>
static unsigned assertions;
#define REQUIRE(x) do { assert(x); assertions++; } while (0)
static void leads(void)
{
    fresh();
    memset(&pf, 0, sizeof pf); pf.src=pf.next=PF_N; pf.lc=PF_TOP;
    for (unsigned i=0;i<NPART;i++) {
        pf.mg[i]=32768;
        trk[i].p[P_VOICE]=(int16_t)(i%3==0 ? V_MONO : i%3==1 ? V_LEGATO : V_POLY);
        trk_note_on(&trk[i],40+i,100);
    }
    REQUIRE(voices_busy()==NPART);
}
static void sustained(void)
{
    leads();
    shed_voice();
    REQUIRE(shed_count==0 && voices_busy()==NPART);
    host_us=5000;
    fm1_alnk0_irq(); REQUIRE(!shed_req && !shed_count);
    fm1_alnk0_irq(); REQUIRE(shed_req && !shed_count);
    fm1_alnk0_irq();
#if NPART >= NVOICE
    REQUIRE(shed_count==1 && voices_busy()==NPART-1);
    REQUIRE(!trk[0].v[0].active); /* admission's oldest-protected policy */
    for (unsigned i=1;i<NPART;i++) REQUIRE(trk[i].v[0].active && trk[i].v[0].gate);
    /* Continuing overload can shed again below a full eight-voice set. */
    for (unsigned i=1;i<NPART;i++) {
        fm1_alnk0_irq();
        REQUIRE(shed_count==i+1 && voices_busy()==NPART-i-1);
        REQUIRE(!trk[i].v[0].active);
    }
    for (unsigned i=0;i<4;i++) fm1_alnk0_irq();
    REQUIRE(shed_count==NPART && !voices_busy());
#else
    REQUIRE(!shed_count && voices_busy()==NPART); /* historical protection */
#endif
}
static void transient_and_recovery(void)
{
    leads();
    for (unsigned i=0;i<16;i++) {
        host_us=i%2 ? 100 : 5000; fm1_alnk0_irq();
        REQUIRE(!shed_req && !shed_count && voices_busy()==NPART);
    }
    host_pending=0;host_us=5000;
    fm1_alnk0_irq();fm1_alnk0_irq();
    REQUIRE(!shed_req && !shed_count);
    host_pending=FM1_AUDIO_HALF;
    fm1_alnk0_irq();fm1_alnk0_irq();
    host_us=100;fm1_alnk0_irq();
#if NPART >= NVOICE
    REQUIRE(shed_count==1); /* queued request uses previous two completed halves */
    for (unsigned i=0;i<8;i++) fm1_alnk0_irq();
    REQUIRE(shed_count==1 && voices_busy()==NPART-1 && !shed_req);
    shed_voice(); REQUIRE(shed_count==1); /* recovery must restore protection */
#else
    REQUIRE(!shed_count && voices_busy()==NPART);
#endif
}
#if NPART >= NVOICE
static void protected_modes_and_retrigger(void)
{
    leads();
    trk[0].p[P_VOICE]=V_UNISON; /* a live mode change with seven peers keeps one lead */
    host_us=5000;fm1_alnk0_irq();fm1_alnk0_irq();fm1_alnk0_irq();
    REQUIRE(shed_count==1 && !trk[0].v[0].active);

    fresh();
    trk[0].p[P_VOICE]=V_POLY;
    trk_note_on(&trk[0],72,100);trk_note_on(&trk[0],40,100);
    voice_t *bass=held(&trk[0],40), *high=held(&trk[0],72);
    REQUIRE(bass && high && bass!=&trk[0].v[0]);
    voice_kill(high);
    for(unsigned i=1;i<NPART;i++) {
        trk[i].p[P_VOICE]=V_MONO;trk_note_on(&trk[i],48+i,100);
    }
    REQUIRE(voices_busy()==NPART);
    host_us=5000;fm1_alnk0_irq();fm1_alnk0_irq();fm1_alnk0_irq();
    REQUIRE(shed_count==1 && !bass->active);

    fresh();
    for(unsigned i=0;i<NPART;i++) {
        if(!i)host_preset(&trk[i],ENGI_PHYS,0);
        pf.mg[i]=32768;trk[i].p[P_VOICE]=V_MONO;
        trk_note_on(&trk[i],40+i,100);
    }
    REQUIRE(heavy_live(ENGI_PHYS,0,0));
    host_us=5000;fm1_alnk0_irq();fm1_alnk0_irq();fm1_alnk0_irq();
    REQUIRE(shed_count==1 && !heavy_live(ENGI_PHYS,0,0));
    trk_note_off(&trk[0],40);
    trk_note_on(&trk[0],60,100);
    REQUIRE(voices_busy()==NPART && heavy_live(ENGI_PHYS,0,0));
    REQUIRE(heavy_get(ENGI_PHYS,0,0)!=0 && trk[0].v[0].note==60 && trk[0].v[0].gate);
    host_us=100;fm1_alnk0_irq(); /* previous overload may retire one more oldest peer */
    REQUIRE(trk[0].v[0].active && trk[0].v[0].gate);
    trk_note_off(&trk[0],60);REQUIRE(!trk[0].v[0].gate);
}
static void priorities(void)
{
    leads();
    host_us=5000;fm1_alnk0_irq();fm1_alnk0_irq();
    /* Release after building the overload history, while the envelope is live. */
    trk_note_off(&trk[NPART-1],40+NPART-1);
    fm1_alnk0_irq();
    REQUIRE(shed_count==1 && !trk[NPART-1].v[0].active && trk[0].v[0].active);
    leads();
    /* Replace the last protected lead with an extra held POLY note. */
    voice_kill(&trk[NPART-1].v[0]);
    trk[0].p[P_VOICE]=V_POLY;trk_note_on(&trk[0],72,100);
    voice_t *extra=held(&trk[0],72);REQUIRE(extra && voices_busy()==NPART);
    host_us=5000;fm1_alnk0_irq();fm1_alnk0_irq();fm1_alnk0_irq();
    REQUIRE(shed_count==1 && !extra->active && trk[0].v[0].active);
    REQUIRE(voices_busy()==NPART-1);
}
#endif
int main(void)
{
    sustained();transient_and_recovery();
#if NPART >= NVOICE
    priorities();protected_modes_and_retrigger();
#endif
    printf("overload: tracks=%u assertions=%u sustained fallback, priorities, transient/recovery and auxiliary exclusion PASS; simulated clock only\n",NPART,assertions);
    return 0;
}
