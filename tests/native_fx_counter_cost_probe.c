/* SPDX-License-Identifier: GPL-3.0-only */
#define NPART 8
#define UI_TEST_NO_MAIN 1
#ifndef TREE_UI
#define TREE_UI "ui_test.c"
#endif
#include TREE_UI
#include <time.h>
#ifdef __APPLE__
#include <libproc.h>
#endif
#include <sys/resource.h>
#include <unistd.h>
static uint64_t ns(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return (uint64_t)ts.tv_sec*1000000000ull+ts.tv_nsec;}
static uint64_t ins(void){
#ifdef __APPLE__
struct rusage_info_v4 r;if(!proc_pid_rusage(getpid(),RUSAGE_INFO_V4,(rusage_info_t*)&r))return r.ri_instructions;
#endif
return 0;
}
static volatile uint32_t sink;
int main(void){ui_power_on();song.g[G_RSIZE]=80;song.g[G_RDAMP]=60;
 int32_t input[CTL],out[2*CTL];puts("model,state,round,calls,ns,instructions,sink");
 for(unsigned model=0;model<2;model++)for(unsigned dense=0;dense<2;dense++)for(unsigned round=0;round<9;round++){
  memset(&fx,0,sizeof fx);rev_clear();fx.rtype=model;for(unsigned i=0;i<CTL;i++)input[i]=dense?(int32_t)(2000+(i*17)%1000):0;
  for(unsigned b=0;b<200;b++){memset(out,0,sizeof out);if(model)rev_spring(input,out,CTL);else rev_room(input,out,CTL);}
  uint64_t i0=ins(),t0=ns();for(unsigned b=0;b<10000;b++){memset(out,0,sizeof out);if(model)rev_spring(input,out,CTL);else rev_room(input,out,CTL);sink+=(uint32_t)out[b%(2*CTL)];}
  uint64_t t1=ns(),i1=ins();printf("%u,%u,%u,10000,%llu,%llu,%u\n",model,dense,round,(unsigned long long)(t1-t0),(unsigned long long)(i1-i0),sink);
 }
 return 0;
}
