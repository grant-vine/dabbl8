/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual pattern-only preparation and motion, virtual NOR. No profile grant. */
#define D8ARR_TEST_NO_MAIN 1
#include "native_arrangement_test.c"
static unsigned late_engine;
static int compatibility_read(void *c,uint32_t off,void *p,uint32_t n)
{
 int rc=arr_read(c,off,p,n);
 if(late_engine&&chain.native.policy.preparing){trk[7].eng_req=(uint8_t)late_engine;late_engine=0;}
 return rc;
}
static d8pool compatibility_pool={NULL,compatibility_read,er,pg,stop};
static uint8_t arena[sizeof main_workspace],screen[sizeof host_screen];
static void destinations(void)
{
 for(unsigned t=0;t<NTRK;t++)proof(!set_engine_of(&trk[t],0),"supported destination setup");
 mix_block(audio,CTL);transport_req=0;
}
static void pattern_source(unsigned engine)
{
 seed();destinations();
 for(unsigned o=0;o<3;o++){
  size_t n=0;proof(!d8pool_load(&pool,o,wire,sizeof wire,&n,7),"read source fixture");
  proof(d8p1_project_decode(&source,wire,n,7),"decode source fixture");
  for(unsigned t=0;t<8;t++){source.project.t[t].engine=(uint8_t)engine;source.project.t[t].p[P_E0]=4;}
  motion_store_t *m=&source.project.motion;m->count=8;m->on=128;
  m->event[0]=(motion_event_t){7u<<6,P_PAN,10};
  m->event[1]=(motion_event_t){(7u<<6)|1,P_PAN|MOTION_LOCK,30};
  m->event[2]=(motion_event_t){7u<<6,P_FM1_ATK,41};
  m->event[3]=(motion_event_t){(7u<<6)|1,P_FM1_ATK|MOTION_LOCK,42};
  m->event[4]=(motion_event_t){7u<<6,P_LN0,43};
  m->event[5]=(motion_event_t){(7u<<6)|1,P_LN0|MOTION_LOCK,44};
  m->event[6]=(motion_event_t){7u<<6,P_E0,45};
  m->event[7]=(motion_event_t){(7u<<6)|1,P_E0|MOTION_LOCK,46};
  proof(d8p1_project_encode(wire,sizeof wire,&n,&source),"encode exact provenance fixture");
  proof(!d8pool_save(&pool,o,wire,n,7),"seed source without a capability grant");
 }
 reset();arrange();
}
static void early_refusal(void)
{
 for(unsigned last_only=0;last_only<2;last_only++) {
 seed();destinations();
 if(last_only){proof(!set_engine_of(&trk[7],12),"only eighth destination selects excluded FM6");mix_block(audio,CTL);}
 reset();chain.native_mode=1;chain.native.policy.valid=1;chain.native.policy.suspended=1;
 cv_begin(240,124,T_BG);cv_rect(1,2,17,19,T_ACCENT);cv_blit(0,20);
 snapshot();memcpy(arena,&main_workspace,sizeof arena);memcpy(screen,host_screen,sizeof screen);
 __typeof__(dma) old_dma=dma;unsigned fences=sync_calls,wiregen=proj_wire_gen;uint32_t req=transport_req;
 proof(d8arr_prepare_pool_mask(&compatibility_pool,last_only?1u:1u<<12)==D8POOL_UNSUPPORTED,"missing destination refuses initially");
 proof(unchanged()&&!memcmp(arena,&main_workspace,sizeof arena)&&!memcmp(screen,host_screen,sizeof screen)&&
       !memcmp(&old_dma,&dma,sizeof dma)&&sync_calls==fences&&proj_wire_gen==wiregen&&transport_req==req&&!reads&&!writes,
       "initial refusal preserves all live/cache/arena/DMA state without access");
 proof(d8arr_prepare_pool_mask(&compatibility_pool,0)==D8POOL_INVALID&&unchanged(),"empty profile seam refuses");
 proof(d8arr_prepare_pool_mask(&compatibility_pool,D8ENGINE_EFFECTIVE_MASK|0x4000)==D8POOL_INVALID&&unchanged(),"foreign mask refuses");
}
}
static void pattern_only_to(unsigned engine,unsigned destination)
{
 pattern_source(engine);
 if(destination){
  for(unsigned t=0;t<NTRK;t++)proof(!set_engine_of(&trk[t],destination),"supported exact destination setup");
  mix_block(audio,CTL);transport_req=0;
 }
 snapshot();uint32_t stored=d8p1_crc32(nor,sizeof nor);
 proof(!d8arr_prepare_pool_mask(&compatibility_pool,1u<<destination)&&live_same()&&!writes,"unavailable source engine permits pattern-only preparation");
 for(unsigned o=0;o<3;o++)for(unsigned t=0;t<8;t++)proof(chain.native.policy.source_engine[o][t]==engine,"exact source engine provenance retained");
 transport_req=0;seq_start();proof(d8arr_running(),"real native sequence starts");
 proof(seq_steps(&trk[0])[0].note[0]==47&&trk[0].p[P_SLEN]==3&&
       !memcmp(trk[0].step,before.tracks[0].step,sizeof trk[0].step),"referenced notes/timing play without replacing editable steps");
 trk[0].p[P_PAN]=-11;trk[1].p[P_PAN]=-22;
 int16_t initial[2][3];unsigned ids[3]={P_FM1_ATK,P_LN0,P_E0};
 for(unsigned t=0;t<2;t++)for(unsigned i=0;i<3;i++)initial[t][i]=trk[t].p[ids[i]];
 for(unsigned step=0;step<3;step++)for(unsigned t=0;t<2;t++){
  motion_step(&trk[t],step,d8arr_motion(&trk[t]),1);
  proof(trk[t].p[P_PAN]==(step==1?30:10),"common source automation/lock/unlock remaps independently");
  for(unsigned i=0;i<3;i++) {
   int16_t value=engine!=destination?initial[t][i]:(int16_t)param_fit(param_desc_of(destination,ids[i]),(step==1?42:41)+2*i);
   proof(trk[t].p[ids[i]]==value,"specific automation/lock/unlock uses exact engine identity");
  }
 }
 proof(!memcmp(before.patches,fm6_patch,sizeof fm6_patch)&&!memcmp(&before.undo_state,&undo,sizeof undo)&&proj_cur==before.slot,
       "source instruments/patches/undo/project identity never adopted");
 proof(stored==d8p1_crc32(nor,sizeof nor)&&!writes,"exact stored sources preserved");
 seq_stop();
}
static void pattern_only(unsigned engine){pattern_only_to(engine,0);}
static void late_refusal(void)
{
 pattern_source(9);chain.native_mode=1;chain.native.policy.valid=1;chain.native.policy.suspended=1;snapshot();late_engine=12;
 proof(d8arr_prepare_pool_mask(&compatibility_pool,1)==D8POOL_UNSUPPORTED,"late unavailable destination refuses publication");
 trk[7].eng_req=before.tracks[7].eng_req; /* remove only the explicit external injection */
 proof(live_same()&&!chain.armed&&!chain.native.policy.valid&&!chain.native.policy.preparing&&!writes&&!transport_req,
       "late refusal discards preparation but adds no live mutation or start");
 proof(!d8arr_continue()&&!chain.running,"discarded preparation cannot Continue");
}
int main(void)
{
 early_refusal();pattern_only(9);pattern_only(1);pattern_only(4);pattern_only(0);pattern_only(13);
#if !FELUCCA_FM4
 pattern_only_to(1,12); /* Raw DIGITAL remains ID1; FM6 must skip its specific motion. */
#endif
 late_refusal();
 pattern_source(9);proof(!d8arr_prepare_pool(&compatibility_pool),"immutable production wrapper still prepares");
 printf("arrangement engine compatibility: %u checks, %u failures; pattern-only refs, exact provenance, apply/unlock and initial/late destination refusals; virtual NOR only\n",checks,failures);
 return !!failures;
}
