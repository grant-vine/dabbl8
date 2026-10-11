/* SPDX-License-Identifier: GPL-3.0-only */
/* Include after actual capture, source equivalence and preflight. Dormant:
 * production write quarantine remains closed. No flash callback or adoption. */
#include "native_migration_capture_binding.h"
static struct {
 d8sv_io source_io;
 uint32_t generation,token,usb,epoch,last,current_length,crc[EDC_TOTAL],plan_crc;
 uint8_t phase,choice;
} d8cb __attribute__((section(".pool")));
enum { CB_IDLE,CB_START,CB_RECEIVE,CB_CHECK,CB_CHECKED,CB_SEALED,CB_FAILED };
static int d8cb_alias(const void *p,size_t n)
{ return migration_alias(p,n)||d8mp_metadata_alias(p,n)||d8ps_overlap(p,n,&d8cb,sizeof d8cb)||d8ps_overlap(p,n,&ed_capture,sizeof ed_capture)||d8ps_overlap(p,n,&d8sv,sizeof d8sv)||d8ps_overlap(p,n,&d8_capture_change,sizeof d8_capture_change)||d8ps_overlap(p,n,&usb,sizeof usb)||d8ps_overlap(p,n,&fm1_ms,sizeof fm1_ms); }
static int d8cb_quiet(void)
{ return flash_ok&&usb.config==1&&!usb.ota_req&&!usb.uboot_req&&!cv_cpu_active&&!transport_busy()&&!transport_req&&!seq_counting()&&!d8_capture_change.changed&&d8_capture_change.epoch==d8cb.epoch&&usb.resets==d8cb.usb&&(uint32_t)(fm1_ms-d8cb.last)<=EDC_TIMEOUT; }
static int d8cb_guard(uint32_t g)
{
 if(!g||g!=d8cb.generation||!main_migration_workspace(g)||d8mp.generation!=g)return D8CB_STALE;
 if(!d8cb_quiet())return D8CB_CHANGED;
 return D8CB_OK;
}
static int d8cb_failed(int rc){d8cb.phase=CB_FAILED;return rc;}
static void d8cb_cancel(void)
{
 uint32_t g=d8cb.generation;
 if(g&&d8sv.generation==g)d8sv_cancel();
 if(g&&main_migration_workspace(g)&&d8mp.generation==g)(void)d8mp_end(g);
 memset(&d8cb,0,sizeof d8cb);
}
static int d8cb_capture_matches(void)
{
 return ed_capture.active&&ed_capture.full&&ed_capture.phase==4&&ed_capture.token==d8cb.token&&ed_capture.usb==d8cb.usb&&ed_capture.epoch==d8cb.epoch&&ed_capture.live_length==d8cb.current_length&&!memcmp(ed_capture.crc,d8cb.crc,sizeof d8cb.crc);
}
/* Private actual coherent capture into existing owned state; no second buffer.
 * This closes the gap between an old completed capture and a new BEGIN. */
