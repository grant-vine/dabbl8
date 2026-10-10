/* SPDX-License-Identifier: GPL-3.0-only */
#define D8OUTPUT_QUEUE_NO_MAIN 1
#include QUEUE_TEST_HEADER
#include <time.h>
#include <libproc.h>
#include <sys/resource.h>
#include <unistd.h>
static uint64_t ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return(uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;}
static uint64_t ins(void){struct rusage_info_v4 r;return proc_pid_rusage(getpid(),RUSAGE_INFO_V4,(rusage_info_t*)&r)?0:r.ri_instructions;}
static uint32_t random_state=17;
static void fill(int32_t *p,unsigned mode){for(unsigned i=0;i<CTL*2;i++){random_state=random_state*1664525u+1013904223u;p[i]=mode==0?0:mode==1?(i&1?-12000:18000):mode==2?((random_state&7)?(int32_t)((random_state>>12)&32767)-16384:0):(int32_t)((random_state>>12)&32767)-16384;}}
static void run(unsigned mode,unsigned full,unsigned reps){int32_t p[CTL*2];fill(p,mode);unsigned acc=0;for(unsigned b=0;b<reps;b++){
 if(full){if(!(b&3))uac_render_start();uac_tap48(p,CTL);acc+=CTL*160;while(acc>=147*48){consume();acc-=147*48;}}
 else{ua_r=ua_w-UA_PRIME48;uac_tap48(p,CTL);}
}}
int main(int argc,char **argv){if(argc>1){FILE*f=fopen(argv[1],"wb");if(!f)return 2;for(unsigned mode=0;mode<4;mode++){cold();stream(1);for(unsigned size=0;size<=CTL;size++){int32_t p[CTL*2];fill(p,mode);ua_r=ua_w-UA_PRIME48;uac_tap48(p,size);oracle();fwrite(ua_ring,sizeof ua_ring,1,f);fwrite(&rs,sizeof rs,1,f);uint32_t meta[]={uq.ring,uq.history,ua_w,ua_r};fwrite(meta,sizeof meta,1,f);consume();fwrite(hs_packet,sizeof hs_packet,1,f);} }for(unsigned rate=0;rate<2;rate++){cold();stream(rate);host_preset(&trk[0],0,5);trk[0].p[P_SLCR]=0;trk_note_on(&trk[0],60,100);for(unsigned block=0;block<80;block++){qa_half=block&1;fm1_alnk0_irq();oracle();fwrite(abuf,sizeof abuf,1,f);fwrite(ua_ring,sizeof ua_ring,1,f);fwrite(&rs,sizeof rs,1,f);for(unsigned packet=0;packet<3;packet++){consume();fwrite(hs_packet,sizeof hs_packet,1,f);}}}fclose(f);return failures!=0;}
 puts("mode,full,round,reps,ns,instructions,overruns");for(unsigned round=0;round<9;round++)for(unsigned mode=0;mode<4;mode++)for(unsigned full=0;full<2;full++){cold();stream(1);run(mode,full,1000);uint64_t i0=ins(),t0=ns();run(mode,full,20000);uint64_t t1=ns(),i1=ins();oracle();printf("%u,%u,%u,20000,%llu,%llu,%u\n",mode,full,round,(unsigned long long)(t1-t0),(unsigned long long)(i1-i0),uac.overruns);}return failures!=0;}
