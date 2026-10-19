# Dabbl8 RC review guide

This guide identifies current review inputs; it does not authorize installation,
device access or release. [RC status](release-candidate-status.md) is the current
source of truth. Earlier v020/v021 review instructions are
[archived](history/rc-review-guide-through-v021-20261019.md), not pending actions.

## Distinguish current and historical inputs

| Input | Identity | Review boundary |
| --- | --- | --- |
| Last observed device | v0.2.4-timing/FM-1_8024, main boot2/USB1, lifetime audio_late2 | Stopped at first post-install health check; original restoration/data unverified. Owner unavailable; no device access authorized. |
| Current private package | v0.2.5-timing/FM-1_8025, package SHA `7a16ea2c4ec9c3e944b84e37a7f27ebefe3501139c6e6b54f17f88680a81fea5` | `build/rc-v025-20261019/`; startup order/boot counter correction, unchanged loader; not installed or RC-qualified |
| Current host preparation | Source HEAD `341da9a` plus source-hashed working changes; repinned56-byte retained reader | Package/kit checks and local review under `build/rc-package-v025-20261019/`; hardware batch unarmed |
| Installed earlier package | v0.2.3-timing, firmware source `c34263f`, package SHA `c2872ecc0aca96e4f92a8530d0e2682287fd48e40ab0749ecd22ff508720f8cb` | Immutable `build/rc-v023-d8a1-20261019/`; preserve its failed-session evidence |
| Historical earlier v023 freeze | Firmware source `e251aa4`, package SHA `c3ebf3933ee5b6088f990cefda17e56ca468e5dc4f619326dba09349898fac83` | Preserved unchanged; lacks D8C1/D8A1; do not substitute it |

The v025 source/actual-code regressions and evidence limits are in
[boot timing qualification](boot-timing-qualification.md). The old lifetime counter
does not locate events within a boot; new coherent per-boot fields retain history
and reject every new-boot event. Reuse the fixed RAM/loader and prior comparisons
by their recorded hash bindings. The v024 freeze/failed session remain immutable.

The separate host follow-up is reviewed in [draft PR #207](https://github.com/grant-vine/dabbl8/pull/207),
with67 host tests and seven clock groups under `build/rc-host-audio-epoch-20261019/`.
It adds strict new-boot epoch checks and actual private-manifest compatibility to
the general collectors. Four host inputs deliberately differ from the frozen
275-file source receipt; all181 pinned firmware inputs and the package/kit remain
unchanged. Do not replace sealed host files or rebuild the firmware for this overlay.

These builds use eight tracks/eight shared sounding voices, full engine mask0x3ffd,
FM4 unavailable, USB audio off. Source-defined caps do not establish every
engine/preset/effect combination's target performance. Felucca and dependency
credits remain; inherited synthesis/effects are not new Dabbl8 development.

## Review locally without repeating completed work

1. Verify the appropriate immutable package/kit checksums and exact source,
   generated headers, compiler flags and component hashes. Do not substitute
   the newest source tree for the source that produced an older installed image.
2. Reuse the independent `e251aa4` source gate, 84 engine/effects cases, 42
   emulator workflows, UI and package/loader checks for unchanged effective
   inputs. The later clock delta has its own focused main-session review and
   receipts under `build/rc-clock-20261019/`. Later D8A1/collector/readback and
   consolidated package changes have focused receipts under
   `build/rc-arrangement-observer-20261019/`, `build/rc-arrangement-capture-20261019/`
   and `build/rc-consolidated-v023-20261019/`; the earlier independent source
   review does not cover those later changes. The v024 correction separately
   reuses COMMIT/sanitizer and linked-RAM review under
   `build/rc-erase-watchdog-20261019/`. Exact new-package consumers and56-byte
   reader receipts are in `build/rc-package-v024-20261019/`; do not repeat the
   broad matrix.
3. Read [retained physical results](hardware-qualification-20261019.md). Own
   project transfer, stopped save/recall, all-populated-slot cold retention and
   upgrade readback have passing evidence. Do not request them again to explain
   a stale issue body. Settings/presets/samples, interruption durability and
   physical routing remain separate where unverified.
4. Inspect [clock measurement](clock-measurement.md), conditional linked-stack
   evidence and [independent recovery preparation](independent-recovery-preparation.md).
   Clock samples are not full ISR/stack qualification. The RP2040-Matrix is not
   a tested drop-in XIAO replacement. Transporter info/dump can upload a RAM loader.
5. Review the [single unarmed hardware batch](rc-hardware-test-plan.md). The
   consolidated package, source/archive/reader pins and kit are now frozen;
   keep both v023 freezes intact. No private backup/vendor firmware belongs
   in the public source, corresponding-source archive or release assets.
6. Reuse the extended virtual-upgrade check under
   `build/rc-roadmap-audit-20261019/`: nonempty synthetic ancillary allocations
   and the full flash outside app/known-record sectors stay byte-exact. Each
   deliberately corrupted ancillary tail is rejected by both comparison layers.
   This strengthens local byte-preservation evidence, not ancillary-format
   reopening or physical durability. No new candidate build is required.
7. Reuse [the later current-format ancillary upgrade check](ancillary-upgrade-qualification.md).
   It runs actual settings, preset, FM6-object and sample readers after a virtual
   upgrade, covering reopening beyond step6's raw byte comparison. Neither check
   establishes physical interruption durability or expands project-only backups
   to include ancillary data. Preserve their distinct scopes.

## Installation, compatibility and remaining gates

First installation resets previous firmware user projects/autosave/settings/
presets/samples within the approved user-state map, after disclosed confirmation.
There is no external-firmware migration promise. Recognized Dabbl8 data must
survive supported own upgrades; unknown/damaged formats require an explicit
recovery/reset decision. The project archive does not include samples/presets/
settings. Keep [own-format limits](native-project-archive.md) explicit.

Felucca's established package/loader flow remains the evaluation route. No new
flash operation, arbitrary recovery protocol, linker boundary or production
deployment is introduced by local review. Source archives and component hashes
identify the tested build inputs; compiler date strings and the patched Mac MIDI
runtime's exact bytes/ABI remain reproduction boundaries.

Public RC still requires complete timing/stack evidence, supported workload and
compatibility limits, independent recovery and a second tester. Release publication
requires separate owner instruction. Check availability before any physical
request; no SAVE, restore, reboot, upload or flash is currently pending.
