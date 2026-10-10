/* SPDX-License-Identifier: GPL-3.0-only */
/* Simulated USB controller, not a firmware HAL replacement. Actual usb.c
 * sie_rd/wr and service logic use these register-level operations. */
#pragma once
#include <stdint.h>
static uint8_t hs_regs[16][32],hs_index;
static unsigned hs_on=1,hs_done=1,hs_data,hs_sends,hs_reset_writes;
static uint32_t hs_packet[64],hs_bytes;
static uint32_t fm1_usb_sie_on(void){return hs_on;}
static void fm1_usb_sie_wr_start(uint32_t r,uint32_t v){
 if(!hs_done)return;
 if(r==14)hs_index=(uint8_t)v;
 else {hs_regs[hs_index][r]=(uint8_t)v;if(hs_index==4&&r==17&&v==0x48)hs_reset_writes++;}
}
static void fm1_usb_sie_rd_start(uint32_t r){hs_data=r==14?hs_index:hs_regs[hs_index][r];}
static uint32_t fm1_usb_sie_done(void){return hs_done;}
static uint32_t fm1_usb_sie_data(void){return hs_data;}
static void fm1_usb_ep4_send(void *p,uint32_t n){hs_sends++;hs_bytes=n;__builtin_memcpy(hs_packet,p,n);}
static void fm1_usb_ep4_txbuf(void *p){(void)p;}
static void fm1_usb_reset(void){}
static void fm1_usb_attach(void *p){(void)p;}
static void fm1_usb_off(void){}
static void fm1_usb_ep0_buf(void *p){(void)p;}
static void fm1_usb_ep0_send(void *p,uint32_t n){(void)p;(void)n;}
static void fm1_usb_ep_send(uint32_t e,void *p,uint32_t n){(void)e;(void)p;(void)n;}
static void fm1_usb_ep_txbuf(uint32_t e,void *p){(void)e;(void)p;}
static void fm1_usb_ep_rxbuf(uint32_t e,void *p){(void)e;(void)p;}
static void fm1_usb_ep_enable(uint32_t e){(void)e;}
static void fm1_usb_rx_sync(void){}
static uint32_t fm1_usb_sof_take(void){return 0;}
