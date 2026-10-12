/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_FIRST_USE_SESSION_H
#define DABBL8_NATIVE_FIRST_USE_SESSION_H
#ifndef D8_FIRST_USE_SESSION
#define D8_FIRST_USE_SESSION (NTRK == 8 && FELUCCA_FLASH && D8_MIGRATION_SESSION)
#endif
#ifndef D8_FIRST_USE_WRITE_ENABLED
#define D8_FIRST_USE_WRITE_ENABLED 0
#endif
#if D8_FIRST_USE_SESSION
#include "native_first_use.h"
static struct {
 d8fu format;
 uint32_t epoch,own,usb,started,generation,shown,wire_crc;
 uint32_t length,offset,header_crc,lcd_faults;
 uint8_t phase,classification,revoked,busy,usb_config,commit_phase,attempted;
} d8fi __attribute__((section(".pool")));
enum { FI_IDLE,FI_REQUIRED,FI_UNCERTAIN,FI_REFUSED,FI_RELEASE,FI_EXECUTE,FI_FAILED };
static int d8fi_blocked(void){return d8fi.phase!=FI_IDLE;}
static void d8fi_revoke(void){if(d8fi_blocked()){d8fi.revoked=1;d8fi.shown=0;}}
static int d8fi_boot_classify(void);
static void d8fi_input(uint32_t,uint32_t);
static void d8fi_draw(void);
static void d8fi_poll(void);
#endif
#endif
