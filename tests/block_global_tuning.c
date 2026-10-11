/* SPDX-License-Identifier: GPL-3.0-only */
/* Identical source compiles against frozen984f and candidate; independent trace digest. */
#define NPART 8
#define main hostsim_main
#ifndef D8_TUNING_HOSTSIM
#define D8_TUNING_HOSTSIM "hostsim.c"
#endif
#include D8_TUNING_HOSTSIM
#undef main
static uint64_t digest=1469598103934665603ULL;
static unsigned checks,cases,failures,silent,unadmitted;
static void mixhash(uint32_t x){for(unsigned i=0;i<4;i++){digest^=(x>>(8*i))&255;digest*=1099511628211ULL;}}
static void observe(int32_t *a,unsigned n){for(unsigned i=0;i<n;i++)mixhash((uint32_t)a[i]);}
static void check(int value){checks++;failures+=!value;}
int main(void){
#ifdef D8_TUNING_OPTIMIZED
 for(int cents=-32768;cents<=32767;cents++){
  song.g[G_TUNE]=(int16_t)cents;global_tuning_t t=global_tuning();
  int coarse=cents>=0?cents*16/100:-((-cents*16+99)/100);
  check(t.tune==coarse);check(t.fine==(cents*16-coarse*100)*2367/16000);
 }
 /* Formula checks are not part of audio trace comparison. */
 checks=0;
#endif
 for(int cents=-50;cents<=50;cents++)for(unsigned engine=0;engine<14;engine++)if(engine!=1)
  for(unsigned mode=0;mode<4;mode++)for(unsigned bank=0;bank<8;bank++){
   memset(trk,0,sizeof trk);memset(&song,0,sizeof song);
   memset(heavy_owner,0,sizeof heavy_owner);memset(heavy_pool,0,sizeof heavy_pool);
   memset(midi_bend_q8,0,sizeof midi_bend_q8);memset(midi_bend_target,0,sizeof midi_bend_target);
   host_tracks_init();song.sel=(uint8_t)bank;fm1_in.notes=0;
   host_preset(&trk[bank],engine,0);trk[bank].p[P_VOICE]=(int16_t)mode;
   trk[bank].p[P_SLCR]=0;trk[bank].p[P_AMODE]=0;trk[bank].p[P_DIST]=0;
   trk_note_on(&trk[bank],engine==10?36:60,100);
   trk_note_on(&trk[bank],engine==10?42:67,92);
   check(voices_busy()>0); if(!voices_busy())unadmitted++; unsigned audible=0;
   int32_t out[CTL*2],direct[CTL];
   for(unsigned block=0;block<4;block++){
    song.g[G_TUNE]=(int16_t)(block==0?cents:block==1?-50:block==2?50:-cents);
    /* Pending keyboard event enters before tuning capture in mix_block. */
    fm1_in.notes=block==1?1u<<3:0;
    mix_block(out,CTL);observe(out,CTL*2);for(unsigned z=0;z<CTL*2;z++)audible|=out[z]!=0;
    check(voices_busy()<=8);mixhash(voices_busy());mixhash((uint32_t)song.g[G_TUNE]);
    for(unsigned t=0;t<8;t++){mixhash((uint32_t)midi_bend_q8[t]);mixhash(trk[t].lfo_ph);
     for(unsigned v=0;v<8;v++){mixhash(trk[t].v[v].active);mixhash(trk[t].v[v].gate);mixhash(trk[t].v[v].env);mixhash(trk[t].v[v].pitch_cur);}}
   }
   /* Direct wrappers must read fresh tuning between calls, without a cached block. */
   song.g[G_TUNE]=-50;mixhash(track_render(&trk[bank],direct,CTL));observe(direct,CTL);
   song.g[G_TUNE]=50;mixhash(track_render(&trk[bank],direct,CTL));observe(direct,CTL);
   for(unsigned i=0;i<CTL;i++)send_c[i]=send_d[i]=send_r[i]=mix_l[i]=mix_r[i]=0;
   song.g[G_TUNE]=(int16_t)cents;mix_part(&trk[bank],CTL);observe(mix_l,CTL);observe(mix_r,CTL);
   if(!audible)silent++;
   cases++;
  }
 /* Simultaneous eight-track common and mixed arrangements. */
 unsigned arrangements=0,mixed_audible=0;
 const unsigned mixed[8]={0,2,3,4,6,7,10,11};
 for(unsigned variety=0;variety<2;variety++)for(unsigned treatment=0;treatment<3;treatment++){
  memset(trk,0,sizeof trk);memset(&song,0,sizeof song);
  memset(heavy_owner,0,sizeof heavy_owner);memset(heavy_pool,0,sizeof heavy_pool);
  memset(&pf,0,sizeof pf);perf_held=perf_latched=perf_act=perf_solo=perf_kill=0;
  memset((void *)perf_k,0,sizeof perf_k);host_tracks_init();fm1_in.notes=0;
  for(unsigned t=0;t<8;t++){
   unsigned engine=variety?mixed[t]:0;host_preset(&trk[t],engine,0);
   trk[t].p[P_VOICE]=V_POLY;trk[t].p[P_SLCR]=0;trk[t].p[P_AMODE]=0;
   trk[t].p[P_ATK]=0;trk[t].p[P_REL]=32;trk[t].p[P_SUS]=127;
   if(treatment){trk[t].p[P_CHOR]=24;trk[t].p[P_DLY]=32;trk[t].p[P_REV]=24;
    trk[t].p[P_M1SRC]=MS_LFO;trk[t].p[P_M1DST]=MD_PAN;trk[t].p[P_M1AMT]=24;
    trk[t].p[P_M2SRC]=MS_VEL;trk[t].p[P_M2DST]=MD_PITCH;trk[t].p[P_M2AMT]=3;}
   trk_note_on(&trk[t],engine==10?36:60+t,100);
  }
  check(voices_busy()==8);
  for(unsigned t=0;t<8;t++){unsigned active=0;for(unsigned v=0;v<8;v++)active+=trk[t].v[v].active!=0;check(active==1);}
  unsigned audible=0,tail_audible=0,send_seen=0,performance_seen=0;
  for(unsigned block=0;block<96;block++){
   int32_t out[CTL*2];song.g[G_TUNE]=(int16_t)((int)(block%101)-50);
   if(treatment==2){perf_k[0]=(int8_t)(block<24?-30:0);perf_k[1]=(int8_t)(block<24?20:0);perf_k[2]=(int8_t)(block<24?10:0);}
   if(block==24)for(unsigned t=0;t<8;t++){trk_note_off(&trk[t],variety&&mixed[t]==10?36:60+t);for(unsigned v=0;v<8;v++)check(!trk[t].v[v].gate);}
   mix_block(out,CTL);observe(out,CTL*2);check(voices_busy()<=8);mixhash(voices_busy());
   performance_seen|=pf.busy!=0;for(unsigned z=0;z<CTL;z++)send_seen|=(send_c[z]!=0?1:0)|(send_d[z]!=0?2:0)|(send_r[z]!=0?4:0);
   for(unsigned z=0;z<CTL*2;z++){if(block<24)audible|=out[z]!=0;else tail_audible|=out[z]!=0;}
   for(unsigned t=0;t<8;t++){mixhash(trk[t].lfo_ph);mixhash(trk[t].tail);for(unsigned v=0;v<8;v++){mixhash(trk[t].v[v].active);mixhash(trk[t].v[v].gate);mixhash(trk[t].v[v].env);mixhash(trk[t].v[v].pitch_cur);}}
  }
  check(audible);check(tail_audible);if(treatment)check(send_seen==7);if(treatment==2)check(performance_seen);mixed_audible+=!!audible;arrangements++;
 }
 check(silent==0);check(unadmitted==0);
 printf("cases=%u checks=%u failures=%u silent=%u unadmitted=%u arrangements=%u audible_arrangements=%u audio_state_trace=%016llx\n",cases,checks,failures,silent,unadmitted,arrangements,mixed_audible,(unsigned long long)digest);
 return failures!=0;
}
