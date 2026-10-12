/* SPDX-License-Identifier: GPL-3.0-only */
/* Optional host timing/instruction probe; no target-cycle or pass-threshold claim. */
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1
#endif
#define D8_DRY_SENDS_NO_MAIN 1
#include "native_dry_sends_test.c"
#ifdef __APPLE__
#include <libproc.h>
#include <sys/resource.h>
#endif
static uint64_t dry_probe_ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;}
static uint64_t instructions(void){
#ifdef __APPLE__
 struct rusage_info_v4 r;if(!proc_pid_rusage(getpid(),RUSAGE_INFO_V4,(rusage_info_t*)&r))return r.ri_instructions;
#endif
 return 0;
}
static volatile uint32_t sink;
int main(void){puts("wet,round,calls,ns,instructions,sink");for(unsigned wet=0;wet<2;wet++)for(unsigned round=0;round<9;round++){
 fresh();for(unsigned t=0;t<8;t++){host_preset(&trk[t],0,5);trk[t].p[P_CHOR]=wet?40:0;trk[t].p[P_DLY]=wet?50:0;trk[t].p[P_REV]=wet?60:0;trk[t].p[P_DIST]=0;trk_note_on(&trk[t],48+t,100);}
 global_tuning_t tuning=global_tuning();
 for(unsigned b=0;b<200;b++){memset(mix_l,0,sizeof mix_l);memset(mix_r,0,sizeof mix_r);memset(send_c,0,sizeof send_c);memset(send_d,0,sizeof send_d);memset(send_r,0,sizeof send_r);for(unsigned t=0;t<8;t++)mix_part_tuned(&trk[t],CTL,tuning);}
 uint64_t i0=instructions(),t0=dry_probe_ns();
 for(unsigned b=0;b<5000;b++){memset(mix_l,0,sizeof mix_l);memset(mix_r,0,sizeof mix_r);memset(send_c,0,sizeof send_c);memset(send_d,0,sizeof send_d);memset(send_r,0,sizeof send_r);for(unsigned t=0;t<8;t++)mix_part_tuned(&trk[t],CTL,tuning);sink+=(uint32_t)mix_l[b%CTL];}
 uint64_t t1=dry_probe_ns(),i1=instructions();printf("%u,%u,40000,%llu,%llu,%u\n",wet,round,(unsigned long long)(t1-t0),(unsigned long long)(i1-i0),sink);
 }return 0;}
