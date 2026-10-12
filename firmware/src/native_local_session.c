/* SPDX-License-Identifier: GPL-3.0-only */
/* Local native editing intent is renewed by real UI consent, never by boot,
 * a protocol packet, an initializer or a caller authorization Boolean. */
#if NTRK == 8 && D8_MIGRATION_SESSION
#define LS_DEADLINE_MS 120000u
static int ls_quiet(void)
{ return flash_ok&&!cv_cpu_active&&!migration_owner&&!transport_req&&!seq_counting()&&
         !d8mc_edit.locked&&!d8mc_edit.busy&&!d8mc_edit.writer&&
         df_automatic_quiet()&&mc_inputs_quiet()&&!usb.ota_req&&!usb.uboot_req; }
static int ls_lifetime(void)
{ return !d8ls.revoked&&d8ls.intent&&d8ls.phase==LS_EXECUTE&&
         usb.resets==d8ls.usb&&usb.config==d8ls.usb_config&&
         d8_capture_change.epoch==d8ls.epoch+d8ls.own&&
         (uint32_t)(fm1_ms-d8ls.started)<=LS_DEADLINE_MS&&ls_quiet(); }
static void d8ls_reset(void) { memset(&d8ls,0,sizeof d8ls); }
static void d8ls_offline(void)
{ d8ls.intent=d8ls.previous_intent=0;d8ls.shown=0;if(d8ls_blocked())d8ls.revoked=1;d8p1_autosave_session_end(); }
static int d8ls_storage_stopped(void)
{ return d8ls.writing?ls_lifetime():ls_quiet(); }
static int d8ls_read_ready(void)
{ return d8ls.intent&&project_native.mode==1&&flash_ok&&!usb.ota_req&&!usb.uboot_req; }
static void ls_fail(int attempted)
{
 if(!flash_ok||project_native.mode!=1||migration_owner||usb.ota_req||usb.uboot_req)d8ls_offline();
 d8ls.intent=d8ls.previous_intent;d8ls.phase=LS_IDLE;d8ls.shown=0;d8ls.writing=d8ls.busy=0;
 if(attempted){d8ls.intent=0;project_native.mode=2;project_native.ready=0;d8p1_autosave_session_end();}
 ui.force=1;
}
static int ls_stopped(void *c)
{ (void)c;return !d8ls.revoked&&ls_quiet(); }
/* Both expected names are already canonical13-byte buffers. Incoming C names
 * are validated and bounded before equality; the target has no strcmp libc. */
static int ls_name_equal(const char expected[13],const char *name)
{
 char canonical[13];return d8pr_name_copy(canonical,name)&&!memcmp(expected,canonical,sizeof canonical);
}
static int ls_noerase(void*c,uint32_t a){(void)c;(void)a;return -1;}
static int ls_noprogram(void*c,uint32_t a,const void*p,uint32_t n){(void)c;(void)a;(void)p;(void)n;return -1;}
static int ls_scan(d8pool_index *index,uint32_t crc[5])
{ return d8own_scan(index,crc,ls_stopped); }

