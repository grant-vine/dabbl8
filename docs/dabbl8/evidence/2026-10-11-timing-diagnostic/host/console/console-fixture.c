
/* Generated from actual candidate console.c. Omitted unrelated diagnostic bodies
 * are read-only stubs. Actual formatter/parser/dispatch/CDC loop remain. Simulated
 * watchdog drains output; no USB hardware or timing claim. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define FELUCCA_FLASH 1
#ifndef DABBL8_TIMING_DIAGNOSTIC
#define DABBL8_TIMING_DIAGNOSTIC 1
#endif
#define NTRK 8
#define HALF_FRAMES 128
#define FS 44100
#define FM1_TICKS_PER_US 24
#define APP_NAME "DABBL8"
#define FELUCCA_VERSION "v0.1.0-test"
#define CI_N 256u
#define CO_N 2048u
#define RING_PUBLISH() do {} while(0)
#include "audio_timing.h"
static struct { uint32_t boots; } felucca_dbg={17};
static struct { uint8_t dtr; } cdc={1};
static struct { uint8_t uboot_req; } usb;
static uint8_t cdc_in[CI_N],cdc_out[CO_N];
static uint32_t ci_r,ci_w,co_r,co_w,fm1_ms;
static char captured[65536];
static uint32_t captured_n,read_enabled=1,drains;
static void drain(void) {
    while(co_r!=co_w) {
        assert(captured_n+1<sizeof captured);
        captured[captured_n++]=(char)cdc_out[co_r++%CO_N];
    }
    captured[captured_n]=0;
}
static void fm1_wdt_feed(void) { fm1_ms++; if(read_enabled){drain();drains++;} }
/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Serial console on the CDC-ACM function (FELUCCA_CDC=1): read-only
 * diagnostics. Runs in the main loop (cdc_task); usb_poll moves the bytes.
 * The baud rate is ignored. Optional timing controls reset diagnostic RAM only;
 * no command writes application memory or flash. `uboot` does what the SysEx
 * soft key does. */
#define CON_LINE 64u

static struct {
    char line[CON_LINE];
    uint32_t len;
    uint8_t dtr_seen, stalled;   /* stalled: the host stopped reading, drop the rest of this reply */
} con;

static void con_putc(char c)
{
    uint32_t t0 = fm1_ms;
    while (co_w - co_r >= CO_N) {                      /* full: wait for usb_poll, briefly */
        if (!cdc.dtr || con.stalled || fm1_ms - t0 > 20u) {
            con.stalled = 1;
            return;
        }
        fm1_wdt_feed();
    }
    cdc_out[co_w % CO_N] = (uint8_t)c;
    RING_PUBLISH();
    co_w++;
}

static void con_puts(const char *s)
{
    while (*s)
        con_putc(*s++);
}

static void con_hex(uint32_t v, uint32_t digits)
{
    while (digits--)
        con_putc("0123456789ABCDEF"[(v >> (digits * 4u)) & 15u]);
}

static void con_dec(int32_t v)
{
    char b[12];
    uint32_t n = 0, u = v < 0 ? (uint32_t)-v : (uint32_t)v;
    if (v < 0)
        con_putc('-');
    do
        b[n++] = (char)('0' + u % 10u);
    while ((u /= 10u) != 0 && n < sizeof b);
    while (n)
        con_putc(b[--n]);
}

static void con_kv(const char *k, int32_t v)            /* "key value\r\n" */
{
    con_puts(k);
    con_putc(' ');
    con_dec(v);
    con_puts("\r\n");
}

static void con_kx(const char *k, uint32_t v)
{
    con_puts(k);
    con_puts(" 0x");
    con_hex(v, 8);
    con_puts("\r\n");
}

static uint32_t con_num(const char **p, int *ok)        /* decimal or 0x hex */
{
    const char *s = *p;
    uint32_t v = 0, base = 10, d, n = 0;
    while (*s == ' ')
        s++;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2, base = 16;
    for (;; s++, n++) {
        char c = *s;
        if (c >= '0' && c <= '9')
            d = (uint32_t)(c - '0');
        else if (base == 16 && c >= 'a' && c <= 'f')
            d = (uint32_t)(c - 'a' + 10);
        else if (base == 16 && c >= 'A' && c <= 'F')
            d = (uint32_t)(c - 'A' + 10);
        else
            break;
        v = v * base + d;
    }
    *ok = n > 0;
    *p = s;
    return v;
}

static int con_word(const char **p, const char *w)       /* match a whole word */
{
    const char *s = *p;
    while (*s == ' ')
        s++;
    while (*w && *s == *w)
        s++, w++;
    if (*w || (*s && *s != ' '))
        return 0;
    *p = s;
    return 1;
}

#if DABBL8_TIMING_DIAGNOSTIC
#include "audio_timing_console.h"
#endif


