/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* Diagnostic incident only: retention requires RAM surviving a reset. This is
 * neither a power-loss journal nor authorization to resume a write. */
#if DABBL8_TIMING_DIAGNOSTIC && (!defined(FELUCCA_LOADER) || !FELUCCA_LOADER)
#define FL_TRACE_ENABLED 1
#define FL_TRACE_MAGIC 0x46545232u
#define FL_TRACE_FORMAT 2
#define FL_TRACE_PENDING (-32768)
enum { FL_TRACE_EMPTY, FL_TRACE_INFLIGHT, FL_TRACE_RETURNED };
enum { FL_TRACE_JEDEC, FL_TRACE_STATUS, FL_TRACE_READ, FL_TRACE_ERASE, FL_TRACE_PROGRAM };
enum { FL_TRACE_ENTRY, FL_TRACE_CACHE_ENTRY, FL_TRACE_SFC_ENTRY, FL_TRACE_SPI_SETUP,
 FL_TRACE_COMMAND, FL_TRACE_PRE_STATUS, FL_TRACE_ERASE_ISSUED, FL_TRACE_PROGRAM_ISSUED,
 FL_TRACE_INVALIDATE, FL_TRACE_CACHE_INVALIDATE, FL_TRACE_WIP, FL_TRACE_SPI_TX,
 FL_TRACE_SPI_RX, FL_TRACE_SFC_EXIT, FL_TRACE_RETURN };
enum { FL_TRACE_FAIL_NONE, FL_TRACE_FAIL_SPI, FL_TRACE_FAIL_PRE_BUSY, FL_TRACE_FAIL_WIP };
static volatile struct {
 uint32_t magic,boot,record_boot,reset_src,current_feed;
 uint32_t address,entry_tick,progress_tick,feed_tick,wait_tick,wait_progress,polls,mmio;
 int32_t result;
 uint8_t state,operation,stage,phase,failure,pre_status,last_status,status_valid;
 uint8_t spi_byte,reset_reason,current_wdt,wdt;
 uint32_t timer_config,timer_period;
} fl_trace
#ifndef FM1_FLASH_DRIVER_TEST
 __attribute__((section(".noinit")))
