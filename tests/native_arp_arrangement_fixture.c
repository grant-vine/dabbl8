/* SPDX-License-Identifier: GPL-3.0-only */
/* Minimal actual native frontend/pool/clock seed for held ARP cases. */
#define D8POOL_RUNTIME_NO_MAIN 1
#include "d8p1_pool_runtime_test.c"
#include "../firmware/src/d8arr_prepare.c"
static d8p1_project_state source;
static int32_t audio[2*CTL];
static uint8_t overlay_wire[D8P1_LIMIT];
static d8pool arr_pool={NULL,rd,er,pg,stop};
static int cat(void*c,d8p1_project_catalog*out){(void)c;return d8p1_catalog_pool(&arr_pool,out);}
static int save_cb(void*c,unsigned o,const char*n){(void)c;return d8p1_save_as_pool(&arr_pool,o,n);}
static int load_cb(void*c,unsigned o){(void)c;return d8p1_load_pool(&arr_pool,o);}
static int rename_cb(void*c,unsigned o,const char*n){(void)c;return d8p1_rename_pool(&arr_pool,o,n);}
static int prep_cb(void*c){(void)c;return d8arr_prepare_pool(&arr_pool);}
static project_native_ops ops={NULL,cat,save_cb,load_cb,rename_cb,prep_cb};
static void arrange(void)
{
 d8p1_arrangement *a=&d8p1_runtime_cache.arrangement;memset(a,0,sizeof *a);a->banks=3;a->scenes=3;a->rows=3;
 for(unsigned b=0;b<3;b++){
  memcpy(a->scene[b].name,"SCENE",5);a->scene[b].name[5]=(char)('1'+b);a->scene[b].bank=(uint8_t)b;
  for(unsigned t=0;t<8;t++){a->bank[b].project[t]=(uint8_t)b;a->bank[b].track[t]=(uint8_t)(7-t);}
  a->row[b].scene=(uint8_t)b;a->row[b].repeat=(uint8_t)(b==0?2:1);
 }
 /* Aliased source tracks have separate destination bases and locks. */
 a->bank[0].track[1]=a->bank[0].track[0];d8p1_runtime_cache.valid=1;
}
static void seed(void)
{
 ui_power_on();mix_block(audio,CTL);blank();size_t n=fixture("tests/fixtures/d8p1/minimal.d8p");
 proof(!d8p1_load_runtime(wire,n,0),"initial native live load");mix_block(audio,CTL);
 proof(d8p1_project_decode(&source,wire,n,0),"actual native source decode");
 for(unsigned o=0;o<3;o++){
  source.project.motion.count=3;source.project.motion.on=128;
  source.project.motion.event[0]=(motion_event_t){7u<<6,P_PAN,10};
  source.project.motion.event[1]=(motion_event_t){(7u<<6)|1,P_PAN|MOTION_LOCK,30};
  source.project.motion.event[2]=(motion_event_t){7u<<6,P_FM1_ATK,41};
  for(unsigned t=0;t<8;t++){
   proj_trk_t *q=&source.project.t[t];q->p[P_SLEN]=3;q->p[P_SDIV]=2;
   for(unsigned k=0;k<NSTEP;k++){q->step[k].time=ST_NOTE;q->step[k].n=1;q->step[k].note[0]=(uint8_t)(40+o*8+t);q->step[k].vel=99;}
  }
  proof(d8p1_project_encode(wire,sizeof wire,&n,&source),"encode distinct eight-track source");
  proof(!d8pool_save(&pool,o,wire,n,7),"seed actual native pool source");
 }
 reset();arrange();proof(!project_native_bind(&ops,1),"explicit test-only trusted native binding");
 writes=0;
}
static void prepare_start(void)
{proof(!chain_prepare()&&chain.armed&&chain.native.policy.valid&&!writes,"actual frontend read-only preparation arms");transport_req=0;seq_start();proof(d8arr_running()&&song.playing&&chain.row==0&&chain.remaining==d8p1_runtime_cache.arrangement.row[0].repeat,"actual sequencer starts native row zero");}
static void boundary(uint32_t carry)
{uint32_t p=div_samples(2);clk_step=15;clk_pos=p-4;events_block(4+carry);}