static void con_status(void){con_puts("status stub\r\n");}
static void con_dbg(void){}
static void con_crash(void){}
static void con_inp(void){}
static void con_params(void){}
static void con_memr(const char *p){(void)p;}
static void con_flr(const char *p){(void)p;}
static void con_exec(const char *p)
{
    if (con_word(&p, "help") || con_word(&p, "?")) {
        con_puts("status  dbg  inp  crash  params  memr ADDR [LEN]  flr OFF [LEN]  uboot yes\r\n");
#if DABBL8_TIMING_DIAGNOSTIC
        con_puts("timing start|stop|report (diagnostic, no storage writes)\r\n");
#endif
    }
#if DABBL8_TIMING_DIAGNOSTIC
    else if (con_word(&p, "timing"))
        con_timing(p);
#endif
    else if (con_word(&p, "status"))
        con_status();
    else if (con_word(&p, "dbg"))
        con_dbg();
    else if (con_word(&p, "crash"))
        con_crash();
    else if (con_word(&p, "inp"))
        con_inp();
    else if (con_word(&p, "params"))
        con_params();
    else if (con_word(&p, "memr"))
        con_memr(p);
#if FELUCCA_FLASH
    else if (con_word(&p, "flr"))
        con_flr(p);
#endif
    else if (con_word(&p, "uboot")) {
        if (con_word(&p, "yes")) {
            con_puts("entering UBOOT\r\n");
            usb.uboot_req = 1;                         /* main loop: same path as the SysEx key */
        } else {
            con_puts("type 'uboot yes'\r\n");
        }
    } else if (*p)
        con_puts("? (help)\r\n");
}

static void cdc_task(void)                              /* main loop */
{
    if (cdc.dtr && !con.dtr_seen) {
        con_puts("\r\n" APP_NAME " ");
        con_puts(FELUCCA_VERSION);
        con_puts(" console - 'help'\r\n> ");
    }
    con.dtr_seen = cdc.dtr;
    while (ci_r != ci_w) {
        char c;
        RING_PUBLISH();                                /* usb_poll (producer) can preempt us */
        c = (char)cdc_in[ci_r % CI_N];
        RING_PUBLISH();
        ci_r++;
        if (c == '\r' || c == '\n') {
            if (c == '\n' && con.len == 0)
                continue;                              /* the LF of a CR LF */
            con_puts("\r\n");
            con.line[con.len] = 0;
            con.stalled = 0;
            con_exec(con.line);
            con.len = 0;
            con_puts("> ");
        } else if (c == 0x7F || c == 0x08) {
            if (con.len) {
                con.len--;
                con_puts("\b \b");
            }
        } else if (c >= ' ' && c < 0x7F && con.len + 1u < CON_LINE) {
            con.line[con.len++] = c;
            con_putc(c);
        }
    }
}

static void command(const char *s) {
    while(*s) {
        assert(ci_w-ci_r<CI_N);
        cdc_in[ci_w++%CI_N]=(uint8_t)*s++;
    }
    cdc_task();drain();
}
int main(void) {
    command("timing report\r\n");
#if DABBL8_TIMING_DIAGNOSTIC
    assert(strstr(captured,"requires a stopped run"));
    command("timing start junk\r");assert(!audio_timing.enabled);
    command("timingstart\r");assert(!audio_timing.enabled);
    command("timing start\r");assert(audio_timing.enabled && audio_timing.generation==1);
    command("timing start\r");assert(audio_timing.generation==1);
    command("timing report\r");assert(!strstr(captured,"d8t1 begin"));
    for(uint32_t i=0;i<1024;i++)audio_timing_record(i*100,i*120,UINT32_MAX-20+i);
    command("timing stop junk\r");assert(audio_timing.enabled);
    command("timing stop\r");assert(!audio_timing.enabled);
    unsigned char state[sizeof audio_timing];
    memcpy(state,(const void*)&audio_timing,sizeof state);
    command("timing report\r");
    assert(!memcmp(state,(const void*)&audio_timing,sizeof state));
    assert(strstr(captured,"d8t1 end") && drains && !con.stalled && !usb.uboot_req);
    const char *begin=strstr(captured,"d8t1 begin");
    const char *end=strstr(begin,"d8t1 end");end=strstr(end,"\r\n")+2;
    assert(begin && end);
    fwrite(begin,1,(size_t)(end-begin),stdout);
    command("timing stop\r");assert(!memcmp(state,(const void*)&audio_timing,sizeof state));
    command("status\r");assert(strstr(captured,"status stub"));
    /* A disconnected DTR reader produces an incomplete frame, not success. */
    captured_n=0;captured[0]=0;co_r=co_w;read_enabled=0;cdc.dtr=0;
    command("timing report\r");assert(con.stalled);
    assert(strstr(captured,"d8t1 begin") && !strstr(captured,"d8t1 end"));
#else
    assert(!audio_timing.enabled && !audio_timing.generation);
    command("timing start\r");assert(!audio_timing.enabled);
    command("help\r");assert(!strstr(captured,"timing start|"));
    assert(!strstr(captured,"d8t1 begin") && !usb.uboot_req);
    command("status\r");assert(strstr(captured,"status stub"));
#endif
    fprintf(stderr,"actual console slice: exact word parsing, argument refusal, dispatch, CDC framing, paused state, read-only status and DTR truncation PASS; simulated only\n");
}
