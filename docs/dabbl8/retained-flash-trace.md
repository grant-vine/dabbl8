<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Retained flash-driver trace

Private `DABBL8_TIMING_DIAGNOSTIC=1` application builds add `fl_*` fields to the
existing read-only CDC `status` response. The record occupies 68 bytes in
`.noinit`. Disabled and loader builds omit it. No new SPI commands, watchdog
feeds, erase regions, timeout policy or recovery authorization are introduced.

`fi_*` remains the outer consent/formatter record. Its `fi_elapsed_ms` is consent
age sampled before I/O. Use the new raw driver ticks to investigate a stall;
that outer value is not an erase duration.

## Incident lifetime

`fl_boot` identifies the current boot. `fl_record_boot` identifies the operation
whose fields follow. `fl_state=1` means the RAM driver entered and has not
published its return. `fl_result=-32768` is the pending sentinel. State 2 means
it returned; the exact result may be success, data returned by a probe, or a
refusal. State 0 has no incident.

A previous boot's INFLIGHT incident survives subsequent JEDEC, status and read
calls. Those calls still execute normally but cannot replace the incident.
Current watchdog/reset context can change without changing its operation fields.
A subsequent admitted erase/program replaces it. The record confers no write
permission and does not resume formatting. Live formatter state is cleared by
startup, and the ordinary first-use consent route remains required.

Retention depends on RAM surviving reset. It is not evidence surviving power
loss. `fl_reset_reason` and `fl_reset_src` describe the current boot's reset;
compare boot identities before attributing the operation to that boot.

## Fields and codes

Operation codes are 0 JEDEC, 1 status, 2 read, 3 erase, 4 program.
`fl_address` is the physical operation address (zero for JEDEC/status).
`fl_phase` identifies the enclosing driver phase. `fl_stage` identifies the
last recorded transition or wait, including a SPI transfer within that phase.

| Code | Stage / phase |
| --- | --- |
| 0 | Driver entry |
| 1 | Cache entry wait |
| 2 | SFC entry wait |
| 3 | SPI setup / XIP switch-off |
| 4 | Command transfer (WREN, read, erase or program) |
| 5 | Existing pre-command status transaction |
| 6 | Erase command and address sent, CS deasserted |
| 7 | Program command and payload sent, CS deasserted |
| 8 | Invalidation entered |
| 9 | Invalidation cache wait |
| 10 | WIP polling; last sampled status transaction completed |
| 11 | SPI transmit wait; `fl_spi_byte` is the outgoing byte |
| 12 | SPI receive wait; outgoing dummy byte is `0xFF` |
| 13 | SFC exit / restoration |
| 14 | Driver return published |

Failure codes are 0 no deliberate failstop recorded, 1 SPI poll-budget failstop,
2 pre-command BUSY failstop, and 3 WIP deadline failstop. The application keeps
its existing RAM containment policy; a failstop does not restore unsafe XIP or
feed the watchdog. Cache/SFC waits and a frozen timer may reach a watchdog reset
with failure 0. That value does not establish successful readiness.

`fl_pre_status` and `fl_last_status` retain existing SR1 observations. Bit 0 is
WIP; bit 1 is WEL. `fl_status_valid=0` means no status byte was received. No extra
status commands or reads of SPI data registers are made for tracing.

`fl_entry_tick` and `fl_progress_tick` are raw 24 MHz TIMER4 counts.
`fl_wait_tick` is the last recorded cache/SFC/SPI wait's entry count;
`fl_wait_progress`, `fl_polls` and `fl_mmio` are sampled wait observations.
Long cache/SFC/SPI waits publish every 4096 polls. WIP SPI transactions and progress
publish on the first and every 4096th WIP iteration; received status changes are
also retained. A SPI wait in an otherwise unsampled iteration publishes its
actual transfer type/byte/entry count at its first 4096-poll checkpoint.
Short unsampled transactions therefore need not appear individually.

TIMER4 wraps modulo 2^32. Within a single boot, elapsed microseconds can be
estimated as `(uint32_t)(later-earlier)/24`. Identical sampled values with many
polls are consistent with a frozen timer; they do not prove its physical cause.
`fl_mmio` is the existing cache/SFC/SPI polling observation, or a WIP status byte,
according to the recorded stage. It is not a continuous register history.

`fl_feed_tick` snapshots the last completed main-loop watchdog-feed invocation.
The P33 helper can time out, so it does not prove the hardware accepted the feed.
`fl_wdt` snapshots the raw post-arm watchdog configuration readback; P33 transport
limitations also apply. `fl_current_feed` and `fl_current_wdt` belong to the
current boot and must not be substituted for the incident snapshot.

## Local evidence boundary

`tests/flash_trace_test.c` drives real HAL control flow with delayed/stuck cache
and SFC waits, pre-command BUSY, SPI failure, WIP timeout, frozen TIMER4, returns
and retained probes. `flash_trace_spi_watchdog_test.c` uses the production SPI
budget and escapes through a bounded fake watchdog during an unsampled WIP SPI
wait. `flash_trace_reset_test.c` combines the actual manual SAVE consent and
formatter with RAM HAL flow on the third erase, retains simulated NOR/noinit,
clears live state, and verifies fresh consent restarts at sector zero.

These fixtures establish diagnostic/lifecycle behavior in a host model. The
virtual NOR mutation hook completes its model erase before the HAL stall hook;
it does not emulate a physical erase's partial progress. They do not identify
the hardware stall's root cause or qualify device writes. Target RAM placement,
branch/data references and timing require their separate target evidence.
