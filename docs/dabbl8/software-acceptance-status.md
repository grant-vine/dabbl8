<!-- SPDX-License-Identifier: GPL-3.0-only -->
# v0.1 software acceptance status — 14 October 2026

**Historical snapshot.** See [RC readiness - 15 October](release-candidate-status.md)
for the current source/package identities, completed v0.1.8 restore, playback and
reboot evidence, and remaining gates. In particular, the static failures and
unperformed own-project restore/reboot checks described below are historical;
they must not be applied to the later package without comparing its evidence.

This bounded review maps live issues #9–#21 to development source
`6c7046d15f269c4b2fc33128ad9c43fdb043f4e7`. The installed private v0.1.5 diagnostic
is separate source `76ea40ba7a673d93d5b59053457c98bbf6d75191`; its
[hardware observations](hardware-initialization-20261014.md) do not qualify later
source or a release. Eight logical tracks continue to share eight sounding voices.

The initial review identified a missing resource-observation interface for #12.
The dated follow-up below records its subsequent implementation and private
[package/hardware handoff](resource-diagnostic-package.md). D8T2 timing alone still
does not provide those observations. The original source coverage map remains a
bounded snapshot; implemented controls and host checks do not close physical
acceptance gates.

## Requirement map

| Issue | Current source and focused fixture coverage | Remaining acceptance |
| --- | --- | --- |
| [#9 sequencing/MIDI](https://github.com/grant-vine/dabbl8/issues/9) | [Actual eight-track recording/playback and simulated USB/TRS routing fixture](../../tests/eight_track_runtime_test.c), [shared voice admission](../../firmware/src/voice.c) and [three-slot project frontend](../../firmware/src/project.c). | Concurrent physical sequencing/routing and three saved slots/shared autosave. Speaker, four-knob response and individual tracks 5–8 keyboard sound are owner-confirmed. |
| [#10 mixer/selection](https://github.com/grant-vine/dabbl8/issues/10) | [Banked mixer/layer/render fixture](../../tests/banked_mixer_test.c), [adopted native render fixture](../../tests/native_state_render_test.c) and [panel undo fixture](../../tests/native_ui_undo_test.c). | Physical gestures/readability and prerequisite #9. Existing host render coverage is bounded, not every possible gesture/state. |
| [#11 editor/own backup](https://github.com/grant-vine/dabbl8/issues/11) | [Companion](../../web/d8companion.html), [command-81 handler](../../firmware/src/native_project_transfer.c), [virtual-NOR transfer fixture](../../tests/native_project_transfer_test.c), [actual client bridge fixture](../../tests/native_project_client_test.mjs) and [own-format compatibility](native-project-archive.md). | Physical connected own-project backup/import/restore. Archives exclude settings, presets and samples; they are not a full instrument backup. Clean-install disclosure and supported version/refusal behavior are documented. |
| [#12 resource bounds](https://github.com/grant-vine/dabbl8/issues/12) | [Shared allocation/lifetime fixture](../../tests/heavy_pool_test.c), implemented [pool/stack resource controls and capture](resource-diagnostic-capture.md), and [private application/package evidence](resource-diagnostic-package.md). | Target workload measurements and supported engine combinations/caps remain pending. Pool reports observed live-owned occupancy, not general RAM allocation high-water; stack reports a surviving-write lower bound, not complete stack safety. Host checks and fixed reservations cannot satisfy the physical gate. |
| [#13 own persistence](https://github.com/grant-vine/dabbl8/issues/13) | [Shared commit-last pool](../../firmware/src/d8pool.c), [cut/refusal fixture](../../tests/d8pool_test.c), [first-use foundation/cut fixture](../../tests/native_first_use_foundation_test.c), [local save/rename/autosave fixture](../../tests/native_local_save_test.c), [own settings](../../firmware/src/native_settings_session.c), [presets](../../firmware/src/native_preset_session.c) and [sample replacement](../../firmware/src/native_sample_session.c). | Physical interrupted-format/reboot recovery, supported own-upgrade data retention and stopped save/recall. Initialization completed once; that does not qualify interruption or recall. Live saving is separately gated. |
| [#14 first target/recovery](https://github.com/grant-vine/dabbl8/issues/14) | [Diagnostic collector](../../tools/dabbl8_timing_capture.py), [scripted transport fixture](../../tests/dabbl8_timing_capture_test.py), [first-use diagnostic](owner-first-use-diagnostic.md) and exact [installed-source receipt](hardware-initialization-20261014.md). | Stress timing, calibrated clock/overhead, stack/pool measurement, routing, save/reboot/own restore and independent recovery. The ten-second idle capture is one limited observation. This UAC-off diagnostic cannot test USB audio rates. |
| [#15 arrangement](https://github.com/grant-vine/dabbl8/issues/15) | [Reference/overlay/fill behavior](native-arrangement-playback.md), [actual arrangement fixture](../../tests/native_arrangement_test.c), [source/engine compatibility fixture](../../tests/arrangement_engine_compatibility_test.c) and [undo fixture](../../tests/native_ui_undo_test.c). | Target memory/timing after features and prerequisite #14; software behavior checkboxes do not replace those gates. |
| [#16 weighted FM budget](https://github.com/grant-vine/dabbl8/issues/16) | [Bounded eight-voice policy](voice-allocation.md) and [actual voice-budget fixture](../../tests/voice_budget_test.c); no weighted expanded-polyphony claim. | Real target engine profiles, measured cost/caps and stressed maximum deadlines. Expanded polyphony is a later experiment, not required to preserve the selected eight shared voices. |
| [#17 donor feature](https://github.com/grant-vine/dabbl8/issues/17) | [Feature priorities](08-interface-and-feature-priorities.md) defer sonic enrichment behind core reliability; [provenance requirements](license-provenance.md) govern any future import. | Select/audit/port only an actual unmet musical requirement after #14. No donor feature is identified as necessary for working v0.1; do not create one from an unchecked optional backlog. |
| [#18 local evidence](https://github.com/grant-vine/dabbl8/issues/18) | Closed issue; [selective local planner/runner](../../ci/local_checks.py) and [policy](../../AGENTS.md). | No new gap identified in this review. Hosted CI remains disabled; do not repeat the completed baseline. |
| [#19 identity/licenses](https://github.com/grant-vine/dabbl8/issues/19) | [Identity contract](identity-profile.md), [USB identity fixture](../../tests/d8identity_usb_test.c), [identity parser fixture](../../tests/d8identity_test.py) and [license inventory](../../tools/license_inventory.py). | Software implementation steps are checked; #14/recovery remains open. A distribution still needs exact corresponding source and notices. |
| [#20 release](https://github.com/grant-vine/dabbl8/issues/20) | Local candidate package/source/notices/checksum preparation is recorded in the issue; the [installed-source evidence](hardware-initialization-20261014.md) names exact artifacts and static failures. | Hardware prerequisites, tested compatibility/engine limits, supported own backup/restore/upgrades, a second tester and separate publication authorization. No public release is qualified. |
| [#21 website](https://github.com/grant-vine/dabbl8/issues/21) | Website-repository review preparation is recorded in the issue; public download/compatibility text depends on #20. | Complete release-specific information after qualification. Website publication remains a separate action, not firmware implementation work. |

## Resource capability follow-up — 14 October 2026

The reviewed host/integration source is now
`045da5c3b998bd3193576c9c1b7dac9f75bb82c2`. The target application is a separate
build from `532847765115d089b776e48b13a6275a20d1800a`; later host fixes preserve its
original source hashes. The [private handoff](resource-diagnostic-package.md)
records exact application/loader identities and the locally validated private
package receipt; independent package review is a separate gate. No installation or device capture is authorized in this phase.

[Pool diagnostic](../../firmware/src/heavy_pool_diagnostic.h),
[cold-start stack readout](../../firmware/hal/fm1_stack_watermark.h),
[main-loop resource console](../../firmware/src/resource_diagnostic_console.h)
and [strict collector](../../tools/dabbl8_resource_capture.py) implement the
previously missing bounded observation interface. Both helpers are default-off;
D8R1 is separate from D8T2 and requires an explicit private resource kit. Pool
start binds actual boot/generation and excludes owner mutation; paused export
copies under a short main-only lock, then scans/prints unlocked. The collector
preserves original frame bytes and rejects invalid, incomplete or foreign rows.

The pool retains initial/request-boundary live-owner censuses, per-engine
independent peaks and allocation failures, with fixed capacity/body/reservation
reported separately. It is not general pool/FX/LCD RAM high-water. Last-observed
ownership can be stale after release. Both application stack windows report
surviving writes since boot, a nonatomic lower bound that can conceal deeper use;
there is no maximum-SP or complete interrupt-nesting/stack-safety claim. Actual
workloads, calibrated timing/observer overhead and supported engine caps remain
pending for #12/#14. The UAC-off candidate cannot qualify USB audio rates.

Recorded local checks pass 24 resource collector/parser, 20 timing transport,
8 builder and 29 planner tests. Real console fixtures cover Full/Core/Breaks in
optimized and strict sanitizer modes; default-off console objects remain equal
to the prior source. The target application links within unchanged allocations,
but SLICE +14% and audio IRQ +48% still fail static budgets; TIMER5 passes. These
checks establish software capability and bounded evidence, not physical resource
qualification, recovery or release acceptance.

## Verification scope

Only affected documentation/link checks are selected for this follow-up. The
recorded implementation/build checks above are reused; no firmware suite, target
rebuild, package generation, device access or hosted CI is performed by this
documentation slice. For the exact focused invocations already wired for these fixtures, see
[local check recipes](../../ci/README.md) and [host runner](../../tests/run_tests.sh).
The unchanged SLICE and audio-interrupt static failures remain explicit in the
installed-source evidence; TIMER5's static pass is not a full deadline clearance.
