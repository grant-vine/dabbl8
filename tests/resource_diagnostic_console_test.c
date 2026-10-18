/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual console/CDC dispatcher + actual pool helper. HAL and stack windows are
 * simulated; this fixture does not render DSP or claim physical measurement. */
#include <assert.h>
#define main hostsim_main
#include "hostsim.c"
#undef main
#define FELUCCA_VERSION "v0.1.0-timing"
#define FELUCCA_UART 0
static uint32_t shed_count;
#define HALF_FRAMES 128u
#define FM1_TICKS_PER_US 24u
#define FM1_GAP_BINS 8u
#define FM1_NCOL 1u
#define FM1_NENC 1u
#define FM1_CRASH_MAGIC 1u
#include "audio_timing.h"
#include "fm1_stack_watermark.h"
static struct {
    uint32_t magic, halves, max_us, nested, in_audio, late, timer_irqs,
        ui_frames, last_us, cpu_q8, boots, stage, page, home, prev_stage,
        prev_page, prev_home, prev_rst, prev_frames;
} felucca_dbg;
static struct { uint32_t magic,count,vec,pc,rets,emu,sp,psr,uptime_ms,early; } fm1_crash;
static struct {
    uint32_t last,gap_max,ticks,gap_hist[FM1_GAP_BINS],cost_sum,cost_max,
        dim_n,dim_sum,on[FM1_NCOL],on_max[FM1_NCOL],enc_moves[FM1_NENC],
        enc_lost[FM1_NENC],press_n,kb_n,press_sum,press_max,kb_sum,kb_max,dac_sum,dac_max;
} fm1_in_stat;
static struct { int32_t enc_rest[FM1_NENC]; } resource_fm1_in;
static uint32_t fm1__dim_k, locked, locks, scans, drain_enabled=1, change_boot;
static char captured[65536]; static unsigned captured_n;
static uint32_t user_words[6080], super_words[1984];
static uint32_t fm1_ticks(void) { return fm1_ms * 24u; }
static uint32_t fm1__lock(void) { assert(!locked); locked=1; locks++; return 0; }
static void fm1__unlock(uint32_t key) { (void)key; assert(locked); locked=0; }
static int fm1_mem_readable(uint32_t a,uint32_t n) { (void)a;(void)n;return 0; }
static uint8_t fm1_peek8(uint32_t a) { (void)a;return 0; }
static void drain(void) {
    assert(!locked);
    while(co_r!=co_w) { assert(captured_n+1<sizeof captured); captured[captured_n++]=(char)cdc_out[co_r++%CO_N]; }
    captured[captured_n]=0;
}
static void fm1_wdt_feed(void) { assert(!locked); fm1_ms++; if(drain_enabled) drain(); }
static fm1_stack_watermarks_t fixture_stack_read(void) {
    assert(!locked); scans++;
    fm1_stack_watermarks_t out=fm1_stack_watermark_read_windows(
        (fm1_stack_window_t){(uintptr_t)user_words,(uintptr_t)(user_words+6080)},
        (fm1_stack_window_t){(uintptr_t)super_words,(uintptr_t)(super_words+1984)});
    out.user.low=0x01c74100;out.user.top=0x01c7a000;
    out.supervisor.low=0x01c7a100;out.supervisor.top=0x01c7c000;
    if(change_boot) { felucca_dbg.boots++;change_boot=0; }
    return out;
}
#define D8_RESOURCE_STACK_READ() fixture_stack_read()
#define fm1_in resource_fm1_in
#ifndef D8_RESOURCE_CONSOLE_SOURCE
#define D8_RESOURCE_CONSOLE_SOURCE "../firmware/src/console.c"
#endif
#include D8_RESOURCE_CONSOLE_SOURCE
#undef fm1_in
static void clear_output(void) { captured_n=0;captured[0]=0;co_r=co_w; }
static void command(const char *s) {
    clear_output();
    while(*s) { assert(ci_w-ci_r<CI_N);cdc_in[ci_w++%CI_N]=(uint8_t)*s++; }
    cdc_task();drain();assert(!locked);
}
#if D8_RESOURCE_DIAGNOSTIC
static void fresh(void) {
    memset(trk,0,sizeof trk);memset(heavy_owner,0,sizeof heavy_owner);
    memset((void*)&d8hp,0,sizeof d8hp);felucca_dbg.boots=17;felucca_dbg.in_audio=0;
    for(unsigned i=0;i<6080;i++) user_words[i]=FM1_STACK_WATERMARK_PATTERN;
    for(unsigned i=0;i<1984;i++) super_words[i]=FM1_STACK_WATERMARK_PATTERN;
    user_words[6070]=0;super_words[1980]=0;
}
static void live(unsigned voice) { trk[0].engine=12;trk[0].v[voice].active=1;assert(heavy_get(12,0,voice)); }
static void emit_frame(void) {
    command("resource report\r");const char *begin=strstr(captured,"d8r1 begin"),*end=strstr(captured,"d8r1 end");
    assert(begin&&end);end=strstr(end,"\r\n")+2;fwrite(begin,1,(size_t)(end-begin),stdout);
}
int main(int argc,char **argv) {
    cdc.dtr=1;fresh();
    /* A retained high line does not greet again; the main loop must see low. */
    con.dtr_seen=1;clear_output();cdc_task();drain();assert(!captured_n);
    cdc.dtr=0;cdc_task();drain();assert(!captured_n);
    cdc.dtr=1;cdc_task();drain();assert(strstr(captured,"console - 'help'"));
    clear_output();cdc_task();drain();assert(!captured_n&&!usb.uboot_req);
    command("resource start junk\r");assert(!d8hp.enabled&&!d8hp.report.generation);
    command("resourcestart\r");assert(!d8hp.enabled);
    felucca_dbg.in_audio=1;command("resource start\r");assert(!d8hp.enabled);felucca_dbg.in_audio=0;
    command("resource report\r");assert(!strstr(captured,"d8r1 begin")&&!scans);
    command("resource start\r");assert(d8hp.enabled&&d8hp.report.generation==1&&strstr(captured,"resource started 00000001 00000011"));
    command("resource start\r");assert(d8hp.report.generation==1&&strstr(captured,"refused"));
    command("resource report\r");assert(!strstr(captured,"d8r1 begin")&&!scans);
    for(unsigned i=0;i<8;i++) live(i);
    trk[1].engine=12;trk[1].v[0].active=1;assert(!heavy_get(12,1,0));assert(heavy_get(12,0,0));
    command("resource stop junk\r");assert(d8hp.enabled);
    command("resource stop\r");assert(!d8hp.enabled&&strstr(captured,"resource stopped"));
    struct d8_heavy_pool_report saved=d8hp.report;
    emit_frame();assert(!memcmp(&saved,(const void*)&d8hp.report,sizeof saved)&&scans==1);
    if(argc==2&&!strcmp(argv[1],"--frame"))return 0;
    command("resource stop\r");assert(strstr(captured,"resource paused"));
    command("status\r");assert(strstr(captured,"resource_helpers 3")&&strstr(captured,"resource_running 0"));
    change_boot=1;command("resource report\r");assert(strstr(captured,"d8r1 begin")&&!strstr(captured,"d8r1 end"));
    command("resource report\r");assert(!strstr(captured,"d8r1 begin"));
    fresh();command("resource start\r");d8hp.report.requests=UINT32_MAX;live(0);
    assert(!d8hp.enabled&&d8hp.report.invalid==D8HP_OVERFLOW);
    command("resource report\r");assert(strstr(captured,"d8r1 pool 00000001 00000002")&&strstr(captured,"d8r1 end"));
    fresh();live(0);heavy_owner[7]=heavy_owner[0];command("resource start\r");
    assert(!d8hp.enabled&&strstr(captured,"resource invalid"));
    command("resource report\r");assert(strstr(captured,"d8r1 pool 00000001 00000004"));
    fresh();command("resource start\r");d8hp.busy=1;d8hp_observe(D8HP_REUSE);d8hp.busy=0;
    command("resource report\r");assert(strstr(captured,"d8r1 pool 00000001 00000001"));
    fresh();d8hp.report.generation=UINT32_MAX;command("resource start\r");assert(strstr(captured,"refused")&&d8hp.report.generation==UINT32_MAX);
    fresh();command("resource start\r");command("resource stop\r");
    command("resource report\r");assert(strstr(captured,"d8r1 pool 00000001 00000000 00000000")&&strstr(captured,"d8r1 end"));
    /* A full output ring and stopped reader must not close a dropped reply. */
    clear_output();co_w=co_r+CO_N;memset(cdc_out,'X',CO_N);cdc.dtr=0;drain_enabled=0;con.stalled=0;
    con_resource_report();drain();assert(con.stalled&&!strstr(captured,"d8r1 end")&&!usb.uboot_req);
    fprintf(stderr,"actual resource console PASS tracks=%u profile=%u locks=%u scans=%u; simulated only\n",NTRK,D8BUILD_PROFILE,locks,scans);
    return 0;
}
#else
int main(void) {
    cdc.dtr=1;felucca_dbg.boots=17;
    command("resource start\r");assert(strstr(captured,"? (help)"));
    command("help\r");assert(!strstr(captured,"resource start|"));
    command("status\r");assert(!strstr(captured,"resource_schema"));
    assert(!scans&&!locks&&!usb.uboot_req);
    fprintf(stderr,"resource console disabled PASS tracks=%u profile=%u; no resource commands/capabilities\n",NTRK,D8BUILD_PROFILE);
    return 0;
}
#endif
