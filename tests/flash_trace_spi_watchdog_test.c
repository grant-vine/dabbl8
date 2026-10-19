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
 printf("Unsampled WIP SPI watchdog: 4 scenarios, %u failures\n",failures);
 return failures!=0;
}
