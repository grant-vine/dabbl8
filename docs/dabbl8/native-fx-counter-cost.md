# Native effects counter cost

Eight-track resident-tail accounting now keeps delay, chorus, comb and active-model allpass totals in local variables during the bounded render call, updating them only when a stored sample crosses zero. It publishes the totals when the bus/model function returns. Reverb stores reuse the sample already read by the DSP. Audio math, buffer dimensions, resets and model-transition behavior are preserved. No automatic save caller, scheduler, ownership grant, output clearing or hardware-driver change is added.

This is partial work toward [memory/timing issue #12](https://github.com/grant-vine/dabbl8/issues/12), with evidence in [the focused audit](evidence/2026-10-10-native-fx-counter-cost/README.md). Exact comparison parent is `47db25dbed1057154b4917d13784999c747e2921` (the combined signature optimization and queued-output guard).

## Publication contract

Production calls are `fm1_alnk0_irq` → `audio_block` → `mix_block` → `fx_buses` → `rev_room`/`rev_spring`. The main loop cannot run inside this synchronous audio interrupt. TIMER5 can interrupt it, but its actual `in_audio` branch scans input, accounts time, optionally services UAC, then returns. That branch does not call the native flash adapter or resident-tail predicate. Every physical automatic-write mutation still performs a fresh eligibility check with interrupts disabled.

The totals deliberately may be stale during rendering. `fx_resident_quiet()` is now explicitly a main-loop predicate, freshly checked inside the existing IRQ-off write fence; it must never be called by an interrupt nested inside rendering. Keeping `volatile` publication does not make partial totals an interrupt-safe snapshot. Future callers must preserve this contract. Audio ISR self-nesting and alternate production render callers were not introduced.

The new regression invokes the actual audio IRQ and actual nested TIMER5 IRQ from a test hook after every production buffer store. Over 80 real audio halves and repeated ROOM/SPRING switches it makes 112448 nested calls, checks that no NOR I/O occurs, deliberately observes 899 stale in-flight comb totals, and independently scans all four resident buffers after every completed half. It also verifies actual USB packet service and complete queue summaries. These are publication-boundary tests, not preemption at every target instruction or physical deadline measurements.

## Results and remaining costs

| Pinned native target | Exact parent | Changed | Inherited budget |
| --- | ---: | ---: | ---: |
| ROOM static weighted loop cost | 303 | 296 | 218, still fails |
| SPRING static weighted loop cost | 200 | 176 | 126, still fails |
| UAC 48 kHz producer cost | 570 | 570 | 504, still fails |
| App bytes | 457560 | 457388 | −172 bytes |
| General RAM / limit | 95748 / 98304 | 95748 / 98304 | unchanged |
| Pool / limit | 334164 / 344064 | 334164 / 344064 | unchanged |

Default four-track app remains byte-identical to the verified baseline: 447924 bytes, SHA-256 `b69e74e98721ca1f1124edec114c824e315102c65e1f89bf437619d8f048387b`. Changed native app SHA-256 is `1601b0a883b2db6a4c754d969385ebe1b366f46b5d732c66a65a84a75ab257a9`. Both builds pass image, memory, MMIO and RAM-text checks (916 instructions, no RAM-text calls). Native target budgets remain explicit failures; no thresholds or golden hashes were rewritten.

Linked direct frames, including saved registers, change ROOM 64→68 B, SPRING 104→104 B and buses 92→104 B. The synchronous buses→ROOM path adds 16 B, and buses→SPRING adds 12 B. These local frame sums exclude callers, interrupt/context frames and deeper callees, and do not prove real stack margin.

On Apple M1 Pro/macOS 26.7.1, Apple clang 21.0.0 `-O2`, four alternating-order parent/current process pairs each make nine batches of 10000 actual model calls after 200 warm-up calls. Inputs are either zero or a deterministic dense pattern. Median per 32-sample call:

| Case | Parent ns | Changed ns | Parent instructions | Changed instructions |
| --- | ---: | ---: | ---: | ---: |
| ROOM zero | 261.3 | 266.4 | 5022.6 | 3982.6 |
| ROOM dense | 260.9 | 265.9 | 5022.6 | 3982.8 |
| SPRING zero | 586.6 | 455.8 | 6791.1 | 4789.1 |
| SPRING dense | 586.5 | 539.7 | 6791.1 | 4816.5 |

ROOM uses fewer host instructions but is about 2% slower in these elapsed measurements; SPRING improves about 22% for zero input and 8% for dense input. These host results and static target estimates do not establish target cycles, worst-case latency or interrupt deadlines. The optional `tests/native_fx_counter_cost_probe.c` measures actual DSP without a pass threshold; Apple process instruction counts are unavailable on other platforms and report zero there.

Actual DAC plus pre-click USB-tap output matches exact parent byte-for-byte over 1200 blocks with note events on all eight tracks, both reverb models/model transitions, DIST/sends, solo, click, output filtering and fixed USB gain: 614400 bytes, SHA-256 `1fe4e0110763d5c3131d9701503a85d9c68429f8def550d4662320c0021c0c6f`. This is a new exact-parent comparison, not a golden replacement.

Optimized and inherited ASan/UBSan runs pass: publication 227895 checks, resident tails 1256615, activity 594684, logical output queues 249563 and combined signature/guard integration 130295. Existing tail tests retain 600 real DSP/full-buffer oracles, the longest silent echo gap, 42 late states at all 29 mutation fences, odd ROOM halfword/32-bit SPRING union cases and reset/residue behavior. Inherited sanitizer exclusions remain signed overflow, shifts, bounds, object size and pointer overflow; this is not strict whole-engine UBSan. No tests were skipped in this focused set. The full suite and new-head CI were not duplicated in this parallel lane.

Earlier local arithmetic and zero-crossing attempts and their target logs are preserved privately. A later already-read-value refinement produced the same target costs as the crossing attempt; it is not claimed as an additional measured reduction. Physical pipeline completion, target deadline/stack measurements, installed-device recovery and release acceptance remain open. No firmware was installed or released.

SPDX-License-Identifier: GPL-3.0-only. Preserve Felucca and dependency notices.
