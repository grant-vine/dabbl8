/* SPDX-License-Identifier: GPL-3.0-only */
/* Whole own-project set transfer. Fixed logical roles and the existing leased
 * arena only; no addresses, external migration or generic editor grant. */
#if NTRK == 8 && D8_MIGRATION_SESSION && D8_PROJECT_TRANSFER_SESSION
enum { PT_OK,PT_BAD,PT_BUSY,PT_STALE,PT_IO,PT_CANCELLED,PT_UNSUPPORTED,PT_INCOMPLETE };
enum { PT_DONE,PT_EXPORT,PT_RECEIVE,PT_CONSENT,PT_RELEASE,PT_APPROVED,PT_PARTIAL };
static uint32_t pt_word(unsigned off){uint32_t v;memcpy(&v,d8ls.name+off,4);return v;}
static void pt_set(unsigned off,uint32_t v){memcpy(d8ls.name+off,&v,4);}
/* Six small words reuse both existing canonical-name buffers, via memcpy. */
static uint32_t pt_extra(unsigned off){uint32_t v;memcpy(&v,d8ls.previous_name+off,4);return v;}
static void pt_extra_set(unsigned off,uint32_t v){memcpy(d8ls.previous_name+off,&v,4);}
#define PT_POOLCRC 0u
#define PT_POOLRECV 4u
#define PT_LIVERECV 8u
#define PT_PRESENCE 0u
#define PT_COMMITTED 4u
#define PT_SCRATCH 8u
static unsigned pt_phase(void){return d8ls.object;}
static d8mp_workspace *pt_workspace(void){return main_migration_workspace(d8ls.signature);}
static int pt_quiet(void)
{
 if(!flash_ok||cv_cpu_active||transport_req||transport_busy()||seq_counting()||!df_automatic_quiet()||
    ((fm1_in.buttons||D8_MC_PENDING_PRESS())&&!(d8ls.operation==4&&pt_phase()==PT_RELEASE&&fm1_in.buttons==(1u<<panel.btn[B_SAVE])))||fm1_in.notes||usb.ota_req||usb.uboot_req||
    d8mc_edit.locked||d8mc_edit.busy||d8mc_edit.writer)return 0;
 for(unsigned i=0;i<7;i++)if(D8_MC_ENCODER_VALUE(i))return 0;
 return !migration_owner||pt_workspace()!=NULL;
}
static int pt_guard(void)
{
 return !d8ls.revoked&&pt_workspace()&&pt_quiet()&&
   usb.resets==d8ls.usb&&usb.config==d8ls.usb_config&&
   d8_capture_change.epoch==d8ls.epoch+d8ls.own&&
   (uint32_t)(fm1_ms-d8ls.started)<LS_DEADLINE_MS;
}
static int pt_lifetime(void)
{ return d8ls.operation==4&&d8ls.phase!=LS_IDLE&&pt_guard(); }
/* The transfer already owns staging; generic capture would borrow it again. */
static int pt_capture(d8mp_workspace*w,uint32_t*length,uint32_t*crc)
{
 uint32_t f=irq_save();int rc=pt_guard()?D8POOL_OK:D8POOL_BUSY;
 if(!rc){project_capture(&w->stage.state.project);
  if(d8p1_runtime_cache.valid)memcpy(&w->stage.state.arrangement,&d8p1_runtime_cache.arrangement,sizeof w->stage.state.arrangement);
  else memset(&w->stage.state.arrangement,0,sizeof w->stage.state.arrangement);
 }irq_restore(f);if(rc||!pt_guard())return D8POOL_BUSY;
 size_t count=0;if(!d8p1_project_encode(w->stage.wire,sizeof w->stage.wire,&count,&w->stage.state))return D8POOL_INVALID;
 *length=(uint32_t)count;*crc=d8p1_crc32(w->stage.wire,count);return D8POOL_OK;
}
static int pt_read(void*c,uint32_t a,void*p,uint32_t n)
{
 (void)c;if(a>D8POOL_BYTES||n>D8POOL_BYTES-a)return -1;
 while(n){unsigned b=a/D8POOL_BLOCK,k=D8POOL_BLOCK-a%D8POOL_BLOCK;if(k>n)k=n;
  if(!pt_quiet()||df_read(NULL,d8pool_mapped_address(b)+a%D8POOL_BLOCK,p,k))return -1;
  a+=k;p=(uint8_t*)p+k;n-=k;
 }return 0;
}
static int pt_plan_read(void*c,uint32_t a,void*p,uint32_t n)
{ d8mp_workspace*w=c;if(!w||a>D8POOL_BYTES||n>D8POOL_BYTES-a)return -1;memcpy(p,w->plan+a,n);return 0; }
static int pt_noerase(void*c,uint32_t a){(void)c;(void)a;return -1;}
static int pt_noprogram(void*c,uint32_t a,const void*p,uint32_t n){(void)c;(void)a;(void)p;(void)n;return -1;}
static int pt_stopped(void*c){(void)c;return pt_lifetime();}
static int pt_known(d8mp_workspace*w,const uint8_t*p,uint32_t n,unsigned mask)
{
 d8p1_view view;
 return n&&n<=D8P1_LIMIT&&d8p1_read(&view,p,n)==1&&d8p1_refs_available(&view,mask)&&
  !d8p1_runtime_engines(&view,D8ENGINE_EFFECTIVE_MASK)&&d8p1_project_decode(&w->stage.state,p,n,mask);
}
/* Inventory first: unsupported/future/ambiguous headers cannot be hidden by
 * the later scratch transform. A sparse damaged sole copy never means absent. */
