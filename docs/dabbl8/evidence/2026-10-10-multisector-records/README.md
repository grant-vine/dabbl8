# Multi-sector record preparation — 2026-10-10

Result: simulated address-independent engine passes. Device integration and issue #13 acceptance remain unfinished.

Each final optimized/strict ASan/UBSan run passes **884,086 assertions**, including **34 erase/program operation-cut positions** and **7,737 payload/header byte-cut positions**. The assertion count includes repeated callback bounds checks; it is not a count of independent power-cut scenarios. Both copies are occupied before replacement. Tests also cover stopped-state changes, every selection-read error, complete commit followed by verification-read failure, write protection, partial erases, corruption, length/identity/version checks, sequence wrap/ties and invalid/unknown/missing-context input.

The complete local upstream/extended suite passed with stable source hashes at the preceding header-clear implementation. A standalone target compile then found that the pinned JieLi compiler lacks `string.h`. Final code removes that include and replaces its sole 32-byte `memset` with a bounded zero loop. Exact source comparison, final focused optimized and strict sanitizer runs, and pinned pi32v2 object compilation pass. This distinction is recorded in `results.json`; complete final-source Linux qualification is pending the draft PR. No golden hashes or assertions were weakened. Initial missing-SDK wrapper and missing-header compiler failures are retained.

The existing runtime/codec/flash/loader sources and golden/CPU/target references are byte-identical to base `400b939df0b96e111f6b0027d311faceebaa57da`. The full suite also passes both existing 8,911-check runtime and 16,806-source asynchronous arena tests. The new engine is not included by `felucca.c`, has no target call site and does not alter a physical map. Standalone target-object compilation is not firmware linking, stack/high-water evidence or a new memory measurement. PR #51 remains the applicable separately measured eight-track image.

Run from the checkout root with the pinned baseline environment:

```sh
sh tests/run_tests.sh
```

The runner compiles the standalone storage test optimized and with full ASan/UBSan, without legacy DSP exclusions. Raw full logs, exact source snapshots and compiler objects remain outside Git under `.local-baseline/d8store-check/`; their hashes are recorded here. The four established local omissions and hardware SKIP remain explicit. Browser conversion sources are unchanged and their actual pinned parity was audited on base PR #51.

See [engine contract](../../multisector-record-engine.md). Approved map, reference binding, actual runtime/editor storage commands, device compilation/link/memory/stack measurements, physical transport exclusion/flash behavior/power cuts and restoration are still required. No flashing, firmware installation, loader/boundary change, merging, releases or deployment.
