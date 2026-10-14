/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#if FL_TRACE_ENABLED
/* Called by the existing read-only status command, after XIP is available. */
static void con_flash_trace(void)
{
 con_kv("fl_boot",fl_trace.boot);con_kv("fl_record_boot",fl_trace.record_boot);
 con_kv("fl_state",fl_trace.state);con_kv("fl_operation",fl_trace.operation);
 con_kv("fl_stage",fl_trace.stage);con_kv("fl_phase",fl_trace.phase);
 con_kv("fl_failure",fl_trace.failure);con_kv("fl_result",fl_trace.result);
 con_kx("fl_address",fl_trace.address);con_kx("fl_entry_tick",fl_trace.entry_tick);
 con_kx("fl_progress_tick",fl_trace.progress_tick);con_kx("fl_feed_tick",fl_trace.feed_tick);
 con_kx("fl_wait_tick",fl_trace.wait_tick);con_kx("fl_wait_progress",fl_trace.wait_progress);
 con_kv("fl_polls",fl_trace.polls);con_kx("fl_mmio",fl_trace.mmio);
 con_kx("fl_pre_status",fl_trace.pre_status);con_kx("fl_last_status",fl_trace.last_status);
 con_kv("fl_status_valid",fl_trace.status_valid);con_kx("fl_spi_byte",fl_trace.spi_byte);
 con_kx("fl_reset_reason",fl_trace.reset_reason);con_kx("fl_reset_src",fl_trace.reset_src);
 con_kx("fl_wdt",fl_trace.wdt);con_kx("fl_current_wdt",fl_trace.current_wdt);
 con_kx("fl_current_feed",fl_trace.current_feed);
}
#endif
