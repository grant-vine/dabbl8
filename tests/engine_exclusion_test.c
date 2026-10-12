/* SPDX-License-Identifier: GPL-3.0-only */
#define EDITOR_TEST_NO_MAIN 1
#include "editor_test.c"
#include <assert.h>
static unsigned checks;
#define R(x) do { ++checks; if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);} }while(0)
static track_t saved[NPART];static heavy_state_t saved_pool[NVOICE];static uint16_t saved_owner[NVOICE];static uint32_t saved_age;
static unsigned expected(void){return D8ENGINE_EFFECTIVE_MASK;}
static void save(void){memcpy(saved,trk,sizeof trk);memcpy(saved_pool,heavy_pool,sizeof heavy_pool);memcpy(saved_owner,heavy_owner,sizeof heavy_owner);saved_age=vage;}
static void same(void){R(!memcmp(saved,trk,sizeof trk));R(!memcmp(saved_pool,heavy_pool,sizeof heavy_pool));R(!memcmp(saved_owner,heavy_owner,sizeof heavy_owner));R(saved_age==vage);}
static void seed(unsigned count)
{
 reset();memset(heavy_owner,0,sizeof heavy_owner);memset(heavy_pool,0,sizeof heavy_pool);vage=0;
 for(unsigned j=0;j<count;j++) {track_t*t=&trk[1+j%7];t->engine=t->eng_req=0;t->p[P_VOICE]=V_POLY;trk_note_on(t,40+j,100);}
 unsigned n=0;for(unsigned p=0;p<NPART;p++)for(unsigned v=0;v<NVOICE;v++)n+=trk[p].v[v].active!=0;R(n==count);
}
static void refusal(unsigned e,unsigned count,unsigned mode,unsigned pending)
{
 seed(count);track_t*t=&trk[0];t->p[P_VOICE]=mode;t->eng_req=e;t->engine=pending?0:e;
 t->nmono=2;t->mono_stack[0]=45;t->mono_stack[1]=47;t->xp_n=2;t->xp_note[0]=49;t->xp_note[1]=51;t->xp_vel[0]=90;t->xp_vel[1]=100;
 if(pending){t->xf_on=1;t->xf=0;}
 save();trk_note_on(t,60,110);same();engine_block(t);same();
 for(unsigned v=0;v<NVOICE;v++){R(heavy_get(e,0,v)==0);same();}
}
static void released_fallback(unsigned e,unsigned count,unsigned mode)
{
 seed(count);track_t*t=&trk[0];t->engine=t->eng_req=e;t->p[P_VOICE]=mode;t->mono_note=60;t->nmono=2;t->mono_stack[0]=60;t->mono_stack[1]=62;
 save();mono_play(t,62,100,0,0);same();trk_note_off(t,60);
 R(!memcmp(saved+1,trk+1,sizeof trk-sizeof trk[0]));R(saved_age==vage);R(!memcmp(saved_pool,heavy_pool,sizeof heavy_pool));R(!memcmp(saved_owner,heavy_owner,sizeof heavy_owner));
 R(t->engine==e&&t->eng_req==e&&t->nmono==1&&t->mono_stack[0]==62);for(unsigned v=0;v<NVOICE;v++)R(!t->v[v].active&&!t->v[v].gate);
}
static void retained_services(void)
{
 reset();int16_t legacy[8]={2,17,19,23,29,31,37,41};phys_legacy(legacy);R(legacy[0]==0&&legacy[6]==64&&legacy[1]==17&&legacy[7]==41);
 uint32_t gen=smp_user_gen;smp_user_scan(0);R(smp_user_gen==gen+1&&usr_nz[0]==0);
 /* The actual legacy selector 1 converts through FM6 while canonical engine1 stays unavailable. */
 R(!set_engine_of(&trk[0],1));R(trk[0].eng_req==12);R(!apply_preset_to(&trk[0],0));R(trk[0].eng_req==12);
 for(unsigned e=0;e<NENGINES;e++) {
  reset();if(!(expected()&(1u<<e))){save();if(e==1){R(!set_engine_of(&trk[0],e));R(trk[0].eng_req==12);}else{R(set_engine_of(&trk[0],e)==D8ENGINE_UNAVAILABLE);same();}continue;}
  R(!set_engine_of(&trk[0],e));R(!apply_preset_to(&trk[0],0));R(trk[0].eng_req==e);engine_block(&trk[0]);trk_note_on(&trk[0],60,100);
  unsigned n=0;for(unsigned v=0;v<NVOICE;v++)n+=trk[0].v[v].active!=0;R(n>0);
 }
}
static void injected_cleanup(void)
{
 if(D8BUILD_PROFILE==0)return; /* Full control keeps inherited reserved DIGITAL callback behavior. */
 for(unsigned e=0;e<NENGINES;e++)if(!(expected()&(1u<<e))) {
  reset();track_t*t=&trk[0];t->engine=t->eng_req=e;t->v[0].active=t->v[0].gate=1;t->v[0].stage=1;t->v[0].vel=100;t->v[0].env=t->v[0].env_out=20000;
  int32_t out[32];memset(out,0,sizeof out);memset(heavy_owner,0,sizeof heavy_owner);trk_note_off(t,60);trk_all_off(t);R(!t->v[0].gate&&t->xp_n==0&&t->nmono==0);
  (void)track_render(t,out,32);R(!t->v[0].active&&!t->v[0].gate&&!t->v[0].stage);
  for(unsigned i=0;i<32;i++)R(out[i]==0);for(unsigned i=0;i<NVOICE;i++)R(heavy_owner[i]==0);
 }
}
static uint32_t hash_bytes(uint32_t h,const void*v,size_t n){const uint8_t*p=v;while(n--)h=(h^*p++)*16777619u;return h;}
static void metadata(void)
{
 for(unsigned e=0;e<NENGINES;e++) {
  const engine_t*d=ENGINES[e];uint32_t h=2166136261u;for(unsigned pi=0;pi<d->npresets;pi++){const preset_t*p=&d->presets[pi];h=hash_bytes(h,p->name,strlen(p->name)+1);h=hash_bytes(h,p->e,sizeof p->e);h=hash_bytes(h,p->env,sizeof p->env);h=hash_bytes(h,&p->fenv,sizeof p->fenv);h=hash_bytes(h,&p->mono,sizeof p->mono);h=hash_bytes(h,p->fx,sizeof p->fx);h=hash_bytes(h,&p->pat,sizeof p->pat);}
  printf("ID%u %s %s %s presets%u hash%08x cap%u sampled%u keep%u oneshot%u ownenv%u knob%u,%u,%u,%u\n",e,d->name,d->page_title[0],d->page_title[1],d->npresets,h,d->poly,d->sampled,d->keep,d->oneshot,d->ownenv,d->knob[0],d->knob[1],d->knob[2],d->knob[3]);
  for(unsigned k=0;k<8;k++){const param_desc_t*a=&d->edit[k];printf("  %s %u %d %d %d %s\n",a->label,a->fmt,a->min,a->max,a->def,a->unit?a->unit:"");if(a->fmt==F_ENUM&&a->names)for(int v=a->min;v<=a->max;v++)printf("    %d %s\n",v,a->names[v]?a->names[v]:"NULL");}
 }
}
int main(int argc,char**argv)

{
 if(argc>1){metadata();return 0;}
 R(NPART==8&&NVOICE==8&&NENGINES==14);R(D8ENGINE_EFFECTIVE_MASK==expected());
#if D8BUILD_PROFILE != 0
 for(unsigned e=0;e<NENGINES;e++)if(!(expected()&(1u<<e)))for(unsigned count=7;count<=8;count++)for(unsigned mode=0;mode<4;mode++)for(unsigned pending=0;pending<2;pending++)refusal(e,count,mode,pending);
 for(unsigned e=0;e<NENGINES;e++)if(!(expected()&(1u<<e)))for(unsigned count=7;count<=8;count++)for(unsigned mode=1;mode<4;mode++)released_fallback(e,count,mode);
 for(unsigned e=14;e<256;e+=17)refusal(e,8,V_MONO,1);
#endif
 retained_services();injected_cleanup();printf("profile%u checks%u PASS; host heavy sizeof%zu align%zu (not target memory evidence)\n",D8BUILD_PROFILE,checks,sizeof(heavy_state_t),_Alignof(heavy_state_t));return 0;
}
