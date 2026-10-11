/* SPDX-License-Identifier: GPL-3.0-only */
/* Connected main-loop workflow. External retention is a host claim checked by
 * the dedicated collector, never a firmware grant. Shipping writes remain
 * quarantined; the private virtual-NOR fixture exercises the complete driver. */
#if NTRK == 8 && D8_MIGRATION_SESSION
#ifndef D8_MC_ENCODER_VALUE
#define D8_MC_ENCODER_VALUE(i) (fm1_in.enc_steps[i])
#endif
#ifndef D8_MC_PENDING_PRESS
#define D8_MC_PENDING_PRESS() (fm1_in.pressed||fm1_in.notes_pressed)
#endif
#define MC_IDLE_MS 15000u
#define MC_HARD_MS 120000u
static struct {
 uint32_t generation,token,request,started,last,epoch,usb,own,offset,rolling;
 d8pool_index verified_index;
 uint32_t verified_crc[5],reread_offset,reread_crc;
 uint32_t original[17],plan_crc,write_start,write_last,write_offset,write_sequence;
 uint8_t write_active,write_block,write_object,write_phase,driver_busy,write_attempted;
 uint8_t phase,resume,choice,attempted,status;
} d8mc __attribute__((section(".pool")));
enum { MC_IDLE,MC_RECEIVE,MC_RESUME_READ,MC_PROOF,MC_CONSENT,MC_RELEASE_BUTTON,
       MC_EXECUTE,MC_REREAD,MC_ADOPT,MC_READY,MC_FAILED };
enum { MC_OK,MC_BAD,MC_STALE,MC_CHANGED,MC_BUSY,MC_UNSUPPORTED,MC_QUARANTINED };
static int mc_inputs_quiet(void)
{
 if(fm1_in.notes||fm1_in.buttons||D8_MC_PENDING_PRESS())return 0;
 for(unsigned i=0;i<7;i++)if(D8_MC_ENCODER_VALUE(i))return 0;
 return 1;
}
static int mc_lifetime(void)
{
 return !d8mc_edit.revoked&&(!d8mc.write_active||!d8mc_edit.operation_revoked)&&flash_ok&&usb.config==1&&!usb.ota_req&&!usb.uboot_req&&
 usb.resets==d8mc.usb&&d8_capture_change.epoch==d8mc.epoch+d8mc.own&&
 (!d8_capture_change.changed||d8mc.own)&&
 (uint32_t)(fm1_ms-((d8mc.write_active||d8mc_edit.writer)?d8mc.write_start:d8mc.started))<=MC_HARD_MS&&
 (uint32_t)(fm1_ms-((d8mc.write_active||d8mc_edit.writer)?d8mc.write_last:d8mc.last))<=MC_IDLE_MS;
}
static int mc_quiet(void)
{ return !cv_cpu_active&&!transport_busy()&&!transport_req&&!seq_counting()&&
         df_automatic_quiet()&&mc_inputs_quiet(); }
static void mc_offline(void)
{ project_native.mode=2;project_native.ready=0;d8p1_autosave_session_end(); }
static void mc_fail(unsigned status)
{
 if(d8mc.attempted||d8mc.phase==MC_ADOPT)mc_offline();
 d8mc.status=(uint8_t)status;d8mc.phase=MC_FAILED;
 d8cb_cancel();d8mx_cancel();d8mc_edit.locked=0;
}
static void mc_progress(void)
{
 d8mc.last=fm1_ms;
 /* A lease refresh is not an identity/epoch reset. Only actual ordered input
  * or completed bounded source/executor progress reaches this function. */
 d8cb.last=fm1_ms;d8sv.last=fm1_ms;
}
static int mc_read(void *context,uint32_t a,void *p,uint32_t n)
{
 (void)context;if(!n||n>256||!mc_lifetime())return -1;
 for(unsigned role=0;role<EDC_RAW;role++){
  uint32_t base=edc_roles[role].a,length=edc_roles[role].n;
  if(a>=base&&a-base<=length&&n<=length-(a-base))return st_read(a,p,n);
  base=edc_roles[role].b;length=edc_roles[role].m;
  if(length&&a>=base&&a-base<=length&&n<=length-(a-base))return st_read(a,p,n);
 }
 return -1;
}
static int mc_valid(void *context,uint32_t g)
{ (void)context;return g==d8mc.generation&&mc_lifetime()&&mc_quiet(); }
static int mc_permit(void *context,uint32_t g)
{
 (void)context;
 return d8mc.phase==MC_EXECUTE&&g==d8mc.generation&&mc_lifetime()&&mc_quiet()&&
 main_migration_workspace(g)&&d8mp.generation==g&&d8mp.phase==4&&
 d8mx.plan_crc==d8mc.plan_crc;
}
/* No shipping macro supplies a RAM writer. These names are only supplied by
 * the private test translation unit's virtual NOR, never by a runtime setter. */