static int pt_scan(const d8pool*s,d8mp_workspace*w,d8pool_index*index,unsigned source,unsigned mask,uint32_t crc[5])
{
 int rc=d8pool_inventory(s,index);if(rc)return rc;
 unsigned used=0;for(unsigned o=0;o<4;o++)if(index->anchors&(1u<<o))used|=1u<<index->object[o].block;
 uint8_t h[32],page[256];
 for(unsigned b=0;b<5;b++){
  uint32_t rolling=~0u;unsigned erased=1;
  for(unsigned off=0;off<D8POOL_BLOCK;off+=256){if(s->read(s->context,b*D8POOL_BLOCK+off,page,256))return D8POOL_IO;
   rolling=d8own_crc(rolling,page,256);for(unsigned i=0;i<256;i++)if(page[i]!=255)erased=0;
  }if(crc)crc[b]=rolling^~0u;if(erased)continue;
  if(s->read(s->context,b*D8POOL_BLOCK,h,32))return D8POOL_IO;
  unsigned object=d8own_get(h+8);uint32_t len=d8own_get(h+16);
  unsigned trusted=d8own_get(h)==D8POOL_MAGIC&&(h[4]==1||h[4]==2)&&h[5]==b&&!h[6]&&!h[7]&&
    object<4&&d8own_get(h+24)==~0u&&d8own_get(h+28)==d8p1_crc32(h,28);
  if(!trusted){
   /* Restore-only admission of a torn noncurrent spare. Pool inventory above
    * already refused magic-incompatible foreign bytes and future headers. */
   if(!(used&(1u<<b))&&index->anchors&&(!source||index->present==15))continue;
   return D8POOL_UNSUPPORTED;
  }
  if(!(index->anchors&(1u<<object)))return D8POOL_INVALID;
  if((h[4]==1&&(!len||len>D8P1_LIMIT))||(h[4]==2&&(len||d8own_get(h+20))))return D8POOL_INVALID;
  /* Known records have no opaque reserved allocation or post-payload tail. */
  for(unsigned off=32;off<D8POOL_BLOCK;off+=256){unsigned k=D8POOL_BLOCK-off;if(k>256)k=256;
   if(s->read(s->context,b*D8POOL_BLOCK+off,page,k))return D8POOL_IO;
   for(unsigned i=0;i<k;i++){unsigned pos=off+i;if((pos<D8POOL_HEADER||pos>=D8POOL_HEADER+len)&&page[i]!=255)return D8POOL_UNSUPPORTED;}
  }
  if(h[4]==2){if(len||d8own_get(h+20))return D8POOL_INVALID;continue;}
  if(!len||len>D8P1_LIMIT)return D8POOL_INVALID;
  const uint8_t *wire=source?w->plan+b*D8POOL_BLOCK+D8POOL_HEADER:w->stage.wire;
  if((!source&&s->read(s->context,b*D8POOL_BLOCK+D8POOL_HEADER,w->stage.wire,len))||
     d8p1_crc32(wire,len)!=d8own_get(h+20))return D8POOL_INVALID;
  unsigned current=(index->present&(1u<<object))&&index->object[object].block==b;
  if(!pt_known(w,wire,len,current?(mask==15u?index->present&7u:mask):7u))return D8POOL_INVALID;
 }
 return index->anchors||source?D8POOL_OK:D8POOL_EMPTY;
}
static void pt_finish(unsigned status)
{
 if(d8ls.signature)main_migration_end(d8ls.signature);d8ls.signature=0;
 d8ls.phase=LS_IDLE;d8ls.shown=0;d8ls.busy=d8ls.writing=0;d8ls.write_phase=(uint8_t)status;
 d8ls.object=status?PT_PARTIAL:PT_DONE;d8ls.intent=d8ls.previous_intent;ui.force=1;
 if(d8ls.attempted&&status){d8ls.intent=0;project_native.mode=2;project_native.ready=0;d8p1_autosave_session_end();}
}
static int pt_begin(unsigned importing,const uint8_t*a,uint32_t n)
{
 if(d8ls_blocked()||migration_owner||!pt_quiet()||d8ls.generation==UINT32_MAX||d8_capture_change.epoch==UINT32_MAX)return PT_BUSY;
 if(importing&&(n!=18||a[2]>15))return PT_BAD;
 uint32_t live=0,livecrc=0,poolcrc=0;
 if(importing&&(!mc_wire_u32(a+3,&live)||!mc_wire_u32(a+8,&livecrc)||!mc_wire_u32(a+13,&poolcrc)||!live||live>D8P1_LIMIT))return PT_BAD;
 d8ls.previous_intent=d8ls.intent;d8ls.operation=4;d8ls.revoked=0;d8ls.attempted=0;d8ls.busy=1;
 d8ls.epoch=d8_capture_change.epoch;d8ls.own=0;d8ls.started=fm1_ms;d8ls.usb=usb.resets;d8ls.usb_config=usb.config;
 uint32_t lease=0;int rc=main_migration_begin(&lease)==D8MP_OK?D8POOL_OK:D8POOL_BUSY;d8ls.signature=lease;
 d8mp_workspace*w=pt_workspace();d8pool reader={NULL,pt_read,pt_noerase,pt_noprogram,pt_stopped};
 if(!rc&&!pt_guard())rc=D8POOL_BUSY;
 if(!rc)rc=pt_scan(&reader,w,&d8ls.index,0,importing?7u:15u,d8ls.crc);
 if(!rc&&!importing)rc=pt_capture(w,&live,&livecrc);
 if(!rc&&!pt_guard())rc=D8POOL_BUSY;
 d8ls.busy=0;
 if(rc){if(lease)main_migration_end(lease);d8ls.signature=0;return rc==D8POOL_BUSY?PT_BUSY:PT_UNSUPPORTED;}
 d8ls.generation++;d8ls.phase=LS_EXECUTE;d8ls.object=importing?PT_RECEIVE:PT_EXPORT;
 d8ls.length=live;d8ls.payload_crc=livecrc;pt_set(PT_POOLCRC,poolcrc);pt_set(PT_POOLRECV,0);pt_set(PT_LIVERECV,0);
 pt_extra_set(PT_PRESENCE,importing?a[2]:d8ls.index.present);pt_extra_set(PT_COMMITTED,0);pt_extra_set(PT_SCRATCH,5);
 if(!importing){if(pt_read(NULL,0,w->plan,D8POOL_BYTES)||!pt_lifetime()){pt_finish(PT_IO);return PT_IO;}
  pt_set(PT_POOLCRC,d8p1_crc32(w->plan,D8POOL_BYTES));
 }
 d8p1_autosave_session_end();return PT_OK;
}
static void d8pt_draw(void)
{
 if(pt_phase()!=PT_CONSENT&&pt_phase()!=PT_RELEASE)return;
 uint32_t token=d8ls.generation,fault=fm1_lcd_timeouts;d8ls.shown=0;style_apply();lcd_fill(0,0,240,240,T_BG);
 if(!own_transfer_text_strip(d8ls.signature,32,"RESTORE OWN PROJECTS",T_THEME,T_BG)||
    !own_transfer_text_strip(d8ls.signature,76,"Replaces saved slots and autosave",T_TEXT,T_BG)||
    !own_transfer_text_strip(d8ls.signature,118,"Interruption can leave partial set",T_ACCENT,T_BG)||
    !own_transfer_text_strip(d8ls.signature,160,"Keep backup to retry restore",T_TEXT,T_BG)||
    !own_transfer_text_strip(d8ls.signature,202,"PRESS AND RELEASE SAVE",T_ACCENT,T_BG)){d8ls.revoked=1;return;}
 uint32_t f=irq_save();if(pt_phase()==PT_CONSENT&&pt_lifetime()&&token==d8ls.generation&&fault==fm1_lcd_timeouts){d8ls.shown=token;d8ls.sequence=fault;}irq_restore(f);
}
static void d8pt_input(uint32_t pressed,uint32_t notes)
{
 if(pt_phase()!=PT_CONSENT&&pt_phase()!=PT_RELEASE){if(pressed||notes)d8ls.revoked=1;return;}
 uint32_t save=1u<<panel.btn[B_SAVE];
 if(notes||(pressed&~save))d8ls.revoked=1;for(unsigned i=0;i<7;i++)if(D8_MC_ENCODER_VALUE(i))d8ls.revoked=1;
 if(pt_phase()==PT_CONSENT&&!d8ls.revoked&&d8ls.shown==d8ls.generation&&pressed==save&&fm1_in.buttons==save&&
    d8_capture_change.epoch==d8ls.epoch&&usb.resets==d8ls.usb&&usb.config==d8ls.usb_config&&
    fm1_lcd_timeouts==d8ls.sequence&&!transport_req&&mi_r==mi_w&&!usb.ota_req&&!usb.uboot_req)d8ls.object=PT_RELEASE;
 else if(pressed)d8ls.revoked=1;
}
static void d8pt_poll(void)
{
 if(pt_phase()==PT_RELEASE&&!fm1_in.buttons&&pt_lifetime())d8ls.object=PT_APPROVED;
 if(d8ls.revoked||usb.resets!=d8ls.usb||usb.config!=d8ls.usb_config||d8_capture_change.epoch!=d8ls.epoch+d8ls.own||
    (uint32_t)(fm1_ms-d8ls.started)>=LS_DEADLINE_MS||
    !pt_lifetime())pt_finish(PT_CANCELLED);
}
static const uint8_t *pt_live(void)
{ d8mp_workspace*w=pt_workspace();return w&&pt_extra(PT_SCRATCH)<5?w->plan+pt_extra(PT_SCRATCH)*D8POOL_BLOCK+D8POOL_HEADER:NULL; }
static int pt_prepare(void)
{
 d8mp_workspace*w=pt_workspace();if(pt_phase()!=PT_RECEIVE||!pt_lifetime())return PT_STALE;
 if(pt_word(PT_POOLRECV)!=D8POOL_BYTES||pt_word(PT_LIVERECV)!=d8ls.length)return PT_INCOMPLETE;
 if(d8p1_crc32(w->plan,D8POOL_BYTES)!=pt_word(PT_POOLCRC)||d8p1_crc32(w->stage.wire,d8ls.length)!=d8ls.payload_crc)return PT_BAD;
 d8pool reader={w,pt_plan_read,pt_noerase,pt_noprogram,pt_stopped};d8pool_index source;
 int rc=pt_scan(&reader,w,&source,1,15,NULL);
 if(rc||source.present!=pt_extra(PT_PRESENCE)||!pt_known(w,w->stage.wire,d8ls.length,source.present&7u))return PT_BAD;
 unsigned used=0;for(unsigned o=0;o<4;o++)if(source.anchors&(1u<<o))used|=1u<<source.object[o].block;
 unsigned scratch=0;while(scratch<5&&(used&(1u<<scratch)))scratch++;if(scratch==5)return PT_BAD;
 /* Only after every original source byte/role is validated: repurpose a truly
  * noncurrent source allocation. This is a sealed WORKING plan, not raw backup. */
 memset(w->plan+scratch*D8POOL_BLOCK,255,D8POOL_BLOCK);
 memcpy(w->plan+scratch*D8POOL_BLOCK+D8POOL_HEADER,w->stage.wire,d8ls.length);
 pt_extra_set(PT_SCRATCH,scratch);pt_extra_set(PT_COMMITTED,d8p1_crc32(w->plan,D8POOL_BYTES));d8ls.previous_name[12]=0;
 if(!pt_lifetime())return PT_STALE;
 d8ls.object=PT_CONSENT;d8ls.shown=0;ui.force=1;return PT_OK;
}
static int pt_write(const uint8_t*a,uint32_t n,uint32_t*ack)
{
 if(pt_phase()!=PT_RECEIVE||!pt_lifetime())return PT_STALE;
 if(n<15||a[7]>1)return PT_BAD;uint32_t off;
 if(!mc_wire_u32(a+8,&off))return PT_BAD;
 uint32_t len=ed_unpack7(a+13,n-13,ed_smp_buf,256),total=a[7]?d8ls.length:D8POOL_BYTES;
 if(!len||off>total||len>total-off||(off&255u)||len!=(total-off>256?256:total-off))return PT_BAD;
 unsigned field=a[7]?PT_LIVERECV:PT_POOLRECV;uint32_t received=pt_word(field);
 if(a[7]&&pt_word(PT_POOLRECV)!=D8POOL_BYTES)return PT_BAD;
 d8mp_workspace*w=pt_workspace();uint8_t*dest=a[7]?w->stage.wire:w->plan;
 if(off<received){if(off+len!=received||memcmp(dest+off,ed_smp_buf,len))return PT_BAD;}
 else{if(off!=received)return PT_BAD;memcpy(dest+off,ed_smp_buf,len);pt_set(field,received+len);}
 *ack=off+len;return PT_OK;
}
static int pt_driver(unsigned program,uint32_t a,const void*p,uint32_t n)
{
 /* Long source CRCs run with IRQs ENABLED. The private synchronous writer
  * rejects handler reentry; only exact page/header checks enter the fence. */
 d8mp_workspace*w=pt_workspace();const uint8_t*wire=w?w->stage.wire:NULL;
 if(!pt_lifetime()||fm1_lcd_timeouts!=d8ls.shown||!d8ls.writing||d8ls.block>=5||d8ls.previous_slot>=4||d8ls.busy||
    (d8ls.length&&d8p1_crc32(wire,d8ls.length)!=d8ls.payload_crc))return -10;
 uint32_t base=d8pool_mapped_address(d8ls.block),f=irq_save(),took=0;int rc=-10;unsigned allowed=0;
 if(pt_lifetime()&&d8ls.writing&&!d8ls.busy&&d8ls.own!=UINT32_MAX&&d8_capture_change.epoch!=UINT32_MAX&&
    FL_STORE_OK(a,n)&&a>=base&&a-base<D8POOL_BLOCK&&n<=D8POOL_BLOCK-(a-base)){
  if(!program)allowed=d8ls.write_phase<2&&a==base+d8ls.write_phase*4096u&&n==4096;
  else if(d8ls.write_phase==2&&a==base&&n==32&&p){const uint8_t*h=p;
   allowed=d8ls.offset==256+d8ls.length&&mc_get32(h)==D8POOL_MAGIC&&h[4]==(d8ls.length?1:2)&&h[5]==d8ls.block&&!h[6]&&!h[7]&&
    mc_get32(h+8)==d8ls.previous_slot&&mc_get32(h+12)==d8ls.sequence&&mc_get32(h+16)==d8ls.length&&
    mc_get32(h+20)==d8ls.payload_crc&&mc_get32(h+24)==~0u&&mc_get32(h+28)==d8p1_crc32(h,28);
  }else if(program&&d8ls.write_phase==2){uint32_t expected=d8ls.length-(d8ls.offset-256);if(expected>256)expected=256;
   allowed=expected&&n==expected&&a==base+d8ls.offset&&p==wire+d8ls.offset-256;
  }
  if(allowed){d8ls.busy=1;d8ls.attempted=1;d8_capture_store_changed();d8ls.own++;
   rc=program?FL_FAR(fl_prog_ram)(a,p,n,&took):FL_FAR(fl_erase4k_ram)(a,&took);
   if(!pt_lifetime())rc=-1;
   if(!rc){if(!program)d8ls.write_phase++;else if(a==base)d8ls.write_phase=3;else d8ls.offset+=n;}
   d8ls.busy=0;
  }
 }irq_restore(f);return rc;
}
static int pt_erase(void*c,uint32_t a)
{(void)c;return pt_driver(0,d8pool_mapped_address(a/D8POOL_BLOCK)+a%D8POOL_BLOCK,NULL,4096);}
static int pt_program(void*c,uint32_t a,const void*p,uint32_t n)
{(void)c;return pt_driver(1,d8pool_mapped_address(a/D8POOL_BLOCK)+a%D8POOL_BLOCK,p,n);}
/* Only the synchronous completed readback/adoption path can expose this
 * narrow binding interval. No token/packet sets this state; mode2 repair is
 * not an ordinary READY grant. It retires immediately after actual binding. */
