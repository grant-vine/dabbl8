/* SPDX-License-Identifier: GPL-3.0-only */
/* Compile against the immutable parent and candidate, then compare exact traces. */
#define NPART 8
#define main dry_hostsim_main
#ifndef TREE_HOSTSIM
#define TREE_HOSTSIM "hostsim.c"
#endif
#include TREE_HOSTSIM
#undef main
#include <assert.h>
static uint64_t trace=1469598103934665603ULL;static unsigned checks,cases;
static void digest(const void*p,size_t n){const uint8_t*b=p;while(n--){trace^=*b++;trace*=1099511628211ULL;}}
#define TEST(x) do{assert(x);checks++;}while(0)
static void fresh(void){
 memset(trk,0,sizeof trk);memset(&song,0,sizeof song);host_tracks_init();
 memset(&fx,0,sizeof fx);rev_clear();memset(dly_buf,0,sizeof dly_buf);memset(cho_buf,0,sizeof cho_buf);
 memset(&fx_tail,0,sizeof fx_tail);memset(&fx_uniform_wait,0,sizeof fx_uniform_wait);
 memset(&pf,0,sizeof pf);pf.src=pf.next=PF_N;pf.lc=PF_TOP;for(unsigned t=0;t<8;t++)pf.mg[t]=32768;
 memset(heavy_owner,0,sizeof heavy_owner);memset(heavy_pool,0,sizeof heavy_pool);
 dc_l=dc_r=dce_l=dce_r=lc_l1=lc_l2=lc_r1=lc_r2=0;memset(lce,0,sizeof lce);
 sb_lp1=sb_lp2=sb_lp3=sb_lp4=sb_env=sb_h1=sb_h2=sb_hl=0;
 memset(midi_bend_q8,0,sizeof midi_bend_q8);memset(midi_bend_target,0,sizeof midi_bend_target);
 memset(&d8arr_mix,0,sizeof d8arr_mix);mod_seed=0x2545f491u;fm1_in.notes=fm1_in.buttons=0;
}
static void observe(const int32_t*out){
 digest(out,CTL*2*sizeof *out);digest(send_c,sizeof send_c);digest(send_d,sizeof send_d);digest(send_r,sizeof send_r);
 digest(&fx,sizeof fx);digest(&fx_tail,sizeof fx_tail);digest(&fx_uniform_wait,sizeof fx_uniform_wait);
 digest(&dc_l,sizeof dc_l);digest(&dc_r,sizeof dc_r);digest(&dce_l,sizeof dce_l);digest(&dce_r,sizeof dce_r);
 TEST(voices_busy()<=8);
}
#ifndef D8_DRY_SENDS_NO_MAIN
int main(void){int32_t out[CTL*2];
 for(unsigned engine=0;engine<14;engine++)if(engine!=1)for(unsigned bank=0;bank<8;bank++)for(unsigned mask=0;mask<8;mask++){
  fresh();track_t*t=&trk[bank];host_preset(t,engine,0);t->p[P_PAN]=(int16_t)((int)bank*16-56);t->p[P_DIST]=(int16_t)(mask&1?35:0);
  t->p[P_CHOR]=(mask&1)?91:0;t->p[P_DLY]=(mask&2)?93:0;t->p[P_REV]=(mask&4)?89:0;
  int16_t saved[3]={t->p[P_CHOR],t->p[P_DLY],t->p[P_REV]};
  /* Zero stored sends become wet through modulation; wet become dry. */
  if(mask==0||mask==7){t->p[P_M1SRC]=MS_EXPR;t->p[P_M1DST]=MD_REV;t->p[P_M1AMT]=mask? -63:63;t->ex_off=0;}
  trk_note_on(t,engine==10?36:60,100);trk_note_on(t,engine==10?42:67,92);
  for(unsigned b=0;b<24;b++){
   if(b==4)t->p[P_SLCR]=2;if(b==8)t->p[P_SLCR]=0;
   if(b==10){pf.mute=1u<<bank;pf.mg[bank]=0;}if(b==12){pf.mute=0;pf.mg[bank]=32768;}
   if(b==13){song.g[G_RTYPE]=1;song.g[G_BPM]=137;}
   if(b==15){t->p[P_M1AMT]=0;t->p[P_CHOR]=t->p[P_DLY]=t->p[P_REV]=0;saved[0]=saved[1]=saved[2]=0;}
   if(b==16){trk_note_off(t,engine==10?36:60);trk_note_off(t,engine==10?42:67);}
   mix_block(out,CTL);observe(out);TEST(t->p[P_CHOR]==saved[0]&&t->p[P_DLY]==saved[1]&&t->p[P_REV]==saved[2]);
  }
  digest(trk,sizeof trk);digest(dly_buf,sizeof dly_buf);digest(cho_buf,sizeof cho_buf);digest(rev_comb,sizeof rev_comb);digest(&rev_u,sizeof rev_u);cases++;
 }
 /* Eight simultaneously sounding tracks, wet/dry transitions and real tails. */
 fresh();for(unsigned t=0;t<8;t++){host_preset(&trk[t],t%3==0?0:t%3==1?3:10,0);trk[t].p[P_CHOR]=40;trk[t].p[P_DLY]=50;trk[t].p[P_REV]=60;trk_note_on(&trk[t],t%3==2?36:48+t,90);}
 for(unsigned b=0;b<96;b++){if(b==12)for(unsigned t=0;t<8;t++)trk[t].p[P_CHOR]=trk[t].p[P_DLY]=trk[t].p[P_REV]=0;mix_block(out,CTL);observe(out);}
 printf("dry sends: cases=%u checks=%u trace=%016llx\n",cases,checks,(unsigned long long)trace);return 0;
}

#endif /* D8_DRY_SENDS_NO_MAIN */
