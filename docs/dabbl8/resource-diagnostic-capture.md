<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Paused resource diagnostic capture

D8R1 is a separate resource protocol for an explicit private eight-track timing
kit with both `D8_HEAVY_POOL_DIAGNOSTIC=1` and
`D8_STACK_WATERMARK_DIAGNOSTIC=1`. Ordinary builds expose no resource commands
or status capabilities. The [builder option](resource-diagnostic-build.md)
colors application stack windows at cold startup; it does not repaint an active
stack or alter loader startup. The installed v0.1.5 timing kit does not gain this
capability from a host-tool update. A host pass is not hardware qualification.

`resource start` begins a new pool observation generation bound to the actual
`felucca_dbg.boots`. An active run or exhausted generation refuses start without
resetting it. `resource stop` pauses it; an already paused generation reports
`resource paused`. Invalid initial census returns `resource invalid`; overflow,
reentry or malformed live ownership automatically pauses an invalid run. Exact
whole-word commands and empty trailing arguments are required. A report requires
a paused generation from the current boot. Only the main-loop console calls the
controls. Start, stop and pool-copy use the existing short main-only input lock;
formatting and the bounded read-only stack scan run after unlocking. The scan
calls the stack wrapper that requires the colored-startup linker symbol.
Nested TIMER5 input/USB-consumer work in the audio path does not mutate voices
or pool owners; this does not make the allocator generally thread-safe.

The [pool metric](heavy-pool-diagnostic.md) records the initial census and
canonical eligible allocator requests. It separates fixed slot capacity, body
size and static reservation from last-observed and peak live-owned occupancy.
An idle run with zero eligible requests is valid and records only its initial
census. Last-observed occupancy can become stale after voices release; it is not
instantaneous occupancy at report time. Per-engine peaks are independent and
must not be summed as a simultaneous total peak. These are ownership/concurrency
observations, not dynamic RAM allocation or the general pool/FX/LCD memory peak.

The [stack metric](stack-watermark-diagnostic.md) reports surviving written depth
since application startup, including earlier work and potentially the report's
own frame. It is a nonatomic lower bound: pattern collisions and reserved but
unwritten frames can conceal deeper use. It is not maximum SP, resettable per
capture, overflow proof or a full RAM high-water measurement. User and supervisor
windows remain the current hardware descriptors (24,320 and 7,936 bytes).

## Frame contract

Rows occur exactly once in this order, use eight uppercase hexadecimal digits
per unsigned 32-bit word, and terminate with CRLF:

| Row | Words |
| --- | --- |
| `d8r1 begin` | generation, current boot |
| `d8r1 config` | tracks, profile ID, effective engine mask, helper bits, UAC enabled |
| `d8r1 pool` | schema, invalid flags, requests, allocations, reuses, failures, censuses, last live-owned slots, peak live-owned slots, capacity slots, body bytes, reserved bytes |
| `d8r1 last` | last observed counts for engine IDs 0–13 |
| `d8r1 peak` | independent peaks for engine IDs 0–13 |
| `d8r1 user` | status, low, top, capacity bytes, untouched bytes, observed written bytes |
| `d8r1 supervisor` | same six stack fields |
| `d8r1 end` | generation, current boot, CRC32 |

Profiles are 0 Full, 1 Core and 2 Breaks. Helper bits must be 3, pool schema 1,
stack status 1. CRC32 covers little-endian words: magic `0x44385231`, all row
words before `end`, with IDs `0x4C`, `0x50`, `0x55`, `0x53` before the two vectors
and two stack rows. Closing generation/boot are repeated, not CRC inputs.
The console omits closure after a detected output stall or changed paused
state/boot. Invalid paused reports retain flags and CRC; the collector saves
and rejects them. CRC is a framing check, not cryptographic attestation.

## Host workflow

Use the existing isolated Python; no additional dependencies are needed:

```sh
python tools/dabbl8_resource_capture.py --manifest PATH/manifest.json \
  --version v0.1.0-timing --workload 'described workload' --seconds 10
```

This defaults to a plan and never opens a port. The manifest uses the timing
collector's verified `source_commit` and `candidates.diagnostic` package,
SHA256 and byte count, plus the builder's resource metadata and actual
`compiler_flags`/`startup_flags` at the same top level. Require typed true
`resource_diagnostic`, `resource_report_schema: D8R1`, false
`resource_collection_initially_enabled`, numeric `resource_engine_profile` and
`resource_engine_mask`, boolean `resource_uac_enabled`, and matching
`engine_profile`. Both helper flags, private identity, timing and eight-track
flags must be explicit; colored application startup must be explicit. Supported
optional CDC/UAC/FM4/SLICE/profile flags must match their documented defaults or
literal values. Conflicting, bare, split and unsupported macro forms for these
configuration fields refuse. The effective mask and requested version must match.
These observations do not attest the installed binary.

An explicitly authorized capture adds `--capture --port /dev/cu.usbmodemEXACT
--output NEW_DIRECTORY`. Opening can assert DTR on macOS; the collector requires
a fresh matching banner and fresh resource capabilities before starting. Its
fixed allowlist contains only `status` and `resource start|stop|report`. It never
sends reset, storage, loader or memory-write commands, and never retries commands.
Stable boot, USB reset and UAC configuration observations bracket the paused
CRC-checked report. The report records requested host duration, outer host elapsed
time, workload label and candidate identity. Duration is not a hardware clock or
workload calibration. Use meaningful workloads separately; idle proves no active
engine concurrency requirement.

The new output retains `journal.jsonl`, `report.response` (raw reply fragments),
`report.frame` (including incomplete/invalid rows), `receipt.json`, and
`report.json` only after protocol/status validation. A driver close/restore error
makes the receipt unsuccessful even if report validation passed. Empty report
files mean failure before report bytes arrived; they are not measurements.
A lost or malformed start ACK leaves ownership unknown and sends no speculative
stop. A known owned run interrupted before its stop receives one cleanup stop;
a lost/malformed stop ACK remains uncertain and is never replayed. These receipts
require explicit inspection by the owner; the collector cannot safely infer the
device state after transport loss.

## Local evidence

Inspect the affected plan before running `--check resource-capture` and
`--check timing-capture` with `ci/local_checks.py`. The resource recipe compiles
the actual console/CDC dispatcher and real pool helper for all three profiles
in optimized and strict ASan/UBSan modes. Its fake stack windows and HAL test
assert that scanning/output happen unlocked. The direct stdlib runner
`tests/dabbl8_resource_capture_test.py` reuses the full-profile binary via
`D8_RESOURCE_CONSOLE_BIN`; no pytest, live port, package build or hardware operation
is needed. `D8_RESOURCE_CAPTURE_EVIDENCE_OUT` retains each simulated scenario's
journal, raw frames and receipts. Tests cover CRC/order/truncation, numeric and
vector consistency, empty eligible workload, actual C-frame parsing, profile and
capability refusal, generation/boot binding, cleanup uncertainty and dry-plan
behavior. Default-off and malformed configuration compile checks separately
preserve ordinary console behavior. Target cost, linker fit and physical resource
workload qualification remain separate checks.
