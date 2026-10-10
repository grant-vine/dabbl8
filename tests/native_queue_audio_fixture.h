/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual audio ISR with simulated DMA; separate diagnostic object from UI. */
#define FM1_AUDIO_HALF 0x80u
static unsigned qa_half,qa_pending=FM1_AUDIO_HALF,qa_acks;
static uint8_t fm1_audio_pending(void){return (uint8_t)qa_pending;}
static void fm1_audio_ack_aux(uint8_t p){(void)p;}
static uint32_t fm1_audio_free_half(void){return qa_half;}
static void fm1_audio_ack_half(void){qa_acks++;}
static void fm1_audio_init(int32_t *p,uint32_t n,void (*isr)(void),uint32_t prio){(void)p;(void)n;(void)isr;(void)prio;}
void isr_alnk0(void){}
#define felucca_dbg queue_audio_dbg
#include "../firmware/src/audio.c"
#undef felucca_dbg
