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
Long cache/SFC waits publish every 4096 polls. WIP SPI transactions and progress
publish on the first and every 4096th WIP iteration; received status changes are
also retained. A SPI wait in an otherwise unsampled iteration publishes its
actual transfer type/byte/entry count on its first unsuccessful poll, then every
4096 polls. Immediately successful unsampled transfers add no publication. A reset
before that first publication, or a blocked MMIO access or barrier, can still
leave the preceding stage: this is not an atomic final-instruction trace.

TIMER4 wraps modulo 2^32. Within a single boot, elapsed microseconds can be
estimated as `(uint32_t)(later-earlier)/24`. Identical sampled values with many
polls are consistent with a frozen timer; they do not prove its physical cause.
`fl_mmio` is the existing cache/SFC/SPI polling observation, or a WIP status byte,
according to the recorded stage. It is not a continuous register history.

`fl_feed_tick` snapshots the last reported watchdog-feed invocation. The main
loop's void P33 helper can time out; this does not prove hardware acceptance.
The guarded project COMMIT path now checks the P33 timeout counter before
publishing its feed receipt and refuses the primitive on a reported timeout.
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

## Retained project COMMIT observations

Timing-diagnostic builds also place the volatile 56-byte `d8pt_diag` in `.noinit`
and expose `pt_*` through the existing read-only `status` command. The boot hook
runs before ordinary BSS/pool clearing. An invalid version magic or exhausted
boot counter clears the observations; otherwise a warm reboot preserves them.
There is no flash journal, power-loss guarantee or write-resumption authority.
The live import session, workspace ownership and observer flag are cleared at
startup. New BEGIN scans and exports cannot append revocation bits to an earlier
import. Only a successfully admitted new import replaces its record.

| Fields | Meaning |
| --- | --- |
| `pt_boot`, `pt_record_boot` | Current boot and boot of the admitted import |
| `pt_reset_reason`, `pt_reset_src` | Current boot reset context |
| `pt_stage` | 0 none, 1 baseline, 2 store, 3 verify, 4 adopt, 5 bind, 6 autosave, 7 done |
| `pt_raw_rc` | -32768 while COMMIT is unfinished; raw observed error on failure; 0 with stage7 on completion |
| `pt_guards`, `pt_revoke` | Existing COMMIT guard and import-revocation observations |
| `pt_address`, `pt_operation` | Last admitted primitive address; 1 erase, 2 program; recorded before the feed, so not proof the command ran |
| `pt_completed` | Primitives that returned successfully and passed the post-return lifetime check |
| `pt_feeds`, `pt_p33_failures` | Feed invocations with no reported P33 timeout; refused invocations with a reported timeout |

The synchronous restore services the watchdog only after exact primitive
admission, with XIP available and IRQs masked for P33, before entering RAM code.
It never feeds a stalled flash/cache/SFC/SPI wait or extends the timeout. Failed
feed transport returns before that primitive; a failed primitive ends COMMIT.
The loader, flash windows, SAVE consent and storage format are unchanged.

`tests/native_commit_watchdog_test.c` exercises the shipping dispatcher and
COMMIT with private NOR effects: completed progress, P33 refusal at multiple
boundaries, NOR failure, reset during the third erase, cleared live authority,
read-only incident preservation and replacement by a new admitted import.
This proves the local control flow, not the physical watchdog failure's cause.
The frozen v023 fixed-address capture remains pinned to v023; do not apply its
old addresses to a newly linked application.

The private v024 package's `read-completion.py` is repinned to the56-byte record
at0x01C7F9C8 and checks its RAM observations against the same status response.
`pt_boot` and main `boots` are independent counters across an image upgrade;
comparing them directly is invalid. Its offline fake-console tests deliberately
use different counter values. The default invocation validates files and prints
a plan without opening a port. Device use still needs fresh owner authorization.
