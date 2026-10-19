/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* FM-1 SPI NOR at runtime: erase / program / read. Used only when
 * FELUCCA_FLASH is 1.
 *
 * Everything that executes while the SFC (XIP) is switched off lives in
 * .ram_text and touches only SFRs, the stack and RAM buffers: no .rodata, no
 * libcalls, no XIP-resident helpers. Call the fl_*_ram() entry points with
 * interrupts disabled. Same sequence as the AC79 SDK flash driver
 * (cpu.a/vm_sfc.c.o: enter_spi_code, norflash_erase, norflash_wait_ok, exit_spi_code).
 */
#pragma once
#include <stdint.h>
#include "fm1_xip.h"

#ifdef FM1_FLASH_DRIVER_TEST
#define RAMFN
#define RAMINL static inline
#else
#define RAMFN  __attribute__((section(".ram_text"), noinline, used))
#define RAMINL static inline __attribute__((always_inline))
#endif

/* XIP (0x02xxxxxx) -> RAM (0x01C0xxxx) is beyond the 23-bit call range:
 * call RAM code through a pointer the compiler cannot fold back. */
static inline void *fl_far(void *p) { void *volatile q = p; return q; }
#define FL_FAR(fn) ((__typeof__(&fn))fl_far((void *)&fn))

#ifndef REG32
#define REG32(a) (*(volatile uint32_t *)(a))
#endif
#define SFC_CON         REG32(0x40200u)            /* bit31 busy; [11:8] read mode */
#ifdef FM1_FLASH_DRIVER_TEST
#define SFCENC_CON      FM1_FLASH_TEST_REG8(0x40300u)
#else
#define SFCENC_CON      (*(volatile uint8_t *)0x40300u)  /* b0 decrypt, b1 plain window */
#endif
#define SFCENC_UNENC_H  REG32(0x40308u)            /* WO, XIP (CPU) address */
#define SFCENC_UNENC_L  REG32(0x4030Cu)            /* WO, XIP (CPU) address */
#define SPI0_CON        REG32(0x11C00u)            /* b15 PND, b14 PCLR, b12 DIR=rx */
#define SPI0_BUF        REG32(0x11C08u)
#define IOMAP_CON0      REG32(0x5101Cu)            /* b5: 1 = SFC owns the flash pins */
#define PORTD_OUT       REG32(0x500C0u)            /* PD0 = flash CS# */
#define PORTD_DIR       REG32(0x500C8u)
#define CACHE_CON       REG32(0x1EEE008u)          /* b14 = cache idle */
#define T4_CNT          REG32(0x10804u)            /* TIMER4, 24 MHz free-running */
#include "fm1_flash_trace.h"

#define FL_XIP(off)     FM1_XIP(off)              /* fm1_xip.h */
#define FL_DATA_LO      0x00097000u                /* Felucca main store */
#define FL_DATA_HI      0x000E0000u
#define FL_GLOB_LO      0x000FC000u                /* Felucca superblock / globals */
#define FL_GLOB_HI      0x000FF000u
#define FL_OTA_LO       0x000E0000u                /* M-UPGRADE loader staging, ota.c */
#define FL_OTA_HI       0x000E5000u
#define FL_AUTO_LO      0x000E5000u                /* 1.2: the autosave (storage.c OBJ_AUTOSAVE), A/B, just above */
#define FL_AUTO_HI      0x000E7000u                /* the OTA staging; 0xE7000 stays free */
/* [off, off + n) inside [lo, hi), without wrapping: off + n can overflow, and
 * the 1 MiB part ignores the high address bits, so a wrapped range lands low. */
#define FL_IN(off, n, lo, hi) ((uint32_t)(off) >= (lo) && (uint32_t)(off) <= (hi) && \
                               (uint32_t)(n) <= (hi) - (uint32_t)(off))
/* Felucca's own store (projects, user samples; settings; the autosave) */
#define FL_STORE_OK(off, n) (FL_IN(off, n, FL_DATA_LO, FL_DATA_HI) || FL_IN(off, n, FL_GLOB_LO, FL_GLOB_HI) || \
                             FL_IN(off, n, FL_AUTO_LO, FL_AUTO_HI))
