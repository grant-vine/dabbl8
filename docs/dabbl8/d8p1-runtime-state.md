# Stopped native project state

The eight-track build now links an in-memory D8P1 loading/capture API and a separate 772-byte arrangement cache. This is a software implementation, not a device upload/backup feature or a hardware-qualified release. The four-track build retains its existing behavior. No editor command, format capability, flash object or loader boundary changes.

## Main-loop contract

Call with interrupts enabled, outside drawing, with transport stopped and no armed/running chain, count-in or pending transport request.

- `d8p1_load_runtime(raw, length, available_projects)` returns 0 for success, 1 for invalid/unsupported data or aliasing, 2 for busy. Input is immutable until copied. It may be external caller-owned bytes or exactly the previously borrowed staging wire buffer. The project bits 0–3 must represent verified supplied context; the function never fetches referenced projects or starts scene playback.
- `d8p1_capture_runtime(out, capacity, written)` returns the same codes. Output and length storage are caller-owned; output may also be exactly the borrowed staging wire. A refused capture preserves output bytes and length. A nonempty historical slot chain refuses rather than silently losing its meaning; convert its original/context through the existing offline workflow first.
- `d8p1_runtime_arrangement()` returns a const main-loop view when a native project is loaded. The separate cache survives drawing and project capture. The next successful historical load invalidates it; a successful native load replaces it. Do not retain the view across loads or access it from an interrupt.

Loading validates known writable format, CRC/fields and reference context before borrowing the arena. It copies into the disjoint bounded wire and decodes into staged state. The final stopped check occurs under the publish guard before transport, panic, music or cache mutation. A late start request survives refusal. Staging can change on late refusal; active music and retained cache do not.

Native adoption keeps the guard through legacy default-sound handling. Nested engine changes use the existing state-preserving guard helper. Capture takes one coherent stopped snapshot under a guard, then serializes the staged snapshot. Encoding/refusal never authorizes persistent writes. These software guard paths and host control flow are tested; physical ICFG behavior, DMA, stack high-water and interrupt latency remain unqualified.

## Existing musical policy

The actual upstream restoration policy remains explicit: globals and engine parameters fit their descriptors, preset indices fit the engine, FM6 patches unpack/sanitize/adopt as owned patches, retired DIGITAL becomes FM6 when FM4 is disabled, and SAMPLE PERC becomes DRUM. Incompatible engine-specific automation is removed by those migrations. Legacy power-on/default-keep preset markers retain their original semantics, including sequence/automation resets where specified. A captured file contains the resulting live values, not a promise of byte identity with an unnormalized input. Preserve original files and the offline migration report.

All eight track owners, 64 steps per track, four-note chords, full drum masks, probability, ratchets, signed motion/locks and independent track-4/track-8 addresses use the real runtime structures. The audio event loop consumes the all-track panic and the host mixer adopts requested engines within the existing eight shared sounding-voice budget. Banks, scenes and rows are retained/captured as metadata; arrangement playback remains future work.

## Evidence and remaining gates

[Runtime evidence](evidence/2026-10-10-d8p1-runtime/README.md) records the actual linked target, memory report, full source-stable regression suite, browser/native parity and failures/skips. The native state/wire arena is still 59,520 bytes; the persistent cache adds 772 pool bytes. No project-sized stack object is introduced.

Device transfer, verified device backups, binding referenced-project data, the approved storage map, multi-sector A/B writes, every-boundary power cuts, physical recovery, high-water/deadline measurements and arrangement playback remain unfinished. Issues #11–#13 and their prerequisites remain open. No flashing, releases, merging or deployment is implied.
