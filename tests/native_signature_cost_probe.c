/* SPDX-License-Identifier: GPL-3.0-only */
/* Optional host-only benchmark; no timing threshold or target deadline claim. */
#define NPART 8
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include "../firmware/src/d8p1_runtime.c"
#include <time.h>
#ifdef __APPLE__
#include <libproc.h>
#include <sys/resource.h>
#include <unistd.h>
#endif
static volatile uint32_t sink;
static unsigned errors;
static uint8_t input[D8P1_LIMIT],wire[D8P1_LIMIT];
static uint64_t clock_ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;}
static uint64_t instructions(void){
#ifdef __APPLE__
 struct rusage_info_v4 ri;if(!proc_pid_rusage(getpid(),RUSAGE_INFO_V4,(rusage_info_t*)&ri))return ri.ri_instructions;
#endif
 return 0;
}
__attribute__((noinline)) static uint32_t native_call(void){uint32_t h=0;if(d8p1_signature_runtime(&h)!=D8RT_OK)errors++;return h;}
__attribute__((noinline)) static uint32_t upstream_call(void){return autosave_sig();}
static void batch(const char*name,const char*method,unsigned round,unsigned reps,uint32_t(*f)(void)){
 uint64_t i0=instructions(),t0=clock_ns();
 for(unsigned i=0;i<reps;i++){__asm__ volatile("":::"memory");sink^=f();}
 uint64_t t1=clock_ns(),i1=instructions();
 printf("batch,%s,%s,%u,%u,%llu,%llu\n",name,method,round,reps,(unsigned long long)(t1-t0),(unsigned long long)((i0&&i1)?i1-i0:0));
}
static void setup(const char*path){FILE*f=fopen(path,"rb");if(!f){perror(path);exit(2);}size_t n=fread(input,1,sizeof input,f);if(ferror(f)||fgetc(f)!=EOF)exit(2);fclose(f);if(d8p1_load_runtime(input,n,15))exit(3);int32_t out[CTL*2];mix_block(out,CTL);}
static void run_case(const char*name){size_t n=0;if(d8p1_capture_runtime(wire,sizeof wire,&n))exit(4);const d8p1_arrangement*a=d8p1_runtime_arrangement();printf("case,%s,wire=%zu,motion=%u,banks=%u,scenes=%u,rows=%u,native=%u,upstream=%u\n",name,n,motion.count,a?a->banks:0,a?a->scenes:0,a?a->rows:0,native_call(),upstream_call());for(unsigned i=0;i<200;i++){sink^=native_call();sink^=upstream_call();}for(unsigned round=0;round<9;round++){if(round&1){batch(name,"upstream",round,1000,upstream_call);batch(name,"native",round,1000,native_call);}else{batch(name,"native",round,1000,native_call);batch(name,"upstream",round,1000,upstream_call);}}}
int main(void){ui_power_on();int32_t audio[CTL*2];mix_block(audio,CTL);puts("record,case,method,round,repetitions,elapsed_ns,instructions");setup("tests/fixtures/d8p1/minimal.d8p");run_case("minimal");setup("tests/fixtures/d8p1/maximum.d8p");run_case("fixture-maximal56");for(unsigned place=0;motion.count<MOTION_MAX && place<512;place++){unsigned exists=0;for(unsigned j=0;j<motion.count;j++)if(motion.event[j].place==place&&MOTION_ID(&motion.event[j])==P_DIST)exists=1;if(!exists){motion.event[motion.count].place=(uint16_t)place;motion.event[motion.count].param=P_DIST;motion.event[motion.count].value=64;motion.count++;}}if(!motion_v1_valid(&motion)||motion.count!=64)exit(5);run_case("maximal64");uint32_t before=native_call();int16_t level=trk[7].p[P_LEVEL];motion_base_valid=1;motion_active[7][P_LEVEL>>5]|=1u<<(P_LEVEL&31);motion_base[7][P_LEVEL]=level;trk[7].p[P_LEVEL]=level==127?126:level+1;printf("stored_base_same,%u\n",native_call()==before);run_case("maximal64-stored-base");motion_base[7][P_LEVEL]=trk[7].p[P_LEVEL];printf("stored_base_change,%u\n",native_call()!=before);puts("end");fprintf(stderr,"errors=%u sink=%u counter=%s\n",errors,sink,instructions()?"available":"unavailable");return errors!=0;}
