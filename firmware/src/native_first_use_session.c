/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual first-boot panel flow. No loader, OTA area, external migration or
 * host packet grants. Formatting is an explicit destructive first-use action. */
#if D8_FIRST_USE_SESSION
#ifndef D8_FIRST_USE_WRITE_ENABLED
#define D8_FIRST_USE_WRITE_ENABLED 0
#endif
#ifndef D8_FI_ERASE
#define D8_FI_ERASE(a,t) FL_FAR(fl_erase4k_ram)(a,t)
#define D8_FI_PROGRAM(a,p,n,t) FL_FAR(fl_prog_ram)(a,p,n,t)
#endif
static int fi_boot_stop(void *c){(void)c;return flash_ok!=0;}
static int fi_noerase(void*c,uint32_t a){(void)c;(void)a;return -1;}
static int fi_noprogram(void*c,uint32_t a,const void*p,uint32_t n){(void)c;(void)a;(void)p;(void)n;return -1;}
static uint32_t fi_headers(void)
{
 uint8_t h[32];uint32_t crc=~0u;
 for(unsigned b=0;b<5;b++) {
  if(st_read(d8pool_mapped_address(b),h,sizeof h))return 0;
  crc=edc_crc_step(crc,h,sizeof h);
 }
 return crc^~0u;
}
static int d8fi_boot_classify(void)
{
 d8pool_physical io={NULL,df_read,fi_noerase,fi_noprogram,fi_boot_stop};
 memset(&d8fi,0,sizeof d8fi);
 int rc=d8fu_probe(&io);
 if(rc==D8FU_OWN)return 0; /* own/future/corrupt committed payload is preserved */
 d8fi.classification=(uint8_t)rc;
 d8fi.phase=rc==D8FU_FIRST_USE?FI_REQUIRED:rc==D8FU_UNCERTAIN?FI_UNCERTAIN:FI_REFUSED;
 d8fi.header_crc=fi_headers();
 return 1; /* no inherited project/settings/preset/sample import */
}
static int fi_quiet(void)
{
 return flash_ok&&!cv_cpu_active&&!transport_req&&!transport_busy()&&!seq_counting()&&
  !d8mc_edit.locked&&!d8mc_edit.busy&&!d8mc_edit.writer&&!d8ls_blocked()&&
  df_automatic_quiet()&&mc_inputs_quiet()&&!usb.ota_req&&!usb.uboot_req;
}
static int fi_lifetime(void *c)
{
 (void)c;return !d8fi.revoked&&d8fi.phase==FI_EXECUTE&&
  main_migration_workspace(d8fi.generation)&&
  d8_capture_change.epoch==d8fi.epoch+d8fi.own&&
  usb.resets==d8fi.usb&&usb.config==d8fi.usb_config&&
  (uint32_t)(fm1_ms-d8fi.started)<120000u&&fi_quiet();
}
static int fi_read(void*c,uint32_t a,void*p,uint32_t n)
{(void)c;if(!fi_lifetime(NULL))return -1;int rc=st_read(a,p,n);return fi_lifetime(NULL)?rc:-1;}
static int fi_ticket(unsigned program,uint32_t a,const void*p,uint32_t n)
{
 if(!fi_lifetime(NULL)||d8fi.busy||!FL_STORE_OK(a,n)||
    d8_capture_change.epoch==UINT32_MAX||d8fi.own==UINT32_MAX)return 0;
 if(d8fi.format.phase==1)return !program&&n==4096&&a==d8fu_address(d8fi.format.sector);
 if(d8fi.format.phase!=3)return 0;
 uint8_t *wire=main_workspace.migration.stage.wire;
 if(d8p1_crc32(wire,d8fi.length)!=d8fi.wire_crc)return 0;
 uint32_t base=d8pool_mapped_address(0);
 if(!program)return d8fi.commit_phase<2&&n==4096&&a==base+4096*d8fi.commit_phase;
 if(d8fi.commit_phase!=2||!p)return 0;
 if(a==base&&n==32) {
  const uint8_t*h=p;
  return d8fi.offset==256+d8fi.length&&mc_get32(h)==D8POOL_MAGIC&&h[4]==1&&!h[5]&&!h[6]&&!h[7]&&
   !mc_get32(h+8)&&mc_get32(h+12)==1&&mc_get32(h+16)==d8fi.length&&mc_get32(h+20)==d8fi.wire_crc&&
   mc_get32(h+24)==~0u&&mc_get32(h+28)==d8p1_crc32(h,28);
 }
 uint32_t expected=d8fi.length-(d8fi.offset-256);if(expected>256)expected=256;
 return expected&&a==base+d8fi.offset&&n==expected&&p==wire+d8fi.offset-256;
}
static int fi_driver(unsigned program,uint32_t a,const void*p,uint32_t n)
{
 uint32_t flags=irq_save(),took=0;int rc=-10;
 if(D8_FIRST_USE_WRITE_ENABLED&&fi_ticket(program,a,p,n)) {
  d8fi.busy=1;d8fi.attempted=1;d8_capture_store_changed();d8fi.own++;
  rc=program?D8_FI_PROGRAM(a,p,n,&took):D8_FI_ERASE(a,&took);
  if(!fi_lifetime(NULL))rc=-1;
  if(!rc&&d8fi.format.phase==3) {
   if(!program)d8fi.commit_phase++;
   else if(a==d8pool_mapped_address(0))d8fi.commit_phase=3;
   else d8fi.offset+=n;
  }
  d8fi.busy=0;
 }
 irq_restore(flags);return rc;
}
static int fi_erase(void*c,uint32_t a){(void)c;return fi_driver(0,a,NULL,4096);}
static int fi_program(void*c,uint32_t a,const void*p,uint32_t n){(void)c;return fi_driver(1,a,p,n);}
static void d8fi_input(uint32_t pressed,uint32_t notes)
{
 if(d8fi.phase==FI_EXECUTE||d8fi.phase==FI_RELEASE) {
  if(notes||pressed)d8fi_revoke();return;
 }
 if(d8fi.phase!=FI_REQUIRED&&d8fi.phase!=FI_UNCERTAIN)return;
 uint32_t save=1u<<panel.btn[B_SAVE];
 if(notes||(pressed&~save)){d8fi.shown=0;return;}
 for(unsigned i=0;i<7;i++)if(D8_MC_ENCODER_VALUE(i)){d8fi.shown=0;return;}
 unsigned phase=d8fi.phase;uint32_t headers=fi_headers();
 if(d8fi.phase==phase&&pressed==save&&fm1_in.buttons==save&&d8fi.shown&&
    d8_capture_change.epoch==d8fi.epoch&&usb.resets==d8fi.usb&&usb.config==d8fi.usb_config&&
    !usb.ota_req&&!usb.uboot_req&&headers==d8fi.header_crc&&fm1_lcd_timeouts==d8fi.lcd_faults) {
  d8fi.phase=FI_RELEASE;d8fi.shown=0;ui.force=1;
 }
}
static void d8fi_draw(void)
{
 if(d8fi.phase==FI_EXECUTE)return; /* arena remains exclusively staged */
 unsigned phase=d8fi.phase;uint32_t epoch=d8_capture_change.epoch,faults=fm1_lcd_timeouts,resets=usb.resets;unsigned config=usb.config;
 d8fi.shown=0;style_apply();lcd_fill(0,0,240,240,T_BG);
 draw_text_box(12,32,216,&AF_M,"DABBL8 FIRST USE",T_THEME,1);
 draw_text_box(12,76,216,&AF_S,"ERASE ALL USER DATA",T_INK,1);
 draw_text_box(12,110,216,&AF_S,"Projects, samples, presets, settings",T_INK,1);
 draw_text_box(12,150,216,&AF_S,phase==FI_UNCERTAIN?"Damaged or unfinished storage":phase==FI_REFUSED||phase==FI_FAILED?"STORAGE ERROR - no automatic reset":"Fresh Dabbl8 storage",T_ACCENT,1);
 draw_text_box(12,190,216,&AF_M,"PRESS AND RELEASE SAVE",T_ACCENT,1);
 lcd_sync();uint32_t flags=irq_save();
 if((phase==FI_REQUIRED||phase==FI_UNCERTAIN)&&d8fi.phase==phase&&
    epoch==d8_capture_change.epoch&&faults==fm1_lcd_timeouts&&resets==usb.resets&&config==usb.config&&!migration_owner&&fi_quiet()) {
  d8fi.lcd_faults=faults;d8fi.epoch=epoch;d8fi.usb=usb.resets;d8fi.usb_config=usb.config;d8fi.started=fm1_ms;d8fi.revoked=0;d8fi.shown=1;
 }
 irq_restore(flags);
}
static void fi_fail(void)
{
 if(d8fi.generation)main_migration_end(d8fi.generation);
 d8fi.generation=0;d8fi.phase=FI_FAILED;d8fi.shown=0;project_native.mode=2;project_native.ready=0;
 d8ls_offline();ui.force=1;ui_message("INITIALIZATION STOPPED");
}
static int fi_stage_default(void)
{
 /* Only fresh factory state is captured. No foreign user cache was imported. */
 uint32_t flags=irq_save();
 chain_defaults(&chain_config);memset(&motion,0,sizeof motion);memset(motion_active,0,sizeof motion_active);motion_base_valid=0;
 for(unsigned g=0;g<G_COUNT;g++)song.g[g]=GP[g].def;
 fm6_init();undo_depth++;
 for(unsigned t=0;t<NTRK;t++){track_startup(&trk[t],t);track_defaults_steps(&trk[t]);}
 undo_depth--;song.sel=0;memset(&d8p1_runtime_cache,0,sizeof d8p1_runtime_cache);
 str_cpy(proj_name,"DABBL8",sizeof proj_name);proj_cur=PROJ_NO_SLOT;
 irq_restore(flags);
 size_t n=0;d8p1_stage_workspace *w=main_d8p1_workspace_try();
 if(!w||d8p1_capture_runtime(w->wire,sizeof w->wire,&n)||
    !d8p1_project_decode(&w->state,w->wire,n,1))return 0;
 uint32_t staged_crc=d8p1_crc32(w->wire,n);
#ifdef D8_FI_STAGE_HOOK
 D8_FI_STAGE_HOOK();
#endif
 if(main_migration_begin(&d8fi.generation))return 0;
 uint32_t headers=fi_headers();
 /* Keep the exact display-bound identity; acquiring/staging cannot refresh
  * consent after a callback changed epoch, USB, wire, or arena ownership. */
 if(!main_migration_workspace(d8fi.generation)||d8fi.revoked||
    d8_capture_change.epoch!=d8fi.epoch||usb.resets!=d8fi.usb||usb.config!=d8fi.usb_config||
    fm1_lcd_timeouts!=d8fi.lcd_faults||headers!=d8fi.header_crc||!fi_quiet()||
    staged_crc!=d8p1_crc32(w->wire,n))return 0;
 d8fi.length=(uint32_t)n;d8fi.wire_crc=staged_crc;
 d8fi.own=0;d8fi.offset=256;d8fi.commit_phase=0;d8fi.phase=FI_EXECUTE;
 d8pool_physical io={NULL,fi_read,fi_erase,fi_program,fi_lifetime};
 return d8fu_begin(&d8fi.format,&io,w->wire,n,d8fi.classification==D8FU_UNCERTAIN)==D8FU_MORE;
}
static void d8fi_poll(void)
{
 if(d8fi.phase==FI_RELEASE) {
  if(d8fi.revoked||usb.resets!=d8fi.usb||usb.config!=d8fi.usb_config||
     d8_capture_change.epoch!=d8fi.epoch||fi_headers()!=d8fi.header_crc||fm1_lcd_timeouts!=d8fi.lcd_faults){fi_fail();return;}
  if(!fi_quiet())return;
  if(!fi_stage_default()){fi_fail();return;}
 }
 if(d8fi.phase!=FI_EXECUTE)return;
 int rc=d8fu_step(&d8fi.format);
 if(rc==D8FU_MORE)return;
 if(rc!=D8FU_COMPLETE||d8fi.commit_phase!=3){fi_fail();return;}
 main_migration_end(d8fi.generation);d8fi.generation=0;
 /* Ancillary state was never imported. Re-scan the now-empty native-owned
  * instrument areas; normal initial settings/panel defaults were already used. */
#if FELUCCA_FLASH
 for(unsigned k=0;k<SMP_USER_SLOTS;k++)smp_user_scan(k);
 up_boot();memset(&persist_saved,0,sizeof persist_saved);persist_pending=0;
#endif
 d8fi.phase=FI_IDLE;d8ls_reset();d8ls.intent=1;project_native.mode=1;project_native.ready=0;
 if(project_native_bind_flash(1)||d8p1_autosave_local_begin()){fi_fail();return;}
 proj_cur=0;ui.force=1;ui_message("DABBL8 READY");
}
#endif
