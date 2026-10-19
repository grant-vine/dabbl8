# Dabbl8 RC review guide

This guide identifies current review inputs; it does not authorize installation,
device access or release. [RC status](release-candidate-status.md) is the current
source of truth. Earlier v020/v021 review instructions are
[archived](history/rc-review-guide-through-v021-20261019.md), not pending actions.

## Distinguish current and historical inputs

| Input | Identity | Review boundary |
| --- | --- | --- |
| Last verified device | v0.2.2-timing/FM-1_8022, package SHA `b3e137b9d4e918d32f6933e0bc92139e96e6d347df1570a3cacce8834cc238ff` | Originals restored, ports closed; retained physical evidence, not a new observation |
| Current frozen private package | v0.2.3-timing/FM-1_8023, firmware source `c34263f`, package SHA `c2872ecc0aca96e4f92a8530d0e2682287fd48e40ab0749ecd22ff508720f8cb` | `build/rc-v023-d8a1-20261019/`; includes D8C1/D8A1; not installed or RC-qualified |
| Current host preparation | Collector/readback `aeef693`; source archive `abb811f` | Frozen source/kit checks and main-session review under `build/rc-consolidated-v023-20261019/`; kit unarmed |
| Historical earlier v023 freeze | Firmware source `e251aa4`, package SHA `c3ebf3933ee5b6088f990cefda17e56ca468e5dc4f619326dba09349898fac83` | Preserved unchanged; lacks D8C1/D8A1; do not substitute it |

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
   review does not cover those later changes.
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
