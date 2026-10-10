/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual audio and TIMER5 IRQs: local DSP totals publish before main resumes. */
static void local_store_hook(void);
#define FT_LOCAL_STORE_HOOK local_store_hook
#define D8OUTPUT_QUEUE_NO_MAIN 1
#include "native_output_queue_test.c"
static unsigned hook_enabled,hook_calls,scans,stale;
static void fm1_timer5_ack(void){}
static void fm1_input_tick(void){scans++;}
#define FELUCCA_TIMER5_ONLY 1
#define felucca_dbg queue_audio_dbg
#include "../firmware/src/main.c"
#undef felucca_dbg
static void local_store_hook(void){
 if(!hook_enabled)return;
 CHECK(queue_audio_dbg.in_audio==1);unsigned rd=reads,wr=writes;
 hs_regs[4][17]=0;fm1_timer5_irq();CHECK(reads==rd&&writes==wr);
 if(!(hook_calls%100)){uint32_t c=0;for(unsigned i=0;i<sizeof rev_comb/2;i++)c+=rev_comb[i]!=0;if(c!=fx_tail.comb)stale++;}
 hook_calls++;
}
static void tails_oracle(void){
 uint32_t d=0,c=0,r=0,a=0;
 for(unsigned i=0;i<DLY_LEN;i++)d+=dly_buf[i]!=0;
 for(unsigned i=0;i<CHO_LEN;i++)c+=cho_buf[i]!=0;
 for(unsigned i=0;i<sizeof rev_comb/2;i++)r+=rev_comb[i]!=0;
 if(fx.rtype)for(unsigned i=0;i<4*(SP_N+1);i++)a+=rev_u.sp[i]!=0;
 else for(unsigned i=0;i<sizeof rev_ap/2;i++)a+=rev_ap[i]!=0;
 CHECK(d==fx_tail.delay&&c==fx_tail.chorus&&r==fx_tail.comb&&a==fx_tail.ap);
}
int main(void){
 cold();tails_oracle();CHECK(fx_resident_quiet());stream(1);host_preset(&trk[0],0,5);trk[0].p[P_SLCR]=0;trk[0].p[P_AMODE]=0;
 trk[0].p[P_CHOR]=trk[0].p[P_DLY]=trk[0].p[P_REV]=100;trk_note_on(&trk[0],60,100);
 hook_enabled=1;
 for(unsigned b=0;b<80;b++){song.g[G_RTYPE]=(b/10)&1u;qa_half=b&1u;fm1_alnk0_irq();tails_oracle();oracle();CHECK(!queue_audio_dbg.in_audio&&!df_automatic_quiet());}
 hook_enabled=0;CHECK(hook_calls>1000&&stale>0&&scans==hook_calls&&queue_audio_dbg.nested==hook_calls);CHECK(hs_sends>0);
 printf("Native FX publication: %u checks, %u failures; %u actual nested TIMER5 IRQs, %u stale-in-flight comb observations, 80 real audio halves with complete counter oracles; main-loop-only reader contract, no device timing claim\n",checks,failures,hook_calls,stale);
 return failures!=0;
}
