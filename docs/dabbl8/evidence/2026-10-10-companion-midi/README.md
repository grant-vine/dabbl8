# Companion MIDI transport evidence — 10 October 2026

Preparatory issue #11 stage based on client commit
`ee12f229fb2fbbb5158fca9198fa9777ba51c8a2`. Both optimized and
ASan/UBSan host C bridges passed 85 transport checks. Complete raw check logs,
source/bridge/log hashes and the pinned full-suite wrapper are preserved here.
The bridges are copies of the preceding stage's compiled real parser/handlers;
no C source changed. The standard suite recompiles them in both modes and now
includes the same transport checks. Node remains pinned to 26.11.0.

The full suite was running at this evidence snapshot; its final result must be
recorded separately after completion. Hosted CI will run on draft creation.
Do not interpret targeted success as a completed full-suite or physical result.

Checks exercise framing and matching, all refusals, negative signed parameters,
malformed/mismatched replies, notes/alien SysEx/WATCH traffic, no retry after
missing acknowledgment, queued-write cancellation, both port disconnects,
external closure, failed send/open/close, abort before/during open/handshake,
listener preservation, ownership and fresh negotiation. Actual C handlers
verify eight-track selection and independent mix destinations, signed parameter
round trips, and four-track read-only refusal. Every real-handler reply checks
zero simulated flash writes/erases.

No browser MIDI permission was requested; no physical ports, flash, backup or
firmware installation were used. Native port promises and delayed wire replies
have explicit limitations documented in the transport contract. The visible
page, physical qualification, new-format project/backup support and #7/#9 gates
remain. Issue #11 stays open. No firmware/loader/map/reference change or new RAM
claim is made. See [transport contracts](../../companion-midi-transport.md).
