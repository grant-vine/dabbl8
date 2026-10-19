/* SPDX-License-Identifier: GPL-3.0-only */
#define D8_PROJECT_TRANSFER_SESSION 1
#define D8_NATIVE_LOCAL_SAVE_NO_MAIN 1
static void pt_begin_lcd_hook(void);
static void pt_begin_read_hook(void);
#define D8_LOCAL_FIRST_USE_LCD_HOOK pt_begin_lcd_hook
#define D8_LOCAL_COMPLETION_READ_HOOK pt_begin_read_hook
#include "native_local_save_test.c"
#define FL_FAR(fn) fn
#define fl_erase4k_ram mc_ram_erase_spy
#define fl_prog_ram mc_ram_program_spy
#include "../firmware/src/native_project_transfer.c"
#undef fl_prog_ram
#undef fl_erase4k_ram
#undef FL_FAR
static uint8_t message[600],source_pool[D8POOL_BYTES],source_live[D8P1_LIMIT];
static uint8_t pt_arena_before[sizeof main_workspace];
static unsigned pt_lcd_event,pt_read_event;
static uint32_t pt_competing_lease;
static void pt_begin_lcd_hook(void)
{
 unsigned event=pt_lcd_event;pt_lcd_event=0;
 if(event==1)d8_capture_store_changed();
 else if(event==2)usb.resets++;
 else if(event==3)usb.config^=1;
 else if(event==4)transport_req=1;
 else if(event==5)fm1_in.notes=host_notes=1;
 else if(event==6)d8ls_revoke();
 else if(event==7)fm1_ms+=LS_DEADLINE_MS;
 else if(event==8){CHECK(!main_migration_begin(&pt_competing_lease));memset(main_migration_workspace(pt_competing_lease),0x6c,sizeof(d8mp_workspace));memcpy(pt_arena_before,&main_workspace,sizeof pt_arena_before);}
 else if(event==9)cv_cpu_active=1;
 else if(event==10){fm1_in.buttons=host_pressed=1u<<panel.btn[B_SAVE];ui_input();}
}
static void pt_begin_read_hook(void)
{
 if(pt_read_event==1){pt_read_event=0;usb.resets++;}
 else if(pt_read_event==2&&d8ls.phase==LS_EXECUTE&&pt_phase()==PT_EXPORT){pt_read_event=0;post_commit_io=2;}
 else if(pt_read_event>=3&&!migration_owner&&d8ls.writing&&d8ls.operation==4&&pt_phase()==PT_APPROVED){
  if(pt_read_event==6&&d8pt_diag.stage!=PTD_AUTOSAVE)return;
  unsigned event=pt_read_event;pt_read_event=0;
  if(event==3||event==6)post_commit_io=2;
  else if(event==4)fm1_ms=d8ls.started+LS_DEADLINE_MS+1;
  else {transport_req=1;events_block(CTL);transport_req=0;}
 }
}
static unsigned pt_request(unsigned n){uint8_t f[600]={0x7d,0x46,0x4c,81};memcpy(f+4,message,n);ed_handle(f,n+4);so_r=so_w;CHECK(ed_out[4]==81&&ed_out[5]==1);return ed_out[7];}
static unsigned export_start(void){memset(message,0,sizeof message);message[0]=1;message[1]=1;return pt_request(2);}
static void pt_token(unsigned op){memset(message,0,sizeof message);message[0]=1;message[1]=(uint8_t)op;mc_put32(message+2,d8ls.generation);}
static unsigned pt_pack_test(const uint8_t*p,unsigned n,uint8_t*out){unsigned z=0;while(n){unsigned k=n>7?7:n,m=0;for(unsigned i=0;i<k;i++)m|=(p[i]>>7)<<i;out[z++]=(uint8_t)m;for(unsigned i=0;i<k;i++)out[z++]=p[i]&127;p+=k;n-=k;}return z;}
static int source_rd(void*c,uint32_t a,void*p,uint32_t n){(void)c;memcpy(p,source_pool+a,n);return 0;}
static int source_er(void*c,uint32_t a){(void)c;memset(source_pool+a,255,4096);return 0;}
static int source_pg(void*c,uint32_t a,const void*p,uint32_t n){(void)c;for(unsigned i=0;i<n;i++)source_pool[a+i]&=((const uint8_t*)p)[i];return 0;}
static d8pool source_store={NULL,source_rd,source_er,source_pg,stop};
static unsigned archived_live_len;
static unsigned import_start(unsigned present){memset(message,0,sizeof message);message[0]=1;message[1]=2;message[2]=(uint8_t)present;mc_put32(message+3,archived_live_len);mc_put32(message+8,d8p1_crc32(source_live,archived_live_len));mc_put32(message+13,d8p1_crc32(source_pool,sizeof source_pool));return pt_request(18);}
static void import_receive(void){
 for(unsigned selector=0;selector<2;selector++){const uint8_t*p=selector?source_live:source_pool;unsigned length=selector?archived_live_len:sizeof source_pool;
  for(unsigned off=0;off<length;off+=256){unsigned count=length-off;if(count>256)count=256;pt_token(4);message[7]=(uint8_t)selector;mc_put32(message+8,off);unsigned n=13+pt_pack_test(p+off,count,message+13);CHECK(pt_request(n)==PT_OK);unsigned before=physical_calls;CHECK(pt_request(n)==PT_OK&&physical_calls==before);}}
}
static void approve_restore(void){pt_token(5);CHECK(pt_request(7)==PT_OK&&pt_phase()==PT_CONSENT);d8ls_draw();CHECK(d8ls.shown==d8ls.generation);mc_ship_tap(B_SAVE);CHECK(pt_phase()==PT_RELEASE);d8ls_poll();CHECK(pt_phase()==PT_APPROVED);}
static void target_fresh(void){boot_seed();CHECK(autosave_boot(1)==1);int32_t audio[CTL*2];mix_block(audio,CTL);cut=-1;partial_failure=0;physical_calls=0;}
static void target_only_zero(void){target_fresh();d8pool_index idx;CHECK(!d8pool_inventory(&seed,&idx));for(unsigned o=1;o<4;o++)memset(nor+d8pool_mapped_address(idx.object[o].block),255,D8POOL_BLOCK);d8p1_autosave_session_end();}
static void retained_reboot(void){cold();project_native_reset();native_as.ready=0;d8ls_reset();memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);memset(&d8_capture_change,0,sizeof d8_capture_change);flash_ok=1;usb.config=0;usb.ota_req=usb.uboot_req=0;ui_prefs&=~PREF_RESTORE_OFF;int32_t audio[CTL*2];mix_block(audio,CTL);reset();unsigned before=physical_calls;(void)autosave_boot(1);CHECK(project_native_owns_storage()&&physical_calls==before);}
static void verify_restored(unsigned present){d8pool_index idx;CHECK(!d8pool_inventory(&seed,&idx)&&idx.present==present);for(unsigned o=0;o<4;o++)if(present&(1u<<o)){d8pool_index src;CHECK(!d8pool_inventory(&source_store,&src));CHECK(idx.object[o].length==src.object[o].length&&idx.object[o].crc==src.object[o].crc);}size_t n=0;CHECK(!d8p1_capture_runtime(wire,sizeof wire,&n)&&n==archived_live_len&&!memcmp(wire,source_live,n));}
static void pt_completion_cases(void)
{
 for(unsigned event=0;event<5;event++){
  target_fresh();CHECK(import_start(15)==PT_OK);import_receive();approve_restore();
  CHECK(!d8pt_diag.stage&&!d8pt_diag.raw_rc&&!d8pt_diag.guards&&!d8pt_diag.revoke);
  pt_read_event=event?event+2:0;pt_token(7);unsigned rc=pt_request(7);
  CHECK(!pt_read_event&&rc==(event?PT_IO:PT_OK));post_commit_io=0;
  verify_restored(15);
  CHECK(!migration_owner&&!d8ls.writing&&!d8ls.busy&&d8ls.phase==LS_IDLE);
  if(event){
   CHECK(project_native_status()==2&&!native_as.ready&&!d8ls.intent);
   CHECK(d8pt_diag.stage==(event==4?PTD_AUTOSAVE:PTD_BIND)&&d8pt_diag.raw_rc);
   if(event==2)CHECK(d8pt_diag.guards&2048u);
   if(event==3)CHECK((d8pt_diag.guards&1u)&&(d8pt_diag.revoke&1u));
   __typeof__(d8pt_diag) retained=d8pt_diag;
   CHECK(export_start()==PT_OK);pt_token(8);CHECK(pt_request(7)==PT_OK);
   CHECK(!memcmp(&retained,&d8pt_diag,sizeof retained));
  }
  else{
   CHECK(d8pt_diag.stage==PTD_DONE&&!d8pt_diag.raw_rc&&!d8pt_diag.guards&&!d8pt_diag.revoke);
   CHECK(project_native_status()==1&&native_as.ready);pt_token(8);CHECK(pt_request(7)==PT_OK);
   unsigned before=physical_calls;CHECK(export_start()==PT_OK&&physical_calls==before);
   CHECK(!memcmp(pt_workspace()->stage.wire,source_live,archived_live_len));
   pt_token(8);CHECK(pt_request(7)==PT_OK);
  }
  printf("Completion event=%u status=%u stage=%u raw_rc=%u guards=%x revoke=%x exact_persisted_and_live=1\n",event,rc,d8pt_diag.stage,d8pt_diag.raw_rc,d8pt_diag.guards,d8pt_diag.revoke);
 }
}
static void pt_begin_arena_cases(void)
{
 for(unsigned importing=0;importing<2;importing++){
  target_fresh();size_t n=0;CHECK(!d8p1_capture_runtime(source_live,sizeof source_live,&n));archived_live_len=(unsigned)n;
  cv_begin(240,8,0x5a5a);cv_blit(0,0);CHECK(dma.p&&!cv_cpu_active&&!migration_owner);
  unsigned consumed=dma_consumed,errors=dma_errors;uint16_t pixels[240*8];memcpy(pixels,dma.p,sizeof pixels);
  unsigned rc=importing?import_start(15):export_start();
  unsigned exact=!memcmp(host_screen,pixels,sizeof pixels);
  printf("BEGIN DMA %s rc=%u consumed=%u errors=%u screen_exact=%u physical_calls=%u\n",importing?"import":"export",rc,dma_consumed-consumed,dma_errors-errors,exact,physical_calls);
  CHECK(rc==PT_OK&&pt_phase()==(importing?PT_RECEIVE:PT_EXPORT)&&pt_workspace());
  CHECK(dma_consumed==consumed+1&&!dma.p&&dma_errors==errors&&exact&&!physical_calls&&!writes&&!memcmp(nor,baseline,sizeof nor));
  uint32_t owner=migration_owner;pt_token(8);message[2]^=1;CHECK(pt_request(7)==PT_STALE&&migration_owner==owner);
  pt_token(8);CHECK(pt_request(7)==PT_OK&&!migration_owner&&!d8ls.signature&&!d8ls_blocked()&&!physical_calls);
 }
 /* An already-owned arena is refused without touching its immutable bytes. */
 target_fresh();uint32_t owner=0;CHECK(!main_migration_begin(&owner));memset(main_migration_workspace(owner),0x6c,sizeof(d8mp_workspace));
 memcpy(pt_arena_before,&main_workspace,sizeof pt_arena_before);unsigned before=reads;
 CHECK(export_start()==PT_BUSY&&migration_owner==owner&&reads==before&&!memcmp(pt_arena_before,&main_workspace,sizeof pt_arena_before)&&!physical_calls);
 CHECK(import_start(15)==PT_BUSY&&migration_owner==owner&&reads==before&&!memcmp(pt_arena_before,&main_workspace,sizeof pt_arena_before)&&!physical_calls);
 CHECK(!main_migration_end(owner));
 /* Each event lands inside the real LCD fence, after the initial admission. */
 for(unsigned importing=0;importing<2;importing++)for(unsigned event=1;event<=9;event++){
  target_fresh();cv_begin(8,8,0x5a5a);cv_blit(0,0);memcpy(pt_arena_before,&main_workspace,sizeof pt_arena_before);
  before=reads;unsigned errors=dma_errors;pt_competing_lease=0;pt_lcd_event=event;
  unsigned rc=importing?import_start(15):export_start();
  unsigned intact=!memcmp(pt_arena_before,&main_workspace,sizeof pt_arena_before);
  printf("BEGIN fence %s event=%u rc=%u scan_reads=%u errors=%u arena_intact=%u owner=%u physical_calls=%u\n",importing?"import":"export",event,rc,reads-before,dma_errors-errors,intact,migration_owner,physical_calls);
  CHECK(!pt_lcd_event&&rc==PT_BUSY&&reads==before&&intact&&dma_errors==errors&&!dma.p&&!physical_calls&&!writes&&!memcmp(nor,baseline,sizeof nor));
  CHECK(!d8ls.signature&&!d8ls_blocked()&&d8ls.phase==LS_IDLE&&migration_owner==(event==8?pt_competing_lease:0));
  if(d8ls.phase!=LS_IDLE){pt_token(8);CHECK(pt_request(7)==PT_OK);}
  if(pt_competing_lease)CHECK(!main_migration_end(pt_competing_lease));
  cv_cpu_active=0;transport_req=0;fm1_in.notes=host_notes=0;
 }
 /* Every failure after acquisition retires the lease; active stale sessions
  * and explicit CANCEL do the same through the shipping dispatcher/poll. */
 for(unsigned importing=0;importing<2;importing++)for(unsigned event=0;event<3;event++){
  target_fresh();if(event==0)post_commit_io=2;
  else if(event==1)nor[d8pool_mapped_address(0)+D8POOL_HEADER]^=1;
  else pt_read_event=1;
  unsigned rc=importing?import_start(15):export_start();
  CHECK(rc==(event==2?PT_BUSY:PT_UNSUPPORTED)&&!migration_owner&&!d8ls.signature&&!d8ls_blocked()&&!physical_calls&&!writes);
  printf("BEGIN failure %s event=%u rc=%u owner=%u physical_calls=%u\n",importing?"import":"export",event,rc,migration_owner,physical_calls);
 }
 target_fresh();song.sel=NTRK;CHECK(export_start()==PT_UNSUPPORTED&&!migration_owner&&!d8ls.signature&&!d8ls_blocked()&&!physical_calls);
 target_fresh();pt_read_event=2;CHECK(export_start()==PT_IO&&!pt_read_event&&!migration_owner&&!d8ls.signature&&!d8ls_blocked()&&!physical_calls);
 printf("BEGIN export copy failure owner=%u signature=%u physical_calls=%u\n",migration_owner,d8ls.signature,physical_calls);
 for(unsigned importing=0;importing<2;importing++){
  target_fresh();CHECK((importing?import_start(15):export_start())==PT_OK);usb.resets++;d8ls_poll();
  CHECK(d8ls.phase==LS_IDLE&&d8ls.write_phase==PT_CANCELLED&&!migration_owner&&!d8ls.signature&&!physical_calls);
 }
 target_fresh();CHECK(export_start()==PT_OK);owner=d8ls.signature;CHECK(!main_migration_end(owner));CHECK(!main_migration_begin(&owner));
 memset(main_migration_workspace(owner),0x6c,sizeof(d8mp_workspace));memcpy(pt_arena_before,&main_workspace,sizeof pt_arena_before);d8ls_poll();
 CHECK(d8ls.phase==LS_IDLE&&!d8ls.signature&&migration_owner==owner&&!memcmp(pt_arena_before,&main_workspace,sizeof pt_arena_before)&&!physical_calls);CHECK(!main_migration_end(owner));
 printf("BEGIN arena regressions: %u checks %u failures\n",checks,failures);
}
#include "native_consent_pixels.h"
static void pt_stable_consent_cases(void)
{
 target_fresh();CHECK(import_start(15)==PT_OK);import_receive();pt_token(5);CHECK(pt_request(7)==PT_OK);
 ui_draw();CHECK(d8ls.shown==d8ls.generation);
 unsigned frames=dma_consumed,calls=physical_calls;
 uint16_t pixels[240*240];memcpy(pixels,host_screen,sizeof pixels);
 for(unsigned i=0;i<100;i++){ui_draw();fm1_ms++;d8ls_poll();}
 CHECK(dma_consumed==frames&&!memcmp(pixels,host_screen,sizeof pixels));
 CHECK(d8ls.shown==d8ls.generation&&!d8ls.revoked&&physical_calls==calls);
 fm1_ms=d8ls.started+LS_DEADLINE_MS-2000u;d8ls_poll();
 pt_lcd_event=10;ui_draw();
 CHECK(pt_lcd_event==10&&d8ls.shown==d8ls.generation&&!d8ls.revoked);
 pt_lcd_event=0;fm1_in.buttons=host_pressed=1u<<panel.btn[B_SAVE];ui_input();
 CHECK(pt_phase()==PT_RELEASE&&!d8ls.revoked);
 CHECK(!df_automatic_quiet()&&pt_quiet());
 fm1_in.buttons|=1u<<panel.btn[B_HOME];CHECK(!pt_quiet());
 fm1_in.buttons=1u<<panel.btn[B_SAVE];fm1_in.notes=1;CHECK(!pt_quiet());fm1_in.notes=0;
 transport_req=1;CHECK(!pt_quiet());transport_req=0;
 for(unsigned i=0;i<30;i++){ui_draw();fm1_ms+=10;d8ls_poll();}
 CHECK(pt_phase()==PT_RELEASE&&!d8ls.revoked&&dma_consumed==frames&&physical_calls==calls);
 host_pressed=0;fm1_in.buttons=0;ui_input();d8ls_poll();
 CHECK(pt_phase()==PT_APPROVED&&physical_calls==calls);
 pt_token(8);CHECK(pt_request(7)==PT_OK&&!migration_owner);

 target_fresh();CHECK(import_start(15)==PT_OK);import_receive();pt_token(5);CHECK(pt_request(7)==PT_OK);
 ui_draw();calls=physical_calls;fm1_ms=d8ls.started+LS_DEADLINE_MS;d8ls_poll();
 CHECK(d8ls.phase==LS_IDLE&&d8ls.write_phase==PT_CANCELLED&&physical_calls==calls&&!migration_owner);

 target_fresh();CHECK(import_start(15)==PT_OK);import_receive();pt_token(5);CHECK(pt_request(7)==PT_OK);
 ui_draw();calls=physical_calls;fm1_lcd_timeouts++;ui_draw();
 mc_ship_tap(B_SAVE);d8ls_poll();
 CHECK(d8ls.phase==LS_IDLE&&physical_calls==calls&&!migration_owner);
 target_fresh();CHECK(import_start(15)==PT_OK);import_receive();pt_token(5);CHECK(pt_request(7)==PT_OK);
 calls=physical_calls;pt_lcd_event=10;ui_draw();d8ls_poll();
 CHECK(!pt_lcd_event&&d8ls.phase==LS_IDLE&&physical_calls==calls&&!migration_owner);
 fm1_in.buttons=host_pressed=0;
 printf("Stable restore consent regressions: %u checks %u failures\n",checks,failures);
}
#ifndef D8_PROJECT_TRANSFER_NO_MAIN
int main(void){
 setvbuf(stdout,NULL,_IONBF,0);
 pt_begin_arena_cases();
 boot_seed();CHECK(autosave_boot(1)==1);int32_t audio[CTL*2];mix_block(audio,CTL);
 message[0]=1;message[1]=0;CHECK(pt_request(2)==PT_OK);
 trk[7].p[P_LEVEL]=91;message[1]=1;unsigned beginrc=pt_request(2);CHECK(beginrc==PT_OK&&pt_phase()==PT_EXPORT&&!physical_calls);if(beginrc)return 1;
 memcpy(source_pool,pt_workspace()->plan,sizeof source_pool);unsigned live_length=d8ls.length;archived_live_len=live_length;memcpy(source_live,pt_workspace()->stage.wire,live_length);
 pt_token(8);CHECK(pt_request(7)==PT_OK&&!migration_owner);
 pt_completion_cases();
 pt_stable_consent_cases();
 memset(message,0,sizeof message);message[0]=1;message[1]=2;message[2]=15;mc_put32(message+3,live_length);mc_put32(message+8,d8p1_crc32(source_live,live_length));mc_put32(message+13,d8p1_crc32(source_pool,sizeof source_pool));
 CHECK(pt_request(18)==PT_OK&&pt_phase()==PT_RECEIVE);
 for(unsigned selector=0;selector<2;selector++){const uint8_t*p=selector?source_live:source_pool;unsigned length=selector?live_length:sizeof source_pool;
  for(unsigned off=0;off<length;off+=256){unsigned count=length-off;if(count>256)count=256;pt_token(4);message[7]=(uint8_t)selector;mc_put32(message+8,off);unsigned n=13+pt_pack_test(p+off,count,message+13);CHECK(pt_request(n)==PT_OK);}}
 pt_token(5);CHECK(pt_request(7)==PT_OK&&pt_phase()==PT_CONSENT&&!physical_calls);
 consent_pixels("restore",76,AF_S.h,160,AF_S.h);CHECK(!physical_calls);
 d8ls_draw();CHECK(d8ls.shown==d8ls.generation);mc_ship_tap(B_SAVE);CHECK(pt_phase()==PT_RELEASE);d8ls_poll();CHECK(pt_phase()==PT_APPROVED);
 pt_token(7);CHECK(pt_request(7)==PT_OK&&d8ls.phase==LS_IDLE&&!migration_owner);CHECK(native_as.ready&&project_native_status()==1&&!d8_instrument_write_allowed());
 uint32_t adopted_sig=0;CHECK(!d8p1_signature_runtime(&adopted_sig)&&native_as.saved==adopted_sig);d8pool_record preserved_as;CHECK(!d8pool_current(&seed,3,&preserved_as));unsigned before_idle=physical_calls;fm1_ms+=AS_IDLE_MS+AS_POLL_MS;CHECK(!d8p1_autosave_session_poll()&&physical_calls==before_idle);d8pool_record after_idle;CHECK(!d8pool_current(&seed,3,&after_idle)&&!memcmp(&after_idle,&preserved_as,sizeof after_idle));

 /* Exact sparse role restoration, mutually referenced projects on a fresh
  * existing own store, independent AS absence and unchanged outside pool. */
 memset(source_pool,255,sizeof source_pool);d8p1_project_state cyclic;
 CHECK(d8p1_project_decode(&cyclic,source_live,archived_live_len,0));
 cyclic.arrangement.banks=cyclic.arrangement.scenes=cyclic.arrangement.rows=1;cyclic.arrangement.row[0].repeat=1;
 for(unsigned o=1;o<=2;o++){for(unsigned t=0;t<8;t++){cyclic.arrangement.bank[0].project[t]=(uint8_t)(3-o);cyclic.arrangement.bank[0].track[t]=(uint8_t)t;}size_t n=0;CHECK(d8p1_project_encode(wire,sizeof wire,&n,&cyclic));CHECK(!d8pool_save(&source_store,o,wire,n,6));}
 target_only_zero();CHECK(import_start(6)==PT_OK);import_receive();approve_restore();memcpy(local_before,nor,sizeof nor);pt_token(7);CHECK(pt_request(7)==PT_OK);verify_restored(6);
 CHECK(!memcmp(nor,local_before,0x97000)&&!memcmp(nor+0x9f000,local_before+0x9f000,0x46000)&&!memcmp(nor+0xe7000,local_before+0xe7000,sizeof nor-0xe7000));
 unsigned operations=physical_calls;CHECK(operations>4);pt_token(7);unsigned before=physical_calls;CHECK(pt_request(7)==PT_OK&&physical_calls==before);pt_token(8);CHECK(pt_request(7)==PT_OK);
 /* Cut at first payload, between cyclic roles, and tombstone header. Retained
  * archive retry gets a new visible confirmation, never cached READY authority. */
 unsigned points[]={2,operations/2,operations-1};
 for(unsigned k=0;k<3;k++){target_only_zero();CHECK(import_start(6)==PT_OK);import_receive();approve_restore();cut=(int)points[k];partial_failure=17;pt_token(7);CHECK(pt_request(7)==PT_IO&&d8ls.phase==LS_IDLE&&!migration_owner);cut=-1;partial_failure=0;transport_req=0;retained_reboot();CHECK(import_start(6)==PT_OK);import_receive();approve_restore();pt_token(7);CHECK(pt_request(7)==PT_OK);verify_restored(6);}
 /* Actual grammar/token/source CRC/unknownfuture and quiet/LCD guards refuse
  * before any destructive primitive. Host source remains available for retry. */
 target_fresh();CHECK(import_start(6)==PT_OK);unsigned untouched=physical_calls;pt_token(5);CHECK(pt_request(7)==PT_INCOMPLETE&&physical_calls==untouched);
 pt_token(4);message[7]=0;mc_put32(message+8,256);unsigned malformed=13+pt_pack_test(source_pool,256,message+13);CHECK(pt_request(malformed)==PT_BAD&&physical_calls==untouched);pt_token(6);message[2]^=1;CHECK(pt_request(7)==PT_STALE&&physical_calls==untouched);pt_token(8);CHECK(pt_request(7)==PT_OK);
 CHECK(import_start(6)==PT_OK);import_receive();pt_workspace()->plan[512]^=1;pt_token(5);CHECK(pt_request(7)==PT_BAD&&physical_calls==untouched);pt_token(8);CHECK(pt_request(7)==PT_OK);
 CHECK(import_start(6)==PT_OK);import_receive();approve_restore();transport_req=1;d8ls_poll();CHECK(d8ls.phase==LS_IDLE&&!migration_owner&&physical_calls==untouched);transport_req=0;
 CHECK(import_start(6)==PT_OK);import_receive();pt_token(5);CHECK(pt_request(7)==PT_OK);local_lcd_event=4;d8ls_draw();CHECK(!d8ls.shown);mc_ship_tap(B_SAVE);d8ls_poll();CHECK(d8ls.phase==LS_IDLE&&!migration_owner&&physical_calls==untouched);
 uint8_t kept_header[32];memcpy(kept_header,source_pool,32);source_pool[4]=99;uint32_t hc=d8p1_crc32(source_pool,28);for(unsigned i=0;i<4;i++)source_pool[28+i]=(uint8_t)(hc>>(8*i));
 CHECK(import_start(6)==PT_OK);import_receive();pt_token(5);CHECK(pt_request(7)==PT_BAD&&physical_calls==untouched);pt_token(8);CHECK(pt_request(7)==PT_OK);memcpy(source_pool,kept_header,32);
 /* Empty own set remains positively owned via tombstones across reboot. */
 memset(source_pool,255,sizeof source_pool);target_fresh();CHECK(import_start(0)==PT_OK);import_receive();approve_restore();pt_token(7);CHECK(pt_request(7)==PT_OK);verify_restored(0);d8pool_index empty;CHECK(!d8pool_inventory(&seed,&empty)&&empty.anchors==15&&!empty.present);CHECK(project_native_boot_readonly(0)==-D8POOL_EMPTY&&project_native_status()==1);
 /* Repeated empty restore is an exact no-write operation, while adopting live. */
 pt_token(8);CHECK(pt_request(7)==PT_OK);retained_reboot();unsigned empty_calls=physical_calls;
 CHECK(import_start(0)==PT_OK);import_receive();approve_restore();pt_token(7);CHECK(pt_request(7)==PT_OK);verify_restored(0);CHECK(physical_calls==empty_calls);
 pt_token(8);CHECK(pt_request(7)==PT_OK);
 printf("Native own project transfer: %u checks %u failures\n",checks,failures);return failures!=0;
}

#endif /* D8_PROJECT_TRANSFER_NO_MAIN */
