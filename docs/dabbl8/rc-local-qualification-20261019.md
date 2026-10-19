# Local RC follow-through — current Felucca backports

Firmware source `e251aa4`; private v0.2.3-timing components. This follow-through
does not modify firmware, boundaries, diagnostic counters, engine profiles,
budgets or golden audio. No device API was used. The original v022 source and
physical evidence remain unchanged. Private evidence is under
`build/rc-local-qualification-20261019/`.

## Stack audit

The current ELF audit includes 1,041 functions and zero-sized assembly wrappers.
The inherited fixed-frame method counts downward stack operations and direct/tail
edges, preserving unresolved operations. Conditional engine closure includes all 40
compiled callbacks. Additional source-reviewed families cover native pool/physical
IO, project frontend, session admission/read/write callbacks, three LARGE value
formatters and the exact flash RAM trampolines. Family unions deliberately include
more callbacks than individual sites can invoke. They require valid bound objects,
compiled descriptors and no arbitrary function-pointer corruption.

All named indirect sites reachable from main are accounted for in that model.
Two call cycles have source bounds: ED_D8_TRACK's allowlist cannot contain itself;
SAMPLE/PERC redirects to DRUM/preset0, which cannot repeat that redirect. A separate
context-sensitive traversal caps each participating function at two occurrences.
The original unresolved-cycle report is retained alongside the bounded result.

| Software fixed-frame contribution | Bytes |
| --- | ---: |
| Startup/main |7,512|
| Main-loop root alone |7,504|
| Audio wrapper and conditional callback path |2,196|
| TIMER5 wrapper/path |200|
| Fatal common handler plus vector-stub RETS push |400|

The software IRQ wrappers each push 28 bytes. Audio plus one nested TIMER5
contributes 2,396 bytes before implicit hardware frames. These are source-conditioned
fixed-frame models, not physical maximums. SP/SSP alias/mode, exception frames,
permitted nesting/fault behavior and the conditional target-family assumptions
still need ABI-qualified evidence. The cached sparse SDK lists `os_cpu.h` but its
blob is unavailable locally with lazy fetching disabled; no dependency was fetched.
The existing 24,320/7,936-byte windows and watermark limitations are unchanged.

Read `linked-stack-audit.py/json` and `qualification-results.json`. Do not mark
complete stack qualification from these numbers or surviving pattern writes.

## Timing and recovery evidence reuse

The exact retained v022 D8T2 frame passes parsing/CRC/window checks: 6,300 samples,
inclusive maximum 57,151 ticks, no late/taint/aux publications. Source FS = 44100
implies 69,659.864 nominal half ticks; `DAC_TICKS = 544` instead documents 69,632 ticks
per 128-frame half (2,901.333us at nominal 24 MHz). The serviced-start mean lies
between 69,624.831 and 69,633.904 ticks. These values are consistent with the latter
cadence, but neither software callbacks nor source constants independently prove
actual DMA cadence or timer accuracy. The 12,481-tick inclusive-only difference
omits entry/prologue, recorder, return and arrival latency; it is not total
deadline headroom or qualification of v023. No old capture or report was rewritten.

All four historical full readback hashes, the exact downloaded stock-v15 package,
14 recovery source hashes, pinned upstream archive and 24,064-byte RAM loader match
the earlier independently reviewed recovery receipt. Reuse its six equality checks
and nine guard cases. No physical recovery trial or unchanged test suite was repeated.
Working-console entry remains the demonstrated scope. Unresponsive-application
entry/RP2040 recovery and the external loader's underlying binary provenance remain
unverified. Owning a board does not close them.

## Exact package and batch preparation

The private package reuses the passing 527,328-byte app and 6,950-byte loader;
neither was rebuilt. Application SHA-256
`d79dae1f71ca7f03aece011ebd2246a271736eee20bd0443216f63a2714c4a76`, loader SHA-256
`d6a2726256681ce39ccc61a4089df885ed32b387d6d26393056b4c69c6d9229a`.
SDK commit/three packaging inputs and the existing compiler hash are rechecked.
Decode/CRC/corruption/truncation checks and exact app/loader/padding comparisons
pass. Actual sanitized OTA/loader tests accept the new package, reject wrong-board
sizes before erase and preserve the exact five native pool blocks/latest autosave
on the v022→v023 virtual upgrade.

The first repinned completion reader correctly refused the changed linked layout.
`d8pt_diag` moved 16 bytes to 0x01C1D250. Only that fixed read span/expected symbol
was updated; all four symbols and immutable ELF/package/helper pins remain checked.
The corrected reader passes 17 fake-console cases. Retain `reader-plan.stderr` as
the initial refusal, and `reader-plan-final.json`/`reader-tests.json` as final proof.

The exact future note schedule was rerun through the current actual C renderer
under fatal ASan/UBSan with only existing DSP arithmetic exclusions:24,823 blocks,
128 note pairs, eight voices/eight heavy bodies, six FM6/one DRUM/one WHEEL,
zero invalid/allocation failures, no holds, unchanged proposal and exact host
rollback. ProposalSHA remains
`678dba08d7f40c4a5e8c8223d703511466ef0aeb4e4d80987e406ef2b8f375c3`.
This focused run binds the hardware schedule to changed firmware without repeating
the 84-case matrix. Inherited unused UI-soak warnings are retained in the compile log.

Batch adapter checks pass 10 fault controls and6,144 MIDI grammar cases, with native
ports unopened and uncertain cleanup preserved. Initial offline setup refusals
(missing MIDI module in the build environment, then missing local fixture) remain
in logs; the existing repaired MIDI runtime and newly rendered identical fixture
resolve them without downloads. Default plan and explicit unarmed preflight refuse
hardware access. The kit stays `hardware_ready:false` with 275 input pins.

The [combined future batch](rc-hardware-test-plan.md) groups two SAVE confirmations,
one cold restart, automated upgrade/readback and targeted measurements, then exact
original restoration. No physical action is requested now. The plan explicitly
retains independent clock/observer/ABI/recovery gates; another narrow clean run
cannot replace those proofs.
