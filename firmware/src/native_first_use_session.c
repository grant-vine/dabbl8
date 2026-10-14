/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual first-boot panel flow. No loader, OTA area, external migration or
 * host packet grants. Formatting is an explicit destructive first-use action. */
#if D8_FIRST_USE_SESSION
#ifndef D8_FIRST_USE_WRITE_ENABLED
#define D8_FIRST_USE_WRITE_ENABLED 1
#endif
#ifndef D8_OWNER_INITIALIZE
#define D8_OWNER_INITIALIZE 0
#endif
#if D8_OWNER_INITIALIZE && !DABBL8_TIMING_DIAGNOSTIC
#error Owner-authorised initialization is restricted to private diagnostic builds
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
static int fi_owner_usb_ready(void)
{ return usb.up&&usb.config==1&&!usb.suspended; }
static uint32_t fi_guards(unsigned executing)
{
 return (d8fi.revoked?FI_G_REVOKED:0u)|(executing&&d8fi.phase!=FI_EXECUTE?FI_G_PHASE:0u)|
  ((executing||d8fi.generation)&&!main_migration_workspace(d8fi.generation)?FI_G_WORKSPACE:0u)|
  (d8_capture_change.epoch!=d8fi.epoch+d8fi.own?FI_G_EPOCH:0u)|
  (usb.resets!=d8fi.usb?FI_G_RESET:0u)|(usb.config!=d8fi.usb_config?FI_G_CONFIG:0u)|
  ((uint32_t)(fm1_ms-d8fi.started)>=120000u?FI_G_DEADLINE:0u)|(!fi_quiet()?FI_G_QUIET:0u)|
  (usb.up!=d8fi.usb_up||usb.suspended?FI_G_USB_STATE:0u);
}
static int fi_lifetime(void *c)
{ (void)c;return !fi_guards(1); }
#if DABBL8_TIMING_DIAGNOSTIC
static void d8fi_diag_boot(uint32_t reset,uint32_t source,uint32_t wdt)
{
 if(d8fi_diag.magic!=FI_DIAG_MAGIC){memset((void *)&d8fi_diag,0,sizeof d8fi_diag);d8fi_diag.magic=FI_DIAG_MAGIC;}
 d8fi_diag.boot++;d8fi_diag.reset_reason=(uint8_t)reset;
 d8fi_diag.reset_src=source;d8fi_diag.wdt=(uint8_t)wdt;
}
static void fi_diag_current(void)
{
 d8fi_diag.current_epoch=d8_capture_change.epoch;d8fi_diag.current_resets=usb.resets;
 d8fi_diag.current_config=usb.config;d8fi_diag.current_usb_state=usb.up|(usb.suspended<<1);
 d8fi_diag.elapsed=fm1_ms-d8fi.started;d8fi_diag.guards|=fi_guards(d8fi.phase==FI_EXECUTE);
}
static void fi_diag_before(unsigned operation,uint32_t address)
{
 d8fi_diag.record_boot=d8fi_diag.boot;d8fi_diag.operation=(uint8_t)operation;d8fi_diag.address=address;
 d8fi_diag.phase=d8fi.phase;d8fi_diag.format_phase=(uint8_t)d8fi.format.phase;
 d8fi_diag.sector=(uint8_t)d8fi.format.sector;d8fi_diag.verify_offset=(uint16_t)d8fi.format.verify_offset;
 d8fi_diag.foundation=d8fi.foundation;d8fi_diag.generation=d8fi.generation;
 d8fi_diag.epoch=d8fi.epoch;d8fi_diag.own=d8fi.own;d8fi_diag.resets=d8fi.usb;
 d8fi_diag.config=d8fi.usb_config;d8fi_diag.usb_state=d8fi.usb_up;
 d8fi_diag.raw_rc=d8fi_diag.result=FI_DIAG_PENDING;d8fi_diag.guards=0;fi_diag_current();
 d8fi_diag.status=operation==FI_OP_READ||operation==FI_OP_ERASE||operation==FI_OP_PROGRAM?FI_DIAG_INFLIGHT:FI_DIAG_ACTIVE;
 __asm__ volatile("" ::: "memory");
}
static void fi_diag_after(int raw)
{ d8fi_diag.raw_rc=raw;fi_diag_current();d8fi_diag.status=FI_DIAG_ACTIVE; }
static void fi_diag_result(int result)
{
 d8fi_diag.result=result;fi_diag_current();
 if(result==D8FU_IO&&!d8fi_diag.raw_rc&&!d8fi_diag.guards&&d8fi_diag.format_phase==2)
  d8fi_diag.guards|=FI_G_VERIFY;
}
#else
#define fi_diag_before(o,a) ((void)0)
#define fi_diag_after(r) ((void)0)
#define fi_diag_result(r) ((void)0)
#endif
#ifndef D8_FI_READ
#define D8_FI_READ(a,p,n) st_read(a,p,n)
#endif
static int fi_read(void*c,uint32_t a,void*p,uint32_t n)
{
 (void)c;fi_diag_before(FI_OP_READ,a);if(!fi_lifetime(NULL))return -1;
 int rc=D8_FI_READ(a,p,n);fi_diag_after(rc);return fi_lifetime(NULL)?rc:-1;
}
static unsigned fi_foundation_object(void)
{
 static const uint8_t objects[4]={OBJ_UPRESET0,OBJ_UPRESET0+1,OBJ_UPFM6,OBJ_SETTINGS};
 return d8fi.foundation<4?objects[d8fi.foundation]:OBJ_COUNT;
}
static const void *fi_foundation_data(uint32_t *n)
{
 unsigned obj=fi_foundation_object();
 if(obj==OBJ_UPRESET0||obj==OBJ_UPRESET0+1){*n=sizeof(up_bank_t);return &up_bank[obj-OBJ_UPRESET0];}
 if(obj==OBJ_UPFM6){*n=sizeof upf;return &upf;}
 if(obj==OBJ_SETTINGS){*n=sizeof persist_saved;return &persist_saved;}
 *n=0;return NULL;
}
static int fi_foundation_ticket(unsigned program,uint32_t a,const void*p,uint32_t n)
{
 unsigned obj=fi_foundation_object();uint32_t length=0;const void *data=fi_foundation_data(&length);
 if(!data||length!=d8fi.length||
    d8fi.wire_crc!=d8p1_crc32(main_workspace.migration.stage.wire,d8fi.format.length)||
    d8fi.ancillary_crc!=st_crc32(data,length)||
    d8fi.ancillary_crc!=st_crc32(st_buf,length))return 0;
 uint32_t base=st_sector(obj,0);
 if(!program)return d8fi.commit_phase==0&&a==base&&n==4096;
 if(d8fi.commit_phase!=1||!p)return 0;
 if(a==base&&n==32) {
  st_hdr_t h;memcpy(&h,p,sizeof h);
  return d8fi.offset==length&&h.magic==ST_MAGIC&&h.type==obj&&!h.slot&&h.seq==1&&h.len==length&&
   h.crc==d8fi.ancillary_crc&&h.rsv[0]==~0u&&h.rsv[1]==~0u&&h.hcrc==st_crc32(&h,sizeof h-4u);
 }
 uint32_t expected=length-d8fi.offset;if(expected>256)expected=256;
 return expected&&a==base+ST_PAYLOAD_OFF+d8fi.offset&&n==expected&&p==st_buf+d8fi.offset;
}
static int fi_ticket(unsigned program,uint32_t a,const void*p,uint32_t n)
{
 if(!fi_lifetime(NULL)||d8fi.busy||!FL_STORE_OK(a,n)||
    d8_capture_change.epoch==UINT32_MAX||d8fi.own==UINT32_MAX)return 0;
 if(d8fi.format.phase==1)return !program&&n==4096&&a==d8fu_address(d8fi.format.sector);
 if(d8fi.format.phase!=3)return 0;
 if(d8fi.foundation<4)return fi_foundation_ticket(program,a,p,n);
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
 fi_diag_before(program?FI_OP_PROGRAM:FI_OP_ERASE,a);
 if(D8_FIRST_USE_WRITE_ENABLED&&fi_ticket(program,a,p,n)) {
  d8fi.busy=1;d8fi.attempted=1;d8_capture_store_changed();d8fi.own++;
#if DABBL8_TIMING_DIAGNOSTIC
  d8fi_diag.own=d8fi.own;fi_diag_current();__asm__ volatile("" ::: "memory");
#endif
  rc=program?D8_FI_PROGRAM(a,p,n,&took):D8_FI_ERASE(a,&took);
  fi_diag_after(rc);if(!fi_lifetime(NULL))rc=-1;
  if(!rc&&d8fi.format.phase==3&&d8fi.foundation<4) {
   if(!program)d8fi.commit_phase=1;
   else if(a==st_sector(fi_foundation_object(),0))d8fi.commit_phase=2;
   else d8fi.offset+=n;
  } else if(!rc&&d8fi.format.phase==3) {
   if(!program)d8fi.commit_phase++;
   else if(a==d8pool_mapped_address(0))d8fi.commit_phase=3;
   else d8fi.offset+=n;
  }
  d8fi.busy=0;
 }
#if DABBL8_TIMING_DIAGNOSTIC
 else {d8fi_diag.guards|=FI_G_TICKET;fi_diag_current();d8fi_diag.status=FI_DIAG_ACTIVE;}
#endif
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
    !d8fi.revoked&&!migration_owner&&!usb.ota_req&&!usb.uboot_req&&usb.up==d8fi.usb_up&&!usb.suspended&&
    (uint32_t)(fm1_ms-d8fi.started)<120000u&&headers==d8fi.header_crc&&fm1_lcd_timeouts==d8fi.lcd_faults) {
  d8fi.phase=FI_RELEASE;d8fi.shown=0;ui.force=1;
 }
}
static void d8fi_draw(void)
{
 if(d8fi.phase==FI_EXECUTE)return; /* arena remains exclusively staged */
 unsigned phase=d8fi.phase,shown=d8fi.shown;uint32_t epoch=d8_capture_change.epoch,faults=fm1_lcd_timeouts,resets=usb.resets;unsigned config=usb.config,up=usb.up;
 d8fi.shown=0;style_apply();lcd_fill(0,0,240,240,T_BG);
 draw_text_box(12,32,216,&AF_M,"DABBL8 FIRST USE",T_THEME,1);
 draw_text_box(12,76,216,&AF_S,"ERASE ALL USER DATA",T_TEXT,1);
 draw_text_box(12,110,216,&AF_S,"Projects, samples, presets, settings",T_TEXT,1);
 draw_text_box(12,150,216,&AF_S,phase==FI_UNCERTAIN?"Damaged or unfinished storage":phase==FI_REFUSED||phase==FI_FAILED?"STORAGE ERROR - no automatic reset":"Fresh Dabbl8 storage",T_ACCENT,1);
 draw_text_box(12,190,216,&AF_M,phase==FI_FAILED||phase==FI_REFUSED?"REBOOT FOR RECOVERY":
   D8_OWNER_INITIALIZE&&phase==FI_REQUIRED?(fi_owner_usb_ready()?"OWNER-AUTHORIZED RESET":"WAITING FOR USB"):"PRESS AND RELEASE SAVE",T_ACCENT,1);
 lcd_sync();uint32_t flags=irq_save();
 if((phase==FI_REQUIRED||phase==FI_UNCERTAIN)&&d8fi.phase==phase&&
    epoch==d8_capture_change.epoch&&faults==fm1_lcd_timeouts&&resets==usb.resets&&config==usb.config&&up==usb.up&&!migration_owner&&!usb.suspended&&
    (!D8_OWNER_INITIALIZE||phase!=FI_REQUIRED||fi_owner_usb_ready())) {
  if(fi_quiet()) {
   d8fi.lcd_faults=faults;d8fi.epoch=epoch;d8fi.usb=usb.resets;d8fi.usb_config=usb.config;d8fi.usb_up=usb.up;d8fi.started=fm1_ms;d8fi.revoked=0;d8fi.shown=1;
  } else {
   /* SAVE may arrive while this same disclosure is redrawn. Retain only the
    * previous visible proposal; a held press never creates or refreshes one. */
   unsigned save_only=fm1_in.buttons==(1u<<panel.btn[B_SAVE])&&!fm1_in.notes;
   for(unsigned i=0;i<7;i++)if(D8_MC_ENCODER_VALUE(i))save_only=0;
   d8fi.shown=shown&&save_only&&!d8fi.revoked&&flash_ok&&!cv_cpu_active&&
    !transport_req&&!transport_busy()&&!seq_counting()&&!d8mc_edit.locked&&!d8mc_edit.busy&&!d8mc_edit.writer&&
    !d8ls_blocked()&&!usb.ota_req&&!usb.uboot_req&&epoch==d8fi.epoch&&
    faults==d8fi.lcd_faults&&resets==d8fi.usb&&config==d8fi.usb_config&&usb.up==d8fi.usb_up&&
    (uint32_t)(fm1_ms-d8fi.started)<120000u&&fi_headers()==d8fi.header_crc;
  }
 }
 irq_restore(flags);
}
static void fi_fail(void)
{
#if DABBL8_TIMING_DIAGNOSTIC
 fi_diag_current();d8fi_diag.status=FI_DIAG_FAILED;
#endif
 if(d8fi.generation)main_migration_end(d8fi.generation);
 d8fi.generation=0;d8fi.phase=FI_FAILED;d8fi.shown=0;project_native.mode=2;project_native.ready=0;
 d8ls_offline();ui.force=1;ui_message("INITIALIZATION STOPPED");
}
static void fi_factory_ancillary(void)
{
 /* Construct factory data; never export possibly inherited/current caches as
  * defaults. The existing globals are reset only in the disclosed formatter. */
 memset(up_bank,0,sizeof up_bank);
 for(unsigned b=0;b<2;b++){up_bank[b].magic=UP_BANK_MAGIC;up_bank[b].rsize=sizeof(up_rec_t);up_bank[b].nslot=UP_PER_BANK;}
 upf_empty();
 memset(&persist_saved,0,sizeof persist_saved);persist_saved.magic=PERSIST_MAGIC;
 persist_saved.palette=palette_to_stored(UI_DEFAULT_INDEX);persist_saved.panel=PANEL_DEFAULT;
 settings_import(&persist_saved,sizeof persist_saved);settings_init();panel_init();persist_pending=0;
}
static int fi_foundation_erase(uint32_t a){return fi_driver(0,a,NULL,4096);}
static int fi_foundation_program(uint32_t a,const void*p,uint32_t n){return fi_driver(1,a,p,n);}
static int fi_foundation_step(void)
{
 uint32_t n=0;const void *data=fi_foundation_data(&n);unsigned obj=fi_foundation_object();
 if(!data||!fi_lifetime(NULL)||d8fi.wire_crc!=d8p1_crc32(main_workspace.migration.stage.wire,d8fi.format.length))return 0;
 d8fi.length=n;d8fi.offset=0;d8fi.commit_phase=0;d8fi.ancillary_crc=st_crc32(data,n);
 /* Copy0 and sequence1 only, after every approved sector was verified erased.
  * The native marker remains absent until all four commit/readback checks pass. */
 int rc=st_save_commit(obj,data,n,0,fi_foundation_erase,fi_foundation_program);
 fi_diag_result(rc);if(rc||!fi_lifetime(NULL)||d8fi.commit_phase!=2)return 0;
 d8fi.foundation++;d8fi.offset=256;d8fi.commit_phase=0;d8fi.length=(uint32_t)d8fi.format.length;
 return 1;
}
static int fi_stage_default(void)
{
 /* Only fresh factory state is captured. No foreign user cache was imported. */
 uint32_t flags=irq_save();
 fi_factory_ancillary();
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
    d8_capture_change.epoch!=d8fi.epoch||usb.resets!=d8fi.usb||usb.config!=d8fi.usb_config||usb.up!=d8fi.usb_up||
    fm1_lcd_timeouts!=d8fi.lcd_faults||headers!=d8fi.header_crc||!fi_quiet()||
    staged_crc!=d8p1_crc32(w->wire,n))return 0;
 d8fi.length=(uint32_t)n;d8fi.wire_crc=staged_crc;
 d8fi.own=0;d8fi.foundation=0;d8fi.offset=256;d8fi.commit_phase=0;d8fi.phase=FI_EXECUTE;
 d8pool_physical io={NULL,fi_read,fi_erase,fi_program,fi_lifetime};
 int rc=d8fu_begin(&d8fi.format,&io,w->wire,n,d8fi.classification==D8FU_UNCERTAIN);
 fi_diag_result(rc);return rc==D8FU_MORE;
}
static void d8fi_poll(void)
{
#if D8_OWNER_INITIALIZE
 /* Private empty-device install authorised by the owner. Preserve the displayed
  * snapshot, existing quiet/lifetime tickets and exact user-state map. Never
  * automatically reset recognized or uncertain Dabbl8 storage. */
 if(d8fi.phase==FI_REQUIRED&&d8fi.classification==D8FU_FIRST_USE&&
    d8fi.shown&&!d8fi.revoked&&fi_quiet()&&fi_owner_usb_ready()) {
  d8fi.phase=FI_RELEASE;d8fi.shown=0;ui.force=1;
 } else if(d8fi.phase==FI_REQUIRED&&!d8fi.shown&&fi_owner_usb_ready())ui.force=1;
#endif
 if(d8fi.phase==FI_RELEASE) {
  fi_diag_before(FI_OP_STAGE,0);
  if(d8fi.revoked||usb.resets!=d8fi.usb||usb.config!=d8fi.usb_config||usb.up!=d8fi.usb_up||usb.suspended||
     d8_capture_change.epoch!=d8fi.epoch||fi_headers()!=d8fi.header_crc||fm1_lcd_timeouts!=d8fi.lcd_faults){fi_diag_result(D8FU_BUSY);fi_fail();return;}
  if(!fi_quiet())return;
  if(!fi_stage_default()){
#if DABBL8_TIMING_DIAGNOSTIC
   d8fi_diag.guards|=FI_G_STAGE;
   if(d8fi_diag.result==FI_DIAG_PENDING)fi_diag_result(D8FU_BAD);
#endif
   fi_fail();return;
  }
 }
 if(d8fi.phase!=FI_EXECUTE)return;
 if(d8fi.format.phase==3&&d8fi.foundation<4) {
  fi_diag_before(FI_OP_FOUNDATION,st_sector(fi_foundation_object(),0));
  if(!fi_foundation_step()){
#if DABBL8_TIMING_DIAGNOSTIC
   if(d8fi_diag.result==FI_DIAG_PENDING)fi_diag_result(D8FU_BAD);
#endif
   fi_fail();
  }
  return;
 }
 fi_diag_before(FI_OP_STEP,d8fu_address(d8fi.format.sector)+d8fi.format.verify_offset);
 int rc=d8fu_step(&d8fi.format);fi_diag_result(rc);
 if(rc==D8FU_MORE)return;
 if(rc!=D8FU_COMPLETE||d8fi.commit_phase!=3){fi_fail();return;}
 main_migration_end(d8fi.generation);d8fi.generation=0;
 /* Ancillary state was never imported. Re-scan empty samples and canonical initialized
  * banks; factory settings/panel defaults were committed before the marker. */
#if FELUCCA_FLASH
 for(unsigned k=0;k<SMP_USER_SLOTS;k++)smp_user_scan(k);
 up_boot();persist_pending=0;
#endif
 d8fi.phase=FI_IDLE;d8ls_reset();d8ls.intent=1;project_native.mode=1;project_native.ready=0;
 fi_diag_before(FI_OP_READY,0);
 int ready=project_native_bind_flash(1);if(!ready)ready=d8p1_autosave_local_begin();
 fi_diag_result(ready);if(ready){fi_fail();return;}
#if DABBL8_TIMING_DIAGNOSTIC
 d8fi_diag.status=FI_DIAG_COMPLETE;d8fi_diag.guards=0;
#endif
 proj_cur=0;ui.force=1;ui_message("DABBL8 READY");
}
#endif
