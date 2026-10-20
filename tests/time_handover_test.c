/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
static volatile uint32_t inherited_timer[3];
#define FM1_TIME_REG32(a) inherited_timer[((a)-0x10800u)/4u]
#include "../firmware/hal/fm1_time.h"
#define FM1_FLASH_TEST_TIMER_ADVANCE() \
 (ft_timer=(uint32_t)(((uint64_t)ft_timer+ft_tick_step)%((uint64_t)inherited_timer[2]+1u)))
#include "flash_trace_fixture.h"

static unsigned checks,failures;
#define CHECK(c) do { checks++;if(!(c)){failures++;printf("FAIL line %d: %s\n",__LINE__,#c);} } while(0)

int main(void)
{
 /* Every divider, run mode and supported clock-source encoding, with either
  * a full-width or inherited short period. Only a valid clock keeps its count. */
 for(unsigned con=0;con<256;con++)for(unsigned short_period=0;short_period<2;short_period++) {
  inherited_timer[0]=con|0x4000u;inherited_timer[1]=1008u;
  inherited_timer[2]=short_period?1031u:UINT32_MAX;
  fm1_time_init();
  CHECK((inherited_timer[0]&0xffu)==9u);
  CHECK(inherited_timer[2]==UINT32_MAX);
  CHECK(inherited_timer[1]==(con==9u&&!short_period?1008u:0u));
 }
 /* Execute the actual WIP wait with counter wrap at the configured period.
  * A short inherited period must not make the program deadline unreachable. */
 for(unsigned short_period=0;short_period<2;short_period++) {
  ft_reset();inherited_timer[0]=9u;inherited_timer[1]=1008u;
  inherited_timer[2]=short_period?1031u:UINT32_MAX;fm1_time_init();
  ft_timer=inherited_timer[1];ft_busy=ft_forever=1;ft_sfc=ft_iomap=0;
  ft_stop_after_status=20000;fl_trace_active=0;
  uint32_t took=0;int jump=setjmp(ft_stop);
  if(!jump)(void)fl_wait_ready(10000u,&took);
  CHECK(jump==1&&ft_contained&&!ft_watchdog);
  CHECK(ft_status_reads<20000);
 }
 printf("timer handover: %u checks, %u failures\n",checks,failures);
 return failures?1:0;
}
