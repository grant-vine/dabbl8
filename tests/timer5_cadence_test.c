/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual TIMER5 schedule against an independent ten-tick event oracle.
 * Simulated scan/transport and clock only; no physical timing claim. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef FELUCCA_UART
#define FELUCCA_UART 1
#endif
#ifndef FELUCCA_UAC
#define FELUCCA_UAC 1
#endif
#ifndef FELUCCA_SIE_IRQ_BUDGET
#define FELUCCA_SIE_IRQ_BUDGET 1
#endif
#define FELUCCA_TIMER5_ONLY 1
#define FM1_TICKS_PER_US 24u
static struct {uint32_t timer_irqs,nested,in_audio;} felucca_dbg;
static uint32_t fm1_ms,t5_nested_ticks,clock_now;
static unsigned events[8],event_count,checks,failures;
static uint64_t trace=UINT64_C(14695981039346656037);
static void check(int ok){checks++;if(!ok)failures++;}
static void event(unsigned tag){if(event_count>=8)abort();events[event_count++]=tag;}
static uint32_t fm1_ticks(void){return clock_now;}
static void sie_irq_begin(void){event(1);}
static void fm1_timer5_ack(void){event(2);}
static void fm1_input_tick(void){event(3);clock_now+=13;}
static void uac_service(void){event(4);clock_now+=17;}
static void usb_poll(void){event(5);}
static void uart_midi_poll(void){event(6);}
static void sie_irq_end(void){event(7);}
#ifndef TIMER5_SOURCE
#define TIMER5_SOURCE "../firmware/src/main.c"
#endif
#include TIMER5_SOURCE
static void hash_word(uint32_t value)
{for(unsigned b=0;b<4;b++){trace^=(value>>(8*b))&255u;trace*=UINT64_C(1099511628211);}}
int main(void)
{
 unsigned pending=0,nested_total=0,expected_ms=0,elapsed=0,previous=0;
 uint32_t time=0,nested_ticks=0;
 for(unsigned i=0;i<20000;i++){
  /* Rotate every phase through nested/non-nested runs, including deferred USB
   * and UART owed together. Occasionally simulate a late timer tick. */
  if(i)time+=i%997==0?7200u:2400u;
  clock_now=time;event_count=0;
  unsigned nested=i%17<11,phase=i%10;
  unsigned usb_due=phase==0||phase==5;
  if(usb_due)pending|=1;
#if FELUCCA_UART
  if(phase==2||phase==7)pending|=2;
#endif
  elapsed+=time-previous;previous=time;
  expected_ms+=elapsed/24000u;elapsed%=24000u;
  unsigned expected[8],count=0;
#if FELUCCA_SIE_IRQ_BUDGET
  expected[count++]=1;
#endif
  expected[count++]=2;expected[count++]=3;
  if(nested){
   nested_total++;nested_ticks+=13u;
#if FELUCCA_UAC
   if(usb_due){expected[count++]=4;nested_ticks+=17u;}
#endif
  }else{
   if(pending&1)expected[count++]=5;
#if FELUCCA_UART
   if(pending&2)expected[count++]=6;
#endif
   pending=0;
  }
#if FELUCCA_SIE_IRQ_BUDGET
  expected[count++]=7;
#endif
  felucca_dbg.in_audio=nested;fm1_timer5_irq();
  check(event_count==count);
  for(unsigned j=0;j<count;j++){check(events[j]==expected[j]);hash_word(events[j]);}
  check(fm1_ms==expected_ms);check(felucca_dbg.timer_irqs==i+1);
  check(felucca_dbg.nested==nested_total);check(t5_nested_ticks==nested_ticks);
  hash_word(fm1_ms);hash_word(t5_nested_ticks);hash_word(felucca_dbg.nested);
 }
 printf("TIMER5 cadence UART=%u UAC=%u SIE=%u: %u checks %u failures trace=%016llx\n",
  FELUCCA_UART,FELUCCA_UAC,FELUCCA_SIE_IRQ_BUDGET,checks,failures,(unsigned long long)trace);
 return failures?1:0;
}
