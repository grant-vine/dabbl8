/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_FIRST_USE_SESSION_H
#define DABBL8_NATIVE_FIRST_USE_SESSION_H
#ifndef D8_FIRST_USE_SESSION
#define D8_FIRST_USE_SESSION (NTRK == 8 && FELUCCA_FLASH && D8_MIGRATION_SESSION)
#endif
#ifndef D8_FIRST_USE_WRITE_ENABLED
#define D8_FIRST_USE_WRITE_ENABLED 1
#endif
#if D8_FIRST_USE_SESSION
#include "native_first_use.h"
static struct {
 d8fu format;
 uint32_t epoch,own,usb,started,generation,shown,wire_crc;
 uint32_t length,offset,header_crc,lcd_faults,ancillary_crc;
 uint8_t phase,classification,revoked,busy,usb_config,commit_phase,attempted,foundation,usb_up;
} d8fi __attribute__((section(".pool")));
enum { FI_G_REVOKED=1,FI_G_PHASE=2,FI_G_WORKSPACE=4,FI_G_EPOCH=8,
 FI_G_RESET=16,FI_G_CONFIG=32,FI_G_DEADLINE=64,FI_G_QUIET=128,
 FI_G_USB_STATE=256,FI_G_TICKET=512,FI_G_STAGE=1024,FI_G_VERIFY=2048 };
#if DABBL8_TIMING_DIAGNOSTIC
/* Retained across reset; record_boot distinguishes this operation from the
 * current boot. INFLIGHT is published before entering a possibly failstop IO. */
enum { FI_DIAG_EMPTY,FI_DIAG_ACTIVE,FI_DIAG_INFLIGHT,FI_DIAG_FAILED,FI_DIAG_COMPLETE };
enum { FI_OP_STAGE,FI_OP_STEP,FI_OP_READ,FI_OP_ERASE,FI_OP_PROGRAM,FI_OP_FOUNDATION,FI_OP_READY };
#define FI_DIAG_MAGIC 0x38464932u
#define FI_DIAG_PENDING (-32768)
static volatile struct {
 uint32_t magic,boot,record_boot,reset_src,address,guards,generation;
 uint32_t epoch,own,current_epoch,resets,current_resets,elapsed;
 int32_t raw_rc,result;
 uint16_t verify_offset;
 uint8_t status,operation,phase,format_phase,sector,foundation;
 uint8_t config,current_config,usb_state,current_usb_state,reset_reason,wdt;
} d8fi_diag __attribute__((section(".noinit")));
static void d8fi_diag_boot(uint32_t,uint32_t,uint32_t);
#endif
enum { FI_IDLE,FI_REQUIRED,FI_UNCERTAIN,FI_REFUSED,FI_RELEASE,FI_EXECUTE,FI_FAILED };
static int d8fi_blocked(void){return d8fi.phase!=FI_IDLE;}
static void d8fi_revoke(void){if(d8fi_blocked()){d8fi.revoked=1;d8fi.shown=0;}}
static int d8fi_boot_classify(void);
static void d8fi_input(uint32_t,uint32_t);
static void d8fi_draw(void);
static void d8fi_poll(void);
#endif
#endif
