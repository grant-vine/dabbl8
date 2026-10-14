/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef FLASH_TRACE_FIXTURE_H
#define FLASH_TRACE_FIXTURE_H
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
static volatile uint32_t ft_sfc,ft_spi,ft_buf,ft_iomap,ft_out,ft_dir,ft_cache,ft_timer;
static volatile uint32_t ft_enc_lo,ft_enc_hi;
static volatile uint8_t ft_enc;
static unsigned ft_irq,ft_high,ft_transfers,ft_fail_transfer,ft_wel,ft_refuse_wel;
static unsigned ft_busy,ft_forever,ft_busy_reads,ft_issued,ft_command,ft_bytes,ft_payload;
static unsigned ft_contained,ft_ticks,ft_mmio,ft_watchdog_limit,ft_watchdog;
static unsigned ft_cache_reads,ft_cache_delay,ft_cache_stuck,ft_inval_stuck,ft_flushes;
static unsigned ft_sfc_reads,ft_sfc_delay,ft_sfc_stuck,ft_exit_stuck,ft_exiting;
static unsigned ft_status_reads,ft_tick_step;
static jmp_buf ft_stop;
static uint32_t ft_seen;
static void ft_observe(void);
static volatile uint32_t *ft_register(uint32_t address)
{
 ft_observe();ft_mmio++;
 if(ft_watchdog_limit&&ft_mmio>=ft_watchdog_limit){ft_watchdog=1;longjmp(ft_stop,2);}
 switch(address) {
 case 0x40200:
  ft_sfc_reads++;
  if((ft_sfc&0x80000000u)&&!ft_sfc_stuck&&!ft_exit_stuck&&ft_sfc_delay&&!--ft_sfc_delay)
   ft_sfc&=~0x80000000u;
  return &ft_sfc;
 case 0x40308:return &ft_enc_hi;
 case 0x4030c:return &ft_enc_lo;
 case 0x11c00:return &ft_spi;
 case 0x11c08:return &ft_buf;
 case 0x5101c:return &ft_iomap;
 case 0x500c0:return &ft_out;
 case 0x500c8:return &ft_dir;
 case 0x1eee008:
  ft_cache_reads++;
  if(!ft_cache_stuck&&ft_cache_delay&&!--ft_cache_delay)ft_cache=0x4000;
  return &ft_cache;
 case 0x10804:ft_ticks++;ft_timer+=ft_tick_step;return &ft_timer;
 default:fprintf(stderr,"Unexpected trace fixture MMIO %x\n",address);longjmp(ft_stop,3);
 }
}
static void ft_delay(uint32_t n)
{
 (void)n;unsigned high=ft_out&1u;
 if(!high&&ft_high)ft_command=ft_bytes=ft_payload=0;
 if(high&&!ft_high) {
  if(ft_command==0x06&&!ft_busy&&!ft_refuse_wel)ft_wel=1;
  if((ft_command==0x20&&ft_bytes==3)||(ft_command==0x02&&ft_bytes==3&&ft_payload)) {
   ft_issued++;ft_busy=1;ft_wel=0;
  }
 }
 ft_high=high;
}
static void ft_csync(void)
{
 if((ft_iomap&0x20)&&!ft_sfc&&ft_exit_stuck&&!ft_exiting){ft_sfc=0x80000000u;ft_exiting=1;}
 if((ft_iomap&0x20)&&(ft_sfc&0x80000000u)&&ft_transfers&&!ft_sfc_stuck&&!ft_exit_stuck)ft_sfc&=~0x80000000u;
 if(ft_high||!(ft_spi&8u))return;
 ft_transfers++;ft_spi&=~0x8000u;
 if(ft_transfers==ft_fail_transfer)return;
 if(ft_spi&0x1000u) {
  if(ft_command==0x05) {
   ft_status_reads++;ft_buf=(ft_busy?1u:0u)|(ft_wel?2u:0u);
   if(ft_busy&&!ft_forever&&ft_busy_reads&&!--ft_busy_reads)ft_busy=0;
  } else if(ft_command==0x35)ft_buf=0;
  else ft_buf=0xa5;
 } else if(!ft_command)ft_command=ft_buf;
 else if(ft_bytes<3)ft_bytes++;
 else ft_payload++;
 ft_spi|=0x8000u;
}
static void ft_flush(uint32_t address)
{
 (void)address;ft_flushes++;
 if(ft_inval_stuck){ft_cache=0;ft_cache_stuck=1;}
}
static _Noreturn void ft_contain(void){ft_contained=1;longjmp(ft_stop,1);}
#define FM1_FLASH_DRIVER_TEST 1
#define DABBL8_TIMING_DIAGNOSTIC 1
#define REG32(a) (*ft_register(a))
#define FM1_FLASH_TEST_REG8(a) ft_enc
#define FM1_FLASH_TEST_CSYNC() ft_csync()
#define FM1_FLASH_TEST_DELAY(n) ft_delay(n)
#ifndef FM1_FLASH_TEST_SPI_POLLS
#define FM1_FLASH_TEST_SPI_POLLS 32u
#endif
#define FM1_FLASH_TEST_FLUSHINV(a) ft_flush(a)
#define FM1_FLASH_TEST_IRQ_OFF() (ft_irq=0)
#define FM1_FLASH_TEST_IRQ_ON() (ft_irq=1)
#define FM1_FLASH_TEST_CONTAIN() ft_contain()
#ifdef FLASH_TRACE_BASELINE
#define FL_TRACE_TICKS() (*ft_register(0x10804u))
#include "../firmware/hal/fm1_flash_trace.h"
#include "../build/flash-trace/baseline_fm1_flash.h"
#else
#include "../firmware/hal/fm1_flash.h"
#endif
static void ft_observe(void)
{if(fl_trace.stage<32u)ft_seen|=1u<<fl_trace.stage;}
static void ft_reset(void)
{
 ft_sfc=0x201;ft_spi=0x33;ft_buf=0;ft_iomap=0x20;ft_out=1;ft_dir=0;ft_cache=0x4000;ft_timer=0;
 ft_enc=0;ft_enc_hi=ft_enc_lo=0;ft_irq=ft_high=1;
 ft_transfers=ft_fail_transfer=ft_wel=ft_refuse_wel=ft_busy=ft_forever=0;
 ft_issued=ft_command=ft_bytes=ft_payload=ft_contained=ft_ticks=ft_mmio=ft_watchdog=0;
 ft_cache_reads=ft_cache_delay=ft_cache_stuck=ft_inval_stuck=ft_flushes=0;
 ft_sfc_reads=ft_sfc_delay=ft_sfc_stuck=ft_exit_stuck=ft_exiting=ft_status_reads=0;
 ft_busy_reads=1;ft_tick_step=24;ft_watchdog_limit=0;ft_seen=0;
}
static int ft_erase(uint32_t address)
{uint32_t took=0;return fl_erase4k(address,&took);}
static char ft_report[2048];static unsigned ft_report_n;
static void con_kv(const char *key,int32_t value)
{ft_report_n+=(unsigned)snprintf(ft_report+ft_report_n,sizeof ft_report-ft_report_n,"%s %d\n",key,value);}
static void con_kx(const char *key,uint32_t value)
{ft_report_n+=(unsigned)snprintf(ft_report+ft_report_n,sizeof ft_report-ft_report_n,"%s 0x%08x\n",key,value);}
#include "../firmware/src/flash_trace_diagnostic.h"
static void ft_print(const char *scenario)
{
 ft_report_n=0;con_flash_trace();
 printf("scenario=%s issued=%u transfers=%u cache_reads=%u sfc_reads=%u flushes=%u status_reads=%u IRQ=%u SFC=%x\n%s",
  scenario,ft_issued,ft_transfers,ft_cache_reads,ft_sfc_reads,ft_flushes,ft_status_reads,ft_irq,ft_sfc,ft_report);
}
#endif
