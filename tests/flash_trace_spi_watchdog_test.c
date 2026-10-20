/* SPDX-License-Identifier: GPL-3.0-only */
/* Keep the production SPI budget: the fake watchdog escapes first. */
#define FM1_FLASH_TEST_SPI_POLLS 50000000u
#include "flash_trace_fixture.h"
int main(void)
{
 unsigned failures=0;
 for(unsigned early=0;early<2;early++)for(unsigned transfer=10;transfer<=11;transfer++) {
  ft_reset();memset((void *)&fl_trace,0,sizeof fl_trace);fl_trace_active=0;
  fl_trace_boot(1,0);fl_trace_watchdog(0x5d);fl_trace_feed();
  ft_forever=ft_keep_wel=1;ft_tick_step=0;ft_timer=0xbabecafe;
  ft_fail_transfer=transfer;ft_watchdog_limit=early?600:10000;
  int jump=setjmp(ft_stop);
  if(!jump){(void)ft_erase(0x99000);failures++;} else {
   unsigned expected=transfer==10?FL_TRACE_SPI_TX:FL_TRACE_SPI_RX;
   unsigned byte=transfer==10?0x05:0xff;
   if(jump!=2||!ft_watchdog||ft_contained||ft_issued!=1||ft_transfers!=transfer||ft_status_reads!=2||
    fl_trace.state!=FL_TRACE_INFLIGHT||fl_trace.stage!=expected||fl_trace.phase!=FL_TRACE_WIP||
    fl_trace.failure||fl_trace.spi_byte!=byte||fl_trace.polls!=(early?1u:8192u)||fl_trace.last_status!=3||
    fl_trace.wait_tick!=0xbabecafe||fl_trace.wait_progress!=0xbabecafe||fl_trace.mmio&0x8000u)
    failures++;
  }
  ft_print(transfer==10?"unsampled-WIP-SPI-TX-watchdog":"unsampled-WIP-SPI-RX-watchdog");
 }
 /* Stop at the NEXT CS assertion, after the delayed transfer and its SR1
  * transaction completed. This cutoff models neither a calibrated watchdog
  * nor a physical stall: it checks that the retained wait was retired. */
 for(unsigned program=0;program<2;program++)for(unsigned rx=0;rx<2;rx++)for(unsigned wrap=0;wrap<2;wrap++) {
  ft_reset();memset((void *)&fl_trace,0,sizeof fl_trace);fl_trace_active=0;
  fl_trace_boot(1,0);fl_trace_watchdog(0x1d);fl_trace_feed();
  ft_forever=ft_keep_wel=1;ft_tick_step=6;ft_timer=wrap?UINT32_MAX-100:100;
  ft_delay_transfer=(program?266u:10u)+rx;ft_stop_after_status=3;
  uint8_t payload[256];memset(payload,0x5a,sizeof payload);
  int jump=setjmp(ft_stop);
  if(!jump){uint32_t took=0;if(program)(void)fl_write(0x9d800,payload,sizeof payload);else(void)fl_erase4k(0x99000,&took);failures++;}
  else if(jump!=2||ft_delayed_completed!=1||ft_stale_pending||ft_status_reads!=3||
   ft_transfers!=(program?267u:11u)||ft_contained||ft_irq||ft_sfc||ft_issued!=1||
   fl_trace.operation!=(program?FL_TRACE_PROGRAM:FL_TRACE_ERASE)||fl_trace.address!=(program?0x9d800u:0x99000u)||
   fl_trace.stage!=FL_TRACE_WIP||fl_trace.phase!=FL_TRACE_WIP||fl_trace.failure||
   fl_trace.mmio!=3||fl_trace.last_status!=3||!fl_trace.status_valid||
   fl_trace.polls||fl_trace.wait_tick||fl_trace.wait_progress) {
    printf("FAIL completed status retains stale wait: program=%u rx=%u wrap=%u stage=%u mmio=%x stalePND=%u\n",
     program,rx,wrap,fl_trace.stage,fl_trace.mmio,ft_stale_pending);failures++;
   }
 }
 printf("Unsampled WIP SPI watchdog: 4 real-wait and 8 completed-status scenarios, %u failures\n",failures);
 return failures!=0;
}
