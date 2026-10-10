# Native effects counter cost evidence

See [scope, publication contract and honest tradeoffs](../../native-fx-counter-cost.md). Functional source/tests commit `9fd6fadab16c1e991d9e11dc5f68454a5a9556ed` compares exact parent `47db25dbed1057154b4917d13784999c747e2921`.

Optimized and inherited sanitizer checks pass; default pinned target remains byte-identical, actual native audio bytes match exact parent, and fresh target builds retain fixed RAM/pool. Target reverb and UAC budgets still fail in both versions. ROOM host elapsed time is slightly worse despite fewer instructions; SPRING improves. Direct helper stack costs increase. No budget, golden, buffer dimension, flash boundary or DSP math was changed.

The boundary test exercises actual audio/TIMER5 IRQ functions and intentionally proves counters can be stale during rendering, with complete post-block oracles. The predicate is a main-loop reader, rechecked fresh with interrupts disabled before mutation. It does not promise coherent nested-ISR reads or real device deadlines.

Raw probes, earlier attempted patches/logs, parent archive, waveform bytes and ELF/disassembly remain private outside Git under `.local-baseline/fx-counter-cost-check`; current target outputs are in isolated `.local-baseline/fx-counter-cost/build/fx-counter-{4,8}`. SHA-256 manifests identify public source/evidence and retained private artifacts. The public optional cost probe uses the same actual model loops as the retained measurement probe. No new-head CI, full-suite duplication, flashing, scheduler, production autosave ownership or hardware pipeline qualification was performed.

Reproduction uses the repository's generated headers and pinned build dependencies documented in the manifest. Focused host commands, from the repository root:

```sh
cc -O2 -w -Ibuild/gen -Ifirmware/src -o build/native_fx_counter_boundary_test firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_fx_counter_boundary_test.c -lm
build/native_fx_counter_boundary_test
cc -O2 -w -Ibuild/gen -Ifirmware/src -o build/native_fx_counter_cost_probe tests/native_fx_counter_cost_probe.c -lm
build/native_fx_counter_cost_probe
```

Inherited sanitizer flags: `-O1 -g -fsanitize=address,undefined -fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow -fno-sanitize-recover=undefined -w`, `ASAN_OPTIONS=detect_leaks=0`. Runner entries cover the publication test normally and under these inherited flags. Static budget reports use `tests/target_budget.py` with the unchanged `tests/target_budget.txt`. No thresholds are attached to the optional cost probe. Host batch variability, IRQ/context stacks, physical pipeline completion and release gates remain open.

SPDX-License-Identifier: GPL-3.0-only.
