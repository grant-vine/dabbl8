/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_LOCAL_SESSION_H
#define DABBL8_NATIVE_LOCAL_SESSION_H
#if NTRK == 8 && D8_MIGRATION_SESSION
#include "d8pool.h"
#ifndef D8_SAMPLE_SESSION
#define D8_SAMPLE_SESSION 0
#endif
#ifndef D8_PROJECT_TRANSFER_SESSION
#define D8_PROJECT_TRANSFER_SESSION 0
#endif
#if D8_PROJECT_TRANSFER_SESSION
static void d8pt_input(uint32_t,uint32_t);
static void d8pt_draw(void);
static void d8pt_poll(void);
static int d8pt_binding(void);
static void d8pt_handle(const uint8_t *,uint32_t);
#endif
#if D8_SAMPLE_SESSION
static void d8sm_input(uint32_t,uint32_t);
static void d8sm_draw(void);
static void d8sm_poll(void);
static void d8sm_handle(const uint8_t *,uint32_t);
#endif
static struct {
 d8pool_index index;
 uint32_t crc[5],generation,shown,epoch,started,usb,signature,own,sequence,offset,length,payload_crc;
 char name[13],previous_name[13];
 uint8_t intent,previous_intent,phase,revoked,busy,object,operation,block,write_phase,writing,attempted,previous_slot,usb_config;
} d8ls __attribute__((section(".pool")));
enum { LS_IDLE,LS_CONSENT,LS_RELEASE,LS_EXECUTE };
#if D8_PROJECT_TRANSFER_SESSION
/* Observations only: never read by an admission or write gate. */
enum { PTD_NONE,PTD_BASELINE,PTD_STORE,PTD_VERIFY,PTD_ADOPT,PTD_BIND,PTD_AUTOSAVE,PTD_DONE };
static volatile struct {
 uint32_t stage,raw_rc,guards,revoke;
#if DABBL8_TIMING_DIAGNOSTIC
 uint32_t magic,boot,record_boot,reset_reason,reset_src;
 uint32_t address,operation,completed,feeds,p33_failures;
#endif
} d8pt_diag
#if DABBL8_TIMING_DIAGNOSTIC && !defined(D8_PROJECT_DIAGNOSTIC_TEST)
 __attribute__((section(".noinit")))
#endif
;
static volatile uint8_t d8pt_observing; /* IRQ-visible BSS: reboot never resumes an import */
#if DABBL8_TIMING_DIAGNOSTIC
#define D8PT_DIAG_RETAINED 1
#define D8PT_DIAG_MAGIC 0x50544431u
static void d8pt_diag_boot(uint32_t reason,uint32_t source)
{
 if(d8pt_diag.magic!=D8PT_DIAG_MAGIC||d8pt_diag.boot==UINT32_MAX){
  volatile uint8_t*p=(volatile uint8_t*)&d8pt_diag;
  for(uint32_t i=0;i<sizeof d8pt_diag;i++)p[i]=0;
  d8pt_diag.magic=D8PT_DIAG_MAGIC;
 }
 d8pt_diag.boot++;d8pt_diag.reset_reason=reason;d8pt_diag.reset_src=source;
}
#endif
static void d8pt_diag_begin(unsigned importing)
{
 d8pt_observing=(uint8_t)importing;
 if(!importing)return; /* exports must preserve the preceding incident */
 d8pt_diag.stage=d8pt_diag.raw_rc=d8pt_diag.guards=d8pt_diag.revoke=0;
#if DABBL8_TIMING_DIAGNOSTIC
 d8pt_diag.address=d8pt_diag.operation=d8pt_diag.completed=d8pt_diag.feeds=d8pt_diag.p33_failures=0;
 d8pt_diag.record_boot=d8pt_diag.boot;
#endif
}
static void d8pt_observe_revoke(uint32_t reason)
{ if(d8pt_observing&&d8ls.operation==4&&d8ls.phase!=LS_IDLE)d8pt_diag.revoke|=reason; }
#endif
static int d8ls_blocked(void) { return d8ls.phase!=LS_IDLE||d8ls.busy; }
static void d8ls_revoke(void) { if(d8ls_blocked()){
#if D8_PROJECT_TRANSFER_SESSION
 d8pt_observe_revoke(0x80000000u);
#endif
 d8ls.revoked=1;d8ls.shown=0;} }
static int ls_scan(d8pool_index *,uint32_t [5]);
static void d8ls_reset(void);
static void d8ls_offline(void);
static int d8ls_autosave_begin(void);
static void d8ls_autosave_finish(int);
static int d8ls_storage_stopped(void);
static int d8ls_request(unsigned,unsigned,const char *);
static void d8ls_input(uint32_t,uint32_t);
static void d8ls_poll(void);
static void d8ls_draw(void);
static int d8ls_read_ready(void);
static int d8ls_writing(void) { return d8ls.writing; }
static int d8ls_native_begin(unsigned,unsigned,const char *);
static int d8ls_native_finish(int);
static int d8ls_driver_erase(uint32_t);
static int d8ls_driver_program(uint32_t,const void *,uint32_t);
#endif
#endif
