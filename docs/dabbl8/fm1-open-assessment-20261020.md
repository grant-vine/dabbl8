<!-- SPDX-License-Identifier: GPL-3.0-only -->
# FM1 Open assessment and completed-work reconciliation

Assessment date: 20 October 2026. Vendor source pinned at
[4d266850f8a46bd5154a0d495c1f60317156c9f7](https://github.com/m-vave/fm1_open/tree/4d266850f8a46bd5154a0d495c1f60317156c9f7),
FM1 Open V14. Three advisors examined hardware/flash, architecture/builds and
licensing/provenance. This is an investigation, not an adopted base or hardware
qualification. No FM-1 access, restore, reset or flash occurred.

## Recommendation

Use FM1 Open immediately as a primary hardware/runtime reference. Preserve the
current Dabbl8 source and evidence while investigating its concrete differences.
A new vendor-derived application is plausible if its initial six sound-source
types and proprietary native SDK are acceptable. It is not a drop-in replacement
that preserves Dabbl8's current thirteen engines, global eight shared voices,
three projects/shared autosave, sound, arrangement and backup behavior.

Do not discard the existing implementation or declare a restart feasible merely
because the vendor demonstrates eight tracks. Before a production base change,
require an offline full link/map with all vendor validators, an explicit
feature/ownership comparison and a cleared licensing composition. A smaller
vendor-based edition is a product tradeoff, not a claim that thirteen engines
are mandatory or that the vendor architecture cannot be extended.

## Official source and scope

The [official announcement](https://forum.m-vave.com/topic/4) identifies
`m-vave/fm1_open` as M-VAVE's project. The
[pinned README](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/README.md)
describes eight tracks, eight sample slots, sixteen demo projects, MIDI routing,
a browser workbench and Windows build scripts. Open APP and board sources are
provided; native instrument/platform implementation is supplied in a binary SDK.
Official provenance does not establish bug-free operation or hardware results on
our FM-1.

| Question | Existing Dabbl8 | Pinned FM1 Open V14 |
| --- | --- | --- |
| Sound sources | 13 selectable engines in full profile; FM4 reserved/disabled | Six fixed source types: DX7, 303, 808, PSG, Pluck, Sampler |
| Voice ownership |Eight globally shared sounding voices |Native renderer/control sealed in SDK; no documented custom-engine/global-budget registration interface |
| Projects |Three named projects, shared autosave, rotating spare; own D8P1/archive |Sixteen project sectors and shared journal; different records and boot selection |
| Platform |Standalone C/assembly, source RAM flash path, existing Mac/container build |RTOS/startup/connectivity/instrument combined native SDK; public service contracts |
| Build evidence |Frozen private v028 with exact target/source checks |All 12 unchanged APP units and generated identity linked through a Mac/container adapter; unchanged image validator passes; exact Windows toolchain contract and hardware remain unqualified |
| Browser |Existing actual-renderer functional evidence; not hardware proof |Separate portable browser DSP and simulated storage/counters; native DSP parity not established |
| Licensing |GPL-3.0-only Felucca/Dabbl8 source lineage |APP GPLv3 with SDK linking permission; native engine separately licensed |

Source types: [instrument.h:10-26](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/app/instrument.h#L10-L26).
Native interface: [fm1_engine_control.h](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/sdk/include/fm1_engine_control.h).
Projects: [instrument.h:325-334](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/app/instrument.h#L325-L334).
Browser distinction: [Simulator.c](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/web/engine/Simulator.c)
and [sound.c](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/web/engine/sound.c).

## What helps the COMMIT investigation now

1. The native SDK's P33 transfer emits a real `csync` after starting the transfer.
   Dabbl8 currently uses a control-register readback at that point. This is a
   specific watchdog-feed ordering hypothesis worth testing. No observed
   evidence yet establishes that the readback is inadequate or that this caused
   the physical reset. An instruction comparison can prove alignment; a host
   model that assumes posted-write failure cannot prove silicon behavior.
2. Vendor SPI byte launch/direction/PND/PCLR ordering and 50-million-poll budget
   agree with the existing broad sequence. Its raw NOR wait has no hidden
   watchdog feed. Some raw timeout results are discarded before XIP restoration;
   copying that handling would not preserve Dabbl8's failstop/readiness policy.
3. The [public flash API](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/sdk/include/fm1.h#L141-L167)
   requires an IRQ-enabled control task and rejects callbacks. That contract
   belongs to the vendor RTOS API; it does not demonstrate that Dabbl8's admitted
   IRQ-off SRAM primitive is wrong or that those APIs are interchangeable.
4. [Vendor timer setup](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/app/board/device.c#L10-L24)
   establishes the full 32-bit TIMER4 period and expected crystal/divider,
   independently supporting the v027 timer correction already made.
5. Public board source provides useful IRQ-token, DMA/RAM-placement, input and
   display references. NOR/SPI/P33/WDT/SFC/exception implementations themselves
   are binary-only in this repository. There is no public C flash-driver fix
   ready to copy into Dabbl8.

The SDK archive contains one native object, `engine_startup.S.o`; relocations
were resolved from its ELF symbol/RELA tables rather than guessed from objdump's
unresolved `call0` annotations. Archive SHA-256:
`cf94df1316d57390499db121ccd9b0375e570160441fc77ae29111fe431725d5`,
matching the [manifest](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/sdk/manifest.json).
P33 barrier location is section `p33_buf+0xC`; this is a relocatable section
offset, not a runtime PC. Raw source/disassembly/advisor evidence is retained
under ignored `build/fm1-open-assessment-20261020/`.

The physical v025 COMMIT failure remains unresolved. No source change, longer
timeout, feed-in-wait, unsafe return, reflash or restore retry follows from this
investigation.

## Feasibility limits and concrete local probe

The [build contract](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/sdk/Build.ps1#L296-L424)
pins Windows compiler hashes and PI32v2 r3/FPU/LTO flags, strong native instrument
definitions, runtime tables, Bluetooth/audio dependencies and minimum runtime
heap 49,152 B. Its linked memory layout differs from Dabbl8's globals/pool map.
No linked vendor ELF/map is supplied in the clone. Pre-GC archive section totals
cannot be treated as retained memory or spare headroom.

The initial unchanged `app/board/device.c` probe passed and is retained in the
assessment evidence. The follow-through then compiled all 12 unchanged APP
units plus the generated identity and linked the complete native SDK image.
The compile/link tail and `Test-Firmware` function are extracted unchanged from
the pinned build script. The adapter supplies POSIX containment/source paths,
cached Linux executable/plugin names and offline Docker command transport.
SDK payload, DSP ABI, coefficients and SRAM-contract checks remain active.
The separate exact Windows executable/DLL hash contract is explicitly
unsatisfied; it was not edited to call Linux tools an official match.

The unchanged linked-image validator passes startup, registration tables,
strong APP callbacks, RTOS ABI, SRAM audio/panel placement, heap minimum,
Bluetooth discovery and USB audio/MIDI presence. Build receipt:

| Observation | Result |
| --- | --- |
| Source | Pinned V14; application and SDK files unmodified |
| Compile/link commands | 12 APP compiles, 1 generated-identity compile, 1 link; all exit 0 |
| APP image sections | 453,736 B; not a packaged firmware size |
| All allocated non-NOBITS sections | 453,788 B, including 52 B boot information |
| `.ram0_data` / `.ram0_bss` | 80,936 B / 195,232 B |
| Linker runtime heap | 247,372 B before boot allocations; minimum check 49,152 B |
| Cache RAM initialized data | 32 B |
| ELF SHA-256 | `3ca1b97170d36086260565662a4e2a0ad7a94fa63f65d3bbe0f4e65d7521a673` |
| Map SHA-256 | `ffed004321cd6a943f6e30a06e9f7604e739facfae6f9ebbc327b5593e65f126` |
| Image validator | Pass; exact original function SHA-256 `316defbdf9a7c24ceaefe68c3b3839e892441266d47165e532976115d1c7e287` |
| Official Windows toolchain / hardware | Unqualified / not accessed |

The heap is not measured free memory after startup, nor a directly comparable
Dabbl8 pool margin. Runtime allocations, engine limits, timing, storage and
actual boot still require separate evidence. No dependency/compiler download,
firmware packaging, hardware access or production base switch occurred.
The isolated runner, command logs, unchanged-source receipt, ELF/map and
validator output are preserved in ignored `build/fm1-open-link-20261020/`.

Reducing the vendor project count mainly releases flash rather than sixteen
resident project buffers. Its browser's per-track capacity model is different
from native opaque state; do not infer device polyphony or timing from it.
The current new-sample transfer budget is about 19.55 seconds total across all
eight slots, not that duration per slot. These are different product/storage
contracts, not free additional space for our fixed map.

The full-link experiment establishes a technically viable native build route.
Before a base decision, compare the six-source edition against desired Dabbl8
behavior, establish an approved non-Windows toolchain contract and resolve any
SDK-linked Felucca licence composition. A supported platform-only SDK could
change the port cost, but no such interface was found. No new production
project/branch was started and no physical COMMIT repair has been established.

## Licensing boundary

The [APP licence](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/LICENSE.md#L3-L19)
allows marked APP code to link with the separately licensed SDK.
[Component terms](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/THIRD_PARTY.md#L25-L63)
grant proprietary-engine use/distribution for FM1 devices with notices, while
withholding its source. A new modified vendor APP can use that route. This
permission does not automatically cover inherited GPL-only Felucca code/assets;
see [GNU's explanation](https://www.gnu.org/licenses/gpl-faq.en.html#GPLIncompatibleLibs).
A name change or replacement HAL does not resolve that rights boundary.

Source-only board/HAL reuse without the proprietary engine is the simpler GPL
path, subject to each file's notices and dependencies. An SDK-linked composition
containing existing Felucca-derived material needs a suitable rights-holder
permission or another established licensing basis before distribution.
This is a composition risk assessment, not a claim that a clean vendor-derived
edition is forbidden. No SDK material was incorporated into Dabbl8.

The bundled Apache-2.0-labelled stock V15 file is 699,956 B and matches our retained
restore file exactly: SHA-256
`db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`.
The original remains outside Git. No vendor firmware or SDK binary is committed.

## GitHub reconciliation actually performed

The owner explicitly requested checkoffs. Nine issue bodies were updated:
[#2](https://github.com/grant-vine/dabbl8/issues/2),
[#12](https://github.com/grant-vine/dabbl8/issues/12),
[#13](https://github.com/grant-vine/dabbl8/issues/13),
[#14](https://github.com/grant-vine/dabbl8/issues/14),
[#20](https://github.com/grant-vine/dabbl8/issues/20),
[#21](https://github.com/grant-vine/dabbl8/issues/21),
[#17](https://github.com/grant-vine/dabbl8/issues/17),
[#53](https://github.com/grant-vine/dabbl8/issues/53) and
[#56](https://github.com/grant-vine/dabbl8/issues/56).
Eleven newly checked items reflect completed software or explicitly bounded
observations/decisions. Original combined acceptance criteria were retained;
unmet physical/release prerequisites were not weakened or checked.

Seven original tasks are already closed: #3-#8 and #18. #4 is complete and must
not be restarted. No new issue closure or PR merge was justified by the present
evidence. The actual v025 installation/warm/cold success and subsequent COMMIT
watchdog failure supersede stale statements that v025 was not installed.
Private v028 adds retained entry-clock diagnostics and remains a local candidate,
not a proven physical reset repair. Its 610,106 B package has SHA-256
`7e1df414687601d96650697c7b9aaaab042a0099ccbe88c27ccebe42be5cd549`;
30 offline package steps and 38 fake-reader cases pass. The Felucca loader is
unchanged. No diagnostic or firmware is offered as a public download here.

The request does not authorize a firmware flash, device access, release,
website deployment, production-infrastructure change or automatic hosted CI.
Existing passing firmware/engine/emulator matrices were reused; this assessment
did not repeat them. Raw before bodies, prepared after bodies and accepted GitHub
command results are in the ignored evidence directory.
