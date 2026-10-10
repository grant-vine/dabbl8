# Companion client evidence — 10 October 2026

Issue #11 client preparation based on protocol commit
`b418052730d883d968ac43c740ad703876a665d7`. Typed JavaScript client and a
test-only C line bridge; no firmware, loader, map, fixture or golden change.

The full pinned upstream/extended suite passed (exit 0). Both optimized and
ASan/UBSan C-bridge sections passed 144 client checks. The actual four-track
handler remains read-only in this companion; the actual expanded handler
round-trips all eight selections, isolated parameters, complete dumps, mixer
and full last-step/drum/chance/ratchet data. Real busy errors are recoverable.
Mocks cover malformed capabilities and acknowledgments, counts five through
seven, closed/stale clients, queued writes and fresh reconnects. Whole replies
are validated before the next queued operation can transmit. Source bounds are
immutable; local invalid inputs send nothing. Every real-handler reply checks
zero simulated flash writes/erases. This is host C, not physical hardware.

Full log is retained outside Git at
`.local-baseline/companion-client-check/final-upstream-suite.log`; exact SHA-256,
source hashes, both integrated check sections and wrapper command are here.
Node is the established 26.11.0 pin. Default Mac Oct10 app/package/loader and
all three reference hashes equal preceding results. Target image was not
rebuilt locally for this JS/test-only stage; new hosted draft CI is pending.

Four local omissions remain explicit (DaisySP, stock CLI restore fixture kept
outside Git, embedded-editor MENU, Emscripten); hardware SKIP remains. No real
MIDI permission, backup restore or installation occurred. No target memory
change is asserted. Physical port lifecycle and the visible companion page
remain to be implemented, together with new-format saves/backups and #7/#9
and hardware gates. Keep issue #11 open. See
[client scope and contracts](../../companion-track-client.md).
