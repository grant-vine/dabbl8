<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Private v0.1.6 resource diagnostic handoff — 14 October 2026

This handoff prepares a private diagnostic package and future hardware work.
**No installation, reset, storage write or device capture is authorized in this
phase.** The FM-1 may remain unplugged. The package is not a qualified release.
The installed v0.1.5 [initialization and limited physical observations](hardware-initialization-20261014.md)
do not qualify this new application.

## Artifact and source boundaries

The exact target application was built once from
`532847765115d089b776e48b13a6275a20d1800a`. Current host capture tools are the
separate reviewed source `045da5c3b998bd3193576c9c1b7dac9f75bb82c2`, including the
fix that preserves original report bytes and refuses malformed endings/foreign
rows. Packaging reuses that application and the verified unchanged Felucca OTA
loader through [the existing packer](../../tools/fm1pkg_make.py); it does not
rebuild the application or introduce a loader or flashing mechanism.

| Item | Recorded identity |
| --- | --- |
| Application | `FM-1_8016`, `v0.1.6-timing`; 525,192 bytes; SHA-256 `c12d8eafce4eb9b0d2690b76a36d82987763c0c67ed648751ea5e192a3e6fbae` |
| Felucca OTA loader | Reused 6,930 bytes; SHA-256 `d71d1b7c12adede951605e758a3580dacccedc427d49e1fceaf51decccc68a4b` |
| Private package | `dabbl8-0.1.6-timing-diagnostic.fwsc`; 610,086 bytes; SHA-256 `648eccca076ad303f5cb093102327dd727ad9d717088530be77b68acf48bb7e5` |

The private folder is `build/timing-diagnostics/resource-v016-5i99i2sc/`.
`diagnostic-manifest.json` has phase `package_validated_not_hardware_qualified`;
its SHA-256 is `5d6debb75ca048a23763f673435d4446c4db0ac40c40abcdb3090d5a1ae6709e`.
Retained `package-check.mjs`/`package-validation.json` use the existing web package
decoder to verify entry CRCs, exact decrypted application, erased padding/map
bounds, the unchanged loader, corrupted-header/payload refusal and ten license
files. `package-build.log`, `SHA256SUMS` and the no-port `capture-plan.json` are
alongside them. Local validation is complete; independent package review is a
separate required gate. No device was accessed or flashed.

The cached SDK receipt records clean tracked source
`d179b4484759423312073f5fbb232501aa491047` and the three expected SPL/config/EQ
blob hashes in the manifest's `inputs`. The original packer CLI reused these
inputs; no application, SDK or loader rebuild occurred during packaging.

Application evidence is retained in
`build/app-only/full/run-g9m73pcr/app-only-manifest.json`. Its original source-input
hashes and commit remain authoritative for that build. A later host-tool or
document commit must not relabel those inputs. Retain the actual package receipt,
corresponding sources and license notices together; SDK SPL/config/EQ components
retain their Apache-2.0 notices, and application/loader sources retain GPL-3.0-only.

`corresponding-source-5328477.tar.gz` preserves the exact tracked application/build
source (12,975,600 bytes; SHA-256
`4d08c0978b30edaf9958022d813fffb7e7b73ab39c811cf9ec78ef3789296f60`).
`corresponding-source-validation.json` verifies all 263 target-manifest source
inputs against archive bytes; `SHA256SUMS` includes the archive. Host-tool fixes
remain separately identified at `045da5c...`. The archive contains tracked source,
not private work, generated builds, Git metadata or vendor restore firmware.

The candidate has eight logical tracks sharing eight voices, Full profile ID 0,
effective engine mask `0x3ffd`, CDC enabled by the existing default, USB audio
disabled, and owner initialization disabled. Both resource helper flags and
colored application startup are enabled; pool collection starts disabled.
The [resource protocol](resource-diagnostic-capture.md) reports observed live-owned
pool occupancy since explicit start and fixed reservation separately. Stack
readout is surviving written depth since boot, a nonatomic lower bound that can
miss pattern collisions/unwritten frames. It does not establish maximum SP,
complete RAM high water or stack safety.

