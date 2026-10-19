# Eight-channel qualification follow-through — 16 October 2026

This is a later local supplement to the unchanged frozen v0.1.8 bundle. The
independent frozen-bundle approval does not cover this new native adapter or
establish a hardware-qualified RC. No device was accessed while preparing it.

## Completed local work

- Fixed the staged test's import structure; preserved the original type-check
  failure. The two pure modules and tests pass basedpyright with zero errors,
  warnings or notes. Broad legacy runners/private native-library bindings are
  not claimed type-clean.
- `build/local-checks/run-gutjktqg/results.json`: actual firmware command 27,
  empty arguments, reports a 50-byte payload for all 256 recording masks without
  selecting a track or writing storage. The helper refuses malformed/armed REC.
- Actual renderer: 128 matched note pairs across channels 1–8, 24,822 blocks,
  peak eight shared voices/two heavy states, empty held-note ownership/queue and
  byte-identical canonical project after draining. No all-eight-heavy-slot claim.
- Pure schedule/preflight tests cover wrong routing/track dumps, partial note
  delivery, newly armed recording, health failure, malformed/unbounded schedules
  and individual note-off cleanup. The 37 selector tests pass.
- The separate native adapter's offline checks cover actual Mido message bytes,
  command/packet/timeout refusal, snapshot release/live-state mismatch, happy
  and failing controller paths, and no-note preflight success/uncertain close.
  Controller/parser publication is injected in these adapter tests; existing
  collector protocols retain their separate evidence. No native port was opened.

## Next physical step: read-only preflight only

Wait for fresh owner readiness before any enumeration/open. Ask the owner to
reconnect the FM-1 with the USB 2 cable, set MASTER low, and leave controls alone.
Do not ask them to load B or change settings: loading/editing may later autosave.
Select the exact observed serial endpoint and require unique Dabbl8 MIDI names.
The prepared helper lives under `build/eight-midi-qualification-20261016/` and
defaults to a filesystem-only plan. Its separate `--preflight --port EXACT`
operation checks a fresh console banner, v0.1.8 identity/resource profile,
boot/USB and flash-operation trace stability, stopped playback, unarmed REC,
CH1–8 routing and existing track parameter dumps. It sends no notes, starts no
diagnostic collection, requests no snapshot, configuration, reset or writes.
Native console opening can affect USB/DTR; stop on failure, do not retry.

Preflight evidence uses a fresh private `preflight-run/` directory and retains
raw journal plus success/failure receipt. Existing output refuses another
attempt. Query or close uncertainty is a failure, not proof the endpoint is
healthy. An unexpected current project is a refusal; do not load/restore it to
make the check pass. Track dumps alone do not establish complete project/patch
identity, so a passing preflight does not yet authorize playing notes.

## Conditional later note workload

Only after fresh owner readiness and authorization, the separate `--capture`
mode opens fresh contexts and repeats the guards immediately before notes. It
requires the existing full native-live snapshot SHA-256
`02028593d968011c2890e4e6d1fd69501246168a0072643eff10af24e9f64d56`.
Read-only command-78 snapshots verify all 17 known stored/current roles before
the session and after both stages. They retain private data under ignored,
owner-only output directories; never commit or distribute them. The existing
capture protocol, release acknowledgement, retained bytes/hashes and all roles
must match. A capture is capped at 120 seconds plus bounded cleanup; no retry.

The workload uses existing sounds without edits: notes
60/64/67/36/72/76/79/84 on channels 1–8, velocity 32, sixteen bursts at 0.5-second
intervals, 0.125-second holds, then ten seconds draining. Two 18-second windows
measure timing and resources separately. Resource collection/console-query
overhead is part of that workload; nominal tick conversions are uncalibrated.
Private snapshots are outside those measurement windows.

The fixed wire grammar permits only read-only capability/route/TRACK/dump queries
and those matched note events; snapshot exchange uses the existing command-78
client with its fixed read-only operations. No program/CC/transport/preset/edit,
restore/import/update or raw write command exists in this adapter. Every status
requires the frozen full/UAC-off profile and stable flash-operation trace.
REC is rechecked through the note schedule. Snapshot, timing, ownership, health,
late-delivery or cleanup failure prevents subsequent stages. Potentially sent
note-ons are tracked before transmission and receive bounded individual offs.
No broad panic/reset/automatic restore follows a failure.

Data equality is claimed only after both stages, final full snapshot comparison
and successful port/context closure. First-stage acceptance alone does not
prove final data equality. This measures only the existing demo combination;
heavier engine/FX setup can autosave and remains outside current permission.
Own-data durability, interrupted writes, USB-audio scope, maximum stack/timing
envelope, recovery and independent reproduction remain RC gates.
