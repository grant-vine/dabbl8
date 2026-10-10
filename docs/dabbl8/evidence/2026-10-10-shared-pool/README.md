# Shared-pool evidence — 2026-10-10

Standalone C implementation of the selected three-project/shared-autosave plan, on base e904f9c46ae283d091fc7b22c3f6805d7a0190f3. Exact source and evidence hashes are in manifest.json; binaries remain local outside Git.

Optimized and strict ASan/UBSan runs each pass 4,546,366 checks: 34 operation cuts, 7,737 program-byte cuts, 8,194 sequential erase-prefix states and 1,000 rotating saves. Full upstream suite passes, including both integrated pool runs, 92 unchanged golden renders, zero audio-health/voice-routing/CPU-budget failures and two timing notes. Default Mac app/package/loader and goldens/budgets remain byte-identical. Target object compiles (1662 text bytes, zero data/BSS); default app RAM is 92116/98304 and pool 331332/344064. Target-memory.json and disassembly record visible frames, not complete callback/IRQ stack upper bounds.

Historical failures are explicit: the first refusal test forgot to reset counters; a generation-boundary fixture selected the obsolete block for reuse instead of retaining it; macOS rejects leak detection; JieLi rejects -fstack-usage and has no llvm-size at the attempted path. Corrected final tests and vendor objdump inspection pass. Two initial logs were superseded before archival; their exact diagnostics/results are retained from tool output in earlier-attempts.txt. Other failed logs and commands are preserved. Checks.json is an attempt history, not an assertion that rejected diagnostic options passed.

ASan and UBSan remain enabled; leak detection is disabled because unavailable on this platform. Manifest records optional DaisySP reference, vendor-fixture restore, editor MENU settings and browser-emulator omissions. Vendor firmware remains outside Git and no restore/install is attempted. No physical NOR driver, legacy erase, migration, runtime adoption, target flash latency or power-loss qualification is claimed. Keep issue #13 open for integration and complete acceptance.

GPL-3.0-only; upstream/dependency notices preserved.
