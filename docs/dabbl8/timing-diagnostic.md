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
run. Its seven-row D8T2 frame contains generation/boot identity, configuration,
first/last half counters, invalid/saturation state, an aligned window and two
histograms protected by a CRC. Historical six-row D8T1 decoding is unchanged. The interrupt allocates no memory and prints or writes no storage.
Collection uses 128 bins per metric and 1,140 bytes of state. Start resets state
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
calibrated whole-interrupt deadline measurement. Recorder/wrapper overhead, stack high-water and transport behaviour remain
separate hardware evidence requirements. The aligned logical window below is
software evidence; it does not establish physical deadlines.

## Historical verification before D8T2

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

## D8T2 aligned logical window

The new `window` row has thirteen unsigned hexadecimal words, in order:
first timer tick, last timer tick, elapsed low word, elapsed high word, window
boot counter, late publications, UAC underrun publications, UAC overrun
publications, three ALNK auxiliary pending observations, taint bits, and UAC
enabled (0 or 1). All thirteen words are CRC protected under magic `0x44385432`.
The other rows retain their D8T1 shapes, with `d8t2` labels. Elapsed ticks are
`(high << 32) | low`; seconds use the nominal 24 MHz clock, without calibration.

The window starts at the entry timer snapshot of the first accepted half and ends
at the endpoint timer snapshot of the last accepted half. It includes gaps between
accepted halves and nested TIMER5 work; it excludes time before the first half and
after the last half, including console start/stop delays. Late/UAC values are
modular differences of the software counters at these same endpoints, rather than
lifetime totals. They describe counter publications, which may differ from the
physical instant an underlying condition began. ALNK bits 0x10, 0x20 and 0x40
are counted as observed pending flags, including auxiliary-only IRQs between halves;
their meanings have not been established as DMA errors. Auxiliary-only IRQs after
the final accepted endpoint do not enter the report's committed counts.

Each endpoint takes one bounded volatile counter double-read around its timer read.
An observed TIMER5/UAC change taints the run; there is no retry or IRQ masking.
Metadata work before the entry timer read and after the endpoint read, collector
work and assembly entry/exit remain outside the inclusive duration. The additional
collector changes diagnostic execution cost and cannot prove an improvement.

Taint bits are 1 for an unstable snapshot, 2 for an ambiguous/reset-sized jump,
4 for boot identity change and 8 for accumulated-counter overflow. Overflow stops
collection without wrapping the affected accumulated counter. Start resets the
window before publishing enabled; stopped reports preserve their state. Parser
validation rejects every tainted, invalid or saturated run. Source counters normally
wrap at 32 bits; elapsed adds each modular endpoint delta into two words. Every
observation gap and source-counter delta must be below 2^31; late increments must
be at most one per accepted half. Reset-sized jumps are refused. An unseen whole
wrap, ABA transition or external reset that aliases a legitimate wrap cannot be
excluded by reading values twice. Independent elapsed/reset evidence is therefore
still required, including for a nonempty consistent report. Contiguous serviced-half
numbers are not physical DMA deadline evidence.

## D8T2 verification scope

The private implementation is based on `4f115916fc957c9e6fdd6ac2ff1d788ce5f5ae4e`.
Later main commits require separate integration qualification. The reusable driver
uses actual ISR/DSP off, paused and collecting executions at four/eight tracks,
normal and inherited DSP sanitizers. Window/session/console slices use strict
sanitizers. Literal fixtures cover UINT32_MAX timer/counter wraps, elapsed carry,
reset/boot refusal, saturation, auxiliary endpoint commitment and injected changes
at both entry and endpoint reads. A private missing-transition mutant fails the
actual ISR snapshot-taint assertion. The actual C console frame has 1,024 samples,
62,904,270 elapsed ticks, 1,024 late, 2,048 underrun and 3,072 overrun publications;
these are synthetic fixture values, not device measurements. Parser fixtures
independently validate those literals, CRC/corruption/truncation, empty runs and
accumulated wraps. Historical actual-C D8T1 frames retain their original decoder
outputs and distribution checks.

Fresh pinned app-only comparisons use retained identical generated headers, the
recorded JieLi archive/SDK and Linux amd64 image. No loader, sample generation,
package construction, installation or device capture is part of these checks.
Diagnostic-off application, ELF and disassembly must match the frozen parent at
four/eight tracks. Static budgets and golden audio hashes are unchanged. Linked
size/RAM evidence is recorded separately from execution cost, stack high-water,
physical deadlines and USB behaviour, all of which remain hardware gates.

For the frozen 4f115 baseline, FM4 disabled and SLICE enabled:

| Tracks | Flag | Application bytes | RAM bytes / 98,304 | Pool bytes / 344,064 | Static budget |
| --- | --- | --- | --- | --- | --- |
| 4 | 0 parent and candidate | 449,168 | 92,116 | 331,332 | Pass |
| 4 | 1 candidate | 451,148 | 93,252 | 331,332 | Pass |
| 8 | 0 parent and candidate | 481,840 | 96,068 | 335,240 | Fail |
| 8 | 1 candidate | 483,844 | 97,204 | 335,240 | Fail |

Diagnostic-off application/ELF/disassembly are byte identical to the parent at both
track counts. Linked RAM increases by 1,136 bytes and eight-track diagnostic RAM
leaves 1,100 bytes before dynamic stack use. The state object itself is 1,140 bytes;
the linked RAM difference is reported independently of the object size. Eight-track budgets fail without any threshold change. Those heuristic
scores do not measure real elapsed time; four-track static pass is likewise not
hardware acceptance. Complete logs, pins, hashes, source snapshots and the initial
source-freeze interpreter failure remain in the private qualification receipt,
with review required before publishing this optional diagnostic.
