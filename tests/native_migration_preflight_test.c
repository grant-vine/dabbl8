/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual UI/LCD arena and actual native codec/pool paths; no physical device. */
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#define NPART 8
#define UI_TEST_NO_MAIN 1
#define UI_ASYNC_LCD 1
static void acquire_hook(void);
#define UI_LCD_SYNC_HOOK acquire_hook
#include "ui_test.c"
#include "../firmware/src/d8p1_runtime.c"
#include "../firmware/src/d8p1_pool_runtime.c"
#include "../firmware/src/native_migration_preflight.c"
#include "../firmware/src/instrument_write_gate.h"
static unsigned checks,failures,acquire_inject;
static uint32_t nested_generation;static int nested_result,trap_expected;
static uint8_t trap_canary[59520];
static void acquire_hook(void){unsigned n=acquire_inject;acquire_inject=0;if(n==1){nested_result=d8mp_begin(&nested_generation);if(trap_expected)memcpy(trap_canary,&main_workspace,sizeof main_workspace);}else if(n==2)cv_cpu_active=1;}
#define PROOF(x) do {checks++;if(!(x)){failures++;fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#x);}}while(0)
static uint8_t plan[D8POOL_BYTES], wire[D8P1_LIMIT], before_plan[D8POOL_BYTES];
static d8p1_project_state source;
static project_t live_before,live_after;
static uint8_t tail_canary[59520-58432];
static unsigned seed_mutations,seed_reads,seed_stops;
static int read_seed(void *c,uint32_t off,void *p,uint32_t n){(void)c;seed_reads++;if(off>D8POOL_BYTES||n>D8POOL_BYTES-off)return -1;memcpy(p,plan+off,n);return 0;}
static int erase_seed(void *c,uint32_t off){(void)c;if(off%D8POOL_SECTOR||off>D8POOL_BYTES-D8POOL_SECTOR)return -1;seed_mutations++;memset(plan+off,255,D8POOL_SECTOR);return 0;}
static int program_seed(void *c,uint32_t off,const void *p,uint32_t n){(void)c;if(!n||n>256||(off&255)+n>256||off>D8POOL_BYTES||n>D8POOL_BYTES-off)return -1;seed_mutations++;for(unsigned i=0;i<n;i++)plan[off+i]&=((const uint8_t*)p)[i];return 0;}
static int stopped_seed(void *c){(void)c;seed_stops++;return 1;}
static d8pool seed={NULL,read_seed,erase_seed,program_seed,stopped_seed};
static void make_plan(unsigned wanted)
{
 memset(plan,255,sizeof plan);seed_mutations=0;memset(&source,0,sizeof source);project_capture(&source.project);
 strcpy(source.project.name,"PLAN");source.project.sel=7;for(unsigned t=0;t<8;t++)source.project.t[t].p[P_LEVEL]=17+t;
 size_t n=0;PROOF(d8p1_project_encode(wire,sizeof wire,&n,&source));
 unsigned manuals=wanted<3?wanted:3;
 for(unsigned o=0;o<manuals;o++)PROOF(!d8pool_save(&seed,o,wire,n,0));
 PROOF(!d8pool_save(&seed,3,wire,n,0));
 if(wanted==4){PROOF(!d8pool_save(&seed,3,wire,n,0));PROOF(!erase_seed(NULL,3*D8POOL_BLOCK)&&!erase_seed(NULL,3*D8POOL_BLOCK+D8POOL_SECTOR));}
 d8pool_index idx;PROOF(!d8pool_inventory(&seed,&idx)&&idx.object[3].block==wanted);
}
/* Bad proposals have valid outer CRCs: refusal must reach semantic/canonical
 * validation, not accidentally pass merely because corruption is unframed. */
