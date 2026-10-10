/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual audio/UAC service and physical adapter; no MMIO/device claims. */
static void queue_publish(void);
#define RING_PUBLISH() queue_publish()
#define FELUCCA_UAC 1
#define FELUCCA_USB_HAL "../../tests/native_queue_usb_hal.h"
#define D8FLASH_QUEUE_TEST 1
#define D8FLASH_RUNTIME_NO_MAIN 1
#include "d8p1_flash_runtime_test.c"
static unsigned oracles,nest_enabled,nesting,nest_calls;
static void queue_publish(void){
 __asm__ volatile("" ::: "memory");
 if(nest_enabled&&!nesting){nesting=1;hs_regs[4][17]=0;uac_service();nest_calls++;
  uint32_t count=0;for(unsigned i=0;i<UA_N;i++)count+=ua_ring[i]!=0;CHECK(count==uq.ring);
  nesting=0;
 }
}
static void oracle(void){
 uint32_t ring=0,hist=0;for(unsigned i=0;i<UA_N;i++)ring+=ua_ring[i]!=0;
#if FELUCCA_UAC_48K
 for(unsigned i=0;i<RS_H;i++)hist+=rs.l[i]!=0||rs.r[i]!=0;
#endif
 CHECK(ring==uq.ring&&hist==uq.history);
#if FELUCCA_UAC_48K
 for(unsigned i=0;i<RS_H;i++)CHECK(rs.l[i]==rs.l[i+RS_H]&&rs.r[i]==rs.r[i+RS_H]);
#endif
 for(unsigned h=0;h<2;h++){uint32_t nz=0;for(unsigned i=0;i<HALF_WORDS;i++)nz|=(uint32_t)abuf[h*HALF_WORDS+i];CHECK((nz!=0)==audio_nonzero[h]);}
 oracles++;
}
/* Test cold reset, never a production strategy for permitting a save. */
static void cold(void){
 ui_power_on();memset(&uac,0,sizeof uac);memset(&uq,0,sizeof uq);memset(&usb,0,sizeof usb);
 memset(ua_ring,0,sizeof ua_ring);memset(ep4tx,0,sizeof ep4tx);
#if FELUCCA_UAC_48K
 memset(&rs,0,sizeof rs);
#endif
 memset(dly_buf,0,sizeof dly_buf);memset(cho_buf,0,sizeof cho_buf);memset(&fx,0,sizeof fx);rev_clear();fx_tail.delay=fx_tail.chorus=0;
 dc_l=dc_r=dce_l=dce_r=lc_l1=lc_l2=lc_r1=lc_r2=0;memset(lce,0,sizeof lce);
 sb_lp1=sb_lp2=sb_lp3=sb_lp4=sb_env=sb_h1=sb_h2=sb_hl=0;fx_lowcut=fx_usb_fixed=0;
 memset(&clk,0,sizeof clk);click_req=0;perf_held=perf_latched=perf_act=perf_solo=0;
 memset(&pf,0,sizeof pf);pf.src=pf.next=PF_N;pf.lc=PF_TOP;for(unsigned i=0;i<8;i++)pf.mg[i]=32768;
 for(unsigned i=0;i<8;i++)trk[i].p[P_SLCR]=trk[i].p[P_AMODE]=trk[i].p[P_DIST]=0;
 memset(hs_regs,0,sizeof hs_regs);hs_on=hs_done=1;hs_sends=hs_reset_writes=0;
 song.master_q12=4096;audio_init();oracle();
}
static void consume(void){hs_regs[4][17]=0;uac_service();}
static void stream(unsigned rate){usb.up=usb.config=1;uac_stream(1);uac.r48=(uint8_t)rate;uac.flowing=1;uac_render_start();}
static void drain(void){int32_t z[CTL*2]={0};for(unsigned b=0;b<160;b++){uac_render_start();uac_tap(z,CTL);consume();oracle();}}
static unsigned queue_kind;
static void queue_disturb(void){
 switch(queue_kind){
 case 0:UA_STORE(&ua_ring[UA_N-1],1);break;
 case 1:rs.l[0]=rs.l[RS_H]=1;uq.history=1;break;
 case 2:uq.packet=1;ep4tx[0]=1;break;
 case 3:uac.last=1;break;
 case 4:uq.uncertain=1;break;
 case 5:abuf[0]=128;audio_nonzero[0]=1;break;
 case 6:abuf[HALF_WORDS]=128;audio_nonzero[1]=1;break;
 }
}
int main(void){
 cold();
#if FELUCCA_UAC_TONE
 CHECK(!uac_output_quiet());flash_ok=1;reset();CHECK(d8p1_autosave_flash(1)==D8POOL_BUSY&&!reads&&!writes);stream(0);int32_t z[CTL*2]={0};uac_tap(z,CTL);oracle();CHECK(uq.ring&&!uac_output_quiet());
 printf("Native output queues benchmark: %u checks, %u failures; autonomous TONE refused\n",checks,failures);return failures!=0;
#else
 CHECK(audio_output_quiet()&&uac_output_quiet());
 /* Real final DAC writes: both stale halves, click after USB tap, then natural drain. */
 for(unsigned h=0;h<2;h++){qa_half=h;click_req=2;fm1_alnk0_irq();oracle();CHECK(audio_nonzero[h]&&!audio_output_quiet());}
 for(unsigned i=0;i<20;i++){qa_half=i%2;fm1_alnk0_irq();oracle();}CHECK(audio_output_quiet());
 /* Silent speaker cannot excuse full-level USB frames. */
 cold();stream(0);song.master_q12=0;fx_usb_fixed=1;host_preset(&trk[0],0,5);trk[0].p[P_SLCR]=0;trk_note_on(&trk[0],60,100);qa_half=0;fm1_alnk0_irq();oracle();CHECK(audio_output_quiet()&&uq.ring&&!uac_output_quiet());
 /* Full ring accounting, consumed slots retained until naturally overwritten. */
 for(unsigned rate=0;rate<2;rate++){
  cold();stream(rate);int32_t a[CTL*2];for(unsigned i=0;i<CTL*2;i++)a[i]=(i&1)?-12000:18000;
  for(unsigned b=0;b<40;b++){uac_render_start();uac_tap(a,CTL);consume();oracle();}
  CHECK(uq.ring&&!uac_output_quiet());drain();CHECK(!uq.ring&&!uq.history&&!uq.packet&&!uac.last&&uac_output_quiet());
  /* An overrun drops input without updating either occupancy/history. */
  ua_r=ua_w-UA_N;uint32_t rc=uq.ring,hc=uq.history,ov=uac.overruns;uac_tap(a,CTL);CHECK(uac.overruns==ov+1&&rc==uq.ring&&hc==uq.history);oracle();
 }
 /* TIMER5 consumes at every actual producer publication boundary. */
 cold();stream(1);nest_enabled=1;int32_t nested[CTL*2];
 for(unsigned i=0;i<CTL*2;i++)nested[i]=(i&1)?-10000:9000;
 for(unsigned b=0;b<100;b++){uac_render_start();uac_tap(nested,CTL);oracle();}
 nest_enabled=0;CHECK(nest_calls>=100&&uq.ring&&!uac_output_quiet());drain();CHECK(uac_output_quiet());
 /* Actual packet send and underrun repeat; metadata does not cancel EP4. */
 cold();stream(0);drain();uac.last=0x12340000;ua_r=ua_w;consume();CHECK(uq.packet&&!uac_output_quiet()&&hs_packet[0]==uac.last);
 uac_stream(0);CHECK(!uac.last&&uq.packet&&!uac_output_quiet());uac_ep4_reset();CHECK(hs_reset_writes&& !uq.packet&&uac_output_quiet());
 cold();stream(0);uac.last=7;ua_r=ua_w;consume();for(unsigned i=0;i<45;i++)uac_service();CHECK(!uac.flowing&&uq.packet&&!uac_output_quiet());
 uac_rate_set(48000);CHECK(!uac.go&&uq.packet&&!uac_output_quiet());consume();CHECK(!uq.packet&&uac.last==7&&!uac_output_quiet());
 uac.flowing=1;uac_render_start();drain();CHECK(uac_output_quiet());
 /* Failed SIE reads cannot be misclassified as a consumed zero packet. */
 cold();stream(0);hs_done=0;consume();CHECK(uq.uncertain&&!uac_output_quiet());hs_done=1;uac_ep4_reset();CHECK(!uac_output_quiet());
 cold();hs_on=0;uac_ep4_reset();CHECK(uq.uncertain&&!uac_output_quiet());
 /* Real runtime + adapter: late nonzero output refuses every mutation. */
 cold();FILE *f=fopen("tests/fixtures/d8p1/minimal.d8p","rb");if(!f)return 2;size_t n=fread(wire,1,sizeof wire,f);fclose(f);CHECK(!d8p1_load_runtime(wire,n,0));
 for(unsigned t=0;t<8;t++)trk[t].p[P_SLCR]=trk[t].p[P_AMODE]=trk[t].p[P_DIST]=0;
 CHECK(!d8p1_capture_runtime(wire,sizeof wire,&n));blank();flash_ok=1;for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&seed,o,wire,n,7));reset();memcpy(baseline,nor,sizeof nor);d8pool_index index;CHECK(!d8pool_inventory(&seed,&index));
 unsigned ops=2+(unsigned)((n+255)/256)+1;
 for(queue_kind=0;queue_kind<7;queue_kind++){
  cold();reset();queue_disturb();CHECK(d8p1_autosave_flash(1)==D8POOL_BUSY&&!reads&&!writes);
  for(unsigned i=0;i<ops;i++){memcpy(nor,baseline,sizeof nor);cold();reset();inject_at=(int)i;irq_inject=queue_disturb;CHECK(d8p1_autosave_flash(1)==D8POOL_IO&&writes==i);cold();reset();unchanged(&index);}
  memcpy(nor,baseline,sizeof nor);cold();reset();after_commit=queue_disturb;CHECK(d8p1_autosave_flash(1)==D8POOL_IO&&!irq_disabled);
  cold();reset();CHECK(!d8p1_restore_flash_autosave(1,1)&&proj_cur==PROJ_NO_SLOT);
 }
 memcpy(nor,baseline,sizeof nor);cold();reset();CHECK(!d8p1_autosave_flash(1)&&!irq_disabled);oracle();
 printf("Native output queues: %u checks, %u failures; %u real audio/UAC full-buffer oracles, 7 queue states x %u late mutation cuts plus postcommit restore; simulated controller, no physical pipeline/timing claim\n",checks,failures,oracles,ops);return failures!=0;
#endif
}
