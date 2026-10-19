<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Arrangement observation — diagnostic preparation

The existing console's `playing` field cannot show whether the native arrangement
prepared correctly or whether all rows ran. Private timing builds now include
one coherent D8A1 line in `status`; no new command or remote playback control is
added. Normal builds omit it. The FM-1 has not been accessed for this change.

The main loop copies19 words under the existing IRQ save/restore guard, then
restores the prior configuration before CRC or output. The snapshot reads timer,
boot/USB epoch, native generation/mode, transport and row/scene shape, pending
fill/row controls, recording, pending transport and count-in. It does not mutate
music, diagnostics, timer registers or storage and adds no work inside the audio
or scan interrupt. There is no new static allocation. The short main-loop guard
still needs its target integration considered in timing qualification.

The exact line is `d8a1` followed by19 uppercase eight-digit hexadecimal words
and CRC32, separated by single spaces. Word order is:

```text
ticks boots usb_resets generation mode playing armed running row scene rows
valid suspended pending fill_pending fill_active recording request count_in
```

CRC32 covers little-endian magic `0x44384131` and those19 words. Native mode0/2
does not expose inactive union contents: generation/rows/valid/suspended/fill
active are zero; row/scene/pending/fill-pending are255. Global transport fields
retain their actual values. A stalled output returns without CRC closure.

`tools/dabbl8_arrangement_state.py` decodes exactly one frame in a bounded ASCII
response and rejects CRC, shape, duplicate-frame and inactive-field violations.
The immutable observation model is deliberately specific to the synthetic C
proposal:16 rows, row equals scene, no fill/row override, no recording/count-in.
It requires stopped readiness, row0 first, every row in order, a stable running
generation and the matching next-generation final Stop at row15 with suspension.
Repeated samples within a row are allowed; skipped rows are never inferred.

The owner PLAY window is bounded at60 host seconds. Samples must advance the
host monotonic clock with no gap above one second; playback must stop within36
host seconds after the first active observation. These are coverage/refusal
bounds for the known120-BPM proposal, not oscillator calibration or a deadline
certificate. Epoch/control changes, unknown cleanup and timeout fail the run.
A completed observation is one-shot. The module imports no transport and neither
requests owner input nor opens a port.

Actual native transition/formatter cases pass with the diagnostic enabled and
disabled in optimized and fatal ASan/UBSan modes. Host tests check C-frame
interoperability, guard/printing separation, immutable music and malformed/stale/
missed-row/early-Stop/generation/timeout refusals. Existing DSP overflow/shift
exclusions are retained in the native fixture. It stubs CDC/guard/time and uses
a standard CRC formatter stub; the unchanged actual CRC utility is covered by
the earlier console interoperability evidence. This is not full hardware runtime.

The linked clock-and-arrangement application at
`build/app-only/full/run-9n195ske/` passes target link/MMIO/static budgets:
527,572 bytes, RAM94,812/98,304 and pool335,696/344,064. Conditional software
stack contributions remain startup/main7,516, audio2,196 and TIMER5 200 bytes.
Hardware SP/SSP/implicit-frame/nesting limits remain unqualified.

The bounded timing/resource collector and post-Stop C-context comparison now
pass local checks. Consolidate and review the new package and kit before any
device session. The old frozen v023 contains neither this frame nor D8C1; do not
substitute it. Require fresh package/profile/epoch and original backup/readiness checks;
normal status text or CRC is not binary attestation. The owner still prepares
sources through the verified C-load/SEQ-hold/SONG/PLAY path. Never stage a
stopped-only snapshot while playback runs or widen the notes-only MIDI guard.

## Collector and readback contract

`tools/dabbl8_arrangement_capture.py` defaults to a plan that opens no port. It
checks the existing package/profile validator, exact D8A1 source-header hash and
the locally rendered 7,385-byte C proposal before native acquisition. Explicit
capture also requires current owner readiness, an explicit USB console path and
a new private output directory. Manifest metadata identifies intended source;
status/version/CRC do not attest the installed binary independently.

Its seven fixed commands are status, timing start/stop/report and resource
start/stop/report. No playback, memory, storage, reset, MIDI or flashing command
is admitted. It acknowledges each observer start, observes every row, waits for
the natural Stop and the proposal's bounded12-second tail, then pauses only
acknowledged owned observers once. Lost/malformed start or stop acknowledgements
remain uncertain; they never trigger retries. Primary, cleanup and close errors
remain separate in the receipt. Any error refuses advancement to readback.
Cancelling the host does not stop music remotely; if playback was active, report
that state and obtain owner direction rather than silently retrying or restoring.

Both reports must pass their existing strict decoders. The resource window must
show all eight heavy slots and the pinned six-FM6/DRUM/WHEEL peak shape without
allocation failure. D8T2 must cover at least11,000 halves with no late/taint/aux
observations. That lower bound checks coverage of this known arrangement;
neither it nor host polling proves a physical deadline or calibrated clock.
The collector's tail wait does not establish silence: the existing firmware
quiet guard still admits or refuses the later stopped-only full snapshot.

`tools/dabbl8_arrangement_readback.py` is an offline library. It verifies two
already accepted, released17-role snapshots and uses the actual native archive
codec to recover logical A/B/C/shared autosave from their five mapped pool
blocks. Physical block relocation is permitted by the codec; raw block number
is not a project identity. No extra device archive export is needed.

The initial live/autosave must match the MIDI proposal; C must match the rendered
arrangement. Final live must match C; final autosave may retain the MIDI proposal
or match C, and the result records which occurred. A/B/C and all11 ancillary
raw/current settings/preset/sample roles must remain byte-identical. Uncertain
release, corrupt metadata/bytes, unknown live/autosave or changed protected data
refuse. The helper does not restore originals or request a device retry.

Local evidence is retained in `build/rc-arrangement-capture-20261019/`:14 actual
console-framing/failure/CLI cases,10 actual native-codec readback cases (fatal
ASan/UBSan),23 existing console cases and45 selector cases. Early fixture failures
are preserved separately. New tools pass strict typing and focused lint; two
pre-existing selector-file lint findings are outside that claim. Existing
render/target/physical evidence is reused, not repeated. No device was accessed.