static int ls_proposal(unsigned operation,unsigned object,const char *name)
{
 if(d8ls_blocked()||object>=4||operation>2||!ls_quiet()||project_native_status()!=1)return D8POOL_BUSY;
 if(d8ls.generation==UINT32_MAX||d8_capture_change.epoch==UINT32_MAX){d8ls_offline();return D8POOL_UNSUPPORTED;}
 d8ls.previous_intent=d8ls.intent;d8ls.busy=1;d8ls.revoked=0;d8ls.shown=0;d8ls.attempted=0;d8ls.writing=0;d8ls.object=(uint8_t)object;d8ls.operation=(uint8_t)operation;
 d8ls.epoch=d8_capture_change.epoch;d8ls.own=0;d8ls.started=fm1_ms;
 d8ls.usb=usb.resets;d8ls.usb_config=usb.config;d8ls.generation++;
 if(!d8pr_name_copy(d8ls.name,name?name:proj_name)){d8ls.busy=0;return D8POOL_INVALID;}
 int rc=d8pr_runtime_status(d8p1_signature_runtime(&d8ls.signature));
 if(!rc)rc=ls_scan(&d8ls.index,d8ls.crc);
 if(!rc&&(d8ls.revoked||d8_capture_change.epoch!=d8ls.epoch||!ls_quiet()))rc=D8POOL_BUSY;
 d8ls.busy=0;
 if(rc){if(rc!=D8POOL_BUSY)d8ls_offline();return rc;}
 d8ls.phase=d8ls.intent?LS_EXECUTE:LS_CONSENT;return D8POOL_OK;
}
static int d8ls_request(unsigned operation,unsigned object,const char *name)
{
 /* No backend caller can fabricate name-screen acceptance. Consent belongs
  * to the actual currently open UI operation, not a retained C pointer. */
 if(!name_on()||nm.slot!=object||
    nm.kind!=(operation==0?NK_PROJ_SAVE:NK_PROJ_RENAME)||object>=3)return D8POOL_UNSUPPORTED;
 char expected[13];unsigned first=0,last=nm.len;
 while(last&&nm.s[last-1]==' ')last--;while(first<last&&nm.s[first]==' ')first++;
 unsigned length=last-first;if(length>12||!name)return D8POOL_INVALID;
 memset(expected,0,sizeof expected);memcpy(expected,nm.s+first,length);
 if(!ls_name_equal(expected,name))return D8POOL_UNSUPPORTED;
 if(d8mc.phase!=MC_IDLE&&d8mc.phase!=MC_FAILED&&d8mc.phase!=MC_READY)return D8POOL_BUSY;
 int rc=ls_proposal(operation,object,name);
 if(rc){ui_message(rc==D8POOL_BUSY?"STOP TO SAVE":"STORAGE ERROR");return rc;}
 ui_message(d8ls.intent?"SAVING":"SAVE TO CONFIRM");ui.force=1;return D8POOL_OK;
}
static void d8ls_input(uint32_t pressed,uint32_t notes)
{
#if D8_SAMPLE_SESSION
 if(d8ls.operation==3&&d8ls.phase!=LS_IDLE){d8sm_input(pressed,notes);return;}
#endif
 uint32_t save=1u<<panel.btn[B_SAVE];
 if(notes||(pressed&~save))d8ls.revoked=1;
 for(unsigned i=0;i<7;i++)if(D8_MC_ENCODER_VALUE(i))d8ls.revoked=1;
 if(d8ls.phase==LS_CONSENT&&!d8ls.revoked&&d8ls.shown==d8ls.generation&&
    d8_capture_change.epoch==d8ls.epoch&&usb.resets==d8ls.usb&&usb.config==d8ls.usb_config&&
    !usb.ota_req&&!usb.uboot_req&&mi_r==mi_w&&!transport_req&&pressed==save&&fm1_in.buttons==save&&!notes)d8ls.phase=LS_RELEASE;
 else if(pressed)d8ls.revoked=1;
}
static int d8ls_native_begin(unsigned operation,unsigned object,const char *name)
{
 if(d8ls.writing||d8ls.busy||!d8ls.intent||d8ls.phase!=LS_EXECUTE||operation!=d8ls.operation||object!=d8ls.object||
    (operation!=1&&(!name||!ls_name_equal(d8ls.name,name))))return D8POOL_UNSUPPORTED;
 if(!ls_lifetime())return D8POOL_BUSY;
 d8ls.busy=1;d8pool_index index;uint32_t crc[5],sig=0;
 int rc=d8pr_runtime_status(d8p1_signature_runtime(&sig));
 if(!rc)rc=ls_scan(&index,crc);
 if(!rc&&(!d8pr_same_index(&index,&d8ls.index)||memcmp(crc,d8ls.crc,sizeof crc)||sig!=d8ls.signature||!ls_lifetime()))rc=D8POOL_BUSY;
 d8ls.busy=0;if(rc)return rc;
 unsigned used=0;for(unsigned o=0;o<4;o++)if(index.present&(1u<<o))used|=1u<<index.object[o].block;
 unsigned block=0;while(block<5&&(used&(1u<<block)))block++;
 if(block==5)return D8POOL_INVALID;
 d8ls.block=(uint8_t)block;d8ls.sequence=(index.present&(1u<<object))?index.object[object].sequence+1u:1u;
 d8ls.offset=256;d8ls.write_phase=0;d8ls.writing=1;d8ls.attempted=0;d8ls.length=0;
 d8ls.previous_slot=proj_cur;memcpy(d8ls.previous_name,proj_name,13);return D8POOL_OK;
}
static int ls_ticket(unsigned program,uint32_t a,const void *p,uint32_t n)
{
 if(!d8ls.writing||d8ls.block>=D8POOL_BLOCKS||d8ls.object>=4||d8ls.operation>2||
    d8ls.length>D8P1_LIMIT||d8ls.offset<256||d8ls.offset>256+d8ls.length||!n||!ls_lifetime()||!FL_STORE_OK(a,n))return 0;
 uint32_t base=d8pool_mapped_address(d8ls.block);
 if(a<base||a-base>=D8POOL_BLOCK||n>D8POOL_BLOCK-(a-base))return 0;
 if(d8ls.write_phase&&(!d8ls.length||d8ls.payload_crc!=d8p1_crc32(main_workspace.d8p1.wire,d8ls.length)))return 0;
 if(!program) {
  if(d8ls.write_phase>=2||a!=base+d8ls.write_phase*4096u||n!=4096)return 0;
  if(!d8ls.write_phase) {
   uint8_t *wire=main_workspace.d8p1.wire;d8p1_view view;uint32_t length=mc_get32(wire+8);
   if(length>D8P1_LIMIT||d8p1_read(&view,wire,length)!=1||!d8p1_refs_available(&view,(d8ls.index.present|(1u<<d8ls.object))&7u))return 0;
   unsigned found=0;for(unsigned c=0;c<view.count;c++)if((view.chunk[c].type&0x7fff)==6) {
    if(memcmp(view.chunk[c].data,d8ls.name,12))return 0;found=1;
   }
   if(!found)return 0;d8ls.length=length;d8ls.payload_crc=d8p1_crc32(wire,length);
  }
  return 1;
 }
 if(d8ls.write_phase!=2||!p)return 0;
 if(a==base&&n==32) {
  const uint8_t*h=p;
  return d8ls.offset==256+d8ls.length&&mc_get32(h)==D8POOL_MAGIC&&h[4]==1&&h[5]==d8ls.block&&!h[6]&&!h[7]&&
   mc_get32(h+8)==d8ls.object&&mc_get32(h+12)==d8ls.sequence&&mc_get32(h+16)==d8ls.length&&
   mc_get32(h+20)==d8ls.payload_crc&&mc_get32(h+24)==~0u&&mc_get32(h+28)==d8p1_crc32(h,28)&&
   d8ls.payload_crc==d8p1_crc32(main_workspace.d8p1.wire,d8ls.length);
 }
 uint32_t expected=d8ls.length-(d8ls.offset-256);if(expected>256)expected=256;
 return expected&&n==expected&&a==base+d8ls.offset&&p==main_workspace.d8p1.wire+d8ls.offset-256;
}
static int ls_driver(unsigned program,uint32_t a,const void *p,uint32_t n)
{
 uint32_t flags=irq_save(),took=0;int rc=-10;
 if(!d8ls.busy&&ls_ticket(program,a,p,n)&&d8_capture_change.epoch!=UINT32_MAX&&d8ls.own!=UINT32_MAX) {
  d8ls.busy=1;d8ls.attempted=1;d8_capture_store_changed();d8ls.own++;
  rc=program?FL_FAR(fl_prog_ram)(a,p,n,&took):FL_FAR(fl_erase4k_ram)(a,&took);
  if(!ls_lifetime())rc=-1;
  if(!rc){if(!program)d8ls.write_phase++;else if(a==d8pool_mapped_address(d8ls.block))d8ls.write_phase=3;else d8ls.offset+=n;}
  d8ls.busy=0;
 }
 irq_restore(flags);return rc;
}
static int d8ls_driver_erase(uint32_t a){return ls_driver(0,a,NULL,4096);}
static int d8ls_driver_program(uint32_t a,const void*p,uint32_t n){return ls_driver(1,a,p,n);}
static int d8ls_native_finish(int rc)
{
 uint32_t flags=irq_save();
 if(!rc&&(!ls_lifetime()||d8ls.write_phase!=3))rc=D8POOL_IO;
 if(rc){proj_cur=d8ls.previous_slot;memcpy(proj_name,d8ls.previous_name,13);}
 d8ls.writing=0;irq_restore(flags);
 if(rc&&(d8ls.attempted||rc!=D8POOL_BUSY))ls_fail(1);
 else if(d8ls.operation==1)d8ls.phase=LS_IDLE;
 return rc;
}
static void d8ls_poll(void)
{
#if D8_SAMPLE_SESSION
 if(d8ls.operation==3&&d8ls.phase!=LS_IDLE){d8sm_poll();return;}
#endif
 if(d8ls.intent&&(!flash_ok||project_native.mode!=1||migration_owner||usb.ota_req||usb.uboot_req))d8ls_offline();
 if(!d8ls_blocked()||d8ls.busy)return;
 if(d8ls.revoked||(uint32_t)(fm1_ms-d8ls.started)>LS_DEADLINE_MS||
    usb.resets!=d8ls.usb||usb.config!=d8ls.usb_config){ls_fail(0);ui_message("SAVE CANCELLED");return;}
 if(d8ls.phase==LS_RELEASE){if(!ls_quiet())return;d8ls.intent=1;d8ls.phase=LS_EXECUTE;}
 if(d8ls.phase!=LS_EXECUTE)return;
 if(!ls_lifetime()){ls_fail(0);ui_message("SAVE CANCELLED");return;}
 if(d8ls.operation==1){ls_fail(0);return;} /* scheduler owns every automatic result */
 int rc=d8ls.operation==0?d8p1_save_as_flash(d8ls.object,d8ls.name,1):d8p1_rename_flash(d8ls.object,d8ls.name,1);
 if(rc){ls_fail(d8ls.attempted||rc!=D8POOL_BUSY);ui_message("STORAGE ERROR");return;}
 d8ls.phase=LS_IDLE;
 /* A successful independent local commit retires the old completed USB
  * receipt. Cancelling a proposal never destroys that earlier read receipt. */
 memset(&d8mc,0,sizeof d8mc);
 /* Bind only after a real local committed operation. This is not a boot grant. */
 if(project_native_bind_flash(1)||(!native_as.ready&&d8p1_autosave_local_begin())){ls_fail(1);ui_message("STORAGE ERROR");return;}
 ui_message(d8ls.operation==0?"SAVED":"RENAMED");
}
/* Only the actual native scheduler calls this immediately before its stopped
 * automatic operation. Each call captures a new proposal; no retained ticket. */
