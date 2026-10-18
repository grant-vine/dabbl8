/* SPDX-License-Identifier: GPL-3.0-only */
/* Exact attractor and hidden delayed-audio witnesses, real render equations. */
#define D8FLASH_RUNTIME_NO_MAIN 1
#include "d8p1_flash_runtime_test.c"
static void fresh_fx(void)
{
 ui_power_on();memset(&fx,0,sizeof fx);rev_clear();memset(dly_buf,0,sizeof dly_buf);memset(cho_buf,0,sizeof cho_buf);
 memset(&fx_tail,0,sizeof fx_tail);memset(&fx_uniform_wait,0,sizeof fx_uniform_wait);
 dc_l=dc_r=dce_l=dce_r=lc_l1=lc_l2=lc_r1=lc_r2=0;memset(lce,0,sizeof lce);
 sb_lp1=sb_lp2=sb_lp3=sb_lp4=sb_env=sb_h1=sb_h2=sb_hl=0;click_req=clk.env=0;
 pf.src=pf.next=PF_N;fx_lowcut=fx_usb_fixed=0;song.master_q12=4096;song.g[G_RTYPE]=0;song.g[G_DMIX]=0;
 for(unsigned t=0;t<NTRK;t++)trk[t].tail=0;
}
static int32_t render_sample(void)
{
 int32_t z=0,w=0;fx_buses(&z,&z,&z,&w,1);
 int32_t l=((w>>2)*(int32_t)song.master_q12)>>10,r=l;master_out(&l,&r);CHECK(l==r);return l;
}
static void room_seed(void)
{
 fresh_fx();
 for(unsigned i=0;i<DLY_LEN;i++)dly_buf[i]=-1;
 for(unsigned i=0;i<sizeof rev_comb/2;i++)rev_comb[i]=-10;
 for(unsigned i=0;i<556;i++)rev_ap[i]=-81;
 for(unsigned i=556;i<997;i++)rev_ap[i]=(i&1)?-83:-82;
 song.g[G_DFDBK]=60;song.g[G_DMIX]=90;song.g[G_RSIZE]=90;song.g[G_RDAMP]=60;
 song.master_q12=1914;fx.dly_lp=-2;for(unsigned i=0;i<4;i++)fx.comb_lp[i]=-11;
 fx_tail.delay=DLY_LEN;fx_uniform_wait.delay=DLY_LEN;fx_uniform_wait.comb=1356;fx_uniform_wait.ap=556;fx_uniform_wait.ap_fixed=441;
 dc_l=dc_r=-1344;}
