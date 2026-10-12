/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual UI and shipping coordinator; private NOR primitive spies, no device. */
static void local_read_hook(void);
#define D8_LOCAL_READ_HOOK local_read_hook
static void local_lcd_hook(void);
static void local_primitive_hook(void);
#define D8_LOCAL_PRIMITIVE_HOOK local_primitive_hook
#define D8_LOCAL_LCD_HOOK local_lcd_hook
#define D8_NATIVE_READONLY_BOOT_NO_MAIN 1
#include "native_readonly_boot_test.c"
static uint8_t local_before[sizeof nor];
static unsigned local_lcd_event,local_primitive_event,local_lcd_count,consent_syncs,local_auto_late,local_auto_failure;
static void local_read_hook(void)
{
 if(local_auto_failure&&native_as.pending&&d8ls.phase==LS_EXECUTE&&d8ls.operation==1&&!d8ls.writing){
  unsigned action=local_auto_failure;local_auto_failure=0;
  if(action==1)post_commit_io=2;
  else {uint8_t *h=nor+d8pool_mapped_address(d8ls.index.object[0].block);h[4]=2;uint32_t crc=d8p1_crc32(h,28);for(unsigned i=0;i<4;i++)h[28+i]=(uint8_t)(crc>>(8*i));}
  memcpy(local_before,nor,sizeof nor);
 }
 if(local_auto_late&&native_as.pending&&d8ls.phase==LS_EXECUTE&&d8ls.operation==1&&!d8ls.writing){
  local_auto_late=0;fm1_in.notes=host_notes=1;ui_input();
 }
}
static void local_primitive_hook(void)
{
 unsigned event=local_primitive_event;local_primitive_event=0;
 if(event==1)main_workspace.d8p1.wire[100]^=1;
 if(event==2)usb.resets++;
 if(event==3)d8_capture_store_changed();
 if(event==4)transport_req=1;
 if(event==5)CHECK(d8ls_driver_erase(d8pool_mapped_address(d8ls.block))==-10);
}

