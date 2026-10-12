/* SPDX-License-Identifier: GPL-3.0-only */
static void fi_lcd_test_hook(void);
static void fi_stage_test_hook(void);
#define D8_FI_STAGE_HOOK fi_stage_test_hook
#define D8_LOCAL_FIRST_USE_LCD_HOOK fi_lcd_test_hook
#define D8_SETTINGS_SESSION_TEST 1
#define D8_FIRST_USE_SESSION 1
#define D8_FIRST_USE_WRITE_ENABLED 1
#define D8_NATIVE_LOCAL_SAVE_NO_MAIN 1
#include "native_local_save_test.c"
#ifndef D8_FI_TEST_RESULT
#define D8_FI_TEST_RESULT(rc,a,n) (rc)
#endif
static int fi_test_erase(uint32_t a,uint32_t*t){(void)t;int rc=mc_virtual_erase(a);return D8_FI_TEST_RESULT(rc,a,4096);}
static int fi_test_program(uint32_t a,const void*p,uint32_t n,uint32_t*t){(void)t;int rc=mc_virtual_program(a,p,n);return D8_FI_TEST_RESULT(rc,a,n);}
#define D8_FI_ERASE(a,t) fi_test_erase(a,t)
#define D8_FI_PROGRAM(a,p,n,t) fi_test_program(a,p,n,t)
#include "../firmware/src/native_first_use.c"
#include "../firmware/src/native_first_use_session.c"
static unsigned fi_lcd_event,fi_stage_event;
static void fi_stage_test_hook(void){if(fi_stage_event==1){fi_stage_event=0;d8_capture_store_changed();}else if(fi_stage_event==2){fi_stage_event=0;main_workspace.migration.stage.wire[200]^=1;}}
static void fi_lcd_test_hook(void){if(fi_lcd_event==1){fi_lcd_event=0;fm1_lcd_timeouts++;}else if(fi_lcd_event==2){fi_lcd_event=0;usb.resets++;}}
#ifndef D8_FIRST_USE_SESSION_NO_MAIN
int main(void)
{
 boot_seed();CHECK(!d8fi_boot_classify());CHECK(!writes&&!physical_calls);
 ls_test_initial_autosave(); /* affected existing/absent/corrupt AS baseline checks */
 /* Actual first-use boot classification, draw, press/release and ticket route. */
 cold();project_native_reset();native_as.ready=0;d8ls_reset();memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);
 memset(&d8_capture_change,0,sizeof d8_capture_change);usb.config=0;usb.ota_req=usb.uboot_req=0;flash_ok=1;
 memset(nor,0xa5,sizeof nor);reset();physical_calls=0;host_pressed=host_notes=0;fm1_in.buttons=fm1_in.notes=0;fm1_ms=0;
 int32_t audio[CTL*2];mix_block(audio,CTL);CHECK(d8fi_boot_classify()&&d8fi.phase==FI_REQUIRED);
 CHECK(fi_quiet());fi_lcd_event=1;d8fi_draw();CHECK(!d8fi.shown&&!physical_calls);
 fi_lcd_event=2;d8fi_draw();CHECK(!d8fi.shown&&!physical_calls);
 d8fi_draw();CHECK(d8fi.shown);uint32_t save=1u<<panel.btn[B_SAVE];
 host_pressed=save;fm1_in.buttons=save;ui_input();CHECK(d8fi.phase==FI_RELEASE&&!physical_calls);
 host_pressed=0;fm1_in.buttons=0;ui_input();
 unsigned steps=0;while(d8fi_blocked()&&d8fi.phase!=FI_FAILED&&steps++<2000)d8fi_poll();
 CHECK(d8fi.phase==FI_IDLE&&steps<2000&&project_native_status()==1&&native_as.ready&&d8ls.intent&&proj_cur==0);
 d8pool_index ix;CHECK(!d8pool_inventory(&seed,&ix)&&ix.present==1);
 CHECK(d8fu_probe(&(d8pool_physical){NULL,df_read,fi_noerase,fi_noprogram,fi_boot_stop})==D8FU_OWN);
 CHECK(!migration_owner);
 /* Ready own storage grants only native tickets, never generic legacy,
  * settings, sample or remote full-project APPLY writes. */
 uint8_t apply[]={0x7d,0x46,0x4c,ED_BACKUP_PUT,2,0},byte=0;
 uint32_t music=st_crc32(trk,sizeof trk);unsigned denied_calls=physical_calls;memcpy(baseline,nor,sizeof nor);
 ed_handle(apply,sizeof apply);so_r=so_w;
 CHECK(d8ls.intent&&music==st_crc32(trk,sizeof trk)&&!memcmp(nor,baseline,sizeof nor)&&physical_calls==denied_calls);
 CHECK(!d8_instrument_write_allowed());
 CHECK(st_save(OBJ_SETTINGS,&byte,1)==D8_INSTRUMENT_QUARANTINED);
 CHECK(capture_hw_prog(0x97000,&byte,1)==D8_INSTRUMENT_QUARANTINED&&capture_hw_erase(0xfc000)==D8_INSTRUMENT_QUARANTINED);
 CHECK(ed_smp_erase(0,1)==D8_INSTRUMENT_QUARANTINED&&physical_calls==denied_calls&&!memcmp(nor,baseline,sizeof nor));
 /* No AS exists yet: local SAVE admission and scheduler baseline are valid,
  * begin is read-only, then a real musical change creates shared AS. */
 unsigned prior=physical_calls;uint32_t signature=0;CHECK(!d8p1_signature_runtime(&signature));
 CHECK(native_as.saved==signature&&!native_as.record);fm1_ms+=AS_IDLE_MS+AS_GAP_MS;
 CHECK(!d8p1_autosave_session_poll()&&physical_calls==prior);
 trk[7].p[P_LEVEL]=91;go_page(GR_SLOTS);song.g[G_SLOT]=1;ui.act=4;mc_ship_tap(B_OCTUP);
 if(ui.confirm==CF_OVR_PROJ)mc_ship_tap(B_OCTUP);CHECK(name_on());mc_ship_tap(B_OCTUP);
 CHECK(d8ls.phase==LS_EXECUTE);d8ls_poll();CHECK(msg_is("SAVED")&&d8ls.intent);
 CHECK(!d8pool_inventory(&seed,&ix)&&ix.present==1);
 trk[7].p[P_LEVEL]=92;fm1_ms+=AS_POLL_MS;CHECK(!d8p1_autosave_session_poll());fm1_ms+=AS_IDLE_MS;
 CHECK(!d8p1_autosave_session_poll());CHECK(!d8pool_inventory(&seed,&ix)&&ix.present==9);
 /* Ordinary own boot preserves storage; future record in late block refuses
  * first-use classification without ever erasing earlier foreign bytes. */
 memcpy(baseline,nor,sizeof nor);prior=physical_calls;CHECK(!d8fi_boot_classify());
 CHECK(physical_calls==prior&&!memcmp(baseline,nor,sizeof nor));
 uint8_t *h=nor+d8pool_mapped_address(4);memset(h,0,32);h[0]='D';h[1]='8';h[2]='S';h[3]='P';h[4]=99;
 uint32_t crc=d8p1_crc32(h,28);for(unsigned i=0;i<4;i++)h[28+i]=(uint8_t)(crc>>(8*i));
 memset(nor+d8pool_mapped_address(0),0xa5,32);CHECK(!d8fi_boot_classify()&&!d8fi_blocked());
 CHECK(physical_calls==prior);
 /* A late staging change cannot refresh the visible proposal epoch/CRC. */
 for(unsigned event=1;event<=2;event++) {
  cold();project_native_reset();native_as.ready=0;d8ls_reset();memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);
  memset(&d8_capture_change,0,sizeof d8_capture_change);usb.config=0;usb.ota_req=usb.uboot_req=0;flash_ok=1;
  memset(nor,0xa5,sizeof nor);reset();physical_calls=0;host_pressed=host_notes=0;fm1_in.buttons=fm1_in.notes=0;
  mix_block(audio,CTL);CHECK(d8fi_boot_classify());d8fi_draw();CHECK(d8fi.shown);
  host_pressed=save;fm1_in.buttons=save;ui_input();CHECK(d8fi.phase==FI_RELEASE);
  host_pressed=0;fm1_in.buttons=0;ui_input();fi_stage_event=event;d8fi_poll();
  CHECK(d8fi.phase==FI_FAILED&&!physical_calls&&!migration_owner);
 }
 printf("First-use actual panel/ticket: %u checks, %u failures; virtual NOR only\n",checks,failures);return failures!=0;
}

#endif /* D8_FIRST_USE_SESSION_NO_MAIN */