static void render_chunk(unsigned n)
{
 /* Match the real render's CTL subdivision of a64-sample caller request. */
 while(n){
  unsigned count=n>CTL?CTL:n;int32_t z[CTL]={0},wet[CTL]={0};fx_buses(z,z,z,wet,count);
  for(unsigned i=0;i<count;i++){int32_t l=((wet[i]>>2)*(int32_t)song.master_q12)>>10,r=l;master_out(&l,&r);CHECK(!l&&!r);}
  n-=count;
 }
}
static void dry_mix_seed(void)
{
 fresh_fx();memset(sl,0,sizeof sl);memset(&pf,0,sizeof pf);pf.src=pf.next=PF_N;
 for(unsigned k=0;k<NTRK;k++){
  track_t *t=&trk[k];
  t->p[P_DIST]=t->p[P_SLCR]=t->p[P_CHOR]=t->p[P_DLY]=t->p[P_REV]=0;
  for(unsigned m=0;m<NMSLOT;m++)t->p[P_M1SRC+3u*m]=0;
 }
}
static uint64_t audio_digest(uint64_t hash,const int32_t *out,unsigned count)
{
 for(unsigned i=0;i<count;i++){
  uint32_t value=(uint32_t)out[i];
  for(unsigned b=0;b<4;b++){hash^=(value>>(8u*b))&255u;hash*=1099511628211ULL;}
 }
 return hash;
}
static void bypassed_dist_tail_retires(void)
{
 dry_mix_seed();track_t *t=&trk[0];int32_t out[CTL*2];
 for(unsigned k=0;k<NTRK;k++)trk[k].tail=16;
 t->dist_hp=36;t->dist_lp1=-45;t->dist_lp2=-46;
 CHECK(!fx_resident_quiet());
 uint64_t trace=1469598103934665603ULL;
 for(unsigned block=0;block<20;block++){
  mix_block(out,CTL);trace=audio_digest(trace,out,CTL*2);
  for(unsigned i=0;i<CTL*2;i++)CHECK(out[i]==0);
  for(unsigned k=0;k<NTRK;k++){
   CHECK(trk[k].tail==(block<16?15u-block:0u));
   for(unsigned v=0;v<NVOICE;v++)CHECK(!trk[k].v[v].active);
   CHECK(!slicer_busy(&trk[k]));
  }
  CHECK(t->dist_hp==36&&t->dist_lp1==-45&&t->dist_lp2==-46);
  CHECK(fx_resident_quiet()==(block>=15));
 }
 printf("Bypassed DIST: blocks=20 tail=%u hp=%d lp1=%d lp2=%d quiet=%d pcm=%016llx\n",
        t->tail,t->dist_hp,t->dist_lp1,t->dist_lp2,fx_resident_quiet(),(unsigned long long)trace);
 /* Retiring bypassed state must not admit a still-referenced DIST tail. */
 t->tail=16;t->p[P_DIST]=1;CHECK(!fx_resident_quiet());
 dry_mix_seed();t=&trk[0];t->p[P_DIST]=1;t->p[P_LEVEL]=127;
 t->tail=16;t->dist_hp=36;t->dist_lp1=-45;t->dist_lp2=-46;
 trace=1469598103934665603ULL;unsigned audible=0;
 for(unsigned block=0;block<20;block++){
  memset(mix_l,0,sizeof mix_l);memset(mix_r,0,sizeof mix_r);
  mix_part(t,CTL);trace=audio_digest(trace,mix_l,CTL);
  CHECK(t->tail==(block<16?15u-block:0u));
  for(unsigned i=0;i<CTL;i++)if(mix_l[i])audible=1;
 }
 CHECK(audible&&t->dist_lp1!=-45);
 printf("Enabled DIST tail: blocks=20 tail=%u hp=%d lp1=%d lp2=%d audible=%u pcm=%016llx\n",
        t->tail,t->dist_hp,t->dist_lp1,t->dist_lp2,audible,(unsigned long long)trace);
 /* Active voice rendering still refreshes the tail and retains its audio. */
 for(unsigned drive=0;drive<=35;drive+=35){
  dry_mix_seed();t=&trk[0];host_preset(t,0,5);
  t->p[P_DIST]=(int16_t)drive;t->p[P_CHOR]=t->p[P_DLY]=t->p[P_REV]=0;
  trk_note_on(t,60,100);trace=1469598103934665603ULL;audible=0;
  for(unsigned block=0;block<24;block++){
   mix_block(out,CTL);trace=audio_digest(trace,out,CTL*2);CHECK(t->tail==16);
   for(unsigned i=0;i<CTL*2;i++)if(out[i])audible=1;
  }
  CHECK(audible&&voices_busy());
  printf("Active DIST: drive=%u tail=%u audible=%u pcm=%016llx\n",drive,t->tail,audible,(unsigned long long)trace);
  trk_note_off(t,60);
 }
}
static void bypassed_dist_reenable_controls(void)
{
 for(unsigned idle=0;idle<2;idle++){
  dry_mix_seed();track_t *t=&trk[0];host_preset(t,0,5);int32_t out[CTL*2];
  t->p[P_DIST]=t->p[P_CHOR]=t->p[P_DLY]=t->p[P_REV]=0;t->p[P_REL]=0;t->p[P_LEVEL]=127;
  t->tail=16;t->dist_hp=36;t->dist_lp1=-45;t->dist_lp2=-46;
  for(unsigned block=0;block<20;block++)mix_block(out,CTL);
  CHECK(t->tail==0&&t->dist_hp==36&&t->dist_lp1==-45&&t->dist_lp2==-46);
  t->p[P_DIST]=1;
  if(idle){
   unsigned audible=0;
   for(unsigned block=0;block<4;block++){
    mix_block(out,CTL);CHECK(t->tail==0&&fx_resident_quiet());
    for(unsigned i=0;i<CTL*2;i++){if(out[i])audible=1;CHECK(out[i]==0);}
   }
   CHECK(t->dist_hp==36&&t->dist_lp1==-45&&t->dist_lp2==-46);
   printf("Idle DIST re-enable: blocks=4 tail=%u hp=%d lp1=%d lp2=%d audible=%u\n",
          t->tail,t->dist_hp,t->dist_lp1,t->dist_lp2,audible);
  }
  trk_note_on(t,60,100);uint64_t trace=1469598103934665603ULL;unsigned audible=0;
  for(unsigned block=0;block<24;block++){
   mix_block(out,CTL);trace=audio_digest(trace,out,CTL*2);
   CHECK(t->tail==16&&voices_busy()&&!autosave_quiet()&&!df_automatic_quiet());
   for(unsigned i=0;i<CTL*2;i++)if(out[i])audible=1;
  }
  CHECK(audible);
  printf("Re-enabled DIST note: idle=%u blocks=24 tail=%u audible=%u pcm=%016llx\n",
         idle,t->tail,audible,(unsigned long long)trace);
  trk_note_off(t,60);unsigned blocks=0;
  while(voices_busy()&&blocks++<4096){mix_block(out,CTL);if(voices_busy())CHECK(!autosave_quiet()&&!df_automatic_quiet());}
  CHECK(!voices_busy());unsigned guarded=0;
  for(unsigned block=0;block<17;block++){
   mix_block(out,CTL);
   if(t->tail&&(t->dist_hp||t->dist_lp1||t->dist_lp2)){CHECK(!fx_resident_quiet()&&!df_automatic_quiet());guarded=1;}
  }
  CHECK(guarded&&t->tail==0);
 }
}
int main(void)
{
 bypassed_dist_tail_retires();
 bypassed_dist_reenable_controls();
 room_seed();unsigned span=delay_samples();if(span<1356)span=1356;
 CHECK(!fx_resident_quiet());
 for(unsigned i=0;i<span;i++){CHECK(render_sample()==0);if(i+1<span)CHECK(!fx_resident_quiet());}
 CHECK(fx_resident_quiet()&&dc_l==-1344&&fx.dly_lp==-2&&rev_comb[0]==-10&&rev_ap[0]==-81);
 /* Changes to the equation revoke positive proof without touching DSP state. */
 song.master_q12=4096;CHECK(!fx_resident_quiet());song.master_q12=1914;CHECK(fx_resident_quiet());
 for(unsigned i=0;i<2048;i++)CHECK(render_sample()==0);
 song.g[G_RTYPE]=1;CHECK(!fx_resident_quiet());song.g[G_RTYPE]=0;
 FT16(ap,&rev_ap[556],-83);CHECK(!fx_resident_quiet());

 /* Actual64 and varying block sizes prove the same active history. */
 room_seed();unsigned total=0,block=0;
 while(total<span){
  unsigned n=(block++&1)?64:17;if(n>span-total)n=span-total;
  render_chunk(n);total+=n;if(total<span)CHECK(!fx_resident_quiet());
 }
 CHECK(fx_resident_quiet());
 unsigned bpm=(unsigned)song.g[G_BPM];
 song.g[G_BPM]=(int16_t)(bpm+1);CHECK(!fx_resident_quiet());
 uint32_t changed=delay_samples();render_chunk(64);
 CHECK(fx_uniform_wait.delay_span==changed&&fx_uniform_wait.delay==changed-64);
 CHECK(!fx_resident_quiet());
 song.g[G_BPM]=(int16_t)bpm;CHECK(!fx_resident_quiet());
 render_chunk(17);CHECK(fx_uniform_wait.delay_span==delay_samples()&&fx_uniform_wait.delay==delay_samples()-17);
 while(fx_uniform_wait.delay)render_chunk(fx_uniform_wait.delay>64?64:fx_uniform_wait.delay);
 CHECK(fx_resident_quiet());
 /* AP2 full441 sweep: no receipt may mature before all cells are covered. */
 fx_uniform_wait.ap_fixed=441;
 for(unsigned i=0;i<441;i++){CHECK(!render_sample());if(i+1<441)CHECK(!fx_resident_quiet());}
 CHECK(fx_resident_quiet());
 uint32_t hidden=(fx.ap_i[1]+300u)%441u;
 FT16(ap,&rev_ap[556+hidden],1000);CHECK(!fx_resident_quiet()&&fx_tail.ap);
 int32_t zero[CTL]={0},wet[CTL];
 for(unsigned i=0;i<14;i++)fx_buses(zero,zero,zero,wet,CTL);
 CHECK(fx_uniform_wait.ap_fixed&&fx_tail.ap&&!fx_resident_quiet());
 /* SPRING's negative LP deadband has exact zero loop/AP input. */
 fresh_fx();fx.rtype=1;song.g[G_RTYPE]=1;fx.sp_lp=-1;fx.dly_lp=-1;song.g[G_DFDBK]=0;
 unsigned spring_span=delay_samples();if(spring_span<SP_LEN)spring_span=SP_LEN;
 for(unsigned i=0;i<spring_span;i++)CHECK(render_sample()==0);
 CHECK(fx_resident_quiet()&&fx.sp_lp==-1&&fx.sp_hp==0);
 FT16(comb,&rev_comb[0],8192);CHECK(!fx_resident_quiet());rev_clear();

 fx.sp_hp=1;CHECK(!fx_resident_quiet());fx.sp_hp=0;
 /* Old, unread history may remain nonuniform under the current delay.
  * Increasing the computed delay revokes before that history can sound. */
 room_seed();uint32_t old_dl=delay_samples(),old_at=(fx.dly_w-old_dl-500u)&(DLY_LEN-1u);
 FT16(delay,&dly_buf[old_at],16384);
 unsigned settle=old_dl>1356?old_dl:1356;
 for(unsigned i=0;i<settle;i++)CHECK(!render_sample());
 CHECK(dly_buf[old_at]==16384&&fx_resident_quiet());
 uint32_t age=(fx.dly_w-old_at)&(DLY_LEN-1u);int old_time=song.g[G_DTIME],larger=-1;uint32_t larger_dl=0;
 for(int candidate=GP[G_DTIME].min;candidate<=GP[G_DTIME].max;candidate++){
  song.g[G_DTIME]=(int16_t)candidate;uint32_t current=delay_samples();
  if(current>age&&current!=old_dl){larger=candidate;larger_dl=current;break;}
 }
 song.g[G_DTIME]=(int16_t)old_time;CHECK(larger>=0);
 if(larger>=0){
  song.g[G_DTIME]=(int16_t)larger;CHECK(!fx_resident_quiet()&&fx_uniform_wait.delay_span==old_dl);
  unsigned audible=0;for(unsigned i=0;i<=larger_dl-age+2;i++)if(render_sample())audible=1;
  CHECK(audible&&!fx_resident_quiet());
 }
 /* A DAC-silent delay line with a future audible impulse is never admitted. */
 fresh_fx();song.g[G_DFDBK]=0;song.g[G_DMIX]=127;
 uint32_t at=(fx.dly_w-delay_samples()+100u)&(DLY_LEN-1u);FT16(delay,&dly_buf[at],16384);
 CHECK(!fx_resident_quiet());unsigned sounded=0;
 for(unsigned i=0;i<200;i++){int32_t out=render_sample();if(out)sounded=1;if(i<100)CHECK(!fx_resident_quiet());}
 CHECK(sounded);
 printf("Stationary FX: %u checks, %u failures; exact ROOM/AP2 and SPRING, block1/17/CTL + split64 request, parameter/model/external revocation, current/expanded-window impulse refusal\n",checks,failures);
 return failures!=0;
}
