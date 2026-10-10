/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual main-loop routing across optimized capture, FX residents and queues. */
#define D8_NATIVE_AUTOSAVE_ROUTE_TEST 1
#define D8_AUTOSAVE_SESSION_NO_MAIN 1
#include "native_autosave_session_test.c"
static unsigned route_cases;
static void route_tick(uint32_t delta){fm1_ms+=delta;autosave_poll();}
static void routed_fixture(void)
{
 idle_fixture();stream(1);drain();
 for(unsigned h=0;h<2;h++){qa_half=h;fm1_alnk0_irq();}
 drain();CHECK(df_automatic_quiet());reset();
}
int main(void)
{
 /* Storage binding routes away from legacy saving but grants no session. */
 routed_fixture();trk[7].p[P_LEVEL]=100;
 route_tick(AS_IDLE_MS+AS_GAP_MS);CHECK(!reads&&!writes&&!native_as.ready);identity();
 begin();reset();trk[7].p[P_LEVEL]=101;route_tick(AS_POLL_MS);
 route_tick(AS_IDLE_MS-AS_POLL_MS);CHECK(!writes);autosave_hold();
 route_tick(AS_POLL_MS);CHECK(!writes);route_tick(AS_IDLE_MS-AS_POLL_MS);
 CHECK(writes&&native_as.writes==1&&!native_as.pending);identity();
 d8p1_autosave_session_end();reset();trk[7].p[P_LEVEL]=102;
 route_tick(AS_IDLE_MS+AS_GAP_MS);CHECK(!reads&&!writes&&project_native_owns_storage());identity();
 /* All seven output states defer actual main-loop save; ordinary queues drain
  * through the real producer/service and both audio IRQ halves. Sticky
  * uncertainty remains refused even after buffer contents drain. */
 for(queue_kind=0;queue_kind<7;queue_kind++){
  routed_fixture();begin();reset();trk[7].p[P_LEVEL]=100;
  d8p1_arrangement *a=&d8p1_runtime_cache.arrangement;memset(a,0,sizeof *a);
  a->banks=1;memcpy(a->bank[0].name,"ROUTE",5);
  a->scenes=1;memcpy(a->scene[0].name,"STATE",5);a->scene[0].apply=1;a->scene[0].level[7]=99;
  a->rows=1;a->row[0].repeat=2;d8p1_runtime_cache.valid=1;
  route_tick(AS_POLL_MS);uint32_t wanted=current_signature();CHECK(!reads&&!writes);
  queue_disturb();oracle();route_tick(AS_IDLE_MS);CHECK(!reads&&!writes&&!df_automatic_quiet());identity();
  drain();for(unsigned h=0;h<2;h++){qa_half=h;fm1_alnk0_irq();}drain();oracle();
  if(queue_kind==4){route_tick(AS_IDLE_MS+AS_GAP_MS);CHECK(!reads&&!writes&&!df_automatic_quiet());route_cases++;continue;}
  CHECK(df_automatic_quiet());route_tick(AS_IDLE_MS);
  CHECK(writes&&native_as.writes==1&&!native_as.pending&&!native_as.err);identity();
  uint32_t stored=0;d8pool_record record;CHECK(!d8p1_autosave_snapshot_flash(&stored,&record,1)&&stored==wanted&&native_as.saved==stored);
  trk[7].p[P_LEVEL]=1;a->scene[0].level[7]=1;CHECK(current_signature()!=wanted);
  CHECK(!d8p1_restore_flash_autosave(1,1)&&proj_cur==PROJ_NO_SLOT);
  CHECK(current_signature()==wanted&&d8p1_runtime_cache.arrangement.scene[0].level[7]==99&&d8p1_runtime_cache.arrangement.row[0].repeat==2);
  route_cases++;
 }
 CHECK(!irq_disabled&&!dma_errors);
 printf("Native combined main-loop autosave: %u checks, %u failures; %u actual queue/drain/save/restore routes; binding alone inactive, hold/end and track8/scene persistence; simulated NOR/DMA, no device qualification\n",checks,failures,route_cases);
 return failures!=0;
}
