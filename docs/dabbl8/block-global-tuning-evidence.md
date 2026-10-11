# Eight-track block tuning optimization evidence

This bounded C optimization computes the global TUNE coarse/fine conversion once per eight-track mix block, after event dispatch and performance setup, and passes its value explicitly to each track. Independent `track_render` and `mix_part` callers still read current tuning on every call. There is no persistent cache. Four-track production code follows the original path.

The control is current-main commit `984f4802ba3e284cd9cd08f4c3606d363877eb49`. The measured production change is commit `0a6ca3018f7a358747f698b7f464dc0594f52d24`; subsequent changes add tests, test integration and this receipt. This control is **not** pristine Felucca `276f72a4e6ea8a12499a7a6819aadf3165126755`.

## Compiled target evidence

Identical generated tables, JieLi clang4.0.1, toolchain archive20250324.1 SHA-256 `f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958`, AC79 SDK `d179b4484759423312073f5fbb232501aa491047`, linux/amd64 container `debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587`.

| Build | App bytes | RAM data+bss | Pool | UAC static cost | Audio IRQ static cost |
|---|---:|---:|---:|---:|---:|
| Frozen4 and candidate4 | 447928 | 92116 /98304 | 331332 /344064 | 504 | 40609 |
| Frozen8 | 478716 | 95812 /98304 | 335240 /344064 | 555 | 55487 |
| Candidate8 | 478700 | 95812 /98304 | 335240 /344064 | 555 | 54302 |

Both four-track app binaries have SHA-256 `3ced057d74785ecd0cc2ccc8f274a1c9e94caa4a6b1203c846014a890d4720fc`.
Frozen8 SHA-256: `32e3eceaa74a4329e17fed0b7812e1d99c37c0e7e2430648f6e4d1886c9a6baa`.
Candidate8 SHA-256: `4a2452a0b1b9a49aa4b0a1e09e93136fbc4fa3c0cf3d877aac406b410ac4aa68`.

App-only build safety checks pass: `.ram_text`916 instructions/no calls, no static memory violations. Eight-track app decreases16 bytes and weighted IRQ cost decreases1185 (2.14%); RAM/pool remain unchanged. The target counter is a deterministic compiled-instruction heuristic, **not hardware cycles**. IRQ instructions2345→2342; divide instructions remain16. Disassembly confirms the global conversion moves out of per-track spans; per-voice bend/fine work remains.

The existing eight-track delivery gates still fail: UAC555 exceeds554.4; IRQ54302 exceeds41252.2. No threshold, golden or coefficient changes. This change alone does not qualify an eight-track release.

## Source ordering and behavior

At the frozen source, global tuning writers are boot initialization, project import and generic main-loop UI/editor globals. The audio event path completes before capture. Modulation writes/restores track parameters, without a global-tuning destination. Nested TIMER5 scans and UAC service return before ordinary USB/UART parsers during audio rendering. No global-tuning writer was identified between track renders. This is a source audit, not on-device concurrency evidence.

`tests/block_global_tuning.py` compiles the same probe against the candidate and checks an immutable receipt measured from the unchanged control:

```
cases=42016 checks=211114 failures=0 silent=0 unadmitted=0 arrangements=6 audible_arrangements=6 audio_state_trace=aeb1daebac64c4a8
```

The42016 single-active-bank cases cover all101 valid tuning values,13 shipping engines (retired engine index1 excluded),4 voice modes and8 banks. Every case requires admitted voices and actual nonzero output. Four rendered blocks change tuning between blocks while a keyboard event enters before capture. Independent renderer calls read-50 then50 and the direct part wrapper reads the current value. Audio and voice/track state are traced. The candidate also checks coarse/fine arithmetic over all65536 int16 inputs; accepted settings remain-50..50.

Six simultaneous eight-track arrangements cover common Analog and mixed Analog/Phase/Lofi/Sample/Trio/Wheel/Drum/Noise engines, each dry, with modulation/sends, and with modulation/sends/performance macros. Initial admission requires8 voices and one perbank. Each renders96 blocks with tuning changes, releases all8 notes at block24 and verifies all gates clear. Held and post-release output must be audible; treated cases require actual chorus/delay/reverb sends and performance activity. This covers these six configurations, not every possible engine/effect combination or complete tail expiry.

Normal and inherited ASan/UBSan frozen/candidate runs match exactly with no diagnostics. Sanitizer exclusions follow existing DSP policy: signed-integer-overflow, shift, bounds, object-size and pointer-overflow; leak checking defaults off. These are not strict whole-engine UBSan passes. All92 existing **four-track** golden renders also pass unchanged. Native8 evidence is a frozen differential trace, not rewritten audio goldens.

## Reproduction and integration

After generating `build/gen` through the normal build setup:

```
python3 tests/block_global_tuning.py
python3 tests/block_global_tuning.py --sanitize
```

Optional `--control /path/to/clean984f` independently reproduces the frozen receipt and checks source identity. `--evidence-dir /private/output/path` preserves compile/run logs and binaries. Neither command rewrites expectations. The normal helper is integrated in `tests/run_tests.sh`; the sanitizer helper runs only inside its existing compiler-support/SANITIZE gate.

Private local logs, disassembly, writer blob pins, source snapshots and SHA-256 manifest are retained in `.local-baseline/language-optimization-research/2026-10-11-tuning-hoist/`. App artifacts are experimental build evidence; no firmware package, installation, flashing, hardware timing or release claim. Further optimization and delivery qualification remain tracked under issues#12 and#56.
