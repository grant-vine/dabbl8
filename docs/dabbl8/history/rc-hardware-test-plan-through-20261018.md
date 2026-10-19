> Historical record only. Do not execute its old next-step instructions.
> Current authority: ../release-candidate-status.md and ../local-work-handoff.md.

# RC hardware qualification plan — updated 18 October 2026

Current candidate is installed private `v0.1.9-timing` / `FM-1_8019`, package
SHA-256 `4ad55e9192c565f3285281a63042ac2d29cc0a555084cb03e267c8be998860d5`.
The owner-authorized no-project-change batch is complete. See the current
release-status document and `build/hardware-batch-v019-20261018/` for passes.
This plan grants no fresh device access, saved-data writes or flashing. Retain
Felucca's existing loader/installer flow; no new updater or flash is needed for
the next proposed project-only session.

## Next combined session: concrete preparation

Read `build/rc-durability-v019-20261018/README.md` and `handoff.json`.
The proposed archive preserves A/B and temporarily fills C/shared autosave with
the same heavy project used as current live. The original rollback archive is
preserved separately. Both pass the native archive codec; actual firmware
transfer/boot paths pass the local virtual-NOR sequence. All actual hardware
consent and power handling remain unqualified by those host checks.

After separate owner readiness/temporary-data approval, the planned three
physical interventions are: SAVE for setup, one normal power-cycle after writes
and collectors are confirmed finished, and SAVE for original restoration.
Automate the identity/REC/routing, full-pool workload, quiet export, readback and
closure stages between interventions. Never queue an unattended confirmation:
request SAVE only while the owner is present and the prompt is visible. Do not
press SAVE on an unexpected first-use or storage-error screen.

Require exact proposed logical data after setup and nonempty AS/current equality
after deliberate reboot. Compare persisted unrelated data independently; do not
assume unsaved runtime settings survive power loss. Intentional restart requires
a new boot/counter baseline, not a fabricated same-boot comparison. The final
rollback must reproduce original logical roles, including C/AS absence; raw pool
generations/history may change. A failure stops the batch before another stage
or write; retain backups and request direction rather than attempting a reset.
No physical local-save/recall or power-cut fault qualification is claimed merely
because a restore confirmation was pressed.

## Reuse first

Do not repeat the unchanged baseline, 84 host engine/FX scenarios, 2,240 banked
mixer cases, optimized performance FX, passing own-project restore/reboot,
idle/demo captures or guarded DRUM workload. The frozen bundle records their
identities and scope. New measurements must address an uncovered condition.
Changing a package/source/profile invalidates automatic reuse of device evidence.

## Gates before any future connection

1. Record the purpose, exact candidate, workload, duration, command allowlist,
   expected current project/routing, cleanup and a fresh output directory.
2. Check frozen `SHA256SUMS` and offline package/source validation. Keep the
   current stock-v15 rollback hash and tested recovery procedure available
   privately; do not include the vendor firmware in a distribution.
3. Require fresh owner readiness: connected and powered, volume low, no keys
   held, no edits, recording or playback. Never infer readiness from an old reply.
   Do not ask the owner to load/change a project as part of a no-write preflight.
4. With explicit readiness/operation permission, check one exact serial endpoint
   has no competing owner. Require unique Dabbl8 MIDI endpoints and a fresh
   matching banner/capabilities. A serial open may affect USB/DTR; treat any
   changed boot/USB counter as failure, not an invitation to retry.
5. One status before and after the bounded preflight: expected identity,
   `playing=0`, `flash=1`, no storage failure or voice shedding, stable boot/USB
   counters. For a zero-late workload require `audio_late=0`; do not reset the
   diagnostic counter to hide existing events. If the baseline is nonzero, stop
   and review its origin before choosing any separate delta-based protocol.
6. Read the exact routing and required track parameters through the existing
   read-only editor queries. Verify recording state independently: the current
   console `status` exposes playback but does not expose `song.rec`. A named
   project or a stopped transport alone is insufficient. If the required state
   cannot be established read-only, the workload is blocked; do not fix it with
   configuration writes under the current permission.

## No-write workload policy

Allowed future payloads must be individually reviewed note-on/off events and
fixed diagnostic/read-only queries; no CC/program changes, transport commands,
preset/engine/global edits, restore/import packets, memory writes or update/reset
commands. Collector start/stop changes diagnostic RAM only. Own-data exports, if
needed, are a separate read-only operation using the existing validated protocol;
retain private archives outside Git and never put them in the frozen bundle.

