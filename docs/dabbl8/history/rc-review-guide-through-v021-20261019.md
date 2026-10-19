> Historical v020/v021 review record. Current source/package/device state is in [the current guide](../rc-review-guide.md). Do not execute its old installation sequence.

# Dabbl8 RC review guide — installed v020 and private v021 candidate

This is a review guide, not a release announcement or permission to install.
The evaluated image is v0.2.0-timing / FM-1_8020. A new public RC version/tag has
not been selected. Keep the tested binary identity; do not rename its embedded
version or infer qualification from a filename.

Latest local successor: **v0.2.1-timing / FM-1_8021**, not installed, under
`build/rc-v021-20261019/`. It adds only the owner-authorized native-pool cleanup
exclusion in the incoming Felucca loader; application/staging/calibration/flash
limits stay unchanged. Its new loader and package are separately hashed and
validated; do not apply the v020 kit's unchanged-loader assertion to v021.
Source/test review passes; physical upgrade and target performance qualification
remain outstanding. The v020 results below retain their installed-version scope.

## Proposed release scope

- FM-1 hardware only: eight sequencer tracks sharing eight sounding voices.
- The evaluated FULL profile retains13 engines (mask0x3ffd); the historical
  DIGITAL/FM4 slot is unavailable. USB audio is disabled; MIDI and diagnostics
  are separate from USB audio. Do not advertise arbitrary engine-card builds.
- Synthesis, effects and the loader originate in Felucca and its credited
  dependencies. Dabbl8 adds the coordinated eight-track workflow, own project
  format/storage, own backup/restore, branding and development diagnostics.
  Inherited features are not newly completed Dabbl8 development.
- One clean installation resets previous firmware's user projects, shared
  autosave, settings, user presets and samples. Sample-slot capacity remains;
  previous sample contents do not. No cross-firmware migration is promised.
- Recognized Dabbl8 data must survive ordinary reboot/upgrade. Unknown or damaged
  own formats must fail safely rather than silently initializing empty data.
  Own save/upgrade durability remains a qualification gate, not an assumed pass.

## Established results and limits

The current diagnostic installed through Felucca's flow, reached a steady normal
interface and passed its60-second stopped boot check. Its original-project restore
was acknowledged and all logical roles were read back by the companion. The
retained trace reported COMMIT complete with zero error/guards/revocation. This
single successful attempt does not explain the preceding v0.1.9 handover failures.

The additional whole-allocation snapshot timed out in its second scan. It remains
refused, with no subsequent device retry. Local host fixes preserve the primary
error, request/scan/timing context and separate cleanup status; they do not prove
that the physical missing-reply cause is fixed. Reuse the successful project
restore instead of demanding another whole-device snapshot for that same result.

The84 engine/effects host cases pass for unchanged effective compiled inputs.
They are not proof of all presets, target deadlines or worst-case stack headroom.
Older eight-heavy-slot and timing measurements apply to their recorded binaries.
RAM has3,564 bytes free; the heavy pool has8,376 free, only184 above reserve.
Watermarks measure surviving writes, not maximum stack-pointer excursion.

## Reviewing the local kit without hardware

The private review kit is assembled under `build/rc-review-kit-20261018/`.
Its index identifies the original frozen package and a later host-only addendum.
The original manifest/README describe the earlier freeze date; current facts are
in this guide, the current status document and the review report. Do not rewrite
historical manifests to change their original authorization or qualification fields.

1. Inspect the kit index and verify its SHA256SUMS before using its files.
2. Inspect `firmware/manifest.json`, `environment.json`, `source-files.json`, the
   corresponding-source archive, app-only build manifest and reused QA receipts.
3. Reuse `firmware/package-validation-3.json` when its validator, decoded inputs
   and package hashes match. The existing `verify-package.mjs` is offline-only
   and can check package identity, entry CRCs, loader, padding and corruption
   refusal when an input changes; it does not open MIDI or install firmware.
4. Read the host-addendum manifest and tests separately. Host changes are not
   embedded in the installed application and require no firmware rebuild/flash.
5. Keep GPL-3.0-only, component notices and corresponding source together. Vendor
   V15 firmware and private projects must never enter this kit or a public release.

## Build provenance and reproduction boundary

The exact firmware input hashes, compiler flags and generated headers are frozen.
Source base33c3b62b40ba1708ccfb6bfb14fb0191bf55bcea has working-tree changes;
those archived bytes, not the clean commit alone, identify this build.

Recorded application command (do not run merely to repeat a passing build):

```sh
python tools/build.py --dabbl8 0.2.0 --app-only --engine-profile full \
  --timing-diagnostic --resource-diagnostic
```

Use the isolated build dependencies recorded in environment.json, JieLi clang
4.0.1, linux/amd64 and the recorded Debian digest. Set JIELI_TOOLCHAIN to the
reviewed local extraction, JIELI_DOCKER_IMAGE to the pinned digest and FELUCCA_UAC=0.
The toolchain archive hash and SDK commit/input hashes are recorded. App-only
builds do not need the SDK; package assembly uses the three unmodified SDK files
and the already verified unchanged Felucca loader. `assemble.py` records the
original local assembly sequence and its machine-specific paths; it is not a
portable one-command release builder. Do not execute it against an existing freeze.
Compiler date strings affect output: source equivalence is not a demonstrated
independent bit-for-bit reproducible build. Retain this limitation explicitly.

The optional native Mac MIDI runtime requires the reviewed patched arm64 Python
3.9 extension. The Python guard refuses other bytes/ABIs. The patch and lifecycle
instructions accompany the host addendum; reproducing the patch alone does not
qualify a new extension. Do not download dependencies or weaken the guard to make
it run. The browser companion follows its separately tested browser MIDI path.

## Installation and recovery boundary

Felucca's established package/loader flow remains the supported evaluation route.
There is no new recovery protocol to test as part of a normal update. Do not flash
while reviewing locally. Any future install needs explicit owner authorization,
verified package identity, connected stable power/data cable and tested rollback
preparation. A diagnostic is not yet a trusted public firmware recommendation.

Stock-v15 USB restoration previously reached the original interface with working
sound/controls. That establishes the exercised loader-available recovery scope;
it does not qualify every USB failure or RP2040 recovery. Preserve the vendor
file privately with its recorded hash; never redistribute it in the project kit.
Owning an RP2040 is not evidence that its wiring/recovery procedure has passed.
No deliberate interruption or destructive test is authorized by this guide.

For remaining gates and the next consolidated physical session, use
[the current status](../release-candidate-status.md) and
[hardware plan](../rc-hardware-test-plan.md). There is no pending SAVE/restore/flash.
