/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_PRESET_SESSION_H
#define DABBL8_NATIVE_PRESET_SESSION_H
#ifndef D8_PRESET_SESSION
#define D8_PRESET_SESSION 0
#endif
#if NTRK == 8 && D8_PRESET_SESSION
#include "d8pool.h"
static struct {
 d8pool_index index;uint32_t crc[5];
 uint32_t epoch,own,started,usb,payload_crc,length,base,sequence,offset;
 uint8_t active,busy,slot,operation,object,phase,bank_done,usb_config,confirm;
} d8up __attribute__((section(".pool")));
static int d8up_ui_begin(unsigned,unsigned,const char *);
static int d8up_save(unsigned,const void *,uint32_t,int);
static void d8up_ui_end(void);
static int d8up_recover_banks(void);
#ifndef D8_UP_LCD_FAULT_COUNT
#define D8_UP_LCD_FAULT_COUNT() fm1_lcd_timeouts
#endif
static uint32_t d8up_display_id(unsigned,unsigned);
static int d8up_display_valid(unsigned,unsigned);
static void d8up_display_done(unsigned,unsigned,uint32_t,uint32_t);
static void d8up_display_clear(void){if(!d8up.active)d8up.sequence=0;}
#endif
#endif