static int d8cb_current_matches(void)
{
 d8mp_workspace *w=main_migration_workspace(d8cb.generation);int rc=d8cb_guard(d8cb.generation);if(rc)return rc;
 fm1_irq_off();
 if(!d8cb_quiet()||main_migration_workspace(d8cb.generation)!=w){fm1_irq_on();return D8CB_CHANGED;}
 project_capture(&w->stage.state.project);
 if(d8p1_runtime_cache.valid)memcpy(&w->stage.state.arrangement,&d8p1_runtime_cache.arrangement,sizeof w->stage.state.arrangement);else memset(&w->stage.state.arrangement,0,sizeof w->stage.state.arrangement);
 fm1_irq_on();
 size_t n=0;if(!d8p1_project_encode(w->stage.wire,sizeof w->stage.wire,&n,&w->stage.state)||n!=d8cb.current_length||d8p1_crc32(w->stage.wire,n)!=d8cb.crc[EDC_LIVE])return D8CB_CHANGED;
 return d8cb_guard(d8cb.generation);
}
static int d8cb_begin(uint32_t *out)
{
 if(!out||d8cb_alias(out,sizeof *out))return D8CB_BAD;
 if(d8cb.phase!=CB_IDLE||d8sv.phase!=SV_IDLE)return D8CB_BUSY;
 if(!edc_valid()||!ed_capture.full||ed_capture.phase!=4)return D8CB_CHANGED;
 d8cb.phase=CB_START;
 d8cb.token=ed_capture.token;d8cb.usb=ed_capture.usb;d8cb.epoch=ed_capture.epoch;d8cb.last=ed_capture.last;d8cb.current_length=ed_capture.live_length;memcpy(d8cb.crc,ed_capture.crc,sizeof d8cb.crc);
 /* Actual RAM roles are rechecked before canvas acquisition. Completion alone
  * does not freeze settings, banks, FM6 or CURRENT against later RAM edits. */
 for(unsigned role=EDC_RAW;role<EDC_TOTAL;role++)if(edc_ram(role,0,0,0)!=EDC_OK){d8cb.phase=CB_FAILED;return D8CB_CHANGED;}
 if(!edc_valid()||!d8cb_capture_matches())return d8cb_failed(D8CB_CHANGED);
 uint32_t g=0;int rc=d8mp_begin(&g);
 if(rc)return d8cb_failed(rc==D8MP_BUSY?D8CB_BUSY:D8CB_BAD);
 d8cb.generation=g;
 /* lcd_sync may have changed the source while acquiring the arena. Never
  * silently start a new capture identity at the transfer boundary. */
 if(!d8cb_capture_matches()||(rc=d8cb_guard(g))||d8cb_current_matches()!=D8CB_OK){d8cb_cancel();return D8CB_CHANGED;}
 for(unsigned role=13;role<EDC_TOTAL;role++)if(st_crc32(edc_current(role),edc_length(role))!=d8cb.crc[role]){d8cb_cancel();return D8CB_CHANGED;}
 if((rc=d8cb_guard(g))){d8cb_cancel();return rc;}
 ed_capture.active=0;d8cb.phase=CB_RECEIVE;*out=g;return D8CB_OK;
}
static int d8cb_receive(uint32_t g,uint32_t off,const void *p,uint32_t n)
{
 if(d8cb.phase!=CB_RECEIVE)return D8CB_BAD;
 int rc=d8cb_guard(g);if(rc)return d8cb_failed(rc);
 rc=d8mp_receive(g,off,p,n);if(rc)return rc==D8MP_STALE?D8CB_STALE:D8CB_BAD;
 return d8cb_guard(g);
}
static int d8cb_source_read(void *context,uint32_t a,void *p,uint32_t n)
{(void)context;return d8cb.source_io.read(d8cb.source_io.context,a,p,n);}
static int d8cb_source_valid(void *context,uint32_t g)
{
 (void)context;
 if(d8cb.phase!=CB_CHECK||d8cb_guard(g)!=D8CB_OK)return 0;
 if(!d8cb.source_io.valid(d8cb.source_io.context,g))return 0;
 return d8cb.phase==CB_CHECK&&d8cb_guard(g)==D8CB_OK;
}
static int d8cb_source_begin(uint32_t g,const d8sv_io *io,unsigned choice)
{
 if(d8cb.phase!=CB_RECEIVE||!io||!io->read||!io->valid||d8cb_alias(io,sizeof *io)||choice>D8SV_CURRENT)return D8SV_BAD;
 if(d8cb_guard(g)!=D8CB_OK)return d8cb_failed(D8CB_CHANGED),D8SV_CHANGED;
 if(d8mp.phase!=1||d8mp.received!=D8POOL_BYTES)return D8SV_BAD;
 d8cb.source_io=*io;d8cb.choice=(uint8_t)choice;d8cb.phase=CB_CHECK;
 d8sv_io wrapped={NULL,d8cb_source_read,d8cb_source_valid};int rc=d8sv_begin(g,&wrapped,d8cb.crc,choice);
 if(rc!=D8SV_MORE)d8cb.phase=CB_FAILED;return rc;
}
static int d8cb_source_step(void)
{
 if(d8cb.phase!=CB_CHECK)return D8SV_BAD;
 int rc=d8sv_step();
 if(rc==D8SV_COMPLETE){
  if(d8cb_guard(d8cb.generation)!=D8CB_OK||memcmp(d8sv.original,d8cb.crc,sizeof d8sv.original)||(d8cb.choice==D8SV_CURRENT&&(d8sv.live_length!=d8cb.current_length||d8sv.live_crc!=d8cb.crc[EDC_LIVE]))){d8cb.phase=CB_FAILED;return D8SV_CHANGED;}
  d8cb.phase=CB_CHECKED;
 }else if(rc!=D8SV_MORE)d8cb.phase=CB_FAILED;
 return rc;
}
static int d8cb_seal(uint32_t g)
{
 if(d8cb.phase!=CB_CHECKED)return D8CB_BAD;
 int rc=d8cb_guard(g);if(rc)return d8cb_failed(rc);
 /* completed() calls valid(), which must still see the checking lifetime. */
 d8cb.phase=CB_CHECK;rc=d8sv_completed(g);d8cb.phase=CB_CHECKED;
 if(rc!=D8SV_COMPLETE||d8cb_guard(g)!=D8CB_OK||(d8cb.choice==D8SV_CURRENT&&(d8sv.live_length!=d8cb.current_length||d8sv.live_crc!=d8cb.crc[EDC_LIVE])))return d8cb_failed(D8CB_CHANGED);
 d8cb.plan_crc=d8sv.plan_crc;d8cb.phase=CB_SEALED;return D8CB_OK;
}
/* Deliberately no stage access/recapture/CRC here: safe across phase4 readback.
 * This is necessary context, NEVER sufficient permission. Trusted coordinator
 * must exclude all edits and serialize observed real driver writers, retain
 * originals independently and obtain consent before any actual write grant. */
static int d8cb_metadata_valid(uint32_t g)
{ return d8cb.phase==CB_SEALED&&d8cb_guard(g)==D8CB_OK&&d8mp.received==D8POOL_BYTES&&(d8mp.phase==1||d8mp.phase==4); }