/* Where the RAM driver may erase / program. The app build allows only its own
 * data regions; the update loader (firmware/loader) defines its own window. */
#ifndef FL_RANGE_OK
#define FL_RANGE_OK(off, n) (FL_STORE_OK(off, n) || FL_IN(off, n, FL_OTA_LO, FL_OTA_HI))
#endif

typedef struct { uint32_t sfc_con, spi_con; } fl_saved_t;

#ifdef FM1_FLASH_DRIVER_TEST
RAMINL void fl_csync(void) { FM1_FLASH_TEST_CSYNC(); }
#else
RAMINL void fl_csync(void) { __asm__ volatile("csync" ::: "memory"); }
#endif
#if !defined(FELUCCA_LOADER) || !FELUCCA_LOADER
/* Readiness is unknown: keep XIP off and IRQs masked until the already-armed
 * hardware watchdog resets. No XIP helper, fault handler or watchdog feed. */
RAMINL __attribute__((noreturn)) void fl_failstop(void)
{
#ifdef FM1_FLASH_DRIVER_TEST
    FM1_FLASH_TEST_CONTAIN();
#else
    for (;;) fl_csync();
#endif
}
#define FL_READY_TIMEOUT() fl_failstop()
#else
#define FL_READY_TIMEOUT() return -4
#endif
RAMINL void fl_cache_idle(void)
{
    FL_TRACE_COUNTER;
    uint32_t v;
    while (!((v=CACHE_CON) & 0x4000u)) { FL_TRACE_POLL(trace_polls,v); }
}
#ifdef FM1_FLASH_DRIVER_TEST
RAMINL void fl_delay(uint32_t n) { FM1_FLASH_TEST_DELAY(n); }
#else
RAMINL void fl_delay(uint32_t n) { volatile uint32_t i = n; while (i--) ; }
#endif

RAMINL void fl_cs(int high)
{
    PORTD_DIR &= ~1u;
    if (high) PORTD_OUT |= 1u; else PORTD_OUT &= ~1u;
    fl_delay(10);
}
RAMINL int fl_spi_wait(void)
{
#ifdef FM1_FLASH_DRIVER_TEST
    uint32_t n = FM1_FLASH_TEST_SPI_POLLS;
#else
    uint32_t n = 50000000u;
#endif
    FL_TRACE_COUNTER;
    uint32_t v;
    fl_csync();
    while (!((v=SPI0_CON) & 0x8000u)) {
        FL_TRACE_SPI_POLL(trace_polls,v);
        if (--n == 0) break;
    }
#if !defined(FELUCCA_LOADER) || !FELUCCA_LOADER
    if (!n) { FL_TRACE_FAIL(FL_TRACE_FAIL_SPI); fl_failstop(); }
#endif
    SPI0_CON |= 0x4000u;
    return n ? 0 : -1;
}
RAMINL void fl_tx(uint8_t b)
{
    SPI0_CON |= 8u; SPI0_CON &= ~0x1000u;
    FL_TRACE_SPI(b,0);
    SPI0_BUF = b; fl_spi_wait();
    SPI0_CON &= ~8u;
}
RAMINL uint8_t fl_rx(void)
{
    uint8_t v;
    SPI0_CON |= 8u; SPI0_CON |= 0x1000u;
    FL_TRACE_SPI(0xFFu,1);
    SPI0_BUF = 0xFFu; fl_spi_wait();
    v = (uint8_t)SPI0_BUF;
    SPI0_CON &= ~8u;
    return v;
}
RAMINL void fl_addr(uint32_t a) { fl_tx((uint8_t)(a >> 16)); fl_tx((uint8_t)(a >> 8)); fl_tx((uint8_t)a); }
RAMINL uint8_t fl_rdsr(uint8_t cmd) { uint8_t v; fl_cs(0); fl_tx(cmd); v = fl_rx(); fl_cs(1); return v; }
RAMINL void fl_wren(void) { fl_cs(0); fl_tx(0x06); fl_cs(1); }

