/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit destructive own-sample replacement. Reuses local coordinator state,
 * editor header scratch and leased main workspace; never opens the generic gate. */
#if NTRK == 8 && D8_MIGRATION_SESSION && D8_SAMPLE_SESSION
#ifndef D8_SM_ERASE
#define D8_SM_ERASE(a,t) FL_FAR(fl_erase4k_ram)(a,t)
#define D8_SM_PROGRAM(a,p,n,t) FL_FAR(fl_prog_ram)(a,p,n,t)
#endif
enum { SM_OK,SM_BAD,SM_BUSY,SM_STALE,SM_IO,SM_CANCELLED };
enum { SM_HEADER_ERASE,SM_DATA_ERASE,SM_DATA,SM_HEADER_A,SM_HEADER_B,SM_MAGIC,SM_DONE };
static uint32_t sm_word(const char *p){uint32_t x;memcpy(&x,p,4);return x;}
static void sm_store(char *p,uint32_t x){memcpy(p,&x,4);}
/* FSMP schema is byte-encoded; staging remains a byte object, not a struct. */
_Static_assert(sizeof(smp_user_hdr_t)==480&&sizeof(smp_zone_t)==28,"FSMP schema1 layout");
static uint32_t sm_raw32(const uint8_t*p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static int sm_header(const uint8_t*h)
{
 uint32_t length=sm_raw32(h+16);
 if(sm_raw32(h)!=SMP_USER_MAGIC||h[4]!=1||h[5]||!h[6]||h[6]>16||h[7]||sm_raw32(h+24)||sm_raw32(h+28)||
    !length||length>SMP_USER_SIZE-SMP_USER_DATA)return 0;
 for(unsigned i=0;i<h[6];i++){const uint8_t*z=h+32+28*i;
  uint32_t off=sm_raw32(z),n=sm_raw32(z+4),ls=sm_raw32(z+8),le=sm_raw32(z+12),rate=sm_raw32(z+16);
  if(!n||off>length||n>2u*SMP_USER_SIZE||off+(n+1u)/2u>length||
     z[24]>88||!rate||rate>4u<<16||ls>le||le>=n||z[25]>z[26])return 0;
 }return 1;
}
static int sm_quiet(void)
{
 if(!flash_ok||cv_cpu_active||transport_req||transport_busy()||seq_counting()||!df_automatic_quiet()||
    fm1_in.buttons||fm1_in.notes||D8_MC_PENDING_PRESS()||usb.ota_req||usb.uboot_req||
    d8mc_edit.locked||d8mc_edit.busy||d8mc_edit.writer)return 0;
 for(unsigned i=0;i<7;i++)if(D8_MC_ENCODER_VALUE(i))return 0;
 return d8ls.phase==LS_EXECUTE?main_migration_workspace(d8ls.signature)!=NULL:!migration_owner;
}
static int sm_lifetime(void)
{
 return d8ls.operation==3&&d8ls.phase!=LS_IDLE&&!d8ls.revoked&&project_native_status()==1&&
  usb.resets==d8ls.usb&&usb.config==d8ls.usb_config&&d8_capture_change.epoch==d8ls.epoch+d8ls.own&&
  (uint32_t)(fm1_ms-d8ls.started)<120000u&&d8p1_crc32(ed_smp_buf,sizeof(smp_user_hdr_t))==d8ls.payload_crc&&
  sm_header(ed_smp_buf)&&sm_quiet();
}
static void sm_finish(unsigned rc)
{
 if(d8ls.phase==LS_EXECUTE&&d8ls.signature)main_migration_end(d8ls.signature);
 if(d8ls.attempted){ed_smp_inval(d8ls.object);smp_user_scan(d8ls.object);}
 d8ls.phase=LS_IDLE;d8ls.shown=0;d8ls.busy=d8ls.writing=0;d8ls.write_phase=rc?SM_HEADER_ERASE:SM_DONE;
 d8ls.previous_name[8]=(char)rc;d8ls.intent=d8ls.previous_intent;ui.force=1;ui_message(rc?"SAMPLE CANCELLED":"SAMPLE READY");
}
static int sm_driver(unsigned program,uint32_t a,const void*p,uint32_t n)
{
 /* Long fingerprint runs with interrupts enabled; the following critical
  * admission binds its epoch/ownership before the first destructive primitive. */
 if(!program&&d8ls.write_phase==SM_HEADER_ERASE&&
    st_crc32(smp_user_xip(d8ls.object),SMP_USER_SIZE)!=sm_word(d8ls.name))return -10;
 uint32_t f=irq_save(),took=0;int rc=-10;uint32_t base=ed_smp_slot(d8ls.object);
 if(d8ls.phase==LS_EXECUTE&&!d8ls.busy&&sm_lifetime()&&d8ls.object<SMP_USER_SLOTS&&
    d8ls.own!=UINT32_MAX&&d8_capture_change.epoch!=UINT32_MAX&&FL_STORE_OK(a,n)&&
    a>=base&&a-base<=SMP_USER_SIZE&&n<=SMP_USER_SIZE-(a-base)){
  const uint8_t*page=main_workspace.migration.stage.wire;unsigned ok=0;
  if(!program)ok=n==4096&&((d8ls.write_phase==SM_HEADER_ERASE&&a==base&&fm1_lcd_timeouts==d8ls.sequence)||
    (d8ls.write_phase==SM_DATA_ERASE&&!(d8ls.offset&4095u)&&a==base+d8ls.offset));
  else if(d8ls.write_phase==SM_DATA)ok=a==base+d8ls.offset&&p==page&&n&&n<=256&&
    n==(d8ls.length-(d8ls.offset-512)>256?256:d8ls.length-(d8ls.offset-512))&&
    d8p1_crc32(page,n)==sm_word(d8ls.previous_name);
  else if(d8ls.write_phase==SM_HEADER_A)ok=a==base+4&&p==ed_smp_buf+4&&n==252;
  else if(d8ls.write_phase==SM_HEADER_B)ok=a==base+256&&p==ed_smp_buf+256&&n==224;
  else if(d8ls.write_phase==SM_MAGIC)ok=a==base&&p==ed_smp_buf&&n==4;
  if(ok){d8ls.busy=1;d8ls.attempted=1;d8_capture_store_changed();d8ls.own++;
   rc=program?D8_SM_PROGRAM(a,p,n,&took):D8_SM_ERASE(a,&took);d8ls.busy=0;
   if(!sm_lifetime()||(program&&d8ls.write_phase==SM_DATA&&d8p1_crc32(page,n)!=sm_word(d8ls.previous_name)))rc=-1;
  }
 }irq_restore(f);return rc;
}
static void d8sm_draw(void)
{
 if(d8ls.phase==LS_EXECUTE)return; /* staged data and arena ownership stay intact */
 uint32_t faults=fm1_lcd_timeouts,token=d8ls.generation,epoch=d8_capture_change.epoch;
 d8ls.shown=0;style_apply();lcd_fill(0,0,240,240,T_BG);
 draw_text_box(12,30,216,&AF_M,"REPLACE SAMPLE",T_THEME,1);
 char label[]="USER SLOT 1";label[10]=(char)('1'+d8ls.object);
 draw_text_box(12,70,216,&AF_M,label,T_TEXT,1);
 draw_text_box(12,108,216,&AF_S,"Previous sample is lost at BEGIN",T_TEXT,1);
 draw_text_box(12,145,216,&AF_S,"Interruption can leave slot empty",T_ACCENT,1);
 draw_text_box(12,188,216,&AF_M,"PRESS AND RELEASE SAVE",T_ACCENT,1);lcd_sync();
 uint32_t f=irq_save();if(d8ls.phase==LS_CONSENT&&token==d8ls.generation&&epoch==d8ls.epoch&&
    faults==fm1_lcd_timeouts&&sm_lifetime()){d8ls.sequence=faults;d8ls.shown=token;}irq_restore(f);
}
static void d8sm_input(uint32_t pressed,uint32_t notes)
{
 uint32_t save=1u<<panel.btn[B_SAVE];
 if(notes||(pressed&~save)){d8ls.revoked=1;return;}
 for(unsigned i=0;i<7;i++)if(D8_MC_ENCODER_VALUE(i)){d8ls.revoked=1;return;}
 if(d8ls.phase==LS_CONSENT&&pressed==save&&fm1_in.buttons==save&&d8ls.shown==d8ls.generation&&
    !d8ls.revoked&&d8ls.sequence==fm1_lcd_timeouts&&d8_capture_change.epoch==d8ls.epoch&&
    usb.resets==d8ls.usb&&usb.config==d8ls.usb_config&&!usb.ota_req&&!usb.uboot_req){d8ls.phase=LS_RELEASE;d8ls.shown=0;}
 else if(pressed)d8ls.revoked=1;
}
static void d8sm_poll(void)
{
 if(d8ls.phase==LS_IDLE)return;
 if(d8ls.revoked||usb.resets!=d8ls.usb||usb.config!=d8ls.usb_config||
    d8_capture_change.epoch!=d8ls.epoch+d8ls.own||(uint32_t)(fm1_ms-d8ls.started)>=120000u){sm_finish(SM_CANCELLED);return;}
 if(d8ls.phase==LS_EXECUTE){if(!sm_lifetime())sm_finish(SM_CANCELLED);return;}
 if(d8ls.phase!=LS_RELEASE||!sm_quiet())return;
 if(!sm_lifetime()||fm1_lcd_timeouts!=d8ls.sequence){sm_finish(SM_CANCELLED);return;}
 d8pool_index index;uint32_t crc[5];int rc=ls_scan(&index,crc);
 if(rc||!d8pr_same_index(&index,&d8ls.index)||memcmp(crc,d8ls.crc,sizeof crc)||
    st_crc32(smp_user_xip(d8ls.object),SMP_USER_SIZE)!=sm_word(d8ls.name)||!sm_lifetime()){
  sm_finish(SM_CANCELLED);return;
 }
 uint32_t generation=0;if(main_migration_begin(&generation)){sm_finish(SM_BUSY);return;}
 d8ls.signature=generation;d8ls.phase=LS_EXECUTE;d8ls.write_phase=SM_HEADER_ERASE;d8ls.offset=512;
 if(sm_driver(0,ed_smp_slot(d8ls.object),NULL,4096)){sm_finish(SM_IO);return;}
 ed_smp_inval(d8ls.object);smp_user_scan(d8ls.object);d8ls.write_phase=SM_DATA;ui.force=1;
}
static unsigned sm_prepare(const uint8_t*a,uint32_t n)
{
 if(n<4||a[2]>=SMP_USER_SLOTS)return SM_BAD;
 if(d8ls_blocked()||migration_owner||project_native_status()!=1||
    d8mc_edit.locked||d8ls.generation==UINT32_MAX||d8_capture_change.epoch==UINT32_MAX)return SM_BUSY;
 if(ed_unpack7(a+3,n-3,ed_smp_buf,sizeof(smp_user_hdr_t))!=sizeof(smp_user_hdr_t)||
    !sm_header(ed_smp_buf))return SM_BAD;
 d8ls.previous_intent=d8ls.intent;d8ls.operation=3;d8ls.phase=LS_CONSENT;d8ls.object=a[2];
 d8ls.epoch=d8_capture_change.epoch;d8ls.own=0;d8ls.usb=usb.resets;d8ls.usb_config=usb.config;d8ls.started=fm1_ms;
 d8ls.generation++;d8ls.revoked=d8ls.busy=d8ls.attempted=d8ls.writing=0;d8ls.shown=0;d8ls.previous_name[8]=0;
 d8ls.payload_crc=d8p1_crc32(ed_smp_buf,sizeof(smp_user_hdr_t));d8ls.length=sm_raw32(ed_smp_buf+16);
 int rc=ls_scan(&d8ls.index,d8ls.crc);sm_store(d8ls.name,st_crc32(smp_user_xip(d8ls.object),SMP_USER_SIZE));
 if(rc||!sm_lifetime()){sm_finish(SM_BUSY);return SM_BUSY;}ui.force=1;return SM_OK;
}
static unsigned sm_write(const uint8_t*a,uint32_t n)
{
 if(n<14||d8ls.phase!=LS_EXECUTE||!sm_lifetime())return SM_STALE;
 uint32_t off;if(!mc_wire_u32(a+7,&off))return SM_BAD;if(off<512||(off&255u)||off>512+d8ls.length)return SM_BAD;
 uint8_t*page=main_workspace.migration.stage.wire;uint32_t len=ed_unpack7(a+12,n-12,page,256);
 if(!len||len>512+d8ls.length-off)return SM_BAD;
 uint32_t expected=d8ls.length-(off-512);if(expected>256)expected=256;if(len!=expected)return SM_BAD;
 if(off<d8ls.offset){if(off+len!=d8ls.offset||memcmp(smp_user_xip(d8ls.object)+off,page,len))return SM_BAD;return SM_OK;}
 if(off!=d8ls.offset)return SM_BAD;
 if(!(off&4095u)){d8ls.write_phase=SM_DATA_ERASE;if(sm_driver(0,ed_smp_slot(d8ls.object)+off,NULL,4096)){sm_finish(SM_IO);return SM_IO;}}
 d8ls.write_phase=SM_DATA;sm_store(d8ls.previous_name,d8p1_crc32(page,len));if(sm_driver(1,ed_smp_slot(d8ls.object)+off,page,len)){sm_finish(SM_IO);return SM_IO;}
 if(memcmp(smp_user_xip(d8ls.object)+off,page,len)){sm_finish(SM_IO);return SM_IO;}d8ls.offset+=len;return SM_OK;
}
static unsigned sm_commit(void)
{
 if(d8ls.phase!=LS_EXECUTE||!sm_lifetime()||d8ls.offset!=512+d8ls.length)return SM_STALE;
 const uint8_t*h=ed_smp_buf;ed_smp_inval(d8ls.object);
 if(st_crc32(smp_user_xip(d8ls.object)+512,d8ls.length)!=sm_raw32(h+20)){sm_finish(SM_BAD);return SM_BAD;}
 d8ls.write_phase=SM_HEADER_A;if(sm_driver(1,ed_smp_slot(d8ls.object)+4,ed_smp_buf+4,252)){sm_finish(SM_IO);return SM_IO;}
 d8ls.write_phase=SM_HEADER_B;if(sm_driver(1,ed_smp_slot(d8ls.object)+256,ed_smp_buf+256,224)){sm_finish(SM_IO);return SM_IO;}
 ed_smp_inval(d8ls.object);if(memcmp(smp_user_xip(d8ls.object)+4,ed_smp_buf+4,476)){sm_finish(SM_IO);return SM_IO;}
 d8ls.write_phase=SM_MAGIC;if(sm_driver(1,ed_smp_slot(d8ls.object),ed_smp_buf,4)){sm_finish(SM_IO);return SM_IO;}
 ed_smp_inval(d8ls.object);if(memcmp(smp_user_xip(d8ls.object),ed_smp_buf,sizeof(smp_user_hdr_t))){sm_finish(SM_IO);return SM_IO;}
 sm_finish(SM_OK);return usr_nz[d8ls.object]==h[6]?SM_OK:SM_IO;
}
static void d8sm_handle(const uint8_t*a,uint32_t n)
{
 unsigned rc=SM_BAD,op=n>1?a[1]:0;uint32_t token=0;
 if(n>=2&&a[0]==1){
  if(op==1)rc=sm_prepare(a,n);
  else if(n>=7&&mc_wire_u32(a+2,&token)&&token==d8ls.generation&&d8ls.operation==3){
   if(op==2&&n==7){d8sm_poll();rc=d8ls.phase==LS_IDLE?(unsigned)(uint8_t)d8ls.previous_name[8]:SM_OK;}
   else if(op==3)rc=sm_write(a,n);
   else if(op==4&&n==7)rc=d8ls.phase==LS_IDLE&&d8ls.write_phase==SM_DONE?SM_OK:sm_commit();
   else if(op==5&&n==7){if(d8ls.phase!=LS_IDLE)sm_finish(SM_CANCELLED);rc=SM_OK;}
  }else rc=SM_STALE;
 }
 ed_b(1);ed_b(op);ed_b(rc);ed_bk_u32(d8ls.generation);ed_b(d8ls.phase);ed_bk_u32(d8ls.offset);
}
#endif
