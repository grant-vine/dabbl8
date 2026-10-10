# Versioned track editing evidence — 10 October 2026

Preparatory issue #11 change, based on backup-preflight commit
`b7c3140c27e262bcca39d9d2ba698c208c91b4e7`. Schema-1 command 77 permits only
selection, mixer, parameter, dump and step operations in expanded runtime;
legacy writes and all project/sample/preset/backup forwarding remain refused.

The actual USB parser/editor handler at eight tracks passed 186 assertions in
optimized and ASan/UBSan builds. Coverage includes all eight destinations,
other-track isolation, full parameter dumps, step 64 (index 63), drum masks,
chance/ratchet, all 123 non-allowlisted opcodes, malformed schema/length/index,
busy song step/timing refusal, live mix while busy and denied legacy writes.
No simulated flash erase or write occurred. Raw test and target-check logs are
committed; command, source/log hashes and exact artifacts are in `results.json`.

Pinned compiler/SDK/linux-amd64-container builds passed for default four tracks
and the expanded target. Default Mac Oct10 image/package/loader hashes exactly
equal preceding mixer/offline-preflight results. Expanded image: 445,104 bytes,
SHA-256 `9b2b3c4446eb52cee3f02484eacb7c548ba1868f8ea9e0082d9eb6913901a05d`,
120 bytes larger than the mixer target. General RAM 95,732/98,304 B (2,572 free),
engine pool 333,328/344,064 B (10,736 free); unchanged 8,192 B pool reserve passes.
No new global state array, loader or flash-map change. No expanded package.

The complete local upstream/extended suite is still running under worker
47666 at this evidence snapshot. Do not claim its terminal result from targeted
checks. The standard suite now runs the new assertions in both modes. Actual
hardware, stack high-water/IRQ ABI, ISR timing and recovery remain unqualified.

See [wire protocol](../../versioned-track-editing.md). The browser companion
still needs capability-driven arrays and controls using this operation; the
existing four-track editor remains read-only on expanded firmware. Project and
backup new-format writes, storage/runtime gates and hardware evidence remain
incomplete. Keep issue #11 open; do not merge or release this preparation as a
qualified eight-track firmware.
