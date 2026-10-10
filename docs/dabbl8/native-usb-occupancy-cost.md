# USB occupancy bookkeeping: rejected native timing candidates

Issue #12 / #56 research, 2026-10-10. Exact tested production parent:
`47db25dbed1057154b4917d13784999c747e2921` (parent
`10c502129dca97a5dd25a2422c42e25b31f8a884`). This is the actual queue-plus-signature integration, not the earlier signature-only source.

**No firmware optimization is accepted here.** Production `firmware/src/usb.c`
is restored byte-for-byte to that parent, SHA-256
`0f18110fc0bb89bda6d0256db015cfc1c388f1bc129e0e21bc072b44f0924033`.
The USB48k timing gate remains open. Nothing was pushed, flashed, released or
installed. No loader, driver, flash boundary, budget or golden hash was changed.

## Pinned native measurements

JieLi clang 4.0.1, toolchain archive SHA-256
`f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958`,
Docker linux/amd64
`debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587`.
The existing baseline SDK pin is `d179b4484759423312073f5fbb232501aa491047`;
these measurements build the application, not a new SDK package/loader.

The unchanged target allowance is 504 with +10% tolerance: integer cost must
be at most 554. The actual parent scores **570** (+13%). All attempts retain
ring contents, FIR math, existing producer publication barriers and nested
TIMER5 service; none creates a new callee to hide work from the estimator.

| Attempt | `uac_tap48` weighted estimate | Result |
| --- | ---: | --- |
| Exact production parent | 570 | Above allowance |
| OR predicates instead of short-circuit pair predicates | 574 | Worse |
| Local ring value, still publish accounting on each store | 572 | Worse |
| Local ring and history block accumulators | 583 | Worse; local stack spills increase |
| Fixed 32-frame history recount at existing publication | 571 | Mixed host benefit; native gate still fails |
| Producer-owned ring/history qualifiers | 566 | Lower estimate, still fails; qualifier change rejected |
| Producer-owned qualifiers plus OR predicates | 578 | Worse |
| Update counts only on zero/nonzero transitions | 574 | Worse |
| Ring-only block accumulator | 580 | Worse |

The most thoroughly checked candidate removes the per-input history increment,
then counts the 32 unique stereo history frames before the **unchanged**
`RING_PUBLISH()` and `ua_w` publication. It scans 128 bytes inside the audio
producer, never the full 512-frame ring with interrupts disabled. Ring-store
accounting, pending packet/last-frame state, sticky uncertainty, reprime and
consumer behavior remain intact. Its exact rejected patch and raw reports are
in [the evidence directory](evidence/2026-10-10-usb-occupancy-cost/).

| Native linked measure | Parent | Fixed history scan |
| --- | ---: | ---: |
| Tap instruction count / weighted estimate | 158 / 570 | 162 / 571 |
| ALNK0 root estimate | 40238 | 40238 |
| TIMER5 root estimate | 35375 | 35375 |
| Unique direct reachable ALNK0 instructions / summed loop weights | 9687 / 47931 | 9691 / 47932 |
| Unique direct reachable TIMER5 instructions / summed loop weights | 2252 / 35520 | 2252 / 35520 |
| Application bytes | 457560 | 457572 |
| General RAM / available | 95748 / 98304 | 95748 / 98304 |
| Pool / available | 334164 / 344064 | 334164 / 344064 |
| RAM text instructions / calls | 916 / 0 | 916 / 0 |

The candidate retains 52 bytes of saved registers, while local storage grows
from the parent's 24 bytes to 28 bytes (76 → 80 bytes combined), excluding
callees/nesting.
This is a disassembly observation, not measured physical stack high water.
The two-accumulator attempt needs 36 local bytes instead of 24.

Parent app SHA-256:
`d4acf29031cfad8ee51318d8bc7ffe7e0cd4fa019a7a9e24a679a5d30f0b2964`.
Fixed-scan app SHA-256:
`9caf426de845923140e930c849907a123ea3dafd4dc947a138cda778f90f278a`.
Both pass image, memory and no-calls-in-RAM checks. The initial private parent
measurement helper had a wrong `check()` argument count after completing the
build; that failure is retained. Checks were subsequently run correctly on
that same ELF/image without rebuilding or changing the parent.

Unique direct-call closure totals count each linked function once; they are
structural comparisons, not execution counts or deadline measurements.
Indirect calls and dynamic repetition are not reconstructed. The native
budget reports include the pre-existing reverb overruns; the fixed scan does
not fix or recalibrate them. ALNK0 and TIMER5 root reports are unchanged, while
the full ALNK0 closure increases by one weighted unit. This is explicitly
**no native gate benefit**.

## Actual Mac work, output and refusal checks

Mac arm64, macOS 26.7.1 (25G241), Apple clang 21.0.0
(clang-2100.1.1.101), isolated Python 3.14.8. The preserved host probe uses
actual `uac_tap48`, `uac_render_start`, packet consumer service and the existing
simulated controller fixture. Nine rounds of 20,000 32-frame blocks per case
use silence, constant stereo, mixed zero/nonzero and pseudorandom patterns.
Process instruction counts come from macOS `proc_pid_rusage`, with monotonic
elapsed time measured separately. Raw rounds and medians are retained.

The fixed scan saves **1.46–1.86%** of actual host instructions per tap and
**1.43–1.82%** across the complete producer-plus-packet-service probe. Median
host tap times are 629–635 ns versus 661–663 ns; complete producer times are
642–648 ns versus 665–677 ns. These separately executed batches are subject to
Mac load and timing noise; host counters/time do not establish pi32v2 timing.
The producer-owned qualifier candidate saves only approximately 0.4% host
instructions, while the qualifier-plus-OR candidate regresses instruction work.

All recorded probe USB queue **overrun counters are zero in the host simulated
controller**. This does not report actual native render deadline misses or
physical USB streaming results. Complete-producer measurements include tap,
render-start and actual packet service, not the whole synthesis audio IRQ.

The fixed scan passes the unchanged queue test in optimized and inherited
sanitizer modes: **249563 checks, zero failures, 1444 full-buffer oracles**
each; seven queue states × 29 late mutation cuts plus postcommit restore.
Nested TIMER5 service still occurs at every original publication observation.
TONE mode passes **73 checks** and continues to refuse quiet writes.
Sanitizer exclusions remain signed-integer-overflow, shift, bounds,
object-size and pointer-overflow; leak detection is disabled, as in the
inherited runtime suite. No existing assertion was removed or relaxed.

Direct parent/candidate byte comparison covers 132 variable-length tap cases
(`n=0..32`, four patterns), and 160 actual audio-IRQ DAC halves at both
44.1/48k rates, including actual ring, FIR state and sent packet bytes.
Both 1,161,568-byte output streams SHA-256:
`3c1933b827e532b8a000b21d5f37796c467e3accf2317ff68f5ffad628067f07`.
This is a comparison artifact, not a rewritten audio golden.

No new full suite, browser run, default four-track rebuild or physical test
was requested for this rejected firmware experiment. Private source snapshots,
compiled IR, images, disassemblies, all failed candidate reports and raw host
artifacts remain under `.local-baseline/usb-occupancy-check`. Checked-in evidence
contains only scoped text, the rejected patch and the optional host probe;
binaries and generated wave assets remain private.

## Next gate

Keep the existing quiet gate and production USB accounting. Issue #12/#56
needs broader native render/USB timing, stack and hardware qualification.
The modest host benefit could inform later compiler/register-pressure research,
but does not justify closing the current native timing gate or weakening any
resident-history, packet or late-cut refusal.
