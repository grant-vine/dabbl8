/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual manual consent/formatter + real RAM HAL flow; retained virtual NOR.
 * The fake watchdog escapes a third-erase frozen-Timer4 WIP wait. */
#include <stdint.h>
static int reset_io_result(int,uint32_t,uint32_t);
#define D8_FI_TEST_RESULT(rc,a,n) reset_io_result(rc,a,n)
#define DABBL8_TIMING_DIAGNOSTIC 1
#define D8_FIRST_USE_SESSION_NO_MAIN 1
#include "native_first_use_session_test.c"
#define fl_inval ft_hal_inval
#define fl_read_ram ft_hal_read_ram
#define fl_erase4k ft_hal_erase4k
#define fl_write ft_hal_write
#define irq_save ft_local_irq_save
#define irq_restore ft_local_irq_restore
#include "flash_trace_fixture.h"
#undef irq_restore
#undef irq_save
#undef fl_write
#undef fl_erase4k
#undef fl_read_ram
#undef fl_inval
#include "../firmware/src/native_first_use_diagnostic.h"
static unsigned restart_inject=1;
static int reset_io_result(int rc,uint32_t address,uint32_t n)
{
 CHECK(!rc&&n==4096&&irq_disabled&&d8fi.busy&&d8fi_diag.status==FI_DIAG_INFLIGHT);
 ft_reset();ft_timer=fm1_ms*24000u;
 if(restart_inject&&physical_calls==3) {
  restart_inject=0;ft_forever=1;ft_tick_step=0;ft_watchdog_limit=240000;
 }
 int raw=ft_erase(address);
 CHECK(raw==0&&fl_trace.state==FL_TRACE_RETURNED&&fl_trace.address==address);
 return rc;
}
static void reset_report(const char *scenario)
{
 ft_print(scenario);ft_report_n=0;con_first_use();printf("%s",ft_report);
 printf("session_phase=%u formatter_phase=%u sector=%u verify=%u own=%u calls=%u writes=%u owner=%u NOR_crc=%x\n",
  d8fi.phase,d8fi.format.phase,d8fi.format.sector,d8fi.format.verify_offset,d8fi.own,
  physical_calls,writes,migration_owner,st_crc32(nor,sizeof nor));
}
static void manual_consent(void)
{
 d8fi_draw();CHECK(d8fi.shown);uint32_t save=1u<<panel.btn[B_SAVE];
 host_pressed=fm1_in.buttons=save;ui_input();CHECK(d8fi.phase==FI_RELEASE);
 fm1_in.buttons=0;ui_input();d8fi_poll();CHECK(d8fi.phase==FI_EXECUTE);
}
int main(void)
{
 fi_consent_fresh();memset((void *)&fl_trace,0,sizeof fl_trace);fl_trace_active=0;
 d8fi_diag_boot(1,0,0);fl_trace_boot(1,0);fl_trace_watchdog(0x5d);fl_trace_feed();
 manual_consent();CHECK(physical_calls==1);
 int jump=setjmp(ft_stop);
 if(!jump) {
  unsigned steps=0;
  while(physical_calls<3&&steps++<100){fm1_ms+=15;d8fi_poll();}
  CHECK(0); /* The third RAM erase must not return. */
 } else {
  CHECK(jump==2&&ft_watchdog&&!ft_contained&&ft_issued==1&&physical_calls==3&&writes==3);
  CHECK(fl_trace.operation==FL_TRACE_ERASE&&fl_trace.state==FL_TRACE_INFLIGHT&&
   fl_trace.address==0x99000&&fl_trace.stage==FL_TRACE_WIP&&!fl_trace.failure&&fl_trace.last_status==1);
  CHECK(d8fi.busy&&d8fi.format.sector==2&&d8fi.format.verify_offset==4096&&d8fi.format.phase==1&&
   d8fi_diag.status==FI_DIAG_INFLIGHT&&d8fi_diag.raw_rc==FI_DIAG_PENDING&&d8fi_diag.own==3);
 }
 reset_report("third-erase-nonreturn");
 memcpy(baseline,nor,sizeof nor);uint32_t record_boot=fl_trace.record_boot;
 unsigned calls=physical_calls,old_writes=writes;
 /* Model firmware startup: retain NOR/noinit, clear the live pool/session and
  * peripheral/BSS state, then perform boot probe reads with XIP restored. */
 if(migration_owner)main_migration_end(migration_owner);
 cold();project_native_reset();native_as.ready=0;d8ls_reset();
 memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);
 memset(&d8_capture_change,0,sizeof d8_capture_change);memset(&d8fi,0,sizeof d8fi);
 memset(&main_workspace,0,sizeof main_workspace);migration_owner=0;
 usb.up=1;usb.suspended=0;usb.config=1;usb.ota_req=usb.uboot_req=0;
 host_pressed=host_notes=0;fm1_in.buttons=fm1_in.notes=0;irq_disabled=0;
 d8fi_diag_boot(4,0x1234,0xc);fl_trace_boot(4,0x1234);fl_trace_active=0;ft_reset();
 fl_trace_watchdog(0x5d);fl_trace_feed();ft_irq=0;
 (void)fl_jedec_ram();(void)fl_status_ram();uint8_t byte;(void)ft_hal_read_ram(0x97000,&byte,1);
 CHECK(d8fi_boot_classify()&&d8fi.phase==FI_REQUIRED&&!d8fi.shown&&!d8fi.busy&&!d8fi.own&&
  !d8fi.format.phase&&!d8fi.format.sector&&!migration_owner&&!project_native.ready);
 CHECK(fl_trace.boot==record_boot+1&&fl_trace.record_boot==record_boot&&fl_trace.state==FL_TRACE_INFLIGHT&&
  fl_trace.address==0x99000&&fl_trace.stage==FL_TRACE_WIP&&fl_trace.last_status==1);
 CHECK(d8fi_diag.record_boot!=d8fi_diag.boot&&d8fi_diag.status==FI_DIAG_INFLIGHT&&d8fi_diag.address==0x99000);
 for(unsigned poll=0;poll<20;poll++){fm1_ms+=15;ui_input();d8fi_poll();}
 CHECK(physical_calls==calls&&writes==old_writes&&!memcmp(nor,baseline,sizeof nor)&&d8fi.phase==FI_REQUIRED);
 d8fi_draw();CHECK(d8fi.shown);
 for(unsigned poll=0;poll<20;poll++){fm1_ms+=15;ui_input();d8fi_poll();}
 CHECK(physical_calls==calls&&writes==old_writes&&!memcmp(nor,baseline,sizeof nor)&&d8fi.phase==FI_REQUIRED);
 reset_report("restart-probes-retain-incident-no-automatic-resume");
 manual_consent();CHECK(physical_calls==calls+1&&d8fi.format.sector==0&&d8fi.format.phase==2&&
  fl_trace.record_boot==fl_trace.boot&&fl_trace.address==0x97000&&fl_trace.state==FL_TRACE_RETURNED);
 CHECK(!memcmp(nor,baseline,sizeof nor)); /* Restart erased already-erased sector 0. */
 reset_report("fresh-manual-consent-restarts-sector-zero");
 printf("Third erase/reset retention: %u checks, %u failures\n",checks,failures);
 return failures!=0;
}
