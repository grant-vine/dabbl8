/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
static unsigned fi_panel_fills;
static void fi_fill_test_hook(uint32_t,uint32_t,uint32_t,uint32_t);
#define UI_FILL_HOOK(x,y,w,h) fi_fill_test_hook(x,y,w,h)
static void fi_lcd_test_hook(void);
static void fi_stage_test_hook(void);
#define D8_FI_STAGE_HOOK fi_stage_test_hook
#define D8_LOCAL_FIRST_USE_LCD_HOOK fi_lcd_test_hook
#define D8_PRESET_SESSION 1
#define D8_UP_ENCODER_VALUE(i) host_enc[i]
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
#define D8_UP_ERASE(a,t) fi_test_erase(a,t)
#define D8_UP_PROGRAM(a,p,n,t) fi_test_program(a,p,n,t)
#include "../firmware/src/native_preset_session.c"
#include "../firmware/src/native_settings_session.c"
#define D8_FI_ERASE(a,t) fi_test_erase(a,t)
#define D8_FI_PROGRAM(a,p,n,t) fi_test_program(a,p,n,t)
#include "../firmware/src/native_first_use.c"
#ifdef D8_FIRST_USE_COMPLETION_TEST
#undef FELUCCA_FLASH
#define FELUCCA_FLASH 1
#endif
#include "../firmware/src/native_first_use_session.c"
#ifdef D8_FIRST_USE_COMPLETION_TEST
#undef FELUCCA_FLASH
#define FELUCCA_FLASH 0
#endif
static unsigned fi_lcd_event,fi_stage_event;
static void fi_fill_test_hook(uint32_t x,uint32_t y,uint32_t w,uint32_t h)
{if(!x&&!y&&w==240&&h==240)fi_panel_fills++;}
static void fi_stage_test_hook(void){if(fi_stage_event==1){fi_stage_event=0;d8_capture_store_changed();}else if(fi_stage_event==2){fi_stage_event=0;main_workspace.migration.stage.wire[200]^=1;}}
static void fi_lcd_test_hook(void)
{
 unsigned event=fi_lcd_event;fi_lcd_event=0;
 if(event==1)fm1_lcd_timeouts++;
 else if(event==2)usb.resets++;
 else if(event==3)host_pressed=fm1_in.buttons=1u<<panel.btn[B_SAVE];
}
static void fi_consent_fresh(void)
{
 if(migration_owner)main_migration_end(migration_owner);
 cold();project_native_reset();native_as.ready=0;d8ls_reset();
 memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);
 memset(&d8_capture_change,0,sizeof d8_capture_change);
 usb.up=1;usb.suspended=0;usb.config=1;usb.ota_req=usb.uboot_req=0;flash_ok=1;
 memset(nor,0xa5,sizeof nor);reset();physical_calls=0;host_pressed=host_notes=0;
 fm1_in.buttons=fm1_in.notes=0;fm1_ms=100;fi_lcd_event=0;
 memset(host_enc,0,sizeof host_enc);memset(host_enc_late,0,sizeof host_enc_late);
 int32_t audio[CTL*2];mix_block(audio,CTL);
 CHECK(d8fi_boot_classify()&&d8fi.phase==FI_REQUIRED);
}
static void fi_consent_race_tests(void)
{
 /* The actual queued edge reaches ui_input after SAVE arrives before or
  * during a redraw of an already visible destructive disclosure. */
 for(unsigned during=0;during<2;during++) {
  fi_consent_fresh();d8fi_draw();CHECK(d8fi.shown);
  uint32_t save=1u<<panel.btn[B_SAVE],started=d8fi.started;
  fm1_ms++;ui.force=1;if(during)fi_lcd_event=3;else host_pressed=fm1_in.buttons=save;
  d8fi_draw();
  printf("scenario=visible-SAVE-%s-redraw buttons=0x%x queued=0x%x shown=%u started=%u old_started=%u\n",
   during?"during":"before",fm1_in.buttons,host_pressed,d8fi.shown,d8fi.started,started);
  CHECK(d8fi.shown&&d8fi.started==started&&host_pressed==save);
  ui_input();CHECK(d8fi.phase==FI_RELEASE&&!physical_calls&&!writes);
  d8fi_poll();CHECK(d8fi.phase==FI_RELEASE&&!physical_calls&&!writes);
  fm1_in.buttons=0;ui_input();d8fi_poll();
  CHECK(d8fi.phase==FI_EXECUTE&&physical_calls==1);
 }
 /* Retained visibility is not a write grant. Activity that arrives after
  * disclosure still blocks the unchanged full quiet gate after SAVE release. */
 static const char *activity[]={"MIDI","voice","effect","audio-output"};
 for(unsigned busy=0;busy<4;busy++) {
  fi_consent_fresh();d8fi_draw();CHECK(d8fi.shown);memcpy(baseline,nor,sizeof nor);
  if(busy==0)mi_w=mi_r+1;else if(busy==1)trk[7].v[0].active=1;
  else if(busy==2)sb_env=1;else audio_nonzero[0]=1;
  host_pressed=fm1_in.buttons=1u<<panel.btn[B_SAVE];d8fi_draw();ui_input();
  CHECK(d8fi.phase==FI_RELEASE&&!physical_calls);
  fm1_in.buttons=0;ui_input();CHECK(!fi_quiet());
  for(unsigned poll=0;poll<3;poll++)d8fi_poll();
  printf("scenario=SAVE-release-busy-%s phase=%u calls=%u\n",activity[busy],d8fi.phase,physical_calls);
  CHECK(d8fi.phase==FI_RELEASE&&!physical_calls&&!writes&&!memcmp(nor,baseline,sizeof nor));
  if(busy==0)mi_w=mi_r;else if(busy==1)trk[7].v[0].active=0;
  else if(busy==2)sb_env=0;else audio_nonzero[0]=0;
  CHECK(fi_quiet());d8fi_poll();CHECK(d8fi.phase==FI_EXECUTE&&physical_calls==1);
 }
 /* A held boot button has never seen a quiet, fully rendered proposal. */
 fi_consent_fresh();uint32_t save=1u<<panel.btn[B_SAVE];host_pressed=fm1_in.buttons=save;
 d8fi_draw();ui_input();CHECK(!d8fi.shown&&d8fi.phase==FI_REQUIRED&&!physical_calls);
 fm1_in.buttons=0;ui_input();d8fi_poll();CHECK(d8fi.phase==FI_REQUIRED&&!physical_calls);
 d8fi_draw();CHECK(d8fi.shown);ui_input();CHECK(d8fi.phase==FI_REQUIRED&&!physical_calls);
 /* A queued tap already released before input dispatch remains unconfirmed. */
 host_pressed=save;ui_input();d8fi_poll();CHECK(d8fi.phase==FI_REQUIRED&&!physical_calls);
 /* USB suspension after the accepted edge also revokes the proposal before
  * the release can stage or erase anything in manual first-use mode. */
 fi_consent_fresh();d8fi_draw();host_pressed=fm1_in.buttons=save;ui_input();
 CHECK(d8fi.phase==FI_RELEASE);memcpy(baseline,nor,sizeof nor);
 usb.suspended=1;fm1_in.buttons=0;ui_input();d8fi_poll();
 printf("scenario=USB-suspend-after-SAVE phase=%u calls=%u\n",d8fi.phase,physical_calls);
 CHECK(d8fi.phase==FI_FAILED&&!physical_calls&&!writes&&!memcmp(nor,baseline,sizeof nor));
 /* Retaining a prior visible proposal must never refresh a changed guard. */
 static const char *guards[]={"epoch","headers","LCD","USB-reset","USB-config","USB-up",
  "USB-suspend","other-button","note","encoder","workspace","transport","OTA","bootloader","revoked","deadline"};
 for(unsigned guard=0;guard<sizeof guards/sizeof guards[0];guard++) {
  fi_consent_fresh();d8fi_draw();CHECK(d8fi.shown);memcpy(baseline,nor,sizeof nor);
  host_pressed=fm1_in.buttons=save;uint32_t lease=0;
  switch(guard) {
   case 0:d8_capture_store_changed();break;
   case 1:nor[d8pool_mapped_address(0)]^=1;memcpy(baseline,nor,sizeof nor);break;
   case 2:ui.force=1;fi_lcd_event=1;break;
   case 3:ui.force=1;fi_lcd_event=2;break;
   case 4:usb.config=0;break;
   case 5:usb.up=0;break;
   case 6:usb.suspended=1;break;
   case 7:host_pressed|=1u<<panel.btn[B_EDIT];fm1_in.buttons=host_pressed;break;
   case 8:host_notes=fm1_in.notes=1;break;
   case 9:host_enc[0]=1;break;
   case 10:CHECK(!main_migration_begin(&lease));break;
   case 11:transport_req=1;break;
   case 12:usb.ota_req=1;break;
   case 13:usb.uboot_req=1;break;
   case 14:d8fi_revoke();break;
   case 15:fm1_ms=d8fi.started+120000u;break;
  }
  d8fi_draw();ui_input();
  printf("scenario=visible-SAVE-revoked-%s shown=%u phase=%u calls=%u\n",guards[guard],d8fi.shown,d8fi.phase,physical_calls);
  CHECK(!d8fi.shown&&d8fi.phase==FI_REQUIRED&&!physical_calls&&!writes&&!memcmp(nor,baseline,sizeof nor));
  if(lease)CHECK(!main_migration_end(lease));
 }
 fi_consent_fresh();
}
static void fi_capture(const char *phase,const char *theme)
{
 const char *directory=getenv("D8_FIRST_USE_CAPTURE_DIR");if(!directory)return;
 char path[1024];int n=snprintf(path,sizeof path,"%s/%s-%s.rgb565",directory,phase,theme);
 CHECK(n>0&&(unsigned)n<sizeof path);if(n<=0||(unsigned)n>=sizeof path)return;
 FILE *f=fopen(path,"wb");CHECK(f!=NULL);
 if(f){CHECK(fwrite(host_screen,sizeof host_screen,1,f)==1);CHECK(!fclose(f));}
}
static void fi_idle_redraw_tests(void)
{
 static uint16_t pixels[240*240];
 static const unsigned palettes[]={UI_DABBL_DARK_INDEX,UI_DABBL_LIGHT_INDEX};
 static const char *themes[]={"dark","light"};
 static const unsigned phases[]={FI_REQUIRED,FI_UNCERTAIN,FI_RELEASE,FI_REFUSED,FI_FAILED};
 static const char *names[]={"required","uncertain","release","refused","failed"};
 for(unsigned theme=0;theme<2;theme++)for(unsigned state=0;state<5;state++) {
  fi_consent_fresh();settings.palette=palettes[theme];palette_set(settings.palette);
  d8fi.phase=(uint8_t)phases[state];ui_draw();
  CHECK(fi_panel_fills>0);CHECK(state<2?d8fi.shown:!d8fi.shown);
  memcpy(pixels,host_screen,sizeof pixels);fi_capture(names[state],themes[theme]);
  unsigned fills=fi_panel_fills,syncs=sync_calls,rows=host_blit_rows,consumed=dma_consumed;
  uint32_t started=d8fi.started;
  for(unsigned frame=0;frame<1024;frame++){fm1_ms++;ui_draw();}
  printf("scenario=idle-%s-%s polls=1024 fills=%u syncs=%u rows=%u DMA=%u started=%u old_started=%u pixels_equal=%u\n",
   names[state],themes[theme],fi_panel_fills-fills,sync_calls-syncs,host_blit_rows-rows,
   dma_consumed-consumed,d8fi.started,started,!memcmp(pixels,host_screen,sizeof pixels));
  CHECK(fi_panel_fills==fills&&sync_calls==syncs&&host_blit_rows==rows&&dma_consumed==consumed);
  CHECK(d8fi.started==started&&!memcmp(pixels,host_screen,sizeof pixels)&&!physical_calls&&!writes);
  for(unsigned line=0;line<2;line++) {
   unsigned visible=0,y0=line?110:76;
   for(unsigned y=y0;y<y0+AF_S.h;y++)for(unsigned x=12;x<228;x++)visible+=host_screen[y*240+x]!=swap16(T_BG);
   CHECK(visible>30);
  }
  fi_capture(names[state],theme?"light-idle":"dark-idle");
 }
 fi_consent_fresh();ui_draw();uint32_t started=d8fi.started;
 unsigned fills=fi_panel_fills;ui.force=1;fm1_ms++;ui_draw();
 CHECK(fi_panel_fills==fills+1&&d8fi.shown&&d8fi.started==started&&!ui.force);
 fills=fi_panel_fills;settings.palette=UI_DABBL_LIGHT_INDEX;palette_set(settings.palette);ui_draw();
 CHECK(fi_panel_fills==fills+1&&d8fi.shown&&d8fi.started==started);
 fills=fi_panel_fills;d8fi.phase=FI_UNCERTAIN;ui_draw();CHECK(fi_panel_fills==fills+1);
 /* A memoized screen still revalidates its grant; unchanged pixels cannot
  * silently renew a stale display snapshot on the next idle poll. */
 static const char *guards[]={"epoch","headers","LCD","USB-reset","USB-config","USB-up","USB-suspend","workspace","revoked","deadline"};
 for(unsigned guard=0;guard<sizeof guards/sizeof guards[0];guard++) {
  fi_consent_fresh();ui_draw();CHECK(d8fi.shown);started=d8fi.started;uint32_t lease=0;
  switch(guard) {
   case 0:d8_capture_store_changed();break;
   case 1:nor[d8pool_mapped_address(0)]^=1;break;
   case 2:fm1_lcd_timeouts++;break;
   case 3:usb.resets++;break;
   case 4:usb.config=0;break;
   case 5:usb.up=0;break;
   case 6:usb.suspended=1;break;
   case 7:CHECK(!main_migration_begin(&lease));break;
   case 8:d8fi_revoke();break;
   case 9:fm1_ms=started+120000u;break;
  }
  uint32_t save=1u<<panel.btn[B_SAVE];host_pressed=fm1_in.buttons=save;
  for(unsigned poll=0;poll<3;poll++)ui_draw();ui_input();
  printf("scenario=idle-invalidated-%s shown=%u started=%u old_started=%u phase=%u calls=%u\n",guards[guard],d8fi.shown,d8fi.started,started,d8fi.phase,physical_calls);
  CHECK(!d8fi.shown&&d8fi.started==started&&d8fi.phase==FI_REQUIRED&&!physical_calls&&!writes);
  if(lease)CHECK(!main_migration_end(lease));
 }
 fi_consent_fresh();
}
#ifndef D8_FIRST_USE_SESSION_NO_MAIN
int main(void)
{
 fi_idle_redraw_tests();fi_consent_race_tests();
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