static int d8pt_binding(void)
{
 return d8ls.operation==4&&d8ls.phase==LS_EXECUTE&&pt_phase()==PT_APPROVED&&
  !d8ls.signature&&!migration_owner&&!d8ls.revoked&&d8ls.intent&&d8ls.write_phase==3&&pt_quiet()&&
  usb.resets==d8ls.usb&&usb.config==d8ls.usb_config&&d8_capture_change.epoch==d8ls.epoch+d8ls.own&&
  fm1_lcd_timeouts==d8ls.shown&&(uint32_t)(fm1_ms-d8ls.started)<LS_DEADLINE_MS;
}
static int pt_adopt_admit(void)
{ return !migration_owner&&!d8ls.revoked&&pt_quiet()&&usb.resets==d8ls.usb&&usb.config==d8ls.usb_config&&
  d8_capture_change.epoch==d8ls.epoch+d8ls.own&&fm1_lcd_timeouts==d8ls.shown; }
static int pt_commit(void)
{
 if(d8ls.phase==LS_IDLE&&d8ls.operation==4&&pt_phase()==PT_DONE&&d8ls.write_phase==PT_OK)return PT_OK;
 if(pt_phase()!=PT_APPROVED||!pt_lifetime()||fm1_lcd_timeouts!=d8ls.sequence)return PT_STALE;
 d8mp_workspace*w=pt_workspace();uint32_t live_len=d8ls.length,live_crc=d8ls.payload_crc;
 if(d8p1_crc32(w->plan,D8POOL_BYTES)!=pt_extra(PT_COMMITTED)||!pt_live()||d8p1_crc32(pt_live(),live_len)!=live_crc)return PT_BAD;
 d8pool source_reader={w,pt_plan_read,pt_noerase,pt_noprogram,pt_stopped},dest={NULL,pt_read,pt_erase,pt_program,pt_stopped};
 d8pool_index source,check;int rc=d8pool_inventory(&source_reader,&source);
 if(rc||source.present!=pt_extra(PT_PRESENCE))return PT_BAD;
 d8ls.writing=1; /* reject ALL handler reentry through scan, commit and adoption */
 uint32_t fresh_crc[5];rc=pt_scan(&dest,w,&check,0,7,fresh_crc);
 if(rc||memcmp(fresh_crc,d8ls.crc,sizeof fresh_crc)||!d8pr_same_index(&check,&d8ls.index)||!pt_lifetime()){d8ls.writing=0;return PT_STALE;}
 d8ls.shown=d8ls.sequence; /* immutable visible-display fault receipt */
 for(unsigned object=0;object<4;object++){
  if(d8pool_inventory(&dest,&check)){rc=D8POOL_INVALID;break;}
  if(!(source.present&(1u<<object))&&!(check.present&(1u<<object)))continue;
  unsigned used=0;for(unsigned o=0;o<4;o++)if(check.anchors&(1u<<o))used|=1u<<check.object[o].block;
  unsigned block=0;while(block<5&&(used&(1u<<block)))block++;if(block==5){rc=D8POOL_INVALID;break;}
  const d8pool_record*r=&source.object[object];d8ls.length=(source.present&(1u<<object))?r->length:0;
  if(d8ls.length)memcpy(w->stage.wire,w->plan+r->block*D8POOL_BLOCK+D8POOL_HEADER,d8ls.length);
  d8ls.payload_crc=d8ls.length?r->crc:0;d8ls.previous_slot=(uint8_t)object;d8ls.block=(uint8_t)block;
  d8ls.sequence=(check.anchors&(1u<<object))?check.object[object].sequence+1u:1u;
  d8ls.offset=256;d8ls.write_phase=0;d8ls.writing=1;
  rc=d8ls.length?d8pool_save(&dest,object,w->stage.wire,d8ls.length,source.present&7u):d8pool_remove(&dest,object);
  if(rc||!pt_lifetime()||d8ls.write_phase!=3){if(!rc)rc=D8POOL_IO;break;}
  d8ls.previous_name[12]|=(char)(1u<<object);
 }
 d8ls.length=live_len;d8ls.payload_crc=live_crc;
 if(!rc){rc=pt_scan(&dest,w,&check,0,15,NULL);if(!rc&&check.present!=source.present)rc=D8POOL_INVALID;}
 if(!rc)for(unsigned o=0;o<4;o++)if(source.present&(1u<<o)){
  const d8pool_record*r=&source.object[o],*got=&check.object[o];
  if(got->length!=r->length||got->crc!=r->crc||pt_read(NULL,got->block*D8POOL_BLOCK+D8POOL_HEADER,w->stage.wire,got->length)||
     memcmp(w->stage.wire,w->plan+r->block*D8POOL_BLOCK+D8POOL_HEADER,r->length)){rc=D8POOL_INVALID;break;}
 }
 if(!rc&&(d8p1_crc32(w->plan,D8POOL_BYTES)!=pt_extra(PT_COMMITTED)||!pt_lifetime()))rc=D8POOL_BUSY;
 if(rc){pt_finish(PT_IO);return PT_IO;}
 memcpy(w->stage.wire,pt_live(),live_len);main_migration_end(d8ls.signature);d8ls.signature=0;
 /* No intervening drawing may borrow wire before actual live adoption. */
 rc=d8p1_load_runtime_mask_guarded(w->stage.wire,live_len,source.present&7u,D8ENGINE_EFFECTIVE_MASK,pt_adopt_admit);
 d8ls.intent=1;
 if(!rc)rc=project_native_bind_flash(1);
 if(!rc)rc=nas_begin(d8ls.intent,0,0); /* retain independently restored AS */
 if(rc){pt_finish(PT_IO);return PT_IO;}
 d8ls.writing=0;d8ls.phase=LS_IDLE;d8ls.object=PT_DONE;d8ls.shown=0;d8ls.write_phase=PT_OK;ui.force=1;return PT_OK;
}
static void pt_pack(const uint8_t*p,uint32_t n)
{while(n){unsigned k=n>7?7:n,mask=0;for(unsigned i=0;i<k;i++)mask|=(p[i]>>7)<<i;ed_b(mask);for(unsigned i=0;i<k;i++)ed_b(p[i]);p+=k;n-=k;}}
static void d8pt_handle(const uint8_t*a,uint32_t n)
{
 unsigned op=n>1?a[1]:0,rc=PT_BAD,selector=0;uint32_t token=0,off=0,total=0,crc=0,count=0;const uint8_t*data=NULL;
 if(n>=2&&a[0]==1&&!d8ls.writing&&!d8ls.busy){
  if(op==0&&n==2){rc=PT_OK;total=D8POOL_BYTES;}
  else if(op==1&&n==2)rc=pt_begin(0,a,n);
  else if(op==2)rc=pt_begin(1,a,n);
  else if(n>=7&&mc_wire_u32(a+2,&token)&&token==d8ls.generation&&d8ls.operation==4){
   if(op==8&&n==7){if(d8ls.phase!=LS_IDLE)pt_finish(PT_CANCELLED);d8ls.object=PT_DONE;rc=PT_OK;}
   else if(op==7&&n==7)rc=pt_commit();
   else if(op==6&&n==7){if(d8ls.phase!=LS_IDLE)d8pt_poll();rc=d8ls.phase==LS_IDLE?d8ls.write_phase:PT_OK;}
   else if(op==5&&n==7)rc=pt_prepare();
   else if(op==4&&n>=15){selector=a[7];rc=pt_write(a,n,&off);total=selector==1?d8ls.length:D8POOL_BYTES;crc=selector==1?d8ls.payload_crc:pt_word(PT_POOLCRC);}
   else if(op==3&&n==15&&pt_phase()==PT_EXPORT&&pt_lifetime()&&a[7]<6&&mc_wire_u32(a+8,&off)){
    selector=a[7];count=a[13]|(uint32_t)a[14]<<7;d8mp_workspace*w=pt_workspace();
    if(selector<2){total=selector?d8ls.length:D8POOL_BYTES;crc=selector?d8ls.payload_crc:pt_word(PT_POOLCRC);data=selector?w->stage.wire:w->plan;rc=PT_OK;}
    else{d8pool reader={w,pt_plan_read,pt_noerase,pt_noprogram,pt_stopped};d8pool_index index;rc=d8pool_inventory(&reader,&index)?PT_BAD:PT_OK;
     if(!rc&&(index.present&(1u<<(selector-2)))){const d8pool_record*r=&index.object[selector-2];total=r->length;crc=r->crc;data=w->plan+r->block*D8POOL_BLOCK+D8POOL_HEADER;}}
    if(off>total||count>256||count>total-off)rc=PT_BAD;if(!rc&&data)data+=off;
   }else rc=PT_STALE;
  }else rc=PT_STALE;
 }else if(d8ls.writing||d8ls.busy)rc=PT_BUSY;
 if(op==1||op==2){token=d8ls.generation;total=D8POOL_BYTES;crc=pt_word(PT_POOLCRC);}
 if(op==6&&d8ls.phase!=LS_IDLE){selector=pt_word(PT_POOLRECV)<D8POOL_BYTES?0:1;off=pt_word(selector?PT_LIVERECV:PT_POOLRECV);total=selector?d8ls.length:D8POOL_BYTES;crc=selector?d8ls.payload_crc:pt_word(PT_POOLCRC);}
 ed_b(1);ed_b(op);ed_b(rc);ed_bk_u32(token);ed_b(op==0?0:pt_phase());ed_b(selector);ed_bk_u32(off);ed_bk_u32(total);ed_bk_u32(crc);
 if(!rc&&op==0){ed_b(15);ed_b(3);ed_b(3);ed_bk_u32(D8POOL_BYTES);ed_bk_u32(D8P1_LIMIT);ed_bk_u32(256);ed_b(2);}
 else if(!rc&&op==1){ed_b(pt_extra(PT_PRESENCE));ed_bk_u32(d8ls.length);ed_bk_u32(d8ls.payload_crc);ed_bk_u32(pt_word(PT_POOLCRC));}
 else if(op==6){ed_b(pt_extra(PT_PRESENCE));ed_b((uint8_t)d8ls.previous_name[12]);}
 else if(!rc&&op==3&&count&&data)pt_pack(data,count);
}
#endif
