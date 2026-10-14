/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "../hal/fm1_stack_watermark.h"
#if D8_HEAVY_POOL_DIAGNOSTIC != D8_STACK_WATERMARK_DIAGNOSTIC
#error "Resource console requires both resource helpers together"
#endif
#define D8_RESOURCE_DIAGNOSTIC D8_HEAVY_POOL_DIAGNOSTIC
#if D8_RESOURCE_DIAGNOSTIC
#if !DABBL8_IDENTITY || !DABBL8_TIMING_DIAGNOSTIC || !FELUCCA_CDC || NTRK != 8
#error "Resource console requires private eight-track Dabbl8 timing CDC"
#endif
#ifndef D8_RESOURCE_STACK_READ
#define D8_RESOURCE_STACK_READ() fm1_stack_watermark_read()
#endif
/* cdc_task is main-loop-only. The lock is main-only and restores interrupts;
 * neither scanning nor formatting runs inside it. Nested TIMER5 input/USB
 * consumer work does not mutate owners/voices in the in_audio path. */
static void con_resource_status(void)
{
    uint32_t key = fm1__lock();
    uint32_t running = d8hp.enabled, generation = d8hp.report.generation;
    fm1__unlock(key);
    con_kv("resource_schema", 1);
    con_kv("resource_helpers", 3);
    con_kv("resource_tracks", NTRK);
    con_kv("resource_profile", D8BUILD_PROFILE);
    con_kx("resource_engine_mask", D8ENGINE_EFFECTIVE_MASK);
    con_kv("resource_uac", FELUCCA_UAC);
    con_kv("resource_running", running);
    con_kx("resource_generation", generation);
}
static void con_resource_ack(const char *text, uint32_t generation, uint32_t boots)
{
    con_puts(text); con_putc(' '); con_hex(generation, 8);
    con_putc(' '); con_hex(boots, 8); con_puts("\r\n");
}
static void con_resource_vector(uint32_t *crc, const char *label, uint32_t id,
                                const uint8_t *values)
{
    con_puts(label); con_timing_crc_word(crc, id);
    for (unsigned i = 0; i < D8HP_ENGINES; i++) con_timing_word(crc, values[i]);
    con_puts("\r\n");
}
static void con_resource_stack(uint32_t *crc, const char *label, uint32_t id,
                               fm1_stack_watermark_t value)
{
    con_puts(label); con_timing_crc_word(crc, id);
    con_timing_word(crc, value.status);
    con_timing_word(crc, (uint32_t)value.low);
    con_timing_word(crc, (uint32_t)value.top);
    con_timing_word(crc, value.capacity_bytes);
    con_timing_word(crc, value.untouched_bytes);
    con_timing_word(crc, value.observed_bytes);
    con_puts("\r\n");
}
static void con_resource_report(void)
{
    struct d8_heavy_pool_report pool;
    uint32_t key = fm1__lock(), boots = felucca_dbg.boots;
    int ready = !felucca_dbg.in_audio && d8hp_snapshot(&pool);
    fm1__unlock(key);
    if (!ready || pool.boots != boots) {
        con_puts("resource report requires a paused run in this boot\r\n"); return;
    }
    fm1_stack_watermarks_t stacks = D8_RESOURCE_STACK_READ();
    uint32_t crc = UINT32_MAX;
    con_timing_crc_word(&crc, 0x44385231u); /* D8R1 */
    con_puts("d8r1 begin");
    con_timing_word(&crc, pool.generation); con_timing_word(&crc, boots);
    con_puts("\r\nd8r1 config");
    con_timing_word(&crc, NTRK); con_timing_word(&crc, D8BUILD_PROFILE);
    con_timing_word(&crc, D8ENGINE_EFFECTIVE_MASK);
    con_timing_word(&crc, 1u | (stacks.enabled ? 2u : 0u));
    con_timing_word(&crc, FELUCCA_UAC);
    con_puts("\r\nd8r1 pool");
    con_timing_word(&crc, pool.schema); con_timing_word(&crc, pool.invalid);
    con_timing_word(&crc, pool.requests); con_timing_word(&crc, pool.allocations);
    con_timing_word(&crc, pool.reuses); con_timing_word(&crc, pool.failures);
    con_timing_word(&crc, pool.censuses);
    con_timing_word(&crc, pool.last_live_owned_slots);
    con_timing_word(&crc, pool.peak_live_owned_slots);
    con_timing_word(&crc, pool.capacity_slots); con_timing_word(&crc, pool.body_bytes);
    con_timing_word(&crc, pool.reserved_bytes); con_puts("\r\n");
    con_resource_vector(&crc, "d8r1 last", 0x4cu, pool.last_engine);
    con_resource_vector(&crc, "d8r1 peak", 0x50u, pool.peak_engine);
    con_resource_stack(&crc, "d8r1 user", 0x55u, stacks.user);
    con_resource_stack(&crc, "d8r1 supervisor", 0x53u, stacks.supervisor);
    key = fm1__lock();
    int same = !d8hp.enabled && !d8hp.busy &&
        d8hp.report.generation == pool.generation &&
        d8hp.report.boots == boots && felucca_dbg.boots == boots &&
        d8hp.report.invalid == pool.invalid;
    fm1__unlock(key);
    if (con.stalled || !same) return;
    con_puts("d8r1 end "); con_hex(pool.generation, 8); con_putc(' ');
    con_hex(boots, 8); con_putc(' '); con_hex(crc ^ UINT32_MAX, 8);
    con_puts("\r\n");
}
static void con_resource(const char *p)
{
    uint32_t command = 0;
    if (con_word(&p, "start")) command = 1;
    else if (con_word(&p, "stop")) command = 2;
    else if (con_word(&p, "report")) command = 3;
    while (*p == ' ') p++;
    if (!command || *p) { con_puts("usage: resource start|stop|report\r\n"); return; }
    if (command == 3) { con_resource_report(); return; }
    uint32_t key = fm1__lock(), boots = felucca_dbg.boots;
    int result = 0;
    if (!felucca_dbg.in_audio)
        result = command == 1 ? d8hp_start(boots) : d8hp_stop();
    uint32_t generation = d8hp.report.generation;
    int paused = !felucca_dbg.in_audio && !d8hp.enabled && !d8hp.busy &&
        generation && d8hp.report.boots == boots;
    fm1__unlock(key);
    if (command == 1) {
        if (result) con_resource_ack(result == 1 ? "resource started" : "resource invalid", generation, boots);
        else con_puts("resource start refused\r\n");
    } else if (result || paused)
        con_resource_ack(result ? "resource stopped" : "resource paused", generation, boots);
    else con_puts("resource not running\r\n");
}
#endif
