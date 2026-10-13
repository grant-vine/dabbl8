/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <setjmp.h>
static _Alignas(4) uint8_t nor[0x100000];
static unsigned completion_reads;
static int owner_io_result(int,uint32_t,uint32_t);
static int owner_read(uint32_t,void*,uint32_t);
#define D8_FI_TEST_RESULT(rc,a,n) owner_io_result(rc,a,n)
#define D8_FI_READ(a,p,n) owner_read(a,p,n)
static void owner_completion_read(void);
#define D8_LOCAL_COMPLETION_READ_HOOK owner_completion_read
#define SMP_USER_XIP(k) (nor+SMP_USER_BASE+(k)*SMP_USER_SIZE)
#define D8_FIRST_USE_COMPLETION_TEST 1
#define DABBL8_TIMING_DIAGNOSTIC 1
#define D8_OWNER_INITIALIZE 1
#define D8_FIRST_USE_SESSION_NO_MAIN 1
#include "native_first_use_session_test.c"
static int owner_raw_error,owner_read_error,owner_guard_after_write,owner_failstop;
static jmp_buf owner_stop;
static int owner_io_result(int rc,uint32_t a,uint32_t n)
{
 (void)n;CHECK(d8fi_diag.status==FI_DIAG_INFLIGHT&&d8fi_diag.address==a&&
  d8fi_diag.raw_rc==FI_DIAG_PENDING&&d8fi_diag.epoch+d8fi_diag.own==d8_capture_change.epoch);
 if(owner_failstop){owner_failstop=0;longjmp(owner_stop,1);}
 if(owner_guard_after_write){owner_guard_after_write=0;d8_capture_store_changed();usb.config=0;}
 if(owner_raw_error){int raw=owner_raw_error;owner_raw_error=0;return raw;}
 return rc;
}
static int owner_read(uint32_t a,void *p,uint32_t n)
{
 CHECK(d8fi_diag.status==FI_DIAG_INFLIGHT&&d8fi_diag.operation==FI_OP_READ&&d8fi_diag.address==a);
 if(owner_read_error){int raw=owner_read_error;owner_read_error=0;return raw;}
 return st_read(a,p,n);
}
static char owner_report[2048];static unsigned owner_report_n;
static void con_kv(const char *k,int32_t v)
{owner_report_n+=(unsigned)snprintf(owner_report+owner_report_n,sizeof owner_report-owner_report_n,"%s %d\n",k,v);}
static void con_kx(const char *k,uint32_t v)
{owner_report_n+=(unsigned)snprintf(owner_report+owner_report_n,sizeof owner_report-owner_report_n,"%s 0x%08x\n",k,v);}
#include "../firmware/src/native_first_use_diagnostic.h"
static void owner_report_fields(const char *scenario)
{
 owner_report_n=0;con_first_use();CHECK(owner_report_n<sizeof owner_report);
 printf("scenario=%s\n%s",scenario,owner_report);
}
static void owner_completion_read(void)
{
 if(d8fi.phase!=FI_EXECUTE||d8fi.format.phase!=4)return;
 if(!completion_reads){up_bank[0].magic=up_bank[1].magic=upf.magic=0;}
 completion_reads++;
}
static void owner_cold(void)
{
 if(migration_owner)main_migration_end(migration_owner);
 cold();d8fi_diag_boot(0,0,0);project_native_reset();
 owner_raw_error=owner_read_error=owner_guard_after_write=owner_failstop=0;native_as.ready=0;d8ls_reset();
 memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);
 memset(&d8_capture_change,0,sizeof d8_capture_change);
 usb.up=usb.suspended=0;usb.config=0;usb.ota_req=usb.uboot_req=0;flash_ok=1;
 reset();physical_calls=completion_reads=0;
 host_pressed=host_notes=0;fm1_in.buttons=fm1_in.notes=0;fm1_ms=0;
 memset(host_enc,0,sizeof host_enc);memset(host_enc_late,0,sizeof host_enc_late);
 int32_t audio[CTL*2];mix_block(audio,CTL);
}
static void owner_fresh(void)
{owner_cold();usb.up=1;usb.config=1;memset(nor,0xa5,sizeof nor);CHECK(d8fi_boot_classify()&&d8fi.phase==FI_REQUIRED);}
static void owner_frame(unsigned frame)
{
 qa_half=frame&1u;fm1_alnk0_irq();
 ui_input();d8fi_poll();d8mc_poll();d8ls_poll();
 settings_poll();autosave_poll();ui_leds();ui_draw();lcd_sync();fm1_ms+=15;
}
static void owner_disclosure(void)
{
 /* Actual LCD pixels must show both destructive disclosure lines on the
  * background in the shipping light and dark palettes. */
 for(unsigned theme=0;theme<2;theme++) {
  settings.palette=theme?6:0;palette_set(settings.palette);d8fi_draw();lcd_sync();
  const unsigned rows[]={76,110};
  for(unsigned line=0;line<2;line++) {
   unsigned visible=0;
   for(unsigned y=rows[line];y<rows[line]+AF_S.h;y++)for(unsigned x=12;x<228;x++)
    visible+=host_screen[y*240+x]!=swap16(T_BG);
   CHECK(visible>0);
  }
 }
 settings.palette=UI_GREY_INDEX;palette_set(settings.palette);d8fi_draw();
}
static void owner_sample(void)
{
 smp_user_hdr_t *h=(smp_user_hdr_t *)(nor+SMP_USER_BASE);memset(h,0,sizeof *h);
 h->magic=SMP_USER_MAGIC;h->version=1;h->nz=1;h->data_len=1;
 h->zone[0]=(smp_zone_t){.n=2,.le=1,.rate=65536,.lo=60,.hi=60};
 smp_user_scan(0);CHECK(usr_nz[0]==1);
}
static void owner_started(void)
{owner_fresh();ui_draw();owner_frame(0);CHECK(d8fi.phase==FI_EXECUTE&&physical_calls==1);}
static void owner_failed_stopped(unsigned calls)
{
 CHECK(d8fi.phase==FI_FAILED&&!migration_owner&&d8fi_diag.status==FI_DIAG_FAILED);
 for(unsigned n=0;n<10;n++)owner_frame(n);
 CHECK(physical_calls==calls&&!project_native.ready);
}
static void owner_diagnostic_scenarios(void)
{
 _Static_assert(sizeof d8fi_diag<=80,"retained diagnostic budget");
 owner_started();unsigned frames=0;
 while(!(d8fi.format.sector==1&&d8fi.format.phase==2&&d8fi.format.verify_offset==0xf00)&&frames++<100)owner_frame(frames);
 CHECK(frames<100&&physical_calls==2);
 d8_capture_store_changed();usb.config=0;owner_frame(++frames);owner_failed_stopped(2);
 CHECK(d8fi_diag.result==D8FU_BUSY&&d8fi_diag.raw_rc==FI_DIAG_PENDING&&
  d8fi_diag.guards==(FI_G_EPOCH|FI_G_CONFIG)&&d8fi_diag.format_phase==2&&d8fi_diag.sector==1&&
  d8fi_diag.verify_offset==0xf00&&d8fi_diag.address==0x98f00&&d8fi_diag.generation&&
  d8fi_diag.config==1&&!d8fi_diag.current_config&&d8fi_diag.current_epoch==d8fi_diag.epoch+d8fi_diag.own+1);
 owner_report_fields("configuration-and-epoch-at-sector1-page15");
 uint32_t record_boot=d8fi_diag.record_boot;d8fi_diag_boot(4,0x1234,0xd);
 CHECK(d8fi_diag.boot!=record_boot&&d8fi_diag.record_boot==record_boot&&
  d8fi_diag.status==FI_DIAG_FAILED&&d8fi_diag.reset_reason==4&&d8fi_diag.reset_src==0x1234);
 CHECK(d8fi_boot_classify()&&d8fi_diag.record_boot==record_boot);owner_report_fields("retained-failure-after-reset");
 owner_started();owner_read_error=-31;owner_frame(1);owner_failed_stopped(1);
 CHECK(d8fi_diag.raw_rc==-31&&d8fi_diag.result==D8FU_IO&&!d8fi_diag.guards&&d8fi_diag.operation==FI_OP_READ);
 owner_report_fields("raw-read-error");
 owner_started();nor[d8fu_address(0)]=0;owner_frame(1);owner_failed_stopped(1);
 CHECK(!d8fi_diag.raw_rc&&d8fi_diag.result==D8FU_IO&&d8fi_diag.guards==FI_G_VERIFY);
 owner_report_fields("non-FF-verification-byte");
 owner_fresh();ui_draw();owner_raw_error=-23;owner_frame(0);owner_failed_stopped(1);
 CHECK(d8fi_diag.raw_rc==-23&&d8fi_diag.result==D8FU_IO&&!d8fi_diag.guards&&d8fi_diag.operation==FI_OP_ERASE);
 owner_report_fields("raw-erase-error");
 owner_fresh();ui_draw();owner_raw_error=-19;owner_guard_after_write=1;owner_frame(0);owner_failed_stopped(1);
 CHECK(d8fi_diag.raw_rc==-19&&d8fi_diag.result==D8FU_BUSY&&d8fi_diag.guards==(FI_G_EPOCH|FI_G_CONFIG));
 owner_report_fields("raw-error-preserved-with-lifetime-invalidation");
 for(unsigned kind=0;kind<5;kind++) {
  owner_started();
  if(kind==0)usb.resets++;
  if(kind==1)d8_capture_store_changed();
  if(kind==2)fm1_ms=d8fi.started+120000u;
  if(kind==3)transport_req=1;
  if(kind==4)usb.up=0;
  d8fi_poll();owner_failed_stopped(1);
  const unsigned reasons[]={FI_G_RESET,FI_G_EPOCH,FI_G_DEADLINE,FI_G_QUIET,FI_G_USB_STATE};
  CHECK(d8fi_diag.result==D8FU_BUSY&&d8fi_diag.guards==reasons[kind]);
 }
 owner_fresh();ui_draw();owner_failstop=1;
 if(!setjmp(owner_stop)){owner_frame(0);CHECK(0);}
 CHECK(d8fi_diag.status==FI_DIAG_INFLIGHT&&d8fi_diag.raw_rc==FI_DIAG_PENDING&&
  d8fi_diag.operation==FI_OP_ERASE&&d8fi_diag.address==d8fu_address(0));
 irq_disabled=0;d8fi_diag_boot(4,0x55,0xd);
 CHECK(d8fi_diag.record_boot!=d8fi_diag.boot&&d8fi_diag.status==FI_DIAG_INFLIGHT);
 owner_report_fields("pre-operation-record-after-simulated-failstop");
 /* Native/torn/future signatures preserve bytes without an owner override. */
 owner_fresh();uint8_t *h=nor+d8pool_mapped_address(0);memcpy(h,"D8SP",4);h[4]=99;
 uint32_t crc=d8p1_crc32(h,28);memcpy(h+28,&crc,4);memcpy(baseline,nor,sizeof nor);
 CHECK(!d8fi_boot_classify());ui_draw();d8fi_poll();CHECK(!physical_calls&&!memcmp(nor,baseline,sizeof nor));
 h[28]^=1;memcpy(baseline,nor,sizeof nor);CHECK(d8fi_boot_classify()&&d8fi.phase==FI_UNCERTAIN);
 ui_draw();d8fi_poll();CHECK(!physical_calls&&!memcmp(nor,baseline,sizeof nor));
 printf("Diagnostic bytes=%zu; pool session bytes=%zu\n",sizeof d8fi_diag,sizeof d8fi);
}
int main(void)
{
 owner_fresh();usb.config=0;ui_draw();d8fi_poll();
 CHECK(d8fi.phase==FI_REQUIRED&&!d8fi.shown&&!physical_calls);
 for(unsigned n=0;n<5;n++)owner_frame(n);
 CHECK(d8fi.phase==FI_REQUIRED&&!physical_calls);
 d8_capture_store_changed();usb.config=1;d8fi_poll();
 CHECK(d8fi.phase==FI_REQUIRED&&!physical_calls); /* visible snapshot must be refreshed */
 ui_draw();CHECK(d8fi.shown);owner_frame(0);
 CHECK(d8fi.phase==FI_EXECUTE&&physical_calls==1);
 owner_fresh();usb.up=0;usb.config=0;ui_draw();d8fi_poll();
 CHECK(d8fi.phase==FI_REQUIRED&&!d8fi.shown&&!physical_calls); /* offline is unproven */
 owner_fresh();owner_disclosure();d8fi.shown=0;d8fi_poll();CHECK(d8fi.phase==FI_REQUIRED&&!physical_calls);
 owner_sample();uint32_t scans=smp_user_gen;ui_draw();CHECK(d8fi.shown);unsigned steps=0;
 while(d8fi_blocked()&&d8fi.phase!=FI_FAILED&&steps++<2000)owner_frame(steps);
 CHECK(d8fi.phase==FI_IDLE&&steps<2000&&physical_calls&&project_native_status()==1);
 CHECK(!migration_owner&&!dma_errors&&smp_user_gen==scans+SMP_USER_SLOTS);
 CHECK(d8fi_diag.status==FI_DIAG_COMPLETE&&!d8fi_diag.guards&&d8fi_diag.record_boot==d8fi_diag.boot);
 owner_report_fields("completed-initialization");
 CHECK(!usr_nz[0]&&!usr_nz[1]&&!usr_nz[2]);
 CHECK(completion_reads>0&&up_bank[0].magic==UP_BANK_MAGIC&&up_bank[1].magic==UP_BANK_MAGIC&&upf.magic==UPF_MAGIC&&!upf.used);
 memcpy(baseline,nor,sizeof nor);owner_cold();CHECK(!d8fi_boot_classify());
 CHECK(!autosave_boot(1)&&project_native_status()==1);owner_frame(0);
 CHECK(!memcmp(nor,baseline,sizeof nor)&&!physical_calls);
 owner_fresh();d8fi.phase=FI_UNCERTAIN;d8fi.classification=D8FU_UNCERTAIN;
 d8fi_draw();d8fi_poll();CHECK(d8fi.phase==FI_UNCERTAIN&&!physical_calls);
 owner_fresh();d8fi.phase=FI_REFUSED;d8fi_draw();d8fi_poll();
 CHECK(d8fi.phase==FI_REFUSED&&!physical_calls);
 owner_fresh();d8fi_draw();usb.resets++;d8fi_poll();
 CHECK(d8fi.phase==FI_FAILED&&!physical_calls);
 owner_fresh();d8fi_draw();host_enc[0]=1;d8fi_poll();
 CHECK(d8fi.phase==FI_REQUIRED&&!physical_calls);
 owner_fresh();ui_draw();owner_frame(0);CHECK(d8fi.phase==FI_EXECUTE&&physical_calls==1);
 usb.resets++;owner_frame(1);CHECK(d8fi.phase==FI_FAILED&&physical_calls==1&&!migration_owner);
 owner_fresh();ui_draw();cut=0;owner_frame(0);
 CHECK(d8fi.phase==FI_FAILED&&physical_calls==1&&!writes&&!migration_owner);
 owner_diagnostic_scenarios();
 printf("Owner-authorised initialization: %u checks, %u failures\n",checks,failures);
 return failures!=0;
}
