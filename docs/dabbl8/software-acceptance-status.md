<!-- SPDX-License-Identifier: GPL-3.0-only -->
# v0.1 software acceptance status — 14 October 2026

This bounded review maps live issues #9–#21 to development source
`6c7046d15f269c4b2fc33128ad9c43fdb043f4e7`. The installed private v0.1.5 diagnostic
is separate source `76ea40ba7a673d93d5b59053457c98bbf6d75191`; its
[hardware observations](hardware-initialization-20261014.md) do not qualify later
source or a release. Eight logical tracks continue to share eight sounding voices.

The current software gap identified here is the ability to measure engine-pool
and stack high-water for #12. Timing collection does not provide those values.
Current-status and own-format documentation contradictions are corrected in this
slice. The linked source and fixtures show implementation coverage; they are not
new passing-test claims or a reason to close physical acceptance gates.

## Requirement map

| Issue | Current source and focused fixture coverage | Remaining acceptance |
| --- | --- | --- |
| [#9 sequencing/MIDI](https://github.com/grant-vine/dabbl8/issues/9) | [Actual eight-track recording/playback and simulated USB/TRS routing fixture](../../tests/eight_track_runtime_test.c), [shared voice admission](../../firmware/src/voice.c) and [three-slot project frontend](../../firmware/src/project.c). | Concurrent physical sequencing/routing and three saved slots/shared autosave. Speaker, four-knob response and individual tracks 5–8 keyboard sound are owner-confirmed. |
| [#10 mixer/selection](https://github.com/grant-vine/dabbl8/issues/10) | [Banked mixer/layer/render fixture](../../tests/banked_mixer_test.c), [adopted native render fixture](../../tests/native_state_render_test.c) and [panel undo fixture](../../tests/native_ui_undo_test.c). | Physical gestures/readability and prerequisite #9. Existing host render coverage is bounded, not every possible gesture/state. |
| [#11 editor/own backup](https://github.com/grant-vine/dabbl8/issues/11) | [Companion](../../web/d8companion.html), [command-81 handler](../../firmware/src/native_project_transfer.c), [virtual-NOR transfer fixture](../../tests/native_project_transfer_test.c), [actual client bridge fixture](../../tests/native_project_client_test.mjs) and [own-format compatibility](native-project-archive.md). | Physical connected own-project backup/import/restore. Archives exclude settings, presets and samples; they are not a full instrument backup. Clean-install disclosure and supported version/refusal behavior are documented. |
| [#12 resource bounds](https://github.com/grant-vine/dabbl8/issues/12) | [Shared heavy-state allocation](../../firmware/src/heavy_state.h), [mixed lifetime fixture](../../tests/heavy_pool_test.c), [timing recorder](../../firmware/src/audio_timing.h) and linked memory evidence. | **Missing diagnostic software:** report engine-pool occupancy high-water and stack high-water. Then measure target workloads and publish supported engine combinations/caps. Linked allocation and host fixtures cannot supply those measurements. |
| [#13 own persistence](https://github.com/grant-vine/dabbl8/issues/13) | [Shared commit-last pool](../../firmware/src/d8pool.c), [cut/refusal fixture](../../tests/d8pool_test.c), [first-use foundation/cut fixture](../../tests/native_first_use_foundation_test.c), [local save/rename/autosave fixture](../../tests/native_local_save_test.c), [own settings](../../firmware/src/native_settings_session.c), [presets](../../firmware/src/native_preset_session.c) and [sample replacement](../../firmware/src/native_sample_session.c). | Physical interrupted-format/reboot recovery, supported own-upgrade data retention and stopped save/recall. Initialization completed once; that does not qualify interruption or recall. Live saving is separately gated. |
| [#14 first target/recovery](https://github.com/grant-vine/dabbl8/issues/14) | [Diagnostic collector](../../tools/dabbl8_timing_capture.py), [scripted transport fixture](../../tests/dabbl8_timing_capture_test.py), [first-use diagnostic](owner-first-use-diagnostic.md) and exact [installed-source receipt](hardware-initialization-20261014.md). | Stress timing, calibrated clock/overhead, stack/pool measurement, routing, save/reboot/own restore and independent recovery. The ten-second idle capture is one limited observation. This UAC-off diagnostic cannot test USB audio rates. |
| [#15 arrangement](https://github.com/grant-vine/dabbl8/issues/15) | [Reference/overlay/fill behavior](native-arrangement-playback.md), [actual arrangement fixture](../../tests/native_arrangement_test.c), [source/engine compatibility fixture](../../tests/arrangement_engine_compatibility_test.c) and [undo fixture](../../tests/native_ui_undo_test.c). | Target memory/timing after features and prerequisite #14; software behavior checkboxes do not replace those gates. |
| [#16 weighted FM budget](https://github.com/grant-vine/dabbl8/issues/16) | [Bounded eight-voice policy](voice-allocation.md) and [actual voice-budget fixture](../../tests/voice_budget_test.c); no weighted expanded-polyphony claim. | Real target engine profiles, measured cost/caps and stressed maximum deadlines. Expanded polyphony is a later experiment, not required to preserve the selected eight shared voices. |
| [#17 donor feature](https://github.com/grant-vine/dabbl8/issues/17) | [Feature priorities](08-interface-and-feature-priorities.md) defer sonic enrichment behind core reliability; [provenance requirements](license-provenance.md) govern any future import. | Select/audit/port only an actual unmet musical requirement after #14. No donor feature is identified as necessary for working v0.1; do not create one from an unchecked optional backlog. |
| [#18 local evidence](https://github.com/grant-vine/dabbl8/issues/18) | Closed issue; [selective local planner/runner](../../ci/local_checks.py) and [policy](../../AGENTS.md). | No new gap identified in this review. Hosted CI remains disabled; do not repeat the completed baseline. |
| [#19 identity/licenses](https://github.com/grant-vine/dabbl8/issues/19) | [Identity contract](identity-profile.md), [USB identity fixture](../../tests/d8identity_usb_test.c), [identity parser fixture](../../tests/d8identity_test.py) and [license inventory](../../tools/license_inventory.py). | Software implementation steps are checked; #14/recovery remains open. A distribution still needs exact corresponding source and notices. |
| [#20 release](https://github.com/grant-vine/dabbl8/issues/20) | Local candidate package/source/notices/checksum preparation is recorded in the issue; the [installed-source evidence](hardware-initialization-20261014.md) names exact artifacts and static failures. | Hardware prerequisites, tested compatibility/engine limits, supported own backup/restore/upgrades, a second tester and separate publication authorization. No public release is qualified. |
| [#21 website](https://github.com/grant-vine/dabbl8/issues/21) | Website-repository review preparation is recorded in the issue; public download/compatibility text depends on #20. | Complete release-specific information after qualification. Website publication remains a separate action, not firmware implementation work. |

## Missing resource measurement capability

[heavy_state.h](../../firmware/src/heavy_state.h) retains eight statically allocated
bodies and current owner keys, with liveness checked on use. It does not retain
maximum concurrent live body occupancy or expose a resource report. Allocated
pool bytes are a static bound; live occupancy high-water is a distinct workload
observation. Stale owner keys alone must not be counted as live allocations.

The [D8T2 recorder](../../firmware/src/audio_timing.h) and
[console report](../../firmware/src/audio_timing_console.h) contain timing,
late/UAC publications and auxiliary observations, with no resource peaks.
[status/dbg](../../firmware/src/console.c) likewise expose no stack or pool
high-water. [Hardware guards](../../firmware/hal/fm1_guard.h) detect limit/write
violations; they do not measure peak stack use. The build helper's
[stack windows](../../tools/build.py) are fixed linked allocations only.

A diagnostic implementation must establish safe startup/measurement boundaries,
cover both application stacks and interrupt nesting, retain workload/source/boot
identity, report invalid or unavailable measurements explicitly, and quantify
its RAM/stack/interrupt cost. Painting or scanning an active stack can corrupt
live frames; periodic stack-pointer sampling can miss a deeper nested peak.
No runtime resource instrumentation is implemented by this documentation change.

## Verification scope

Only the affected documentation check is selected for this slice. Existing source
and fixture references were inspected directly; the firmware suite, vendor build,
package generation, device writes and hosted CI are not repeated. For the exact
focused invocations already wired for these fixtures, see
[local check recipes](../../ci/README.md) and [host runner](../../tests/run_tests.sh).
The unchanged SLICE and audio-interrupt static failures remain explicit in the
installed-source evidence; TIMER5's static pass is not a full deadline clearance.
