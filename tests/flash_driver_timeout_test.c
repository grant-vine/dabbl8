/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>

static volatile uint32_t sfc, spi, buf, iomap, port_out, port_dir, cache, timer;
static volatile uint32_t enc_lo, enc_hi;
static volatile uint8_t enc;
static unsigned irq_on, checks, failures, transfers, fail_transfer, cs_high;
static unsigned wel, refuse_wel, busy, busy_forever, busy_reads, issued;
static unsigned command, bytes, payload, contained, tick_reads, tick_count;
static uint8_t received;
static uint32_t tick_step, tick_script[4];
static jmp_buf stop;

static volatile uint32_t *test_register(uint32_t address)
{
    switch (address) {
    case 0x40200: return &sfc;
    case 0x40308: return &enc_hi;
    case 0x4030c: return &enc_lo;
    case 0x11c00: return &spi;
    case 0x11c08: return &buf;
    case 0x5101c: return &iomap;
    case 0x500c0: return &port_out;
    case 0x500c8: return &port_dir;
    case 0x1eee008: return &cache;
    case 0x10804:
        timer = tick_reads < tick_count ? tick_script[tick_reads] : timer + tick_step;
        tick_reads++;
        return &timer;
    default: fprintf(stderr,"Unexpected MMIO address %x\n",address); longjmp(stop,2);
    }
}

static void test_delay(uint32_t n)
{
    (void)n;
    unsigned high = port_out & 1u;
    if (!high && cs_high) command=bytes=payload=0;
    if (high && !cs_high) {
        if (command==0x06 && !busy && !refuse_wel) wel=1;
        if ((command==0x20 && bytes==3) || (command==0x02 && bytes==3 && payload)) {
            issued++; busy=1; wel=0;
        }
    }
    cs_high=high;
}

static void test_csync(void)
{
    if (cs_high || !(spi & 8u)) return;
    transfers++; spi &= ~0x8000u;
    if (transfers==fail_transfer) return;
    if (spi & 0x1000u) {
        if (command==0x05) {
            buf = (busy?1u:0u) | (wel?2u:0u);
            if (busy && !busy_forever && busy_reads && !--busy_reads) busy=0;
        } else if (command==0x35) buf=0;
        else buf=0xa5;
    } else if (!command) command=buf;
    else if (bytes<3) bytes++;
    else payload++;
    spi |= 0x8000u;
}

static _Noreturn void test_contain(void)
{
    contained=1;
    longjmp(stop,1);
}

#define FM1_FLASH_DRIVER_TEST 1
#define REG32(a) (*test_register(a))
#define FM1_FLASH_TEST_REG8(a) enc
#define FM1_FLASH_TEST_CSYNC() test_csync()
#define FM1_FLASH_TEST_DELAY(n) test_delay(n)
#define FM1_FLASH_TEST_SPI_POLLS 32u
#define FM1_FLASH_TEST_FLUSHINV(a) ((void)(a))
#define FM1_FLASH_TEST_IRQ_OFF() (irq_on=0)
#define FM1_FLASH_TEST_IRQ_ON() (irq_on=1)
#define FM1_FLASH_TEST_CONTAIN() test_contain()
#include "../firmware/hal/fm1_flash.h"

static void reset(void)
{
    sfc=0x201; spi=0x33; buf=0; iomap=0x20; port_out=1; port_dir=0;
    cache=0x4000; timer=0; enc=0; enc_lo=enc_hi=0;
    irq_on=1; cs_high=1; transfers=fail_transfer=wel=refuse_wel=busy=busy_forever=0;
    issued=command=bytes=payload=contained=tick_reads=tick_count=0;
    busy_reads=1; tick_step=2400; received=0;
}

#define CHECK(name, condition) do { checks++; if (!(condition)) { failures++; \
    printf("FAIL %s: line %d\n",name,__LINE__); } } while (0)

enum operation { ERASE, PROGRAM, READ, STATUS, JEDEC };
static int run(enum operation op)
{
    uint32_t took=0; uint8_t data=0x5a;
    if (op==ERASE) return fl_erase4k(0x97000,&took);
    if (op==PROGRAM) return fl_write(0x97000,&data,1);
    irq_on=0;
    int rc=op==READ?fl_read_ram(0x97000,&data,1):
           op==STATUS?(int)fl_status_ram():(int)fl_jedec_ram();
    received=data;
    irq_on=1;
    return rc;
}

static void expect_return(const char *name,enum operation op,int expected)
{
    int jump=setjmp(stop);
    if (!jump) {
        int rc=run(op);
        CHECK(name,rc==expected);
        CHECK(name,sfc==0x201 && (iomap&0x20) && irq_on && !contained);
    } else CHECK(name,0);
}

static void expect_containment(const char *name,enum operation op)
{
    int jump=setjmp(stop);
    if (!jump) {
        int rc=run(op);
        failures++; checks++;
        printf("FAIL %s: expected RAM containment, returned=%d SFC=%x IRQ=%u\n",name,rc,sfc,irq_on);
    } else {
        CHECK(name,jump==1 && contained);
        CHECK(name,sfc==0 && !(iomap&0x20) && !irq_on);
    }
}

int main(void)
{
    for (enum operation op=ERASE;op<=PROGRAM;op++) {
        const char *kind=op==ERASE?"erase":"program";
        reset(); expect_return(kind,op,0); CHECK(kind,issued==1);
        reset(); refuse_wel=1; expect_return("WEL refusal",op,-3); CHECK(kind,issued==0);
        reset(); busy=busy_forever=1; expect_containment("busy before command",op); CHECK(kind,issued==0);
        reset(); busy_forever=1; tick_step=24000024u;
        expect_containment("busy operation timeout",op); CHECK(kind,issued==1);
        for (unsigned transfer=1;transfer<=(op==ERASE?11u:12u);transfer++) {
            reset(); fail_transfer=transfer; expect_containment("SPI timeout",op);
            CHECK(kind,transfers==transfer);
        }
        reset(); timer=UINT32_MAX-1000u; expect_return("normal timer wrap",op,0);
        uint32_t limit=(op==ERASE?500000u:10000u)*24u, start=UINT32_MAX-11u;
        reset(); busy_forever=1; tick_count=3;
        tick_script[0]=start; tick_script[1]=start+limit; tick_script[2]=start+limit+24u;
        expect_containment("deadline and wrap",op); CHECK(kind,tick_reads==3);
        reset(); tick_count=3; tick_script[0]=start; tick_script[1]=tick_script[2]=start+limit;
        expect_return("ready at deadline",op,0); CHECK(kind,tick_reads==3);
    }
    for (enum operation op=READ;op<=JEDEC;op++) {
        reset(); expect_return("normal read/status/JEDEC",op,op==JEDEC?0xa5a5a5:0);
        if (op==READ) CHECK("read bytes",received==0xa5);
        for (unsigned transfer=1;transfer<=(op==READ?6u:op==STATUS?4u:4u);transfer++) {
            reset(); fail_transfer=transfer; expect_containment("read/status/JEDEC SPI timeout",op);
            CHECK("stopped transfer",transfers==transfer);
        }
    }
    printf("Flash HAL: %u checks, %u failures\n",checks,failures);
    return failures!=0;
}
