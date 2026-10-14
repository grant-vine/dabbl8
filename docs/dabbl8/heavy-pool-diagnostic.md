<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Observed heavy-pool ownership diagnostic

This helper adds a bounded software measurement for issue #12. It is disabled
by default with `D8_HEAVY_POOL_DIAGNOSTIC=0` (only literal 0 or 1 is accepted); the installed v0.1.5 diagnostic is
unchanged. Console commands, a host collection protocol and target build wiring
are not implemented in this slice. No physical resource measurement is claimed.

The eight-track shared pool always reserves all eight bodies. The helper measures
**observed live-owned slots**, not dynamic RAM allocation, general `.pool` usage,
FX/canvas buffers or stack high-water. Static capacity, body size and reservation
bytes are separate report fields. Four-track Full uses inherited per-track state
and has no shared-pool measurement; curated Core/Breaks already require eight
tracks. Engine limits, allocator admission and state contents remain unchanged.

## Recorded scope

The [existing accessor](../../firmware/src/heavy_state.h) calls the
[diagnostic helper](../../firmware/src/heavy_pool_diagnostic.h) only after a
canonical live request reaches an existing owner, completes a new allocation,
or fails because no reclaimable body exists. Invalid, inactive and noncanonical
requests are not counted as allocation failures. Stale owner keys are excluded
with the actual `heavy_live` predicate. Duplicate live owner keys invalidate the
census instead of counting one logical voice twice; duplicate inactive keys are
excluded as stale. This is scoped live-owner validation, not proof of complete
allocator integrity. GRAIN/DRUM count one complete body per
active track; WHEEL/PHYS/FM6 and reverse SLICE count their live voice bodies.

A run records an initial census and one census per observed canonical request,
requests/allocations/reuses/failures, last observed live-owned slots and its
observed peak, plus last/peak vectors indexed by the stable fourteen engine IDs.
The last vector sums to the corresponding last total. Each engine peak is an
independent maximum across observations; those peaks must not be summed into a
simultaneous total. The last values are not instantaneous occupancy after
release, engine changes or collection stop. Unobserved transitions and unowned
active voices are outside this metric.

Schema 1 binds the caller-supplied boot counter and a generation. Every new run
increments generation; exhaustion refuses. The future production caller must
supply `felucca_dbg.boots`, not a host-provided value. Source/profile and external
boot observations still need a separately retained capture receipt.

## Helper lifecycle and failure semantics

`d8hp_start(boots)` is a serialized main-loop operation. It returns 0 without
resetting an active/busy/exhausted run, 1 when recording starts, or 2 when an
invalid new run remains paused. The caller must exclude all track/owner mutation
through the initial census and enable publication. The helper does not mask
interrupts. A separate reentry flag detects a nested canonical accessor even
while start resets the disabled report; other mutation sources still require
the caller's exclusion. Recording is published only after the initial census.

`d8hp_stop()` pauses the run without refreshing its last observation. A snapshot
is available only after recording stops and the recorder is not busy.
`d8hp_snapshot(out)` returns 0 with output unchanged when unavailable/running,
1 for a snapshot within the declared scope, or 2 for an explicitly invalid
snapshot. Invalid flags retain reentry, counter saturation, malformed/duplicate
live owner keys or an invalid internal outcome. Such a snapshot cannot qualify resource limits. Counters never wrap into
an apparent small valid value. A subsequent explicit start can begin a new
generation; it never silently resets a running collection.

There is no heap allocation, printing, flash operation or ownership mutation in
the recorder. Enabled observation scans at most eight owner keys and updates
fourteen engine counters; duplicate-key detection adds at most 28 comparisons. Its enabled interrupt and startup cost still needs
target measurement. Default-off host preprocessing/object equivalence is a
separate proof from target compilation and physical cost.

## Pending production capture integration

A later reviewed integration must add distinct start/stop/report console
commands under this separate diagnostic flag, bind the actual boot counter,
provide the bounded startup mutation exclusion, and export only paused reports.
Printing belongs to the main loop. A new report schema and host parser must
refuse invalid/running/unknown configurations and retain source/profile,
generation, before/after boot/reset identity and workload attribution. D8T2
must remain a timing report; it must not be relabelled as a resource measurement.

Target RAM/stack and startup/observation overhead must be checked before enabling
that integration. This helper does not complete stack measurement, issue #12,
physical deadlines, qualified engine caps or a release gate.

## Focused verification

[The fixture](../../tests/heavy_pool_diagnostic_test.c) includes the actual host
core. Eight-track Full/Core/Breaks are tested in optimized and strict ASan/UBSan
modes, without DSP sanitizer exclusions because the measurement scenarios do
not render DSP. It covers initial/live/stale owner censuses, exact eight-slot
capacity, canonical rejection, a forced ninth-body refusal, reuse, coherent
last vectors, independent per-engine peaks, fixed reservation bytes, paused
output preservation, active-start refusal, generation, saturation, malformed
owners, duplicate live/stale keys, invalid internal outcomes and reentry during
startup before report reset, startup census and recording.

Baseline capability absence, disabled four/eight-track Full/Core/Breaks
preprocessed/object equivalence and the existing real mixed-engine lifecycle
fixture are recorded separately. Four-track enabled measurement and invalid
flag values are explicit refusal cases. No complete-suite rerun, dependency
download, vendor firmware build, loader/flash-map change or device operation is
performed for this helper.