No-write means more than avoiding SAVE. `native_autosave_session.c` polls a
runtime signature and can save changed music after the idle interval when a
session has intent. `project.c` routes native autosave there. Therefore engine,
preset, effects, arrangement, routing and settings changes are **not** safe
temporary setup under the present constraint. A reboot is not a safe way to undo
such changes. Do not disable autosave by changing a persisted preference.

Any new note-only schedule must first pass the actual host renderer with the
exact loaded D8P1 configuration, MIDI routing/recording state, bounded queues,
voice/pool ownership, matching note-offs and byte-identical project before/after.
If it needs a new live adapter, test refusal before first note, partial-send
cleanup, stale identity/configuration, disconnect and timeout offline first.
Existing `drum_capture.py` is hard-coded to its completed DRUM contract; it is
not an eight-channel or general engine/FX driver. Do not rerun it or widen its
allowlist without new local evidence.

## Uncovered measurements and eligibility

| Gate | Future measurement | Current no-write eligibility |
| --- | --- | --- |
| Concurrent MIDI/USB with current sounds | New locally validated note-only schedule plus bounded read-only queries; separate timing and resource windows | Conditional on exact existing project and recording state; no setup edits. Do not substitute the already-passing DRUM schedule. |
| Full heavy-pool / expensive engine+FX combinations | Eight live owned slots and sustained render load, realistic release tails, concurrent transport/UI | Blocked under current permission: existing demo reaches two heavy slots; changing engine/FX setup can autosave. Host eight-slot success is not physical qualification. |
| Timing conversion | Retain raw D8T2 ticks, counter boundaries and independently bounded elapsed-time/clock evidence | Requires an agreed calibration/error-bound procedure. Host command duration and nominal 24 ticks/us alone do not calibrate a clock or ISR coverage. |
| Observer overhead | Matched workload with resource observer paused versus active, stable boot/profile; timing observer overhead stated separately | Only after workload eligibility. Existing separate windows do not measure active resource-observer overhead. A different uninstrumented firmware requires separate build/flash permission. |
| Stack envelope | Observe current colored user/supervisor windows during eligible worst-case load, plus static/call-path review | D8R1 is a surviving-write lower bound, not max SP or overflow proof. No invented numerical headroom acceptance. |
| Mixer/bank gestures and arrangement | Owner performs exact bounded gestures on existing music, including release/stop and eight-track routing | Read-only navigation must be distinguished from edits. Recording/arrangement mutations are excluded now. |
| Own durability | Populate three projects/shared autosave, local save/recall, own upgrade, interrupted commit/format and reboot | Blocked: explicitly requires saved-data and possibly flash/reset/power-cut permission. Keep existing virtual-NOR evidence and tested restore scope. |
| USB audio | Qualify declared rates with UAC enabled | Installed candidate is UAC-off. Resolve release scope first; no implicit enabled rebuild. |
| Independent recovery/install | Another tester repeats supported clean install, own restore and rollback | Separate future authorization and recovery procedure. Stock rollback was tested; unwired RP2040/loader-unavailable recovery is not qualified. |

## Failure and cleanup contract

- Stop scheduling new notes/stages on the first late event, allocation/ownership
  failure, storage error, changed identity/boot/USB state, unexpected packet,
  timeout, host scheduling overrun or uncertain cleanup. Keep raw replies and
  monotonic timestamps. No automatic retry, reconnect, reset or reflash.
- Track a note as potentially held **before** attempting note-on. On abort,
  attempt one matched note-off for each potentially held note on the original
  endpoint if still available. Record failed/uncertain sends; do not claim silence
  from host bookkeeping. No broad panic/reset/CC workaround on another endpoint.
- Stop only collectors this session successfully started. Do not stop/reset
  another session's collector. Close MIDI input/output and serial contexts and
  record closure. A failed stop/close ends all device work and requires direction.
- Obtain final read-only health/project comparison only if the connection and
  cleanup remain healthy. Otherwise preserve evidence and stop; do not perform
  extra probes to manufacture a successful end state.
- Compare original/final logical data if a supported read-only snapshot was
  taken. Treat any persistent-data difference as failure, even if playback sounds
  correct. No automatic restore under no-write permission.
- Ask the owner to stop touching controls and keep power on after a failure.
  Do not request a power cut or unplug during a possible write. Only normal
  completed-test disconnection is a routine teardown.

## Release decision

The frozen package is a private qualification candidate, not RC approval.
Publish no release tag/download or installation claim from host results alone.
Name measured supported combinations, USB-audio scope, known limits, exact
source/artifact hashes and recovery coverage. Independent final package review
and reproduction remain open. Request the FM-1 only with a concrete, locally
qualified eligible workload; no connection is requested merely to repeat a pass.