static int d8ls_autosave_begin(void)
{
 if(!d8ls.intent||!native_as.ready||!native_as.enabled)return D8POOL_UNSUPPORTED;
 return ls_proposal(1,3,NULL);
}
static void d8ls_autosave_finish(int rc)
{
 if(d8ls.operation==1&&d8ls.phase==LS_EXECUTE&&!d8ls.writing)ls_fail((rc&&rc!=D8POOL_BUSY)||d8ls.attempted);
}
#ifndef D8_LS_LCD_FAULT_COUNT
#define D8_LS_LCD_FAULT_COUNT() fm1_lcd_timeouts
#endif
static void d8ls_draw(void)
{
#if D8_SAMPLE_SESSION
 if(d8ls.operation==3&&d8ls.phase!=LS_IDLE){d8sm_draw();return;}
#endif
 uint32_t lcd_faults=D8_LS_LCD_FAULT_COUNT();
 uint32_t generation=d8ls.generation,epoch=d8_capture_change.epoch;
 unsigned phase=d8ls.phase;
 d8ls.shown=0;
 style_apply();lcd_fill(0,0,240,240,T_BG);
 char title[20];str_cpy(title,d8ls.operation==2?"RENAME SLOT ":"SAVE SLOT ",sizeof title);
 unsigned end=str_len(title);title[end]=(char)('1'+d8ls.object);title[end+1]=0;
 draw_text_box(12,32,216,&AF_M,title,T_THEME,1);
 draw_text_box(12,76,216,&AF_M,d8ls.name,T_INK,1);
 draw_text_box(12,122,216,&AF_S,"Saving and autosave this session",T_INK,1);
 draw_text_box(12,168,216,&AF_M,"PRESS AND RELEASE SAVE",T_ACCENT,1);
 draw_text_box(12,204,216,&AF_S,"Other input cancels",T_DIM,1);
 lcd_sync();
 /* Publish visible consent only after the actual display transfer. Events
  * arriving during that transfer invalidate this exact proposal. */
 if(phase!=LS_CONSENT)return; /* Held SAVE/release frames cannot grant anew. */
 uint32_t flags=irq_save();
 if(D8_LS_LCD_FAULT_COUNT()==lcd_faults&&d8ls.phase==LS_CONSENT&&!d8ls.busy&&!d8ls.revoked&&generation==d8ls.generation&&
    epoch==d8_capture_change.epoch&&epoch==d8ls.epoch&&usb.resets==d8ls.usb&&
    usb.config==d8ls.usb_config&&ls_quiet())d8ls.shown=generation;
 else d8ls.revoked=1;
 irq_restore(flags);
}
#endif