static int mc_write_enabled(void)
{
#ifdef D8_MC_VIRTUAL_NOR_FIXTURE
 return 1;
#else
 return 0;
#endif
}
static int mc_native_address(uint32_t a,uint32_t n)
{
 for(unsigned b=0;b<5;b++){uint32_t base=d8pool_mapped_address(b);
  if(a>=base&&a-base<=D8POOL_BLOCK&&n<=D8POOL_BLOCK-(a-base))return 1;
 }
 return 0;
}
static uint32_t mc_get32(const uint8_t*p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static int mc_ticket(unsigned operation,uint32_t a,const void *p,uint32_t n)
{
 if(!mc_write_enabled()||!mc_lifetime()||!mc_quiet())return 0;
 if(d8mc.phase==MC_EXECUTE){
  if(!mc_permit(NULL,d8mc.generation))return 0;
  uint32_t target=d8mx_target();
  if(!operation)return d8mx.phase==MX_ERASE&&a==target&&n==4096;
  d8mp_workspace*w=main_migration_workspace(d8mc.generation);
  uint32_t logical=d8mx.block*D8POOL_BLOCK+d8mx.sector*4096u;
  uint32_t length=d8mx.phase==MX_HEADER?32:256u-(d8mx.offset&255u);
  return (d8mx.phase==MX_HEADER||d8mx.phase==MX_PAYLOAD)&&a==target+d8mx.offset&&
    n==length&&p==w->plan+logical+d8mx.offset;
 }
 /* Existing native mapped writers still perform their own capture/ref/commit
  * validation. Session recognition alone cannot reach a shipping RAM writer. */
 if(d8mc.phase!=MC_READY||!d8mc.write_active||!mc_native_address(a,n))return 0;
 uint32_t base=d8pool_mapped_address(d8mc.write_block);
 if(!operation)return d8mc.write_phase<2&&a==base+d8mc.write_phase*4096u&&n==4096;
 if(d8mc.write_phase!=2)return 0;
 if(a==base&&n==32){
  const uint8_t*h=p;d8p1_view view;
  return p&&mc_get32(h)==D8POOL_MAGIC&&h[4]==1&&h[5]==d8mc.write_block&&!h[6]&&!h[7]&&
    mc_get32(h+24)==UINT32_MAX&&
    mc_get32(h+8)==d8mc.write_object&&mc_get32(h+12)==d8mc.write_sequence&&
    mc_get32(h+16)==d8mc.write_offset-256&&mc_get32(h+20)==d8p1_crc32(main_workspace.d8p1.wire,d8mc.write_offset-256)&&
    mc_get32(h+28)==d8p1_crc32(h,28)&&
    d8p1_read(&view,main_workspace.d8p1.wire,d8mc.write_offset-256)==1;
 }
 return a==base+d8mc.write_offset&&n&&n<=256&&(a&255u)+n<=256&&
    d8mc.write_offset+n<=D8POOL_BLOCK&&p==main_workspace.d8p1.wire+d8mc.write_offset-256;
}
static int mc_driver(unsigned operation,uint32_t a,const void *p,uint32_t n)
{
 /* Exactly one IRQ interval. No nested st_*/
 uint32_t flags=irq_save();int rc=-10;
 if(!d8mc.driver_busy&&mc_ticket(operation,a,p,n)){
  d8mc.driver_busy=1;
  if(d8mc.phase==MC_EXECUTE&&!d8mc.attempted)mc_offline();
  if(d8mc.write_active)d8mc.write_attempted=1;else d8mc.attempted=1;
  /* Count the exact own mutation monotonically before leaving the driver
   * window. changed is never cleared; unrelated observations remain visible. */
  if(d8mc.own==UINT32_MAX){d8mc.driver_busy=0;irq_restore(flags);return -1;}
  d8_capture_store_changed();d8mc.own++;
#ifdef D8_MC_VIRTUAL_NOR_FIXTURE
  rc=operation?D8_MC_VIRTUAL_PROGRAM(a,p,n):D8_MC_VIRTUAL_ERASE(a);
#endif
  if(!mc_lifetime()||!mc_quiet()){d8mc_edit.revoked=1;rc=-1;}
  if(!rc&&d8mc.write_active){
   if(!operation)d8mc.write_phase++;
   else if(a==d8pool_mapped_address(d8mc.write_block))d8mc.write_phase=3;
   else d8mc.write_offset+=n;
   d8mc.write_last=fm1_ms;
  }
  d8mc.driver_busy=0;
 }
 irq_restore(flags);return rc;
}
static int d8mc_driver_erase(uint32_t a){return mc_driver(0,a,NULL,4096);}
static int d8mc_driver_program(uint32_t a,const void*p,uint32_t n){return mc_driver(1,a,p,n);}
static int mc_erase(void*c,uint32_t a){(void)c;return d8mc_driver_erase(a);}
static int mc_program(void*c,uint32_t a,const void*p,uint32_t n){(void)c;return d8mc_driver_program(a,p,n);}
static int d8mc_native_begin(unsigned object)
{
 if(object>=4)return D8POOL_INVALID;
 if(!d8mc_session_valid()||!mc_write_enabled())return D8POOL_UNSUPPORTED;
 if(d8mc.write_active||d8mc_edit.locked||d8mc_edit.busy||!mc_quiet())return D8POOL_BUSY;
 /* A fresh operation may clear only its own old cancellation, before any
  * callbacks. An input observed during preflight must never be forgotten. */
 d8mc_edit.operation_revoked=0;
 d8mc_edit.locked=d8mc_edit.busy=d8mc_edit.writer=1;d8mc.write_start=d8mc.write_last=fm1_ms;
 d8pool_mapped mapped;d8pool_physical io={NULL,df_read,df_erase,df_program,df_stopped};
 int rc=d8pool_mapped_open(&mapped,&io,1);d8pool_index index;
 if(!rc)rc=d8pool_inventory(&mapped.pool,&index);d8pool_mapped_close(&mapped);
 if(rc||d8mc_edit.operation_revoked||!mc_lifetime()||!d8mc_session_valid()||!mc_quiet()){d8mc_edit.locked=d8mc_edit.busy=d8mc_edit.writer=0;return rc?rc:D8POOL_BUSY;}
 unsigned used=0;for(unsigned o=0;o<4;o++)if(index.present&(1u<<o))used|=1u<<index.object[o].block;
 unsigned b=0;while(b<5&&(used&(1u<<b)))b++;
 if(b==5){d8mc_edit.locked=d8mc_edit.busy=d8mc_edit.writer=0;return D8POOL_INVALID;}
 d8mc.write_object=(uint8_t)object;d8mc.write_block=(uint8_t)b;d8mc.write_phase=0;
 d8mc.write_offset=256;d8mc.write_sequence=index.present&(1u<<object)?index.object[object].sequence+1u:1u;
 d8mc.write_attempted=0;d8mc.write_active=1;return D8POOL_OK;
}
static int d8mc_native_finish(int rc)
{
 if(!rc&&(!d8mc_session_valid()||!mc_lifetime()||!mc_quiet()||d8mc.write_phase!=3))rc=D8POOL_IO;
 if(rc&&!d8mc.write_attempted&&d8mc_session_valid()&&
    (d8mc_edit.operation_revoked||!mc_quiet()))rc=D8POOL_BUSY;
 d8mc.write_active=0;d8mc_edit.locked=d8mc_edit.busy=d8mc_edit.writer=0;
 if((rc&&d8mc.write_attempted)||!d8mc_session_valid()){mc_offline();d8mc.phase=MC_FAILED;d8mc.status=MC_CHANGED;}
 return rc;
}
static int d8mc_session_valid(void)
{
 return (d8mc.phase==MC_READY||d8mc.phase==MC_ADOPT)&&!d8mc_edit.revoked&&
 flash_ok&&usb.config==1&&!usb.ota_req&&!usb.uboot_req&&usb.resets==d8mc.usb&&
 d8_capture_change.epoch==d8mc.epoch+d8mc.own;
}
static void d8mc_input(uint32_t pressed,uint32_t notes)
{
 unsigned save=1u<<panel.btn[B_SAVE];
 if(notes||(pressed&~save))d8mc_revoke();
 if(d8mc.phase==MC_CONSENT&&(pressed&save)&&fm1_in.buttons==save&&!notes)
  d8mc.phase=MC_RELEASE_BUTTON;
 else if(pressed)d8mc_revoke();
 for(unsigned i=0;i<7;i++)if(D8_MC_ENCODER_VALUE(i))d8mc_revoke();
}
static int mc_resume_read(void*c,uint32_t off,void*p,uint32_t n)
{ (void)c;d8mp_workspace*w=main_migration_workspace(d8mc.generation);
  if(!w||(d8mp.phase!=1&&d8mp.phase!=4)||off>D8POOL_BYTES||n>D8POOL_BYTES-off||!mc_lifetime())return -1;
  memcpy(p,w->plan+off,n);return 0; }
static int mc_resume_stopped(void*c)
{ (void)c;return main_migration_workspace(d8mc.generation)&&(d8mp.phase==1||d8mp.phase==4)&&mc_lifetime()&&mc_quiet(); }
static int mc_resume_validate(void)
{
 d8mp_workspace*w=main_migration_workspace(d8mc.generation);
 if(!w||!mc_lifetime()||!mc_quiet())return 0;
 /* Rotating old records/spare are valid native recovery inputs, unlike a
  * canonical proposed initializer. Select actual latest inventory then decode
  * every selected object with the actual available-project mask. */
 d8pool pool={NULL,mc_resume_read,d8mp_noerase,d8mp_noprogram,mc_resume_stopped};d8pool_index index;
 if(d8pool_inventory(&pool,&index)||!(index.present&8u))return 0;
 unsigned mask=index.present&7u;
 /* Absence may not be inferred from damaged residue. Full known sets may
  * retain a torn shared spare; sparse sets require every other allocation
  * erased or a fully verified older record. */
 for(unsigned block=0;block<5;block++){
  const uint8_t*raw=w->plan+block*D8POOL_BLOCK;
  if(d8mp_erased(raw,D8POOL_BLOCK))continue;
  int valid=mc_get32(raw)==D8POOL_MAGIC&&raw[4]==1&&raw[5]==block&&!raw[6]&&!raw[7]&&
    mc_get32(raw+8)<4&&mc_get32(raw+16)&&mc_get32(raw+16)<=D8P1_LIMIT&&
    mc_get32(raw+24)==~0u&&mc_get32(raw+28)==d8p1_crc32(raw,28)&&
    mc_get32(raw+20)==d8p1_crc32(raw+D8POOL_HEADER,mc_get32(raw+16));
  if(!valid){if(index.present!=15)return 0;continue;}
  size_t encoded=0;unsigned n=mc_get32(raw+16);
  if(!d8p1_project_decode(&w->stage.state,raw+D8POOL_HEADER,n,mask)||
     !d8p1_project_encode(w->stage.wire,sizeof w->stage.wire,&encoded,&w->stage.state)||
     encoded!=n||memcmp(w->stage.wire,raw+D8POOL_HEADER,n))return 0;
 }
 for(unsigned object=0;object<4;object++)if(index.present&(1u<<object)){
  size_t n=0,encoded=0;
  if(d8pool_load(&pool,object,w->stage.wire,sizeof w->stage.wire,&n,mask)||
     !d8p1_project_decode(&w->stage.state,w->stage.wire,n,mask))return 0;
  const uint8_t*raw=w->plan+index.object[object].block*D8POOL_BLOCK+D8POOL_HEADER;
  if(!d8p1_project_encode(w->stage.wire,sizeof w->stage.wire,&encoded,&w->stage.state)||
     n!=encoded||memcmp(raw,w->stage.wire,n))return 0;
 }
 d8mc.plan_crc=d8p1_crc32(w->plan,D8POOL_BYTES);return mc_lifetime()&&mc_quiet();
}
static int mc_adoption_read(void*c,uint32_t a,void*p,uint32_t n)
{
 (void)c;uint8_t*out=p;
 if(!mc_native_address(a,n))return -1;
 while(n){uint32_t k=n>256?256:n;
  if(!d8mc_session_valid()||!mc_quiet()||st_read(a,out,k)||!d8mc_session_valid()||!mc_quiet())return -1;
  a+=k;out+=k;n-=k;
 }
 return 0;
}
static int mc_adoption_stopped(void*c)
{ (void)c;return d8mc_session_valid()&&mc_quiet(); }
static void mc_adopt_begin(void)
{
 if(!mc_lifetime()||!mc_quiet()){mc_fail(MC_CHANGED);return;}
 d8mp_workspace*w=main_migration_workspace(d8mc.generation);
 if(!w||d8p1_crc32(w->plan,D8POOL_BYTES)!=d8mc.plan_crc){mc_fail(MC_CHANGED);return;}
 d8pool pool={NULL,mc_resume_read,d8mp_noerase,d8mp_noprogram,mc_resume_stopped};
 if(d8pool_inventory(&pool,&d8mc.verified_index)||!(d8mc.verified_index.present&8u)){mc_fail(MC_CHANGED);return;}
 for(unsigned b=0;b<5;b++)d8mc.verified_crc[b]=d8p1_crc32(w->plan+b*D8POOL_BLOCK,D8POOL_BLOCK);
 d8mc.reread_offset=0;d8mc.reread_crc=~0u;d8mc.phase=MC_REREAD;
}
static void mc_adopt_finish(void)
{
 if(!mc_lifetime()||!mc_quiet()){mc_fail(MC_CHANGED);return;}
 /* Only disjoint sealed metadata crosses release. No stage/plan pointer.
  * Input exclusion remains active through actual flash reread and adoption. */
 d8mc.phase=MC_ADOPT;d8cb_cancel();d8mx_cancel();
 d8pool_mapped mapped;d8pool_physical io={NULL,mc_adoption_read,df_erase,df_program,mc_adoption_stopped};
 int rc=d8pool_mapped_open(&mapped,&io,1);d8pool_index actual;
 if(!rc)rc=d8pool_inventory(&mapped.pool,&actual);
 if(!rc&&actual.present!=d8mc.verified_index.present)rc=D8POOL_INVALID;
 for(unsigned o=0;!rc&&o<4;o++)if(actual.present&(1u<<o)){
  const d8pool_record*a=&actual.object[o],*b=&d8mc.verified_index.object[o];
  if(a->block!=b->block||a->sequence!=b->sequence||a->length!=b->length||a->crc!=b->crc)rc=D8POOL_INVALID;
 }
 if(!rc)rc=d8p1_load_pool(&mapped.pool,3);d8pool_mapped_close(&mapped);
 if(rc||!d8mc_session_valid()||!mc_quiet()||project_native_bind_flash(1)||
    !d8mc_session_valid()||d8p1_autosave_session_begin(1,0)){mc_fail(MC_CHANGED);return;}
 d8mc.phase=MC_READY;d8mc.status=MC_OK;d8mc_edit.locked=0;ui.force=1;
}
static void mc_poll_core(void)
{
 if(d8mc.phase==MC_IDLE||d8mc.phase==MC_FAILED)return;
 if(d8mc.phase==MC_READY){if(!d8mc_session_valid()){mc_offline();d8mc.phase=MC_FAILED;d8mc.status=MC_CHANGED;}return;}
 if(!mc_lifetime()){mc_fail(MC_CHANGED);return;}
 if(d8mc.phase==MC_RESUME_READ){
  if(!mc_quiet()){mc_fail(MC_CHANGED);return;}
  d8mp_workspace*w=main_migration_workspace(d8mc.generation);
  if(!w){mc_fail(MC_STALE);return;}
  unsigned role=d8mc.offset/D8POOL_BLOCK;uint32_t off=d8mc.offset%D8POOL_BLOCK;
  if(mc_read(NULL,edc_roles[role].a+off,ed_smp_buf,256)||!mc_lifetime()||
     d8cb_receive(d8mc.generation,d8mc.offset,ed_smp_buf,256)){mc_fail(MC_CHANGED);return;}
  d8mc.rolling=edc_crc_step(d8mc.rolling,ed_smp_buf,256);d8mc.offset+=256;
  if(d8mc.offset%D8POOL_BLOCK==0){
   if((d8mc.rolling^~0u)!=d8mc.original[role]){mc_fail(MC_CHANGED);return;}
   d8mc.rolling=~0u;
  }
  mc_progress();
  if(d8mc.offset==D8POOL_BYTES){if(!mc_resume_validate()){mc_fail(MC_UNSUPPORTED);return;}d8mc.phase=MC_CONSENT;}
 }else if(d8mc.phase==MC_PROOF){
  int rc=d8cb_source_step();
  if(rc==D8SV_MORE)mc_progress();
  else if(rc==D8SV_COMPLETE&&d8cb_seal(d8mc.generation)==D8CB_OK){
   d8mc.plan_crc=d8cb.plan_crc;mc_progress();d8mc.phase=MC_CONSENT;
  }else mc_fail(MC_CHANGED);
 }else if(d8mc.phase==MC_RELEASE_BUTTON){
  if(!mc_quiet())return; /* Consent never extends either deadline. */
  if(d8mc.resume){mc_adopt_begin();return;}
  if(!mc_write_enabled()){mc_fail(MC_QUARANTINED);return;}
  d8mc.phase=MC_EXECUTE;d8mx_io io={NULL,mc_read,mc_erase,mc_program,mc_permit};
  if(d8mx_begin(d8mc.generation,&io,d8mc.original)!=D8MX_MORE)mc_fail(MC_CHANGED);
 }else if(d8mc.phase==MC_REREAD){
  if(!mc_quiet()){mc_fail(MC_CHANGED);return;}
  unsigned b=d8mc.reread_offset/D8POOL_BLOCK;uint32_t within=d8mc.reread_offset%D8POOL_BLOCK;
  if(mc_read(NULL,d8pool_mapped_address(b)+within,ed_smp_buf,256)||!mc_lifetime()||!mc_quiet()){mc_fail(MC_CHANGED);return;}
  d8mc.reread_crc=edc_crc_step(d8mc.reread_crc,ed_smp_buf,256);d8mc.reread_offset+=256;
  if(d8mc.reread_offset%D8POOL_BLOCK==0){
   if((d8mc.reread_crc^~0u)!=d8mc.verified_crc[b]){mc_fail(MC_CHANGED);return;}
   d8mc.reread_crc=~0u;
  }
  mc_progress();if(d8mc.reread_offset==D8POOL_BYTES)mc_adopt_finish();
 }else if(d8mc.phase==MC_EXECUTE){
  int rc=d8mx_step();if(rc==D8MX_MORE)mc_progress();
  else if(rc==D8MX_COMPLETE){mc_progress();mc_adopt_begin();}
  else mc_fail(MC_CHANGED);
 }
}
static void d8mc_poll(void)
{ if(d8mc_edit.busy)return;d8mc_edit.busy=1;mc_poll_core();d8mc_edit.busy=0; }
static int mc_wire_u32(const uint8_t*a,uint32_t*n)
{
 if(a[4]>15)return 0;*n=0;for(unsigned i=0;i<5;i++)*n|=(uint32_t)a[i]<<(7*i);return 1;
}
static void mc_reply(unsigned status)
{ ed_b(1);ed_b(status);ed_b(d8mc.phase);ed_bk_u32(d8mc.generation);ed_bk_u32(d8mc.request);ed_bk_u32(d8mc.offset);ed_bk_u32(d8mc.own); }
static void mc_handle_core(const uint8_t*a,uint32_t n)
{
 unsigned status=MC_BAD;
 if(n>312||!a||migration_alias(a,n)||d8ps_overlap(a,n,ed_smp_buf,sizeof ed_smp_buf)||d8ps_overlap(a,n,&d8mc,sizeof d8mc)){mc_reply(status);return;}
 for(unsigned i=0;i<n;i++)if(a[i]>127){mc_reply(status);return;}
 if(n<2||a[0]!=1){mc_reply(status);return;}
 unsigned op=a[1];uint32_t g=0,request=0;
 if(op==1){
  uint32_t token=0;
  if(n!=10||!mc_wire_u32(a+2,&token)||a[7]>1||a[8]>1||(a[7]&&a[8]!=D8SV_PERSISTED)||a[9]!=1){mc_reply(status);return;}
  if(d8mc.phase!=MC_IDLE&&d8mc.phase!=MC_FAILED){mc_reply(MC_BUSY);return;}
  if(!edc_valid()||!ed_capture.full||ed_capture.phase!=4||ed_capture.token!=token||!mc_quiet()){mc_reply(MC_CHANGED);return;}
  memset(&d8mc,0,sizeof d8mc);d8mc_edit.revoked=0;d8mc_edit.locked=1;
  d8mc.started=d8mc.last=fm1_ms;d8mc.epoch=d8_capture_change.epoch;d8mc.usb=usb.resets;
  d8mc.token=token;d8mc.resume=a[7];d8mc.choice=a[8];d8mc.rolling=~0u;
  memcpy(d8mc.original,ed_capture.crc,sizeof d8mc.original);
  if(d8cb_begin(&g)!=D8CB_OK){mc_fail(MC_CHANGED);mc_reply(MC_CHANGED);return;}
  d8mc.generation=g;
  if(!mc_lifetime()||!mc_quiet()){mc_fail(MC_CHANGED);mc_reply(MC_CHANGED);return;}
  d8mc.phase=d8mc.resume?MC_RESUME_READ:MC_RECEIVE;mc_reply(MC_OK);return;
 }
 if(n<12||!mc_wire_u32(a+2,&g)||!mc_wire_u32(a+7,&request)){mc_reply(status);return;}
 if(!g||g!=d8mc.generation||!request||d8mc.request==UINT32_MAX||request!=d8mc.request+1){mc_reply(MC_STALE);return;}
 if(op==4&&n==12){d8mc.request=request;mc_fail(MC_CHANGED);mc_reply(MC_OK);return;}
 if(op==5&&n==12){d8mc.request=request;mc_reply(d8mc.status);return;} /* No lease refresh. */
 if(!mc_lifetime()){mc_fail(MC_CHANGED);mc_reply(MC_CHANGED);return;}
 if(op==2&&d8mc.phase==MC_RECEIVE&&n>=19){
  uint32_t off=0;unsigned count=a[17]|(unsigned)a[18]<<7;
  if(!mc_wire_u32(a+12,&off)||!count||count>256||n!=19+count+(count+6)/7){mc_reply(status);return;}
  unsigned pos=19,out=0;
  while(out<count){unsigned k=count-out;if(k>7)k=7;unsigned bits=a[pos++];if(bits>>k){mc_reply(status);return;}
   for(unsigned i=0;i<k;i++)ed_smp_buf[out++]=a[pos++]|((bits>>i&1)<<7);
  }
  if(d8cb_receive(g,off,ed_smp_buf,count)!=D8CB_OK){mc_reply(status);return;}
  d8mc.offset=d8mp.received;d8mc.request=request;mc_progress();status=MC_OK;
 }else if(op==3&&n==12){
  if(d8mc.phase==MC_RECEIVE){d8sv_io io={NULL,mc_read,mc_valid};
   int begun=d8cb_source_begin(g,&io,d8mc.choice);
   if(begun!=D8SV_MORE){mc_fail(begun==D8SV_UNSUPPORTED?MC_UNSUPPORTED:MC_CHANGED);mc_reply(d8mc.status);return;}
   d8mc.phase=MC_PROOF;
  }
  uint8_t before=d8mc.phase;uint32_t offset=d8mc.offset;
  mc_poll_core();
  if(d8mc.phase!=MC_CONSENT&&before==MC_CONSENT&&offset==d8mc.offset){mc_reply(status);return;}
  d8mc.request=request;status=d8mc.phase==MC_FAILED?d8mc.status:MC_OK;
 }
 mc_reply(status);
}
static void d8mc_handle(const uint8_t*a,uint32_t n)
{ if(d8mc_edit.busy){mc_reply(MC_BUSY);return;}d8mc_edit.busy=1;mc_handle_core(a,n);d8mc_edit.busy=0; }

#endif
