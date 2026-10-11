# Optional audio timing diagnostic

This diagnostic is disabled by default. It adds bounded, paired timing histograms
for a later controlled hardware investigation; it does not prove that eight-track
firmware meets its audio deadlines. No device was flashed or measured for this work.

`tools/build.py --dabbl8 0.1.0 --timing-diagnostic` selects an experimental diagnostic
build. It refuses `--release` and a disabled CDC console. Every invocation owns a
fresh `build/timing-diagnostics/run-*` folder, including generated headers, temporary
sample WAVs/cache, application and package outputs. It marks the version with
`-timing`, uses a distinct diagnostic package name and writes a manifest explicitly
stating that hardware qualification and flashing authorisation are absent. These
labels do not prevent someone choosing a diagnostic package in an installer.
Ordinary outputs and previous diagnostic runs are preserved. This is a build
interface, not an instruction to install firmware. Loader source, flash boundaries
and ordinary package construction are unchanged.

When a separately authorised diagnostic firmware is running, the main-loop console
accepts `timing start`, `timing stop` and `timing report`. A report requires a stopped
run. Its six-row D8T1 frame contains generation/boot identity, configuration,
first/last half counters, invalid/saturation state and two histograms protected
by a CRC. The interrupt allocates no memory and prints or writes no storage.
Collection uses 128 bins per metric and 1,064 bytes of state. Start resets state
before enabling the single audio-interrupt writer; stop/report rely on the existing
single-core, non-reentrant main-loop and non-self-nesting audio-interrupt model.
Target publication, timer calibration and physical interleavings still need checking.

`tools/dabbl8_timing_report.py` reads a saved console capture without connecting to
or changing the device. It rejects incomplete, inconsistent, saturated, invalid or
extra frames. Percentiles are nearest-rank bin bounds, not interpolated exact values.
The nominal 24 MHz clock gives approximately 42.67 microseconds per bin. Render
measurements retain the existing microsecond quantisation and exclude nested TIMER5
work. The inclusive C interval includes nesting but excludes the histogram recorder,
assembly wrapper/prologue and portions of entry/exit. Neither metric represents a
calibrated whole-interrupt deadline measurement. Per-run late/UAC-overrun and elapsed
snapshots, recorder/wrapper overhead, stack high-water and transport behaviour remain
separate hardware evidence requirements.

## Verification on current main

The port is based on `fbadbca894419c9626306eb773f38ae22baed52d` and retains its
protected-voice overload fallback. Twelve actual ISR/DSP executions with simulated
clocks compare diagnostics off, paused and collecting at four/eight tracks, normally
and with upstream DSP sanitizer exclusions. Audio and voice-state traces agree.
Session boundary/wrap/saturation tests and the console slice use strict sanitizers;
parser tests check complete framing/CRC and distributions; six orchestration fixtures
check isolated paths, refusal and preservation. `tests/audio_timing_test.py` preserves
fresh per-invocation logs, source hashes and an explicit result; it runs in the host
suite. Simulated CDC rings and unrelated console stubs do not verify USB hardware.

Actual pinned JieLi app-only compilation/linking compares current main and the
candidate with diagnostics off: both are 480,284 bytes with SHA-256
`36b2b0000e6d5457219b29242a1601c734059aab39acc263779ffa51b270b24d`;
application bytes and ELF are identical. RAM is 96,068/98,304 bytes and pool usage
335,240/344,064 bytes. Diagnostic application size is 481,564 bytes, SHA-256
`0f39d0c2371fd29f89786769e074692394bbc3762442620b5181c69389d8c10b`;
RAM is 97,124 bytes, leaving only 1,180 bytes, with the same pool usage. These
compilations reuse identical retained generated headers; they do not exercise
package construction, sample regeneration, installation or dynamic stack use.

Both unchanged static budget checks fail. Control IRQ score is 55,767 versus
37,502; diagnostic score is 105,839. UAC score remains 555 versus 504. The IRQ
heuristic weights backwards address spans, including compiler return joins; those
scores are not measured execution-time ratios. Budgets and golden hashes remain
unchanged. A diagnostic compiler/link success is not release or hardware acceptance.

Raw host logs, target environment/results, retained budget failures and hash manifest
are in [the evidence folder](evidence/2026-10-11-timing-diagnostic/). Complete target
objects, ELF, disassembly and source snapshots remain in the ignored local baseline
folder. Earlier actual sample-generation checks on the identical generator source
cover SLICE enabled/disabled, byte-identical WAV/header output and preservation of an
altered ordinary WAV, with the old generator as a negative control.