RAMINL int fl_enter(fl_saved_t *s)
{
    uint32_t mode;
    FL_TRACE_MARK(FL_TRACE_CACHE_ENTRY);FL_TRACE_WAIT(FL_TRACE_CACHE_ENTRY);
    fl_cache_idle();
    s->sfc_con = SFC_CON;
    s->spi_con = SPI0_CON;
    mode = (s->sfc_con >> 8) & 0xFu;
    if (mode == 6u || mode == 7u)                 /* continuous-read XIP: not handled here */
        return -1;
    FL_TRACE_MARK(FL_TRACE_SFC_ENTRY);FL_TRACE_WAIT(FL_TRACE_SFC_ENTRY);
    FL_TRACE_COUNTER;
    uint32_t v;
    fl_csync();
    while ((int32_t)(v=SFC_CON) < 0) { FL_TRACE_POLL(trace_polls,v); } /* SFC idle */
    FL_TRACE_MARK(FL_TRACE_SPI_SETUP);
    SFC_CON = 0;                                  /* XIP OFF from here on */
    IOMAP_CON0 &= ~0x20u;                         /* pins -> SPI0 / GPIO */
    SPI0_CON = 0;
    SPI0_CON = 0x1029u;                           /* SDK default */
    fl_csync();
    return 0;
}
RAMINL void fl_exit(const fl_saved_t *s)
{
    FL_TRACE_MARK(FL_TRACE_SFC_EXIT);FL_TRACE_WAIT(FL_TRACE_SFC_EXIT);
    FL_TRACE_COUNTER;
    uint32_t v;
    SPI0_CON = s->spi_con;
    IOMAP_CON0 |= 0x20u;
    fl_csync();
    while ((int32_t)(v=SFC_CON) < 0) { FL_TRACE_POLL(trace_polls,v); }
    SFC_CON = s->sfc_con;                         /* XIP back on */
    fl_csync();
}
RAMINL void fl_inval(uint32_t off, uint32_t len)
{
    uint32_t a = FL_XIP(off) & ~31u, e = FL_XIP(off) + len;
    FL_TRACE_MARK(FL_TRACE_INVALIDATE);FL_TRACE_WAIT(FL_TRACE_CACHE_INVALIDATE);
    fl_cache_idle();
    for (; a < e; a += 32u) {
        fl_csync();
#ifdef FM1_FLASH_DRIVER_TEST
        FM1_FLASH_TEST_FLUSHINV(a);
#else
        __asm__ volatile("flushinv [%0]" :: "r"(a) : "memory");
#endif
        fl_csync();
        fl_cache_idle();
    }
}
RAMINL int fl_wait_ready(uint32_t max_us, uint32_t *took_us)
{
    FL_TRACE_MARK(FL_TRACE_WIP);
    uint32_t t0 = T4_CNT, dt;
    FL_TRACE_COUNTER;
    for (;;) {
        dt = (T4_CNT - t0) / 24u;
        FL_TRACE_SAMPLE(trace_polls);
        uint8_t status=fl_rdsr(0x05);
        FL_TRACE_STATUS(status,0);FL_TRACE_WIP_PROGRESS(trace_polls,status);
        if (!(status & 1u)) { *took_us = dt; return 0; }
        FL_TRACE_POLL(trace_polls,status);
        if (dt > max_us) { *took_us = dt; FL_TRACE_FAIL(FL_TRACE_FAIL_WIP); FL_READY_TIMEOUT(); }
    }
}

/* ---- entry points: IRQs OFF, args/buffers in RAM ---- */

