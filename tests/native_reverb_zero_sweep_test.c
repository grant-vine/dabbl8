/* SPDX-License-Identifier: GPL-3.0-only */
/* Real render stores, independent full-union oracle; no physical timing proof. */
#define D8FLASH_RUNTIME_NO_MAIN 1
#include "d8p1_flash_runtime_test.c"
static unsigned sweep_blocks;
static uint32_t sweep_rng=0x9abc1234;
static uint32_t random_sweep(void){sweep_rng^=sweep_rng<<13;sweep_rng^=sweep_rng>>17;sweep_rng^=sweep_rng<<5;return sweep_rng;}
static void proof(void){
 unsigned comb=0,ap=0;
 for(unsigned i=0;i<sizeof rev_comb/sizeof rev_comb[0];i++)comb+=rev_comb[i]!=0;
 /* Scan the entire union, including unused entries and its odd final int16. */
 for(unsigned i=0;i<sizeof rev_u.ap/sizeof rev_u.ap[0];i++)ap+=rev_u.ap[i]!=0;
 if(!fx_tail.comb&&!fx_rev_wait.comb)CHECK(!comb);
 if(!fx_tail.ap&&!fx_rev_wait.ap)CHECK(!ap);
 if(fx_resident_quiet())CHECK(!comb&&!ap);
 CHECK(fx_rev_wait.comb<=SP_LEN&&fx_rev_wait.ap<=556u);
}
static void render(unsigned model,const int32_t *input,unsigned n){
 int32_t output[CTL]={0};
 if(model)rev_spring(input,output,n);else rev_room(input,output,n);
 proof();sweep_blocks++;
}
static void cold_sweep(unsigned model){
 memset(&fx,0,sizeof fx);rev_clear();fx.rtype=model;
 fx.comb_i[0]=1115;fx.comb_i[1]=1187;fx.comb_i[2]=1276;fx.comb_i[3]=1355;
 fx.ap_i[0]=555;fx.ap_i[1]=440;fx.sp_w=65534;
}
int main(void){
 ui_power_on();int32_t input[CTL]={0};
 for(unsigned model=0;model<2;model++){
  cold_sweep(model);proof();render(model,input,0);CHECK(!fx_rev_wait.comb&&!fx_rev_wait.ap);
  for(unsigned block=0;block<2500;block++){
   unsigned n=1+random_sweep()%CTL;
   for(unsigned i=0;i<n;i++)input[i]=(block<400&&(random_sweep()&3))?(int32_t)(random_sweep()%32769)-16384:0;
   if(block%137==0){song.g[G_RSIZE]=random_sweep()%128;song.g[G_RDAMP]=random_sweep()%128;}
   render(model,input,n);
  }
  /* Synthetic all-zero baseline with retained wait demonstrates bounded
   * conservative deferral. Production never clears state to gain eligibility. */
  cold_sweep(model);input[0]=32767;render(model,input,1);CHECK(fx_rev_wait.comb>0);
  uint32_t wait=fx_rev_wait.comb;
  memset(rev_comb,0,sizeof rev_comb);memset(&rev_u,0,sizeof rev_u);
  memset(fx.comb_lp,0,sizeof fx.comb_lp);fx.sp_lp=fx.sp_hp=fx.sp_he=0;
  memset(input,0,sizeof input);render(model,input,0);CHECK(fx_rev_wait.comb==wait);
  for(unsigned i=0;i<wait;i++){render(model,input,1);if(i+1<wait)CHECK(fx_rev_wait.comb>0);}
  CHECK(!fx_rev_wait.comb&&!fx_rev_wait.ap);proof();
 }
 /* Actual render nonzeros followed by external zero/nonzero updates once
  * wrapped counted guards back to zero. Reverb guards now latch until clear. */
 cold_sweep(1);fx.sp_w=0;int32_t pulse[1]={16384},pulse_out[1]={0};rev_spring(pulse,pulse_out,1);
 CHECK(rev_comb[0]!=0&&rev_u.sp[0]!=0&&!fx_tail.comb&&!fx_tail.ap);
 FT16(comb,&rev_comb[0],0);FT16(comb,&rev_comb[sizeof rev_comb/2-1],1);
 CHECK(fx_tail.comb&&!fx_resident_quiet());
 FT32(ap,&rev_u.sp[0],0);FT32(ap,&rev_u.sp[sizeof rev_u.sp/4-1],1);
 CHECK(fx_tail.ap&&!fx_resident_quiet());
 for(unsigned i=0;i<64;i++){
  FT16(comb,&rev_comb[sizeof rev_comb/2-1],(int16_t)(i&1));
  FT32(ap,&rev_u.sp[sizeof rev_u.sp/4-1],(int32_t)(i&1));
  CHECK(fx_tail.comb&&fx_tail.ap&&!fx_resident_quiet());
 }
 rev_clear();proof();CHECK(!fx_tail.comb&&!fx_tail.ap);
 /* Every entry, including inactive union/line storage, stays refused under
  * an external tracked disturbance until its explicit clear/reset. */
 for(unsigned model=0;model<2;model++){
  cold_sweep(model);FT16(comb,&rev_comb[sizeof rev_comb/2-1],1);
  for(unsigned b=0;b<140;b++)render(model,input,CTL);
  CHECK(fx_tail.comb&&!fx_resident_quiet());rev_clear();proof();
  FT16(ap,&rev_u.ap[sizeof rev_u.ap/2-1],-1);
  for(unsigned b=0;b<140;b++)render(model,input,CTL);
  CHECK(fx_tail.ap&&!fx_resident_quiet());rev_clear();proof();
 }
 cold_sweep(0);render(0,input,1);render(1,input,1);
 CHECK(fx_tail.comb&&fx_tail.ap&&!fx_resident_quiet());rev_clear();proof();
 for(unsigned b=0;b<600;b++){
  song.g[G_RTYPE]=(b/7)&1u;unsigned n=1+random_sweep()%CTL;int32_t zero[CTL]={0},wet[CTL];
  fx_buses(zero,zero,input,wet,n);proof();
 }
 printf("Native reverb zero sweep: %u checks, %u failures; %u actual render blocks, entire buffer/union silence oracle; conservative latency, simulated only\n",checks,failures,sweep_blocks);
 return failures!=0;
}