Linked RAM data/BSS is 94,724/98,304 bytes; the static pool is
335,688/344,064 bytes, leaving 8,376 against the existing 8,192-byte reserve.
These are allocation bounds, not workload measurements. The unchanged static
budgets still **FAIL** SLICE (+14%) and the audio IRQ (+48%); TIMER5 passes.
Passing build/link checks and local host tests do not clear these failures.

## Local preparation only

The retained package receipt establishes local identity, checksums and source
correspondence; separate review and owner permission still precede installation.
From the checkout, inspect the existing resource collector plan:

```sh
python tools/dabbl8_resource_capture.py \
  --manifest build/timing-diagnostics/resource-v016-5i99i2sc/diagnostic-manifest.json \
  --version v0.1.6-timing \
  --workload 'future separately authorized idle resource baseline' --seconds 10
```

Plan mode checks the selected package/metadata and opens no device. The manifest
must retain target application source `5328477...` separately from host-tool
source `045da5c...`; version strings and local hashes are observational, not
cryptographic device attestation. A plan does not authorize flashing or capture.

## Future phases, each separately authorized

1. **Installation and read-only boot stability.** Use the existing
   [Felucca USB-MIDI installer](../../tools/fm1_install.py) after verifying the
   exact package, owner-selected MIDI port and recovery readiness. It performs
   the existing running-firmware check, Felucca loader write, restart and identity
   verification; its `--port` matches part of a MIDI name, not a CDC path. Retain
   its result and fresh read-only status before continuing. Stop on any failure,
   identity mismatch, reset or unexpected storage disclosure. Recognized Dabbl8
   storage must be preserved; do not press SAVE to format existing own storage.
   There is no automatic owner reset in this candidate. The earlier v0.1.5
   initialization consent does not authorize a new reset or establish persistence.

2. **One idle resource observation.** After stable boot and explicit owner
   confirmation of idle controls/playback, authorize one bounded capture. Only
   then add `--capture --port /dev/cu.usbmodemEXACT --output NEW_PRIVATE_DIRECTORY`
   to the plan command. The collector requires a fresh banner, matching resource
   capabilities, boot/generation-bound paused CRC frame and stable USB/UAC status.
   Stop after a failed or ambiguous receipt; preserve raw bytes and inspect
   explicitly rather than replaying commands. Zero eligible requests records
   only the initial pool census, not concurrent engine use. Stopping collection
   does not stop music. Stack depth includes work before the capture.

3. **Stopped own-data persistence.** Separately agree the own-project data to
   test and retain its backup before writes. Save/recall three distinct slots,
   verify the shared autosave, then qualify the companion's implemented
   command-81 own-project export/import/restore and an independent reboot/recall.
   Visible SAVE confirmation remains required for restore. The own-project
   archive excludes settings, presets and samples; it is not a full instrument
   backup. Stop on damaged/unknown-store refusal or unexpected formatting; never
   substitute a reset for proving retention. See [own-format limits](native-project-archive.md).

4. **Concurrent workload and timing/caps.** Separately bound tracks 1–8 playback,
   engine/effect combinations and USB/TRS routing, then measure stressed timing
   with clock calibration and matched instrumentation-overhead comparison.
   Include the IRQ/recorder work excluded from current timing fields before
   claiming a full deadline. Use pool observations and stack lower bounds within
   their stated limits, then publish only measured supported combinations/caps.
   Keep the static failures explicit until resolved. Current UAC-off firmware
   cannot qualify USB audio streaming/rates; that needs a separately prepared
   candidate and test authorization.

Release compatibility, interrupted-format/reboot recovery, independent
failed-application recovery, a second tester and public distribution remain open.
The [acceptance map](software-acceptance-status.md) retains those issue gates;
this handoff closes no issue and authorizes none of the future device phases.
