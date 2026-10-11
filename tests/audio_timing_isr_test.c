/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual ALNK0 ISR and DSP, simulated clock/DMA; fresh processes compare build
 * flag 0 with flag 1 paused and collecting. No target timing claim. */
static void nested_policy_probe(void);
static unsigned clock_probes, record_probes;
#define AUDIO_CLOCK_TEST_HOOK() do { nested_policy_probe(); clock_probes++; } while(0)
#define D8_TIMING_RECORD_TEST_HOOK() do { nested_policy_probe(); record_probes++; } while(0)
#define AUDIO_TEST_NO_MAIN 1
#include "audio_test.c"
#undef AUDIO_TEST_NO_MAIN
#include <assert.h>
static void nested_policy_probe(void) { assert(felucca_dbg.in_audio == 1u); }
static uint64_t digest=UINT64_C(14695981039346656037);
static void hash_word(uint32_t value)
{
    for(unsigned j=0;j<4;j++) {
        digest^=(value>>(8*j))&255u;
        digest*=UINT64_C(1099511628211);
    }
}
int main(int argc,char **argv)
{
    (void)argv;
    fresh();
    host_us=500;host_nest=30;
    memset(&pf,0,sizeof pf);pf.src=pf.next=PF_N;pf.lc=PF_TOP;
    for(unsigned i=0;i<NTRK;i++) {
        pf.mg[i]=32768;
        trk[i].p[P_VOICE]=V_MONO;
        trk[i].p[P_DIST]=trk[i].p[P_CHOR]=trk[i].p[P_DLY]=trk[i].p[P_REV]=0;
        trk_note_on(&trk[i],48+i*2,100);
    }
#if DABBL8_TIMING_DIAGNOSTIC
    if(argc>1)assert(audio_timing_start());
#else
    assert(argc==1);
#endif
    unsigned audible=0;
    for(unsigned i=0;i<256;i++) {
        if(i==128)for(unsigned k=0;k<NTRK;k++)trk_note_off(&trk[k],48+k*2);
        fm1_alnk0_irq();
        assert(felucca_dbg.last_us==470 && !felucca_dbg.in_audio);
        for(unsigned j=0;j<HALF_WORDS;j++) {
            hash_word((uint32_t)abuf[j]);audible|=(abuf[j]!=0);
        }
        hash_word(voices_busy());
        for(unsigned k=0;k<NTRK;k++)for(unsigned j=0;j<NVOICE;j++) {
            hash_word(trk[k].v[j].active);hash_word(trk[k].v[j].gate);
            hash_word(trk[k].v[j].note);hash_word((uint32_t)trk[k].v[j].env);
        }
    }
    assert(audible && clock_probes);
#if DABBL8_TIMING_DIAGNOSTIC
    assert(record_probes == (argc > 1 ? 512u : 0u));
#else
    assert(!record_probes);
#endif
    printf("actual audio parity: tracks=%u halves=%u legacy_us=%u legacy_max=%u cpu_q8=%u shed=%u trace=%016llx audible=1\n",
           NTRK,audio_halves,felucca_dbg.last_us,felucca_dbg.max_us,song.cpu_q8,shed_count,(unsigned long long)digest);
#if DABBL8_TIMING_DIAGNOSTIC
    if(argc>1) {
        assert(audio_timing.render.count==256 && audio_timing.inclusive.count==256);
        assert(audio_timing.render.max_ticks==470 && audio_timing.inclusive.max_ticks==1500);
        assert(audio_timing.render.bins[0]==256 && audio_timing.inclusive.bins[1]==256);
        assert(audio_timing.first_half==1 && audio_timing.last_half==256);
        host_pending=0;fm1_alnk0_irq();assert(audio_timing.render.count==256);
        assert(audio_timing_stop());
        host_pending=FM1_AUDIO_HALF;fm1_alnk0_irq();assert(audio_timing.render.count==256);
        assert(audio_timing_start());assert(audio_timing.generation==2);
        host_clock=UINT32_MAX-100;fm1_alnk0_irq();
        assert(audio_timing.render.max_ticks==470 && audio_timing.inclusive.max_ticks==1500);
        assert(audio_timing.render.count==1 && !audio_timing.invalid && !audio_timing.saturated);
        assert(audio_timing_stop());
        puts("actual timing ISR: paired accounting, auxiliary exclusion, pause/reset and timer wrap PASS; simulated clock only");
    } else assert(!audio_timing.render.count);
#endif
}
