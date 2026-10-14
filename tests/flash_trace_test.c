/* SPDX-License-Identifier: GPL-3.0-only */
#include "flash_trace_fixture.h"
#include <stddef.h>
static unsigned ft_checks,ft_failures;
#define CHECK(name,c) do {ft_checks++;if(!(c)){ft_failures++;printf("FAIL %s line %u\n",name,__LINE__);}}while(0)
static void fresh(void)
{
 ft_reset();memset((void *)&fl_trace,0,sizeof fl_trace);fl_trace_active=0;
 fl_trace_boot(1,0x12);fl_trace_watchdog(0x5d);fl_trace_feed();
}
static void returned(const char *name,int expected)
{
 uint32_t saved_sfc=ft_sfc&0x7fffffffu;
 int jump=setjmp(ft_stop);
 if(!jump) {
  int rc=ft_erase(0x99000);
  CHECK(name,rc==expected&&ft_irq&&ft_sfc==saved_sfc&&(ft_iomap&0x20)&&!ft_contained);
  CHECK(name,fl_trace.state==FL_TRACE_RETURNED&&fl_trace.result==rc&&fl_trace.stage==FL_TRACE_RETURN&&!fl_trace_active);
 } else CHECK(name,0);
 ft_print(name);
}
static void stalled(const char *name,unsigned stage,unsigned phase,unsigned reason,unsigned reset_kind)
{
 int jump=setjmp(ft_stop);
 if(!jump){(void)ft_erase(0x99000);CHECK(name,0);} else {
  CHECK(name,(unsigned)jump==reset_kind);
  CHECK(name,fl_trace.state==FL_TRACE_INFLIGHT&&fl_trace.result==FL_TRACE_PENDING&&
   fl_trace.address==0x99000&&fl_trace.operation==FL_TRACE_ERASE&&fl_trace.stage==stage&&
   fl_trace.phase==phase&&fl_trace.failure==reason&&fl_trace.record_boot==fl_trace.boot);
  CHECK(name,fl_trace.entry_tick>fl_trace.feed_tick&&fl_trace.wdt==0x5d);
  CHECK(name,!ft_irq&&((stage==FL_TRACE_CACHE_ENTRY||stage==FL_TRACE_SFC_ENTRY)?
   ft_sfc!=0:ft_sfc==0||(stage==FL_TRACE_SFC_EXIT&&ft_sfc==0x80000000u)));
 }
 ft_print(name);
}
int main(void)
{
 _Static_assert(sizeof fl_trace<=80,"retained flash incident budget");
 fresh();returned("erase-success",0);
 CHECK("normal command/status count",ft_issued==1&&ft_status_reads==3&&ft_transfers==11&&ft_flushes==128);
 CHECK("success raw ticks",fl_trace.progress_tick>fl_trace.entry_tick&&fl_trace.feed_tick>0&&fl_trace.last_status==0&&fl_trace.pre_status==2);
 CHECK("issued/invalidation stages observed",(ft_seen&(1u<<FL_TRACE_ERASE_ISSUED))&&(ft_seen&(1u<<FL_TRACE_CACHE_INVALIDATE)));
 fresh();ft_refuse_wel=1;returned("write-enable-refusal",-3);CHECK("refusal issues no erase",!ft_issued&&fl_trace.pre_status==0&&!fl_trace.failure);
 fresh();ft_sfc=0x601;returned("unsupported-SFC-mode",-2);CHECK("mode refusal no SPI",!ft_transfers&&!ft_issued);
 fresh();ft_cache=0;ft_cache_delay=8193;returned("delayed-cache-entry",0);
 CHECK("delayed cache observed",ft_cache_reads>=8193&&ft_issued==1&&ft_flushes==128);
 fresh();ft_sfc=0x80000201u;ft_sfc_delay=8194;returned("delayed-SFC-entry",0);
 CHECK("delayed SFC observed",ft_sfc_reads>=8194&&ft_issued==1&&ft_flushes==128);
 fresh();ft_cache=0;ft_cache_stuck=1;ft_watchdog_limit=9000;
 stalled("stuck-cache-entry",FL_TRACE_CACHE_ENTRY,FL_TRACE_CACHE_ENTRY,FL_TRACE_FAIL_NONE,2);
 CHECK("cache wait progress",fl_trace.polls==8192&&fl_trace.mmio==0&&fl_trace.wait_progress>fl_trace.wait_tick&&!ft_issued&&!ft_transfers);
 fresh();ft_sfc=0x80000201u;ft_sfc_stuck=1;ft_watchdog_limit=9000;
 stalled("stuck-SFC-entry",FL_TRACE_SFC_ENTRY,FL_TRACE_SFC_ENTRY,FL_TRACE_FAIL_NONE,2);
 CHECK("SFC wait progress",fl_trace.polls==8192&&fl_trace.mmio==0x80000201u&&!ft_issued&&!ft_transfers);
 fresh();ft_inval_stuck=1;ft_watchdog_limit=9000;
 stalled("stuck-cache-invalidation",FL_TRACE_CACHE_INVALIDATE,FL_TRACE_INVALIDATE,FL_TRACE_FAIL_NONE,2);
 CHECK("invalidation command already issued",ft_issued==1&&ft_flushes==1&&fl_trace.pre_status==2&&fl_trace.polls>=4096);
 fresh();ft_exit_stuck=1;ft_watchdog_limit=9000;
 stalled("stuck-SFC-exit",FL_TRACE_SFC_EXIT,FL_TRACE_SFC_EXIT,FL_TRACE_FAIL_NONE,2);
 CHECK("exit ordering observed",ft_issued==1&&ft_status_reads==3&&ft_flushes==128&&ft_spi==0x33&&(ft_iomap&0x20));
 fresh();ft_busy=ft_forever=1;
 stalled("pre-command-busy",FL_TRACE_SPI_RX,FL_TRACE_PRE_STATUS,FL_TRACE_FAIL_PRE_BUSY,1);
 CHECK("busy status preserved",!ft_issued&&fl_trace.pre_status==1&&fl_trace.last_status==1&&ft_status_reads==1&&ft_contained);
 fresh();ft_fail_transfer=2;
 stalled("SPI-prestatus-TX-timeout",FL_TRACE_SPI_TX,FL_TRACE_PRE_STATUS,FL_TRACE_FAIL_SPI,1);
 CHECK("SPI command observation",ft_transfers==2&&!ft_status_reads&&!ft_issued&&fl_trace.spi_byte==0x05&&!fl_trace.status_valid);
 fresh();ft_fail_transfer=3;
 stalled("SPI-prestatus-RX-timeout",FL_TRACE_SPI_RX,FL_TRACE_PRE_STATUS,FL_TRACE_FAIL_SPI,1);
 CHECK("SPI read observation",ft_transfers==3&&!ft_status_reads&&!ft_issued&&fl_trace.spi_byte==0xff);
 fresh();ft_forever=1;ft_tick_step=24000024;
 stalled("WIP-deadline-failstop",FL_TRACE_WIP,FL_TRACE_WIP,FL_TRACE_FAIL_WIP,1);
 CHECK("deadline after command",ft_issued==1&&ft_status_reads>1&&ft_contained&&fl_trace.last_status==1&&ft_flushes==128);
 fresh();ft_forever=1;ft_tick_step=0;ft_timer=0x12345678;ft_watchdog_limit=240000;
 stalled("frozen-Timer4-WIP-watchdog",FL_TRACE_WIP,FL_TRACE_WIP,FL_TRACE_FAIL_NONE,2);
 CHECK("frozen raw ticks",fl_trace.progress_tick==0x12345678&&ft_status_reads>4096&&ft_issued==1&&ft_watchdog&&!ft_contained&&fl_trace.polls>=4096);
 /* A completed call cannot leave the next boot looking genuinely pending. */
 fresh();returned("successful-call-before-reset",0);uint32_t record_boot=fl_trace.record_boot;
 fl_trace_boot(4,0x55);fl_trace_active=0;ft_reset();ft_irq=0;(void)fl_jedec_ram();
 CHECK("completed call permits boot probe",fl_trace.state==FL_TRACE_RETURNED&&fl_trace.record_boot!=record_boot&&fl_trace.operation==FL_TRACE_JEDEC);
 fresh();ft_busy=ft_forever=1;
 stalled("incident-before-reset",FL_TRACE_SPI_RX,FL_TRACE_PRE_STATUS,FL_TRACE_FAIL_PRE_BUSY,1);
 uint8_t incident[sizeof fl_trace];memcpy(incident,(const void *)&fl_trace,sizeof incident);
 record_boot=fl_trace.record_boot;fl_trace_boot(4,0x456);fl_trace_active=0;ft_reset();
 fl_trace_watchdog(0x5d);fl_trace_feed();ft_irq=0;
 (void)fl_jedec_ram();(void)fl_status_ram();uint8_t data=0;(void)fl_read_ram(0x97000,&data,1);
 CHECK("boot reads preserve in-flight incident",fl_trace.record_boot==record_boot&&fl_trace.boot==record_boot+1&&
  fl_trace.state==FL_TRACE_INFLIGHT&&fl_trace.address==0x99000&&fl_trace.failure==FL_TRACE_FAIL_PRE_BUSY&&data==0xa5&&!fl_trace_active);
 CHECK("incident fields byte-preserved",!memcmp((const uint8_t *)&fl_trace+20,incident+20,offsetof(__typeof__(fl_trace),reset_reason)-20));
 ft_print("retained-after-JEDEC-status-read");
 ft_irq=1;returned("fresh-write-replaces-incident",0);
 CHECK("replacement boot identity",fl_trace.record_boot==fl_trace.boot&&fl_trace.failure==FL_TRACE_FAIL_NONE);
 printf("Retained flash trace: %u checks, %u failures; bytes=%zu\n",ft_checks,ft_failures,sizeof fl_trace);
 return ft_failures!=0;
}
