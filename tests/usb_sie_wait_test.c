/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual USB transaction and TIMER5 paths, simulated controller only. */
#include "native_queue_usb_hal.h"
#include <stdlib.h>
#include <assert.h>
static void d8_capture_store_changed(void) { abort(); }
static unsigned long polls;
static unsigned transactions,alternating,fail_at,slow,transaction_polls,starts;
static unsigned execute_failed_write, dma_sends;
static uint32_t counted_done(void) { polls++; return slow ? ++transaction_polls>=slow : hs_done; }
static void counted_read_start(uint32_t r) {
    transaction_polls=0;
    starts++;
    if(fail_at) hs_done=++transactions!=fail_at;
    else if(alternating) hs_done=(++transactions)%2==0;
    fm1_usb_sie_rd_start(r);
}
static void counted_write_start(uint32_t r,uint32_t v) {
    transaction_polls=0;
    starts++;
    if(fail_at) hs_done=++transactions!=fail_at;
    else if(alternating) hs_done=(++transactions)%2==0;
    if (!hs_done && execute_failed_write) {
        hs_done=1; fm1_usb_sie_wr_start(r,v); hs_done=0;
    } else fm1_usb_sie_wr_start(r,v);
}
static void counted_ep_send(uint32_t e,void *p,uint32_t n) {
    dma_sends++; fm1_usb_ep_send(e,p,n);
}
static void counted_ep0_send(void *p,uint32_t n) {
    dma_sends++; fm1_usb_ep0_send(p,n);
}
#define fm1_usb_sie_done counted_done
#define fm1_usb_sie_rd_start counted_read_start
#define fm1_usb_sie_wr_start counted_write_start
#define fm1_usb_ep_send counted_ep_send
#define fm1_usb_ep0_send counted_ep0_send
#define FELUCCA_USB_HAL "../../tests/native_queue_usb_hal.h"
#define HALF_FRAMES 128
#define T_CDC 1
#define NTRK 8
#define main unused_uac_suite_main
#include "uac_test.c"
#undef main
static struct {uint32_t timer_irqs,nested,in_audio;} felucca_dbg;
static uint32_t t5_nested_ticks;
static unsigned scans;
static uint32_t fm1_ticks(void) { return 0; } /* No elapsed-time claim. */
static void fm1_timer5_ack(void) {}
static void fm1_input_tick(void) { scans++; }
#define FM1_TICKS_PER_US 24
#define FELUCCA_TIMER5_ONLY 1
#include "../firmware/src/main.c"
static void ready(void) {
    alternating=fail_at=slow=execute_failed_write=dma_sends=0; hs_on=hs_done=1;
    memset(hs_regs,0,sizeof hs_regs); hs_index=0;
    memset(&usb,0,sizeof usb); memset(&uac,0,sizeof uac); uq.uncertain=0;
    usb_start(); assert(usb.up); polls=transactions=starts=0;
}
static unsigned failures;
static void expect(int ok,const char *scenario) {
    printf("%s: %s\n",ok ? "PASS" : "FAIL",scenario);
    if (!ok) failures++;
}
static void successful_publications(void) {
    /* Given two MIDI events spanning the ring end, when a send completes. */
    ready(); mo_r=MQ-1; mo_w=MQ+1;
    midi_out_q[MQ-1]=0x643c9009; midi_out_q[0]=0x003c8008;
    ep1_tx();
    expect(usb.up && mo_r==mo_w && usb.tx_pkts==1 && hs_regs[1][S_TXCSR1]==1 &&
           ep1tx[0]==9 && ep1tx[4]==8 && dma_sends==1,
           "MIDI commits completed packet in order across ring wrap");
    /* Given two CDC bytes spanning the ring end, when a send completes. */
    ready(); co_r=CO_N-1; co_w=CO_N+1; cdc.tx_pkts=0;
    cdc_out[CO_N-1]=19; cdc_out[0]=23; ep3_tx();
    expect(usb.up && co_r==co_w && cdc.tx_pkts==1 && hs_regs[3][S_TXCSR1]==1 &&
           ep3tx[0]==19 && ep3tx[1]==23 && dma_sends==1,
           "CDC commits completed packet in order across ring wrap");
    /* Given a primed stream, when the packet arm completes. */
    ready(); usb.config=uac.alt=uac.queued=uac.go=1; uac.fill_min=UA_LO;
    ua_r=0; ua_w=64; hs_sends=0;
    for(unsigned i=0;i<64;i++) ua_ring[i]=i+1;
    uac_service();
    expect(usb.up && ua_r==44 && uac.pkts==1 && uac.frames==44 && uac.acc==100 &&
           uac.last==44 && uac.flowing && !uac.wait && hs_sends==1 && hs_bytes==176 &&
           hs_packet[0]==1 && hs_packet[43]==44 && uq.packet && !uq.uncertain,
           "UAC commits completed packet with exact samples and cadence");
    /* Given a SET_ADDRESS request, when its status and next service complete. */
    ready(); const uint8_t setup[8]={0,5,7,0,0,0,0,0};
    memcpy(ep0buf,setup,sizeof setup); hs_regs[0][S_CSR0]=1;
    ep0_service();
    expect(usb.has_pend_addr && usb.pend_addr==7 && hs_regs[0][S_FADDR]==0,
           "SET_ADDRESS waits until after setup status");
    hs_regs[0][S_CSR0]=0; ep0_service();
    expect(usb.up && !usb.has_pend_addr && hs_regs[0][S_FADDR]==7,
           "SET_ADDRESS commits completed address write");
    /* Given a control response, when its final chunk arm completes. */
    ready(); static const uint8_t payload=19;
    usb.e0_tx=1; usb.e0_src=&payload; usb.e0_left=1; ep0_service();
    expect(usb.up && usb.e0_src==&payload+1 && !usb.e0_left && !usb.e0_tx &&
           hs_regs[0][S_CSR0]==0x0a && dma_sends==1,
           "EP0 commits completed final chunk");
}
static void late_send_failures(void) {
    for (unsigned fault=3;fault<=4;fault++) {
        /* Given one MIDI packet, when the late CSR read/arm fails. */
        ready(); mo_r=0; mo_w=1; midi_out_q[0]=0x643c9009;
        fail_at=fault; ep1_tx();
        printf("MIDI fault=%u mo_r=%u tx=%u arm=%u dma=%u\n",fault,mo_r,usb.tx_pkts,hs_regs[1][S_TXCSR1],dma_sends);
        expect(!usb.up && mo_r==0 && usb.tx_pkts==0 && hs_regs[1][S_TXCSR1]==0,
               "MIDI retains queue until completed arm");
        /* Given one CDC byte, when the late CSR read/arm fails. */
        ready(); co_r=0; co_w=1; cdc_out[0]=19; cdc.tx_pkts=0;
        fail_at=fault; ep3_tx();
        printf("CDC fault=%u co_r=%u tx=%u arm=%u dma=%u\n",fault,co_r,cdc.tx_pkts,hs_regs[3][S_TXCSR1],dma_sends);
        expect(!usb.up && co_r==0 && cdc.tx_pkts==0 && hs_regs[3][S_TXCSR1]==0,
               "CDC retains queue until completed arm");
        /* Given an EP0 chunk, when its late INDEX/CSR write fails. */
        static const uint8_t payload=19;
        ready(); usb.e0_tx=usb.e0_zlp=1; usb.e0_src=&payload; usb.e0_left=1;
        fail_at=fault; ep0_service();
        printf("EP0 fault=%u left=%u tx=%u zlp=%u dma=%u\n",fault,usb.e0_left,usb.e0_tx,usb.e0_zlp,dma_sends);
        expect(!usb.up && usb.e0_src==&payload && usb.e0_left==1 && usb.e0_tx && usb.e0_zlp,
               "EP0 retains chunk until completed arm");
    }
    /* Given a primed stream, when the final arm fails after DMA staging. */
    ready(); usb.config=uac.alt=uac.queued=uac.go=1; uac.wait=7; uac.fill_min=UA_LO;
    ua_r=0; ua_w=64; uac.last=0xabcdef01;
    for(unsigned i=0;i<64;i++) ua_ring[i]=0x12345678;
    fail_at=3; hs_sends=0; uac_service();
    printf("UAC fault=3 ua_r=%u pkts=%u frames=%u acc=%u last=%08x sends=%u arm=%u\n",
           ua_r,uac.pkts,uac.frames,uac.acc,uac.last,hs_sends,hs_regs[4][S_TXCSR1]);
    expect(!usb.up && ua_r==0 && !uac.pkts && !uac.frames && !uac.acc &&
           uac.last==0xabcdef01 && uac.wait==7 && !uac.flowing && hs_sends==1 && uq.uncertain,
           "UAC retains ring and packet state until completed arm");
    for(unsigned rate=0;rate<2;rate++) {
        /* Given an underrunning adjusted packet, when the arm fails at either rate. */
        ready(); usb.config=uac.alt=uac.go=1; uac.r48=uac.ring48=(uint8_t)rate;
        ua_r=0; ua_w=1; ua_ring[0]=0; uq.packet=1; hs_sends=0; fail_at=3;
        uac_service();
        printf("UAC adjusted fault rate48=%u ua_r=%u adj=%u underruns=%u packet=%u\n",
               rate,ua_r,uac.adj_down,uac.underruns,uq.packet);
        expect(!usb.up && ua_r==0 && !uac.acc && !uac.adj_down && !uac.underruns &&
               !uac.pkts && !uac.frames && uq.packet && uq.uncertain && hs_sends==1,
               "UAC retains adjustment, underrun and prior packet summary at failed arm");
    }
    /* Given exactly two 10000-poll readiness waits, when the IRQ allowance is exhausted. */
    ready(); usb.config=uac.alt=uac.queued=uac.go=1; uac.fill_min=UA_LO;
    ua_r=0; ua_w=64; hs_sends=0; slow=10000; sie_irq_begin();
    uac_service();
    printf("UAC exact allowance ua_r=%u sends=%u polls=%lu starts=%u arm=%u\n",
           ua_r,hs_sends,polls,starts,hs_regs[4][S_TXCSR1]);
    expect(!usb.up && ua_r==0 && !hs_sends && polls==20000 && starts==2 && uq.uncertain,
           "UAC refuses DMA and consumption at exact readiness exhaustion");
    sie_irq_end();
    /* Given a pending address, when FADDR completion is missing. */
    ready(); usb.has_pend_addr=1; usb.pend_addr=7; fail_at=1;
    ep0_service();
    printf("SET_ADDRESS up=%u pending=%u address=%u\n",usb.up,usb.has_pend_addr,hs_regs[0][S_FADDR]);
    expect(!usb.up && usb.has_pend_addr && hs_regs[0][S_FADDR]==0,
           "SET_ADDRESS remains pending until completed address write");
    /* Given a write accepted by the fake without completion, when the arm times out. */
    ready(); mo_r=0; mo_w=1; midi_out_q[0]=0x643c9009; fail_at=4; execute_failed_write=1;
    ep1_tx();
    unsigned prior_starts=starts; unsigned long prior_polls=polls;
    ep1_tx();
    printf("ambiguous MIDI mo_r=%u arm=%u starts=%u polls=%lu\n",mo_r,hs_regs[1][S_TXCSR1],starts,polls);
    expect(!usb.up && mo_r==0 && hs_regs[1][S_TXCSR1]==1 && starts==prior_starts && polls==prior_polls,
           "accepted but uncompleted write retains software state and latches fault");
}
static void receive_count_failures(void) {
    for(unsigned fault=4;fault<=5;fault++) {
        /* Given a received MIDI packet, when a count read fails. */
        ready(); mi_r=mi_w=0; usb.rx_pend=1;
        ep1rx[0]=9; ep1rx[1]=0x90; ep1rx[2]=60; ep1rx[3]=100;
        hs_regs[1][S_RXCSR1]=1; hs_regs[1][S_RXCOUNT1]=4;
        fail_at=fault; ep1_rx();
        expect(!usb.up && mi_w==0 && !usb.rx_pkts && usb.rx_pend,
               "MIDI count failure retains pending receive without adoption");
        /* Given a received CDC byte, when a count read fails. */
        ready(); ci_r=ci_w=0; cdc.rx_pend=1; cdc.rx_pkts=0;
        ep3rx[0]=19; hs_regs[3][S_RXCSR1]=1; hs_regs[3][S_RXCOUNT1]=1;
        fail_at=fault; ep3_rx();
        expect(!usb.up && ci_w==0 && !cdc.rx_pkts && cdc.rx_pend,
               "CDC count failure retains pending receive without adoption");
    }
}
static void nested_timer_failure(void) {
    /* Given slow EP4 readiness nested inside audio, when ten TIMER5 ticks run. */
    ready(); scans=0; unsigned nested=felucca_dbg.nested; felucca_dbg.in_audio=1;
    usb.config=uac.alt=uac.queued=1; ua_r=0; ua_w=1;
    ua_ring[0]=0x12345678; ep4tx[0]=0xabcdef01;
    hs_regs[4][S_TXCSR1]=1; hs_sends=0; slow=12000;
    for(unsigned i=0;i<10;i++) fm1_timer5_irq();
    expect(!usb.up && polls==20000 && scans==10 && felucca_dbg.nested==nested+10 &&
           !hs_sends && ua_r==0 && ep4tx[0]==0xabcdef01 && sie_irq_budget==UINT32_MAX,
           "nested TIMER5 shares exact allowance and restores main-loop budget");
    felucca_dbg.in_audio=0;
    /* Given a disabled SIE, when the independent audio producer taps a block. */
    int32_t audio[8]={8192,-8192,16384,-16384,24576,-24576,32767,-32767};
    uac.feed=1; ua_w=ua_r=0; unsigned prior_starts=starts;
    uac_tap(audio,4);
    expect(ua_w==4 && ua_ring[0]!=0 && starts==prior_starts && !usb.up && uq.uncertain,
           "audio producer progresses while SIE remains faulted");
}
int main(void) {
    ready(); hs_regs[0][0]=17; assert(sie_rd(0)==17 && polls==1);
    hs_done=0; (void)sie_rd(0);
    assert(!usb.up && usb.timeouts==1 && uq.uncertain);
    unsigned long stopped=polls;
    hs_done=1;
    for(unsigned i=0;i<100;i++) { (void)sie_rd(0); sie_wr(0,19); }
    assert(!usb.up && usb.timeouts==1 && polls==stopped && hs_regs[0][0]==17);
    ready(); hs_regs[0][0]=17; assert(sie_rd(0)==17 && polls==1);
    hs_done=0; sie_wr(0,19); assert(!usb.up && usb.timeouts==1 && uq.uncertain);
    stopped=polls; hs_done=1; sie_wr(0,19); assert(polls==stopped && hs_regs[0][0]==17);
    ready(); hs_on=0; (void)sie_rd(0); assert(!usb.up && uq.uncertain && polls==0);
    usb_start(); assert(!usb.up);
    hs_on=hs_done=1; fm1_ms=1001; usb_retry(fm1_ms); assert(usb.up);
    for(unsigned fault=1;fault<=2;fault++) {
        ready(); usb.config=1; uac.alt=1; uac.queued=1;
        ua_r=0; ua_w=1; ua_ring[0]=0x12345678; ep4tx[0]=0xabcdef01;
        hs_regs[4][17]=1; hs_sends=0; fail_at=fault;
        uac_service();
        assert(!usb.up && !hs_sends && ua_r==0 && ep4tx[0]==0xabcdef01);
        ready(); mo_r=0; mo_w=1; midi_out_q[0]=0x12345678; ep1tx[0]=0xa5;
        hs_regs[1][17]=1; fail_at=fault; ep1_tx();
        assert(!usb.up && mo_r==0 && ep1tx[0]==0xa5);
        ready(); co_r=0; co_w=1; cdc_out[0]=19; ep3tx[0]=0xa5;
        hs_regs[3][17]=1; fail_at=fault; ep3_tx();
        assert(!usb.up && co_r==0 && ep3tx[0]==0xa5);
        ready(); static const uint8_t payload=19;
        usb.e0_tx=1; usb.e0_src=&payload; usb.e0_left=1; ep0buf[0]=0xa5;
        hs_regs[0][17]=2; fail_at=fault; ep0_service();
        assert(!usb.up && usb.e0_left==1 && ep0buf[0]==0xa5);
    }
    ready(); alternating=1; scans=0;
    fm1_timer5_irq();
    assert(scans==1 && !usb.up && transactions==1 && polls==20000);
    ready(); slow=12000;
    for(unsigned i=0;i<5;i++) fm1_timer5_irq();
    assert(!usb.up && polls==20000); /* Individually successful waits share one IRQ cap. */
    assert(sie_irq_budget==UINT32_MAX); /* Main-loop retry is outside that IRQ cap. */
    ready(); slow=20000; sie_irq_begin(); (void)sie_rd(0);
    assert(usb.up && !sie_irq_budget && starts==1);
    (void)sie_rd(0); assert(!usb.up && starts==1 && polls==20000);
    sie_irq_end();
    successful_publications();
    late_send_failures();
    receive_count_failures();
    nested_timer_failure();
    printf("%s: USB controller fixture, %u failed regressions; successful IO, first timeout containment, main-loop retry, late publication and nested TIMER5 (%lu polls); no hardware timing qualification\n",
           failures ? "FAIL" : "PASS",failures,polls);
    return failures ? 1 : 0;
}
