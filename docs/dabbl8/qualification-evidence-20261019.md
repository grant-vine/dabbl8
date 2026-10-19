# Timing, stack and recovery qualification — 19 October 2026

Private candidate: v0.2.1-timing / FM-1_8021. The frozen package is unchanged.
This work performed no device discovery/open, notes, reset, loader execution,
storage writes or flashing. It completes bounded offline checks, not physical
qualification of the new candidate. Local evidence: `build/rc-qualification-20261019/`.

## Linked stack audit

The exact frozen ELF/symbols/disassembly were inspected:1,033 functions, including
zero-sized assembly interrupt wrappers that a C-function-only scan would miss.
Fixed downward stack adjustments/register pushes and named direct/tail edges
are included. Branch targets without annotations use actual encoded instruction
length. Unparsed pushes, dynamic stack changes, cycles and indirect/interior/ROM
transfers are explicitly reported, never silently treated as zero-cost bounds.

A source-reviewed conservative engine closure adds all40 compiled callback
functions to each engine-callback caller (`chord_kit`, `kb_engine`, `seq_step`,
`voice_start`, `fm1_alnk0_irq`). This covers any compiled engine, including more
callbacks than any one voice uses. Assumptions: valid engine indices and immutable
engine descriptors, with these indirect sites restricted to those descriptors.
Under that assumption the named fixed-frame paths are2,196 bytes for audio and
200 bytes for TIMER5, with software wrapper pushes included. Both conditional
interrupt graphs now have no unresolved transfers or call cycles. These are scoped
fixed-frame results, not complete physical stack bounds.

The linker allocates24,320 user-stack bytes and7,936 supervisor-stack bytes.
Hardware exception frames, SP/SSP mode, permitted nesting/fault behavior and
unresolved main-loop callbacks still need proof. The known main-loop named path
is7,804 bytes but has27 gap functions; it is not a whole-program upper bound.
Two source cycles are control-bounded: editor envelopes cannot contain another
ED_D8_TRACK envelope; retired SAMPLE/PERC redirects to DRUM/preset0, which cannot
repeat the same branch. Their explanation does not close all main-loop gaps.
Watermark observations remain lower bounds on surviving writes.

Read `linked-stack-audit.json`, `linked-stack-audit.py` and
`stack-cycle-source-review.json`. No stack allocations or candidate code changed.

## Retained timing audit

Four existing D8T2 frames were independently reparsed and checked, without
repeating playback or MIDI workloads:39,529 samples from the prior v018/v019
captures. Maximum inclusive measurements were48,014–54,619 ticks. Remaining
fraction against the nominal128-frame/44.1kHz deadline was21.6–31.1 percent.
These figures omit interrupt arrival/entry, recorder and exit costs; they are
not total deadline headroom and do not qualify v021.

The60-second host journal encloses the target window. Its one-sided timer-rate
lower estimate is23,995,779.8 ticks/second, conditional on journal ordering,
valid target elapsed deltas and host monotonic-clock scale. It supplies no
upper rate bound and is not full timer calibration. Mean serviced-start cadence
is distinct from actual DMA completion cadence: contiguous software counts
cannot rule out missed DMA events. No counter/hash/budget was changed.

Read `timing-evidence-audit.json` and its script. Remaining timing proof concerns
exact candidate identity, omitted observer/IRQ work, actual clock/DMA behavior
and the supported concurrent effects/UI/routing envelope.

## Independent verification of historical stock recovery

The existing13 October physical stock-v15 recovery evidence was reconstructed
from preserved private inputs. Both1MiB backups agree. A new plan derived from
the exact separately obtained stock package equals the saved expected image;
the complete restored readback equals that plan. All143 changed sectors lie in
`[0x4000,0x93000)`. Bootloader and every non-application byte are preserved.
The owner's original-interface, working-sound and working-controls confirmations
are reused, alongside the prior independent documentation/evidence review.
Do not repeat the physical stock restore just to reconfirm these same facts.

The former temporary RAM-loader dependency was missing. The exact recorded
upstream commit was recovered privately; its24,064-byte loader matches the
historical SHA-256. The helper/source was retained locally, and a fresh isolated
runtime uses already cached Python dependencies. Nine RAM-readback/write-gate
cases pass with USB discovery and claiming explicitly prohibited in the tests.
The reference OS opener stays disabled. No RAM loader was executed. All external
source notices remain with the private cache; no vendor or loader binary is
added to Git or the frozen candidate.

This closes the **independent evidence check of the already demonstrated stock
restore**, not a new independent physical reproduction or universal recovery
qualification. The demonstrated entry depended on a working firmware console.
Loader-unavailable/unresponsive-device entry, RP2040 wiring/recovery and process-
death/power-loss cleanup remain unqualified. The external RAM loader's underlying
binary provenance remains unestablished despite its exact historical hash.

Read `stock-recovery-evidence.json`, `recovery-input-repair.json` and
`recovery-guard-tests.log`. Private backups remain outside public source.

## Remaining boundary

Do not mark full timing/stack qualification or public RC approval from these
checks. The next device work is the prepared owner-authorized upgrade/persistence
batch plus only genuinely uncovered measurements. No hardware operation is queued.
A working-console-only recovery claim must stay explicitly narrower than recovery
from an unresponsive application; any wider claim needs the missing physical test.

## Entry and nesting follow-through

The source explicitly configures TIMER5 at priority4 above audio priority3 and
allows the timer to nest into rendering. The two reviewed fixed-frame components
sum to2,396 bytes under one such nesting, before implicit hardware frames or any
main/fault context; this is not a complete stack maximum. Startup writes SP and
SSP but does not independently settle their hardware mode/alias semantics. Fatal
entry also pushes a4-byte RETS word in its vector stub before the common handler;
the common-handler root alone is not the complete fatal path.

Source tracing identifies flash-RAM trampolines, engine descriptor callbacks and
bound native project operations among the main-loop gaps. Those gaps remain open
until every applicable target/context is bounded. See the separately pinned
`build/rc-owner-ready-20261019/stack-entry-followthrough.json`. No earlier reviewed
report, candidate binary, or checksum manifest was changed. Further device tests
alone cannot replace the missing callback/ABI proof.