static void local_lcd_hook(void)
{
 if(local_lcd_count){local_lcd_count--;if(local_lcd_count)return;}
 unsigned event=local_lcd_event;local_lcd_event=0;
 if(event==1)d8ls.generation++;
 if(event==2)d8_capture_store_changed();
 if(event==3)usb.resets++;
}
static void ls_test_boot(unsigned sparse)
{
 boot_seed();
 if(sparse){d8pool_index index;CHECK(!d8pool_inventory(&seed,&index));
  for(unsigned object=0;object<3;object++)memset(nor+d8pool_mapped_address(index.object[object].block),255,D8POOL_BLOCK);
  memcpy(baseline,nor,sizeof nor);
 }
 CHECK(autosave_boot(1)==1&&project_native_status()==1&&usb.config==0);
 int32_t audio[CTL*2];mix_block(audio,CTL);trk[7].p[P_LEVEL]=91;
}
static void ls_test_name(unsigned object,unsigned rename)
{
 go_page(GR_SLOTS);song.g[G_SLOT]=(int16_t)(object+1);ui.act=4;mc_ship_tap(rename?B_EDIT:B_OCTUP);
 if(ui.confirm==CF_OVR_PROJ)mc_ship_tap(B_OCTUP);
 CHECK(name_on()&&nm.kind==(rename?NK_PROJ_RENAME:NK_PROJ_SAVE));
 if(rename){strcpy(nm.s,"LOCAL RENAME");nm.len=nm.cur=12;}
 mc_ship_tap(B_OCTUP);CHECK(d8ls.phase==LS_CONSENT||d8ls.phase==LS_EXECUTE);
}
static void ls_test_untouched(void)
{
 uint32_t base=d8pool_mapped_address(d8ls.block);
 CHECK(d8ls.block<5&&!memcmp(nor,local_before,base)&&!memcmp(nor+base+D8POOL_BLOCK,local_before+base+D8POOL_BLOCK,sizeof nor-base-D8POOL_BLOCK));
}
static void ls_test_commit(void)
{
 memcpy(local_before,nor,sizeof nor);
 if(d8ls.phase==LS_CONSENT){unsigned before=sync_calls;ui_draw();consent_syncs=sync_calls-before;
  CHECK(d8ls.shown==d8ls.generation&&d8ls.shown);
  uint32_t save=1u<<panel.btn[B_SAVE];host_pressed=save;fm1_in.buttons=save;fm1_ms++;ui_input();
  d8ls_poll();CHECK(d8ls.phase==LS_RELEASE&&!d8ls.intent);ui_draw();CHECK(!d8ls.revoked&&d8ls.phase==LS_RELEASE);
  host_pressed=0;fm1_in.buttons=0;fm1_ms++;ui_input();
 }
 d8mc_poll();d8ls_poll();ls_test_untouched();
}
static unsigned ls_test_level(unsigned object)
{
 size_t n=0;CHECK(!d8pool_load(&seed,object,out,sizeof out,&n,7));CHECK(d8p1_project_decode(&boot_state,out,n,7));
 return boot_state.project.t[7].p[P_LEVEL];
}
int main(void)
{
 /* Stable-field oracle is independent of either production helper. The
  * predicate has ordinary reads; this is not concurrent atomicity evidence. */
 static const uint8_t flags[]={0,1,2,255};
 uint8_t prior_locked=d8mc_edit.locked,prior_phase=d8ls.phase,prior_busy=d8ls.busy;
 for(unsigned phase=0;phase<4;phase++)for(unsigned locked=0;locked<4;locked++)for(unsigned busy=0;busy<4;busy++) {
  d8mc_edit.locked=flags[locked];d8ls.phase=(uint8_t)phase;d8ls.busy=flags[busy];
  int expected=flags[locked]!=0||phase!=0||flags[busy]!=0;
  CHECK(d8mc_blocked()==expected);
 }
 d8mc_edit.locked=prior_locked;d8ls.phase=prior_phase;d8ls.busy=prior_busy;
 /* Bounded name equivalence uses the actual firmware helper, including empty
  * and full12-byte names. Overlong/malformed/NULL input cannot mint a match. */
 char empty[13]={0},one[13]="A",twelve[13]="ABCDEFGHIJKL";
 CHECK(ls_name_equal(empty,"")&&ls_name_equal(one,"A")&&ls_name_equal(twelve,"ABCDEFGHIJKL"));
 CHECK(!ls_name_equal(one,"")&&!ls_name_equal(one,"AB")&&!ls_name_equal(twelve,"ABCDEFGHIJKLM")&&!ls_name_equal(twelve,"ABCDEFGHIJK"));
 CHECK(!ls_name_equal(empty,NULL)&&!ls_name_equal(one,"A\n")&&!ls_name_equal(one,"A\x7f")&&!ls_name_equal(one,"A\x80"));
 ls_test_boot(0);ls_test_name(0,0);CHECK(msg_is("SAVE TO CONFIRM")&&!writes&&!physical_calls);
 ls_test_commit();CHECK(msg_is("SAVED")&&physical_calls&&d8ls.intent&&native_as.ready);CHECK(ls_test_level(0)==91);
 unsigned calls=physical_calls;CHECK(d8p1_save_as_flash(1,"FABRICATED",1)==D8POOL_UNSUPPORTED&&physical_calls==calls);
 /* Idle USB disconnect retains standalone intent. Rename uses the actual name
  * acceptance pathway, while another backend call still has no proposal. */
 usb.resets++;ls_test_name(0,1);CHECK(d8ls.phase==LS_EXECUTE);ls_test_commit();
 CHECK(msg_is("RENAMED")&&d8ls.intent&&physical_calls>calls);CHECK(!strcmp(proj_name,"LOCAL RENAME"));
 trk[7].p[P_LEVEL]=103;fm1_ms+=AS_POLL_MS;CHECK(!d8p1_autosave_session_poll());
 fm1_ms+=AS_IDLE_MS+AS_POLL_MS;CHECK(!d8p1_autosave_session_poll());CHECK(ls_test_level(3)==103&&native_as.writes==1&&d8ls.phase==LS_IDLE);
 calls=physical_calls;CHECK(d8p1_autosave_flash(1)==D8POOL_UNSUPPORTED&&physical_calls==calls);
 /* A late automatic admission refusal retires its exact proposal. The next
  * main-loop poll must not reinterpret it as a rename or acknowledge a save. */
 trk[7].p[P_LEVEL]=104;fm1_ms+=AS_POLL_MS;CHECK(!d8p1_autosave_session_poll());
 calls=physical_calls;local_auto_late=1;fm1_ms+=AS_GAP_MS+AS_IDLE_MS+AS_POLL_MS;
 CHECK(d8p1_autosave_session_poll()==D8POOL_BUSY&&!local_auto_late&&physical_calls==calls&&d8ls.phase==LS_IDLE&&d8ls.intent);
 char refusal_message[sizeof ui.msg];memcpy(refusal_message,ui.msg,sizeof ui.msg);d8ls_poll();
 CHECK(d8ls.phase==LS_IDLE&&physical_calls==calls&&!memcmp(refusal_message,ui.msg,sizeof ui.msg));
 CHECK(fm1_in.notes==1&&ls_test_level(3)==103);
 fm1_in.notes=host_notes=0;fm1_ms+=AS_RETRY_MS+AS_IDLE_MS+AS_POLL_MS;
 CHECK(!d8p1_autosave_session_poll()&&d8ls.intent&&d8ls.phase==LS_IDLE&&ls_test_level(3)==104);
 /* Local session intent never enables generic legacy/settings/sample writes,
  * remote full-project APPLY, or a forged backend request. */
 uint8_t apply[]={0x7d,0x46,0x4c,ED_BACKUP_PUT,2,0};
 uint32_t music=st_crc32(trk,sizeof trk);memcpy(baseline,nor,sizeof nor);calls=physical_calls;
 ed_handle(apply,sizeof apply);so_r=so_w;
 CHECK(d8ls.intent&&music==st_crc32(trk,sizeof trk)&&!memcmp(nor,baseline,sizeof nor)&&physical_calls==calls);
 uint8_t byte=0;CHECK(!d8_instrument_write_allowed());
 CHECK(capture_hw_prog(0x97000,&byte,1)==D8_INSTRUMENT_QUARANTINED&&capture_hw_erase(0xfc000)==D8_INSTRUMENT_QUARANTINED);
 CHECK(ed_smp_erase(0,1)==D8_INSTRUMENT_QUARANTINED&&physical_calls==calls&&!memcmp(nor,baseline,sizeof nor));
 /* An actual current UI proposal opens one narrow primitive ticket. Malformed
  * physical addresses/blocks must refuse before entering even the spy. */
 ls_test_name(0,0);CHECK(d8ls.phase==LS_EXECUTE&&d8ls.intent);
 CHECK(!d8ls_native_begin(0,0,d8ls.name));calls=physical_calls;
 static const uint32_t bad_address[]={0,0x4000,0xe0000,0xfc000,0xa0000,0xffffffff};
 for(unsigned i=0;i<sizeof bad_address/sizeof *bad_address;i++)CHECK(d8ls_driver_erase(bad_address[i])==-10&&physical_calls==calls);
 unsigned valid_block=d8ls.block;d8ls.block=5;CHECK(d8ls_driver_erase(0)==-10&&physical_calls==calls);d8ls.block=(uint8_t)valid_block;
 CHECK(d8ls_driver_program(d8pool_mapped_address(valid_block),&byte,UINT32_MAX)==-10&&physical_calls==calls);
 /* No draw, a pre-render key, and events during LCD sync never grant consent. */
 for(unsigned event=0;event<7;event++) {
  ls_test_boot(0);ls_test_name(0,0);
  if(event){if(event<4){boot_lcd_late=(int)consent_syncs;boot_event=(int)event;}else {local_lcd_event=event-3;local_lcd_count=consent_syncs;}ui_draw();CHECK(!d8ls.shown);}
  mc_ship_tap(B_SAVE);d8ls_poll();CHECK(!d8ls.intent&&!physical_calls&&!writes&&!memcmp(nor,baseline,sizeof nor));
 }
 /* Actual between-block dispatcher: a blocked local proposal must revoke
  * without consuming queued/external input, stopping audio or altering music.
  * This is the precise source route compiled into ALNK, not a guard model. */
 for(unsigned event=0;event<4;event++) {
  ls_test_boot(0);ls_test_name(0,0);ui_draw();CHECK(d8ls.shown==d8ls.generation);
  uint8_t tracks_before[sizeof trk],song_before[sizeof song],cache_before[sizeof d8p1_runtime_cache];
  memcpy(tracks_before,trk,sizeof trk);memcpy(song_before,&song,sizeof song);memcpy(cache_before,&d8p1_runtime_cache,sizeof d8p1_runtime_cache);
  if(event==0)transport_req=1;
  if(event==1)midi_in_event(0x643c9009u);
  if(event==2)midi_in_overflow=1;
  if(event==3)fm1_in.notes=1;
  uint32_t read=mi_r,write=mi_w,transport=transport_req,overflow=midi_in_overflow,notes=fm1_in.notes;
  events_block(CTL);
  CHECK(d8ls.revoked&&!d8ls.shown&&!d8ls.intent&&d8ls.phase==LS_CONSENT);
  CHECK(mi_r==read&&mi_w==write&&transport_req==transport&&midi_in_overflow==overflow&&fm1_in.notes==notes);
  CHECK(!memcmp(tracks_before,trk,sizeof trk)&&!memcmp(song_before,&song,sizeof song)&&!memcmp(cache_before,&d8p1_runtime_cache,sizeof d8p1_runtime_cache));
  CHECK(!physical_calls&&!writes&&!memcmp(nor,baseline,sizeof nor));
  d8ls_poll();CHECK(d8ls.phase==LS_IDLE&&!d8ls.intent&&!physical_calls);
  /* External fixture release/drain only after proving the dispatcher retained
   * the event. This is not production admission or cancellation behavior. */
  mi_r=mi_w;midi_in_overflow=0;transport_req=0;fm1_in.notes=0;
 }
 /* Canonical initialization may leave all three slots genuinely absent; each
  * actual SAVE adds one stable slot without a new format or blank inference. */
 ls_test_boot(1);d8pool_index index;CHECK(!d8pool_inventory(&seed,&index)&&index.present==8);
 for(unsigned object=0;object<3;object++){ls_test_name(object,0);ls_test_commit();CHECK(msg_is("SAVED")&&ls_test_level(object)==91);}
 CHECK(!d8pool_inventory(&seed,&index)&&index.present==15);
 /* Sparse damaged allocation cannot masquerade as an empty project slot. */
 ls_test_boot(1);nor[d8pool_mapped_address(0)+300]=0;memcpy(baseline,nor,sizeof nor);
 go_page(GR_SLOTS);song.g[G_SLOT]=1;ui.act=4;mc_ship_tap(B_OCTUP);CHECK(name_on());mc_ship_tap(B_OCTUP);
 CHECK(msg_is("STORAGE ERROR")&&!d8ls.intent&&!physical_calls&&!memcmp(nor,baseline,sizeof nor));
 /* Late primitive events cannot reuse the operation. Reentry refuses while
  * the raw driver owns its one IRQ window; a changed staging wire cannot be
  * programmed further or acquire a commit header. */
 for(unsigned event=1;event<=5;event++) {
  ls_test_boot(0);ls_test_name(0,0);ui_draw();mc_ship_tap(B_SAVE);local_primitive_event=event;d8ls_poll();
  if(event==5)CHECK(msg_is("SAVED")&&d8ls.intent&&ls_test_level(0)==91);
  else {CHECK(msg_is("STORAGE ERROR")&&!d8ls.intent&&physical_calls==1);CHECK(ls_test_level(0)==27);}
 }
 /* Independently established unreadable/unsupported storage after admission
  * ends local intent even before the first mutation. A fabricated backend
  * refusal above did not have this authority or this effect. */
 for(unsigned automatic=0;automatic<2;automatic++)for(unsigned action=1;action<=2;action++) {
  ls_test_boot(0);
  if(automatic){ls_test_name(0,0);ls_test_commit();trk[7].p[P_LEVEL]=111;fm1_ms+=AS_POLL_MS;CHECK(!d8p1_autosave_session_poll());
   calls=physical_calls;local_auto_failure=action;fm1_ms+=AS_IDLE_MS+AS_POLL_MS;CHECK(d8p1_autosave_session_poll()!=D8POOL_OK&&!local_auto_failure);
  } else {
   ls_test_name(0,0);ui_draw();mc_ship_tap(B_SAVE);calls=physical_calls;
   if(action==1)post_commit_io=2;
   else {uint8_t *h=nor+d8pool_mapped_address(d8ls.index.object[0].block);h[4]=2;uint32_t crc=d8p1_crc32(h,28);for(unsigned i=0;i<4;i++)h[28+i]=(uint8_t)(crc>>(8*i));}
   memcpy(local_before,nor,sizeof nor);d8ls_poll();
  }
  CHECK(!d8ls.intent&&d8ls.phase==LS_IDLE&&!native_as.ready&&project_native_status()==2&&physical_calls==calls&&!memcmp(nor,local_before,sizeof nor));
  post_commit_io=0;
 }
 /* Each primitive cut preserves a valid prior/current record; uncertainty
  * revokes the editing intent and prevents an automatic retry. */
 for(unsigned failure=0;failure<29;failure++) {
  ls_test_boot(0);ls_test_name(0,0);ui_draw();mc_ship_tap(B_SAVE);
  memcpy(local_before,nor,sizeof nor);cut=(int)failure;partial_failure=failure%3==0?1:failure%3==1?128:0;d8ls_poll();ls_test_untouched();
  CHECK(msg_is("STORAGE ERROR")&&!d8ls.intent&&project_native_status()==2&&!native_as.ready);
  CHECK(ls_test_level(0)==(failure==28&&partial_failure>=32?91u:27u));calls=physical_calls;CHECK(d8p1_save_as_flash(0,"RETRY",1)==D8POOL_UNSUPPORTED&&physical_calls==calls);
 }
 printf("Local native coordinator: %u checks, %u failures; actual UI/save/rename/autosave, sparse creation, visible consent and primitive cuts; no virtual permission define\n",checks,failures);return failures!=0;
}
