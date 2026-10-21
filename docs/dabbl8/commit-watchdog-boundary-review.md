<!-- SPDX-License-Identifier: GPL-3.0-only -->
# COMMIT boundary review — 20 October 2026

**The physical v025 COMMIT watchdog failure remains unresolved.** Three advisors
reviewed current upstream, system boundaries and the actual COMMIT invariants.
The locally reproduced timer defect is fixed in private v027; neither that fix
nor the v026 trace correction is established as the device-reset repair.
No FM-1 discovery, port access, restore, reset, saved-data change or flash was
performed. Do not repeat the failed restore on the strength of these host passes.

## Reproduced defect and correction

The application TIMER4 initializer checked only control bits0–3. It accepted
the expected run/source with a wrong divider in bits4–7, or a short inherited
period. A short period can repeatedly wrap before the NOR program deadline,
making the WIP timeout unreachable for some counter phases. The corrected
application accepts continuity only with control bits0–7 equal to9 and period
`UINT32_MAX`; otherwise it establishes the existing 24MHz/full-width contract.
An already correct clock keeps its count. The updater's historical branch and
the sealed loader binary are unchanged. No NOR traffic, deadline, watchdog feed,
flash map, project format or engine/effect behavior changes.

`tests/time_handover_test.c` executes the actual initializer over512 inherited
configurations, followed by the actual RAM WIP wait with full/short counter
periods. The original initializer fails79 of1540 assertions; the correction
passes1540 in optimized and fatal ASan/UBSan modes. Independently removing either
the period or divider check is rejected. Loader initialization's optimized host
LLVM IR is identical; the target loader is reused byte-for-byte, not rebuilt.

The current upstream header, with only its three MMIO lvalues substituted,
reproduces the same79 host failures. This establishes the inherited-clock defect
in upstream source. It does **not** establish upstream users' physical resets.

Attribution is limited: the saved v025 failure contains no TIMER4 CON/PRD
snapshot. Its large captured tick value rules out the demonstrated tiny period
at that instant, and the earlier cold-boot uptime advanced close to wall time.
Those observations weigh against this defect explaining the physical incident.

## Upstream and independent review

The live Felucca main tip is
[`5738617968a22b5d4c0d3470cfcc95559052f446`](https://github.com/hugelton/Felucca/tree/5738617968a22b5d4c0d3470cfcc95559052f446),
Felucca1.6.1, matching the cached application source. Its linked BSP was cloned
separately at
[`33cac693903e753e8783baeed05c19cadfb04812`](https://github.com/hugelton/felucca-bsp/tree/33cac693903e753e8783baeed05c19cadfb04812).
Both retain the same SPI/NOR status and P33 watchdog design; no newer verified
COMMIT repair was found. No branch was upgraded or BSP code imported. Its
additional flash ranges are project-specific and must not be adopted here.

The BSP's CPU note describes a partly characterized multiply/load hazard. The
exact frozen RAM flash driver contains neither pair-register multiplies nor
halfword loads, so that instruction mix is absent from this suspended-XIP path.
No general DSP rewrite is justified by this incident.

The invariant advisor replayed the captured address0x9D800 at completed9/feeds10,
checking lease, IRQ fence, exact source bytes and sealed plan. A full synthetic
restore completed116 primitives and8602 checks under ASan/UBSan. This probe uses
virtual NOR; it does not exercise the physical peripheral or prove write safety.
The emitted trace/source RAM intervals are outside configured write guards.
The target RAM audit passes3960 instructions/12216 bytes with exact encoded-byte
matching, no calls and no escaping direct branches. The RAM bytes are identical
to v026: the new correction executes during application startup.

## Package and local evidence

- Private candidate: `build/rc-v027-20261020/`, FM-1_8027/v0.2.7-timing.
- Package610106B, SHA-256
  `6b76115ced3c3d452657bd4fe779f56cf781fb6c3469da358380aced2482e757`.
- Application529008B; RAM94796/98304 (3508 free); pool335696/344064
  (8368 free;176 above the existing reserve). Budgets unchanged.
- Full mask0x3ffd, eight tracks/eight shared voices, USB audio off. All previous
  engines/effects retained. Felucca loader/installer and dependency notices kept.
-27 selected local steps pass with a stable source receipt:
  `build/local-checks/run-uiokhstj/results.json`. Includes timer1540, trace75,
  SPI12 cases, HAL160, first-use5554 and reset-retention401 per mode, plus runner
  tests. No hosted CI, new dependency installation or device action.
-30 exact-package consumer commands and28 fake console/cleanup cases pass.
  Previous84 engine/effects,42 emulator workflows,67 host-overlay tests and
  seven clock groups are reused for unchanged inputs; no repeat matrix.

Investigation, pinned upstream clones, red/green/mutation evidence and advisor
reports are retained under `build/rc-commit-investigation-20261020/`. Earlier
v026 evidence/package remains immutable. The first27-step run passed commands
but its receipt correctly refused changing advisor report files; the final run
has a stable receipt. A private helper's first invocation lacked its required
output-directory variable; the failure is retained and the corrected invocation
passes. Neither was a device failure.

## Remaining causal gap

The retained SPI label is not an atomic final-instruction trace; v026 fixed a
reproduced stale label. Source/static checks cannot distinguish a physical bus
stall, rejected watchdog feed, frozen peripheral clock or CPU exception during
XIP suspension. Exception reporting itself resides in XIP, so its absence is
not evidence that no exception occurred. No controlled local reproducer currently
selects one of these causes, and no speculative feed, retry, longer timeout or
unsafe XIP restoration has been introduced.

Current installed image remains v025, and original-project restoration/readback
is unverified. Preserve its incident and canonical original backup. Physical
COMMIT repair, exact original readback, full timing/stack, independent recovery,
second tester and publication approval remain external release gates. Hardware
work requires fresh owner direction; no batch is armed or queued.
