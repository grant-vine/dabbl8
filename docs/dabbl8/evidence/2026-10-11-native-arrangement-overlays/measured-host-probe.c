/* SPDX-License-Identifier: GPL-3.0-only; private Mac/virtual-DMA measurement. */
#define FELUCCA_UAC 1
#define FELUCCA_USB_HAL "../../tests/native_queue_usb_hal.h"
#include "arrangement_fixture.h"
#include "native_queue_audio_fixture.h"
#include <libproc.h>
#include <sys/resource.h>
static uint64_t instructions(void){struct rusage_info_v4 r;if(proc_pid_rusage(getpid(),RUSAGE_INFO_V4,(rusage_info_t*)&r))return 0;return r.ri_instructions;}
static uint64_t nanos(void){struct timespec s;clock_gettime(CLOCK_MONOTONIC,&s);return (uint64_t)s.tv_sec*1000000000+s.tv_nsec;}
static void bench_fixture(unsigned apply){
 seed();
 for(unsigned i=0;i<3;i++){
#if BENCH_OVERLAYS
 d8p1_scene_state *s=&d8p1_runtime_cache.arrangement.scene[i];s->apply=apply;s->mute=1;for(unsigned t=0;t<8;t++){s->level[t]=100;s->pan[t]=0;s->transpose[t]=12;}
#endif
 }
 prepare_start();
 for(unsigned o=0;o<3;o++)for(unsigned t=0;t<8;t++)for(unsigned k=0;k<NSTEP;k++){
 step_t *s=&chain.source[o].step[t][k];s->n=4;s->hit=255;s->time=ST_NOTE;s->vel=99;for(unsigned j=0;j<4;j++)s->note[j]=(uint8_t)(60+t+j);
 }
 song.master_q12=4096;for(unsigned t=0;t<8;t++){host_preset(&trk[t],0,5);trk[t].eng_req=0;trk[t].p[P_MUTE]=0;trk[t].p[P_AMODE]=0;trk[t].p[P_CHRD]=CH_OFF;trk[t].p[P_QUANT]=0;trk[t].p[P_LEVEL]=100;trk[t].p[P_CHOR]=trk[t].p[P_DLY]=trk[t].p[P_REV]=0;}
 mix_block(audio,CTL);for(unsigned t=0;t<8;t++){trk_all_off(&trk[t]);trk_note_on(&trk[t],(uint8_t)(60+t),100);}
}
static void pre(unsigned kind){
 usb.config=1;uac.alt=uac.flowing=uac.go=uac.r48=uac.ring48=1;ua_r=ua_w;chain.running=1;chain.row=0;chain.remaining=kind==1?1:2;chain.native.policy.bank=chain.native.policy.scene=0;chain.native.policy.pending=D8ARR_NONE;
#if BENCH_OVERLAYS
 chain.native.policy.fill_pending=D8ARR_NONE;chain.native.policy.fill_active=0;d8arr_scene_publish();
#endif
 song.playing=1;song.g[G_CLOCK]=0;clk_step=kind?15:0;clk_pos=kind?div_samples(2)-1:0;cin_left=0;transport_req=0;panic_req=0;shed_req=0;
 for(unsigned t=0;t<8;t++){
 track_t *q=&trk[t];q->seq_idx=0;q->seq_pos=0;q->rat_left=q->rh_n=0;q->seq_n=kind?12:0;
 for(unsigned j=0;j<12;j++)q->seq_notes[j]=(uint8_t)(20+j);
 if(kind){memcpy(motion_base[t],q->p,sizeof q->p);for(unsigned j=0;j<4;j++)motion_active[t][j]=j==3?7u:0xffffffffu;}
 }
 if(kind==2){
#if BENCH_OVERLAYS
 if(d8arr_request_fill(2))exit(5);
#else
 if(d8arr_request_row(2))exit(5);
#endif
 }
}
int main(int argc,char **argv){
 unsigned kind=argc>1?atoi(argv[1]):0,apply=argc>2?atoi(argv[2]):0,reps=argc>3?atoi(argv[3]):256;
 bench_fixture(apply);unsigned gates=0;for(unsigned t=0;t<8;t++)for(unsigned v=0;v<NVOICE;v++)gates+=trk[t].v[v].gate!=0;if(gates!=8)exit(10);uint64_t ns=0,ins=0;unsigned transitions=0,nonzero_outputs=0;volatile uint32_t sink=0;
 for(unsigned i=0;i<reps+16;i++){
 pre(kind);qa_half=i&1;uint64_t i0=instructions(),t0=nanos();fm1_alnk0_irq();uint64_t t1=nanos(),i1=instructions();
 if(i>=16){ns+=t1-t0;ins+=i1-i0;}sink+=(uint32_t)abuf[(i&1)*HALF_WORDS];for(unsigned j=0;j<HALF_WORDS;j++)if(abuf[(i&1)*HALF_WORDS+j]){nonzero_outputs++;break;}
 if(kind){if(chain.row!=(kind==2?2:1))exit(6);for(unsigned t=0;t<8;t++)if(trk[t].seq_n!=12)exit(7);transitions++;}
 }
 printf("%u,%u,%u,%llu,%llu,%u,%u,%u,%u,%u\n",kind,apply,reps,(unsigned long long)ns,(unsigned long long)ins,transitions,qa_acks,(unsigned)sink,NVOICE,nonzero_outputs);
 return failures?9:0;
}