static RAMFN uint32_t fl_jedec_ram(void)                 /* expect 0x856014 */
{
    fl_saved_t s; uint32_t id;
    FL_TRACE_BEGIN(FL_TRACE_JEDEC,0);
    if (fl_enter(&s)) { FL_TRACE_END(0);return 0; }
    FL_TRACE_MARK(FL_TRACE_COMMAND);
    fl_cs(0); fl_tx(0x9F);
    id = (uint32_t)fl_rx() << 16; id |= (uint32_t)fl_rx() << 8; id |= fl_rx();
    fl_cs(1);
    fl_exit(&s);
    FL_TRACE_END((int32_t)id);return id;
}

static RAMFN uint32_t fl_status_ram(void)                /* SR1 | SR2 << 8 */
{
    fl_saved_t s; uint32_t v;
    FL_TRACE_BEGIN(FL_TRACE_STATUS,0);
    if (fl_enter(&s)) { FL_TRACE_END(-1);return 0xFFFFFFFFu; }
    FL_TRACE_MARK(FL_TRACE_PRE_STATUS);
    v = fl_rdsr(0x05) | ((uint32_t)fl_rdsr(0x35) << 8);
    fl_exit(&s);
    FL_TRACE_STATUS(v,1);FL_TRACE_END((int32_t)v);return v;
}

#ifndef FL_ERASE_PREPARE
#define FL_ERASE_PREPARE() ((void)0)
#endif
static RAMFN int fl_erase4k_ram(uint32_t off, uint32_t *took_us)
{
    fl_saved_t s; int rc;
    FL_TRACE_BEGIN(FL_TRACE_ERASE,off);
    if (!FL_RANGE_OK(off, 0x1000u) || (off & 0xFFFu))
        { FL_TRACE_END(-1);return -1; }
    FL_ERASE_PREPARE();                 /* before XIP is suspended; caller already holds IRQs off */
    if (fl_enter(&s)) { FL_TRACE_END(-2);return -2; }
    FL_TRACE_MARK(FL_TRACE_COMMAND);
    fl_wren();
#if !defined(FELUCCA_LOADER) || !FELUCCA_LOADER
    FL_TRACE_MARK(FL_TRACE_PRE_STATUS);
    uint8_t status = fl_rdsr(0x05);
    FL_TRACE_STATUS(status,1);
    if (status & 1u) { FL_TRACE_FAIL(FL_TRACE_FAIL_PRE_BUSY);fl_failstop(); }
    if (!(status & 2u)) rc = -3;
#else
    if (!(fl_rdsr(0x05) & 2u)) rc = -3;           /* WEL did not set */
#endif
    else {
        FL_TRACE_MARK(FL_TRACE_COMMAND);
        fl_cs(0); fl_tx(0x20); fl_addr(off); fl_cs(1);
        FL_TRACE_MARK(FL_TRACE_ERASE_ISSUED);
        fl_inval(off, 4096u);
        rc = fl_wait_ready(500000u, took_us);     /* generic NOR max 400 ms */
    }
    fl_exit(&s);
    FL_TRACE_END(rc);return rc;
}

static RAMFN int fl_prog_ram(uint32_t off, const uint8_t *src, uint32_t n, uint32_t *took_us)
{
    fl_saved_t s; int rc; uint32_t i;
    FL_TRACE_BEGIN(FL_TRACE_PROGRAM,off);
    if (!FL_RANGE_OK(off, n) ||
        n == 0 || n > 256u || ((off & 0xFFu) + n) > 256u) { FL_TRACE_END(-1);return -1; } /* one page, no wrap */
    if (fl_enter(&s)) { FL_TRACE_END(-2);return -2; }
    FL_TRACE_MARK(FL_TRACE_COMMAND);
    fl_wren();
#if !defined(FELUCCA_LOADER) || !FELUCCA_LOADER
    FL_TRACE_MARK(FL_TRACE_PRE_STATUS);
    uint8_t status = fl_rdsr(0x05);
    FL_TRACE_STATUS(status,1);
    if (status & 1u) { FL_TRACE_FAIL(FL_TRACE_FAIL_PRE_BUSY);fl_failstop(); }
    if (!(status & 2u)) rc = -3;
#else
    if (!(fl_rdsr(0x05) & 2u)) rc = -3;
#endif
    else {
        FL_TRACE_MARK(FL_TRACE_COMMAND);
        fl_cs(0); fl_tx(0x02); fl_addr(off);
        for (i = 0; i < n; i++) fl_tx(src[i]);
        fl_cs(1);
        FL_TRACE_MARK(FL_TRACE_PROGRAM_ISSUED);
        fl_inval(off, n);
        rc = fl_wait_ready(10000u, took_us);      /* generic NOR max 3 ms */
    }
    fl_exit(&s);
    FL_TRACE_END(rc);return rc;
}

