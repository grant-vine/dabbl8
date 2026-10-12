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
static int d8ls_blocked(void) { return d8ls.phase!=LS_IDLE||d8ls.busy; }
static void d8ls_revoke(void) { if(d8ls_blocked()){d8ls.revoked=1;d8ls.shown=0;} }
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
