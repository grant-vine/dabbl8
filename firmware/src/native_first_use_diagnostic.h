/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_FIRST_USE_DIAGNOSTIC_H
#define DABBL8_NATIVE_FIRST_USE_DIAGNOSTIC_H
#if D8_FIRST_USE_SESSION && DABBL8_TIMING_DIAGNOSTIC
/* Used by the read-only CDC console; boot != record_boot means retained data
 * from an earlier boot. Status separates a completed reset from a failure. */
static void con_first_use(void)
{
 con_kv("fi_boot",d8fi_diag.boot);con_kv("fi_record_boot",d8fi_diag.record_boot);
 con_kv("fi_status",d8fi_diag.status);con_kv("fi_operation",d8fi_diag.operation);
 con_kv("fi_raw_rc",d8fi_diag.raw_rc);con_kv("fi_result",d8fi_diag.result);
 con_kx("fi_guards",d8fi_diag.guards);con_kx("fi_address",d8fi_diag.address);
 con_kv("fi_phase",d8fi_diag.phase);con_kv("fi_format_phase",d8fi_diag.format_phase);
 con_kv("fi_sector",d8fi_diag.sector);con_kx("fi_verify_offset",d8fi_diag.verify_offset);
 con_kv("fi_foundation",d8fi_diag.foundation);con_kv("fi_generation",d8fi_diag.generation);
 con_kx("fi_epoch",d8fi_diag.epoch);con_kx("fi_own",d8fi_diag.own);con_kx("fi_current_epoch",d8fi_diag.current_epoch);
 con_kv("fi_resets",d8fi_diag.resets);con_kv("fi_current_resets",d8fi_diag.current_resets);
 con_kv("fi_config",d8fi_diag.config);con_kv("fi_current_config",d8fi_diag.current_config);
 con_kv("fi_usb_state",d8fi_diag.usb_state);con_kv("fi_current_usb_state",d8fi_diag.current_usb_state);
 con_kv("fi_elapsed_ms",d8fi_diag.elapsed);con_kx("fi_reset_reason",d8fi_diag.reset_reason);
 con_kx("fi_reset_src",d8fi_diag.reset_src);con_kx("fi_wdt",d8fi_diag.wdt);
}
#endif
#endif
