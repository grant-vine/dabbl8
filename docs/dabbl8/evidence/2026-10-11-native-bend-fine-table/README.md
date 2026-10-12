<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Exact native fine-bend table

Native eight-track rendering computes the fine pitch-bend fraction for every
track, even while idle. The inherited floor division by 16 leaves a remainder
from 0 through 15, including for negative bends. A 16-byte constant table returns
exactly the inherited truncating quotient `(remainder * 2367) / 2560`. Casting to
unsigned and masking the low four bits obtains the same remainder without a
signed-right-shift assumption. Smoothing, the whole bend, glide, modulation,
voice limits and engine/effect state are unchanged. Four-track code retains the
original expression.

The maintained test compiles the actual production arithmetic block for both
track counts. An independent 64-bit remainder oracle checks all current MIDI
RPN targets (`-6397..6397`, the 24.99-semitone maximum) and eight defined signed
edge cases. Both normal and strict ASan/UBSan runs pass 12,803 inputs per track
count. Malformed source extraction refuses; wrong table/index and extraction
mutants are caught. The inherited floor expression is undefined below
`-2147483632`, outside the MIDI domain; this change does not claim arbitrary
`INT_MIN` safety.

Fresh actual renderer tests pass **44 executions with 22 matching audio/state
traces** across current four/eight tracks, every FM4/SLICE configuration, and
separate experimental FULL/Core/Breaks profiles. Eight actual sequencer
pitch-lock/automation/bend runs match; two pointer-oracle checks and four compiled
renderer mutants validate the comparisons. The first raw GRAIN-heavy hash
comparison differed because sample-zone pointer addresses moved between
executables. That mismatch is retained. The final comparison preserves recognized
zone identity and pointed contents plus all other state bytes; invalid live zone
pointers are refused. No audio hashes were rewritten.

The wider MIDI/motion/mod matrix has **80 passing executions and 32 matched
existing failures**. Original native MIDI tests retain five old four-channel
routing expectations in each configuration (16 failing executions). Original
native motion has eight normal failures and eight ASan historical stack
overflows. A separately bound overlay of the repaired motion fixture from draft
PR #139 passes all 16 native executions. It does not replace the original failed
results. Modulation assertion results match; 15 raw kernel instruction-count
measurement lines differ in nine pairs. Exact known cost-line formats were
checked separately, preserving every value. These noisy host counters are not
target speed measurements. The initial overly strict sealing assertion is also
retained.

Sixteen pinned, app-only vendor builds preserve four-track app/ELF/disassembly
bytes across every FM4/SLICE combination. Native FULL/Core/Breaks apps each add
16 bytes; RAM and pool sizes stay unchanged. The audio IRQ listing loses one
division (17 to 16 listed instructions; 16 to 15 within the checker loops).
Instrumented native rendering replaces 512 fine-bend divisions with 512 table
queries over 64 blocks, both idle and loaded. The static IRQ scores change from
55,767 to 55,063 (FULL), 59,270 to 58,566 (Core), and 58,359 to 57,639 (Breaks).
All still exceed the inherited limit of 41,252.2 (37,502 plus 10%); UAC remains
562 against 554.4 (504 plus 10%). Reduced profiles also retain the checker's
excluded-function absence errors. Budgets/checkers are unchanged.

`qualification.json` binds current parent `406de63335eb55248db21005efd17261b167b03a`
and the separate fixed curated source `84aac1d8235e3a1da395951a46736f759213ec27`
plus three current planning documents. Generated inputs, compiler archive,
container digest, fixed build date, flags and artifacts are pinned there. It does
not qualify profiles as merged to main or reuse a live CI result.

ROM table access and net interrupt duration require device measurement. No
hardware deadlines, complete call-stack bounds, firmware package, loader,
recovery procedure or release is qualified here. No voice-cache or zero-send
experiment is included; goldens, write permissions and flash boundaries remain
unchanged.