static void bad_or_referenced_auto(unsigned kind)
{
 d8pool_index idx;PROOF(!d8pool_inventory(&seed,&idx));size_t n=0;
 if(kind<2){
  source.arrangement.banks=1;for(unsigned t=0;t<8;t++){source.arrangement.bank[0].project[t]=(kind==1&&t==7)?3:t%3;source.arrangement.bank[0].track[t]=t;}
  source.arrangement.scenes=1;source.arrangement.scene[0].apply=7;source.arrangement.rows=1;source.arrangement.row[0].repeat=2;
  PROOF(d8p1_project_encode(wire,sizeof wire,&n,&source));
 }else if(kind==2){FILE *f=fopen("tests/fixtures/d8p1/unknown-optional.d8p","rb");PROOF(f!=NULL);if(!f)return;n=fread(wire,1,sizeof wire,f);PROOF(fgetc(f)==EOF&&!ferror(f));fclose(f);d8p1_view v;PROOF(d8p1_read(&v,wire,n)==2);}
 else {
  PROOF(d8p1_project_encode(wire,sizeof wire,&n,&source));uint8_t zero=0;d8ps_w16(wire+n,7);d8ps_w16(wire+n+2,1);d8ps_w32(wire+n+4,d8p1_crc32(&zero,1));wire[n+8]=0;n+=9;wire[20]++;d8ps_w32(wire+8,n);d8ps_w32(wire+12,0);d8ps_w32(wire+12,d8p1_crc32(wire,n));d8p1_view v;PROOF(d8p1_read(&v,wire,n)==1);
 }
 uint8_t *b=plan+idx.object[3].block*D8POOL_BLOCK;memset(b+D8POOL_HEADER,255,D8POOL_BLOCK-D8POOL_HEADER);memcpy(b+D8POOL_HEADER,wire,n);d8ps_w32(b+16,n);d8ps_w32(b+20,d8p1_crc32(wire,n));d8ps_w32(b+28,d8p1_crc32(b,28));
}
static uint32_t receive_plan(void)
{
 uint32_t g=0;PROOF(d8mp_begin(&g)==D8MP_OK&&g);
 for(unsigned off=0;off<D8POOL_BYTES;off+=256)PROOF(d8mp_receive(g,off,plan+off,256)==D8MP_OK);
 return g;
}
static void expected_trap(unsigned action)
{
 pid_t p=fork();PROOF(p>=0);if(p<0)return;
 if(!p){signal(SIGILL,SIG_DFL);signal(SIGTRAP,SIG_DFL);if(action==0)main_project_workspace();else if(action==1)main_d8p1_workspace();else cv_begin(1,1,T_BG);_exit(99);}
 int status=0;waitpid(p,&status,0);PROOF(WIFSIGNALED(status)&&(WTERMSIG(status)==SIGILL||WTERMSIG(status)==SIGTRAP));
}
static void reciprocal_signal(int n)
{
 (void)n;const uint8_t *p=(const uint8_t*)&main_workspace;unsigned bad=!migration_owner;
 for(unsigned i=0;i<sizeof main_workspace;i++)if(p[i]!=trap_canary[i])bad=1;
 _exit(bad?78:77);
}
static void reciprocal_trap(unsigned action)
{
 pid_t p=fork();PROOF(p>=0);if(p<0)return;
 if(!p){trap_expected=1;acquire_inject=1;nested_generation=0;
  signal(SIGILL,reciprocal_signal);signal(SIGTRAP,reciprocal_signal);
  if(action==0)memset(main_project_workspace(),0,sizeof(project_t));else cv_begin(10,10,T_SURF);
  _exit(99);
 }
 int status=0;waitpid(p,&status,0);PROOF(WIFEXITED(status)&&WEXITSTATUS(status)==77);
}
/* Actual API callers must refuse without callbacks, publication, or damage
 * to the owner's complete immutable plan. */