#endif
;
/* Cleared by normal startup BSS initialization after fl_trace_boot(). */
static uint8_t fl_trace_active,fl_trace_sampling,fl_trace_byte,fl_trace_rx;
static uint32_t fl_trace_spi_tick;
#define FL_TRACE_INL static inline __attribute__((always_inline))
#ifndef FL_TRACE_TICKS
#define FL_TRACE_TICKS() T4_CNT
#endif
FL_TRACE_INL void fl_trace_boot(uint32_t reason,uint32_t source)
{
 if(fl_trace.magic!=FL_TRACE_MAGIC) {
  volatile uint8_t *p=(volatile uint8_t *)&fl_trace;
  for(uint32_t i=0;i<sizeof fl_trace;i++)p[i]=0;
  fl_trace.magic=FL_TRACE_MAGIC;
 }
 fl_trace.boot++;fl_trace.reset_reason=(uint8_t)reason;fl_trace.reset_src=source;
 fl_trace.current_feed=0;fl_trace.current_wdt=0;
}
FL_TRACE_INL void fl_trace_watchdog(uint32_t config)
{ fl_trace.current_wdt=(uint8_t)config; }
FL_TRACE_INL void fl_trace_feed(void)
{ fl_trace.current_feed=FL_TRACE_TICKS(); }
FL_TRACE_INL void fl_trace_begin(uint32_t operation,uint32_t address)
{
 fl_trace_active=!(fl_trace.state==FL_TRACE_INFLIGHT&&fl_trace.record_boot!=fl_trace.boot&&
                  operation!=FL_TRACE_ERASE&&operation!=FL_TRACE_PROGRAM);
 if(!fl_trace_active)return;
 fl_trace_sampling=1;
 fl_trace.timer_config=REG32(0x10800u);fl_trace.timer_period=REG32(0x10808u);
 fl_trace.record_boot=fl_trace.boot;fl_trace.operation=(uint8_t)operation;fl_trace.address=address;
 fl_trace.entry_tick=fl_trace.progress_tick=FL_TRACE_TICKS();
 fl_trace.feed_tick=fl_trace.current_feed;fl_trace.wdt=fl_trace.current_wdt;
 fl_trace.wait_tick=fl_trace.wait_progress=fl_trace.polls=fl_trace.mmio=0;
 fl_trace.failure=FL_TRACE_FAIL_NONE;fl_trace.pre_status=fl_trace.last_status=fl_trace.status_valid=0;
 fl_trace.spi_byte=0;fl_trace.result=FL_TRACE_PENDING;
 fl_trace.phase=fl_trace.stage=FL_TRACE_ENTRY;fl_trace.state=FL_TRACE_INFLIGHT;
}
FL_TRACE_INL void fl_trace_mark(uint32_t stage)
{
 if(!fl_trace_active)return;
 fl_trace.progress_tick=FL_TRACE_TICKS();fl_trace.phase=fl_trace.stage=(uint8_t)stage;
}
FL_TRACE_INL void fl_trace_wait_begin(uint32_t stage)
{
 if(!fl_trace_active||!fl_trace_sampling)return;
 fl_trace.progress_tick=fl_trace.wait_tick=fl_trace.wait_progress=FL_TRACE_TICKS();
 fl_trace.polls=0;fl_trace.stage=(uint8_t)stage;
}
FL_TRACE_INL void fl_trace_wait_progress(uint32_t polls,uint32_t mmio)
{
 if(!fl_trace_active)return;
 fl_trace.wait_progress=fl_trace.progress_tick=FL_TRACE_TICKS();
 fl_trace.polls=polls;fl_trace.mmio=mmio;
}
FL_TRACE_INL void fl_trace_spi(uint32_t byte,uint32_t rx)
{
 if(!fl_trace_active)return;
 fl_trace_byte=(uint8_t)byte;fl_trace_rx=(uint8_t)rx;fl_trace_spi_tick=FL_TRACE_TICKS();
 if(!fl_trace_sampling)return;
 fl_trace.spi_byte=(uint8_t)byte;
 fl_trace.progress_tick=fl_trace.wait_tick=fl_trace.wait_progress=fl_trace_spi_tick;
 fl_trace.polls=0;fl_trace.stage=rx?FL_TRACE_SPI_RX:FL_TRACE_SPI_TX;
}
FL_TRACE_INL void fl_trace_spi_progress(uint32_t polls,uint32_t mmio)
{
 if(!fl_trace_active)return;
 fl_trace.spi_byte=fl_trace_byte;fl_trace.stage=fl_trace_rx?FL_TRACE_SPI_RX:FL_TRACE_SPI_TX;
 fl_trace.wait_tick=fl_trace_spi_tick;fl_trace_wait_progress(polls,mmio);
}
FL_TRACE_INL void fl_trace_status(uint32_t status,uint32_t pre)
{
 if(!fl_trace_active)return;
 if(!fl_trace.status_valid||fl_trace.last_status!=(uint8_t)status) {
  fl_trace.last_status=(uint8_t)status;fl_trace.status_valid=1;
 }
 if(pre)fl_trace.pre_status=(uint8_t)status;
}
FL_TRACE_INL void fl_trace_fail(uint32_t reason)
{
 if(!fl_trace_active)return;
 if(reason==FL_TRACE_FAIL_SPI) {
  fl_trace.spi_byte=fl_trace_byte;fl_trace.stage=fl_trace_rx?FL_TRACE_SPI_RX:FL_TRACE_SPI_TX;
  fl_trace.wait_tick=fl_trace_spi_tick;
 }
 fl_trace.progress_tick=FL_TRACE_TICKS();fl_trace.failure=(uint8_t)reason;
}
FL_TRACE_INL void fl_trace_end(int32_t result)
{
 if(!fl_trace_active)return;
 fl_trace_mark(FL_TRACE_RETURN);fl_trace.result=result;fl_trace.state=FL_TRACE_RETURNED;
 fl_trace_active=0;
}
#define FL_TRACE_SAMPLE(n) (fl_trace_sampling=(((n)&4095u)==0))
#define FL_TRACE_WIP_PROGRESS(n,v) do { if(fl_trace_active&&fl_trace_sampling){fl_trace.stage=FL_TRACE_WIP;fl_trace_wait_progress(n,v);} } while(0)
#define FL_TRACE_SPI_POLL(n,v) do { if(((++(n))&4095u)==0)fl_trace_spi_progress(n,v); } while(0)
#define FL_TRACE_BEGIN(o,a) fl_trace_begin(o,a)
#define FL_TRACE_MARK(s) fl_trace_mark(s)
#define FL_TRACE_WAIT(s) fl_trace_wait_begin(s)
#define FL_TRACE_PROGRESS(n,v) fl_trace_wait_progress(n,v)
#define FL_TRACE_SPI(b,r) fl_trace_spi(b,r)
#define FL_TRACE_STATUS(v,p) fl_trace_status(v,p)
#define FL_TRACE_FAIL(r) fl_trace_fail(r)
#define FL_TRACE_END(r) fl_trace_end(r)
/* Sample long cache/SFC/SPI waits every 4096 polls, never each poll. */
#define FL_TRACE_POLL(n,v) do { if(((++(n))&4095u)==0)FL_TRACE_PROGRESS(n,v); } while(0)
#define FL_TRACE_COUNTER uint32_t trace_polls=0
#else
#define FL_TRACE_ENABLED 0
#define FL_TRACE_SAMPLE(n) ((void)0)
#define FL_TRACE_WIP_PROGRESS(n,v) ((void)0)
#define FL_TRACE_SPI_POLL(n,v) ((void)0)
#define FL_TRACE_BEGIN(o,a) ((void)0)
#define FL_TRACE_MARK(s) ((void)0)
#define FL_TRACE_WAIT(s) ((void)0)
#define FL_TRACE_PROGRESS(n,v) ((void)0)
#define FL_TRACE_SPI(b,r) ((void)0)
#define FL_TRACE_STATUS(v,p) ((void)0)
#define FL_TRACE_FAIL(r) ((void)0)
#define FL_TRACE_END(r) ((void)0)
#define FL_TRACE_POLL(n,v) ((void)0)
#define FL_TRACE_COUNTER
#endif