static RAMFN int fl_read_ram(uint32_t off, uint8_t *dst, uint32_t n)   /* 0x0B fast read */
{
    fl_saved_t s; uint32_t i;
    FL_TRACE_BEGIN(FL_TRACE_READ,off);
    if (fl_enter(&s)) { FL_TRACE_END(-2);return -2; }
    FL_TRACE_MARK(FL_TRACE_COMMAND);
    fl_cs(0); fl_tx(0x0B); fl_addr(off); fl_tx(0xFF);
    for (i = 0; i < n; i++) dst[i] = fl_rx();
    fl_cs(1);
    fl_exit(&s);
    FL_TRACE_END(0);return 0;
}

/* ---- XIP side (normal .text) ---- */

/* Once at boot, before any XIP read of the data region: map the data area
 * as an unencrypted XIP window (as the SDK's IOCTL_SET_VM_INFO does). */
static void fl_plain_window_init(void)
{
    SFCENC_CON &= (uint8_t)~2u;
    SFCENC_UNENC_L = FL_XIP(0x93000u);            /* 0x0208F000 */
    SFCENC_UNENC_H = 0x07FFFFFFu;
    SFCENC_CON |= 2u;
#ifdef FM1_FLASH_DRIVER_TEST
    FM1_FLASH_TEST_IRQ_OFF();
#else
    __asm__ volatile("cli" ::: "memory");
#endif
    fl_inval(0x93000u, 0x100000u - 0x93000u);     /* drop any decrypted lines */
#ifdef FM1_FLASH_DRIVER_TEST
    fl_csync(); FM1_FLASH_TEST_IRQ_ON();
#else
    __asm__ volatile("csync\n\tsti" ::: "memory");
#endif
}

#ifdef FM1_FLASH_DRIVER_TEST
static inline uint32_t irq_save(void) { FM1_FLASH_TEST_IRQ_OFF(); return 0; }
static inline void irq_restore(uint32_t f) { (void)f; fl_csync(); FM1_FLASH_TEST_IRQ_ON(); }
#else
static inline uint32_t irq_save(void) { __asm__ volatile("cli" ::: "memory"); return 0; }
static inline void irq_restore(uint32_t f) { (void)f; __asm__ volatile("csync\n\tsti" ::: "memory"); }
#endif

static int fl_erase4k(uint32_t off, uint32_t *took_us)
{
    uint32_t f = irq_save();
    int rc = FL_FAR(fl_erase4k_ram)(off, took_us);
    irq_restore(f);
    return rc;
}

static int fl_write(uint32_t off, const uint8_t *src_ram, uint32_t n)
{
    uint32_t took;
    while (n) {
        uint32_t chunk = 256u - (off & 0xFFu);
        int rc;
        uint32_t f;
        if (chunk > n) chunk = n;
        f = irq_save();
        rc = FL_FAR(fl_prog_ram)(off, src_ram, chunk, &took);
        irq_restore(f);
        if (rc) return rc;
        off += chunk; src_ram += chunk; n -= chunk;
    }
    return 0;
}

/* Read back through XIP (needs fl_plain_window_init) and compare. */
static int fl_verify_xip(uint32_t off, const uint8_t *ref, uint32_t n)
{
    const volatile uint8_t *p = (const volatile uint8_t *)FL_XIP(off);
    uint32_t i;
    for (i = 0; i < n; i++) if (p[i] != ref[i]) return -(int)(i + 1);
    return 0;
}