static void held_callers(uint32_t g)
{
 d8mp_workspace *w=main_migration_workspace(g);PROOF(w!=NULL);
 uint32_t hash=d8p1_crc32(w,sizeof *w),signature=0x12345678;
 unsigned reads=seed_reads,stops=seed_stops,mutations=seed_mutations;
 size_t n=123;uint8_t out[32],before[32];memset(out,0xa5,sizeof out);memcpy(before,out,sizeof out);
 d8p1_project_catalog catalog,old;memset(&catalog,0xa5,sizeof catalog);old=catalog;
 d8pool_record record,prior;memset(&record,0xa5,sizeof record);prior=record;
 PROOF(!main_project_workspace_try()&&!main_d8p1_workspace_try());
 PROOF(d8p1_load_runtime(wire,0,7)==D8RT_BUSY);
 PROOF(d8p1_capture_runtime(out,sizeof out,&n)==D8RT_BUSY&&n==123&&!memcmp(out,before,sizeof out));
 PROOF(d8p1_signature_runtime(&signature)==D8RT_BUSY&&signature==0x12345678);
 PROOF(d8p1_catalog_pool(&seed,&catalog)==D8POOL_BUSY&&!memcmp(&catalog,&old,sizeof old));
 PROOF(d8p1_save_pool(&seed,0)==D8POOL_BUSY);
 PROOF(d8p1_save_as_pool(&seed,1,"BUSY")==D8POOL_BUSY);
 PROOF(d8p1_load_pool(&seed,0)==D8POOL_BUSY);
 PROOF(d8p1_restore_pool_autosave(&seed,1)==D8POOL_BUSY);
 PROOF(d8p1_rename_pool(&seed,0,"BUSY")==D8POOL_BUSY);
 PROOF(d8p1_autosave_snapshot_pool(&seed,&signature,&record)==D8POOL_BUSY&&signature==0x12345678&&!memcmp(&record,&prior,sizeof prior));
 PROOF(!pn_quiet());
 PROOF(seed_reads==reads&&seed_stops==stops&&seed_mutations==mutations&&d8p1_crc32(w,sizeof *w)==hash);
}
static void late_callers(void)
{
 for(unsigned action=0;action<3;action++) {
  size_t n=0;PROOF(d8p1_project_encode(wire,sizeof wire,&n,&source));
  d8p1_project_catalog catalog,old;memset(&catalog,0xa5,sizeof catalog);old=catalog;
  uint32_t signature=0x12345678;acquire_inject=1;nested_generation=0;nested_result=99;
  int rc=action==0?d8p1_load_runtime(wire,n,7):action==1?d8p1_signature_runtime(&signature):d8p1_catalog_pool(&seed,&catalog);
  PROOF(rc==(action==2?D8POOL_BUSY:D8RT_BUSY)&&nested_result==D8MP_OK&&nested_generation);
  PROOF(signature==0x12345678&&!memcmp(&catalog,&old,sizeof old));
  held_callers(nested_generation);PROOF(!d8mp_end(nested_generation));
 }
}
int main(void)
{
 ui_power_on();project_capture(&live_before);PROOF(sizeof main_workspace==59520&&sizeof(d8mp_workspace)==58432&&offsetof(d8mp_workspace,stage)==0&&offsetof(d8mp_workspace,plan)==17472);
 for(unsigned wanted=0;wanted<5;wanted++){
  make_plan(wanted);memcpy(before_plan,plan,sizeof plan);unsigned mutations=seed_mutations;
  cv_begin(240,124,T_SURF);cv_rect(3,4,50,40,T_ACCENT);uint32_t pixels=pixels_hash(cv_px,CV_MAX);cv_blit(0,20);unsigned consumed=dma_consumed;
  memcpy(tail_canary,(uint8_t*)&main_workspace+sizeof(d8mp_workspace),sizeof tail_canary);
  uint32_t g=receive_plan();PROOF(dma_consumed==consumed+1&&!dma.p&&!dma_errors&&pixels_hash(host_screen+20*240,CV_MAX)==pixels);
  d8mp_workspace *w=main_migration_workspace(g);PROOF(w&&(uintptr_t)w%_Alignof(d8mp_workspace)==0);
  uint32_t frame=ui.frame,hash=d8p1_crc32(w->plan,sizeof w->plan);ui_draw();PROOF(ui.frame==frame&&ui.force&&d8p1_crc32(w->plan,sizeof w->plan)==hash);
  uint32_t other=0xdeadbeef;PROOF(d8mp_begin(&other)==D8MP_BUSY&&other==0xdeadbeef);PROOF(d8mp_end(g+1)==D8MP_STALE&&main_migration_workspace(g)==w);
  held_callers(g);
  if(wanted==0){expected_trap(0);expected_trap(1);expected_trap(2);PROOF(!memcmp(w->plan,before_plan,sizeof plan));}
  d8mp_result result;memset(&result,0xa5,sizeof result);PROOF(d8mp_validate(g,&result)==D8MP_OK&&result.generation==g&&result.crc==hash&&result.index.object[3].block==wanted);for(unsigned t=0;t<8;t++)PROOF(w->stage.state.project.t[t].p[P_LEVEL]==17+(int)t);
  PROOF(d8mp_receive(g,0,plan,256)==D8MP_BUSY&&d8mp_validate(g,&result)==D8MP_BUSY);
  PROOF(!memcmp(w->plan,before_plan,sizeof plan)&&seed_mutations==mutations&&!memcmp(tail_canary,(uint8_t*)&main_workspace+sizeof(d8mp_workspace),sizeof tail_canary));PROOF(d8mp_end(g)==D8MP_OK&&!main_migration_workspace(g));
  PROOF(d8mp_end(g)==D8MP_STALE&&d8mp_receive(g,0,plan,256)==D8MP_STALE);cv_begin(2,2,T_BG);cv_blit(0,0);
 }
 /* Checked acquire/receive failures preserve the live owner and output. */
 make_plan(3);cv_begin(2,2,T_BG);uint32_t g=123;PROOF(d8mp_begin(&g)==D8MP_BUSY&&g==123);cv_blit(0,0);
 PROOF(d8mp_begin(NULL)==D8MP_BAD&&d8mp_begin((uint32_t*)&main_workspace)==D8MP_BAD);
 PROOF(!d8mp_begin(&g));d8mp_workspace *w=main_migration_workspace(g);uint32_t first=w->plan[0];
 PROOF(d8mp_receive(g,1,plan,256)==D8MP_BAD&&d8mp_receive(g,0,plan,257)==D8MP_BAD&&d8mp_receive(g,0,NULL,1)==D8MP_BAD);
 PROOF(d8mp_receive(g,0,w->plan,256)==D8MP_BAD&&d8mp_receive(g,0,&d8mp,sizeof d8mp)==D8MP_BAD&&w->plan[0]==first);
 PROOF(d8mp_receive(g,UINT32_MAX,plan,1)==D8MP_BAD&&d8mp_receive(g+1,0,plan,256)==D8MP_STALE);
 PROOF(!d8mp_receive(g,0,plan,1)&&d8mp_receive(g,1,plan+1,256)==D8MP_BAD&&!d8mp_receive(g,1,plan+1,255));
 d8mp_result result,unchanged;memset(&result,0xa5,sizeof result);unchanged=result;
 PROOF(d8mp_validate(g,&result)==D8MP_BUSY&&!memcmp(&result,&unchanged,sizeof result));PROOF(!d8mp_end(g));
 /* Full-set padding, spare, last object, optional content and reference refusal
  * are checked after all bytes arrive and preserve published result. */
 for(unsigned fault=0;fault<8;fault++){
  make_plan(3);
  if(fault==0)plan[32]=0;
  if(fault==1)plan[4*D8POOL_BLOCK+4095]=0;
  if(fault==2)plan[3*D8POOL_BLOCK+D8POOL_HEADER+6000]^=1;
  if(fault==3){memset(plan+3*D8POOL_BLOCK,255,D8POOL_BLOCK);}
  if(fault==4){d8pool_index idx;PROOF(!d8pool_inventory(&seed,&idx));plan[idx.object[3].block*D8POOL_BLOCK+D8POOL_HEADER+idx.object[3].length]=0;}
  if(fault>=5)bad_or_referenced_auto(fault-4);
  g=receive_plan();memset(&result,0xa5,sizeof result);unchanged=result;
  PROOF(d8mp_validate(g,&result)==D8MP_INVALID&&!memcmp(&result,&unchanged,sizeof result));PROOF(!d8mp_end(g));
 }
 make_plan(3);bad_or_referenced_auto(0);g=receive_plan();w=main_migration_workspace(g);
 PROOF(d8mp_validate(g,(d8mp_result*)w->plan)==D8MP_BAD&&d8mp_validate(g,(d8mp_result*)&d8mp)==D8MP_BAD);PROOF(!d8mp_validate(g,&result)&&w->stage.state.arrangement.banks==1&&w->stage.state.arrangement.bank[0].project[7]==1&&w->stage.state.arrangement.bank[0].track[7]==7&&w->stage.state.arrangement.rows==1);PROOF(!d8mp_end(g));
 /* Synchronous LCD completion can be instrumented with nested acquisition or
  * changed CPU ownership: outer acquisition must not steal/mint a token. */
 g=0xdeadbeef;acquire_inject=1;nested_generation=0;nested_result=99;
 PROOF(d8mp_begin(&g)==D8MP_BUSY&&g==0xdeadbeef&&nested_result==D8MP_OK&&main_migration_workspace(nested_generation));
 PROOF(!d8mp_end(nested_generation));g=42;acquire_inject=2;
 PROOF(d8mp_begin(&g)==D8MP_BUSY&&g==42&&!migration_owner);cv_cpu_active=0;
 reciprocal_trap(0);reciprocal_trap(1);late_callers();
 /* Generation exhaustion refuses acquisition; no wrap/stale-token reuse. */
 uint32_t saved=migration_generation;migration_generation=UINT32_MAX;g=42;PROOF(d8mp_begin(&g)==D8MP_BUSY&&g==42);migration_generation=saved;
 project_capture(&live_after);PROOF(!memcmp(&live_before,&live_after,sizeof live_before)&&!d8p1_runtime_cache.valid&&!project_native_status()&&!d8_instrument_write_allowed());
 printf("Native migration preflight: %u checks, %u failures; complete immutable plan, actual C codec/pool and UI/DMA arena; no writes/authority/adoption/device qualification\n",checks,failures);return failures?1:0;
}
