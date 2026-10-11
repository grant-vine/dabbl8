# Connected migration and native session prototype

This is an experimental software workflow. Shipping eight-track firmware keeps
instrument writes quarantined. The end-to-end fixture uses a private virtual NOR
adapter compiled only into its host test; there is no firmware permission setter
or shipping flash writer behind this prototype. Hardware, storage timing, recovery
and audible-deadline qualification remain required before production activation.

The dedicated simulation tool connects the ordinary full 17-role collector to
exclusive archive publication and readback, a second independently validated
originals copy, explicit source selection, actual source-derived conversion, live
firmware capture binding, canonical executor validation, physical SAVE consent,
bounded execution, flash reread, RAM adoption, catalog and shared autosave session.
The ordinary collector still ends its capture on completion or failure. Only its
private, successful connected-workflow callback consumes the live capture before
that cleanup. Archive JSON and host retention claims cannot grant firmware write
permission; supplied helper or bridge identities remain unqualified.

## Source and reboot policies

APPLY chooses PERSISTED or CURRENT explicitly. The actual firmware importer
compares the selected converted legacy sources and selected autosave byte-for-byte
with the complete proposal. All original raw roles and current logical roles are
retained; the historical fourth project is archival only, never aliased or remapped.
Unselected generations and fourth-slot references remain conservatively checked.

RESUME deliberately adopts the existing persisted native autosave. It requires a
new full post-boot capture and explicit SAVE consent, validates actual rotating
shared-spare inventory and canonical selected records, and performs no erase or
program. Sparse inventories cannot infer absence from damaged residue. All
session permission disappears on reboot; no settings or loader marker is written.
A post-boot native capture is not a substitute for the earlier original Felucca
archive or vendor recovery firmware. Native recognition alone does not authorize
writing or silently adopt RAM.

## Lifetimes and serialization

An ordered proposal chunk or successful bounded verification step refreshes a
15-second idle lease. Status, malformed, unsupported, replayed and empty requests
do not refresh it. The 120-second hard cap starts at live handoff; consent cannot
extend it. USB configuration ABA, reset, source observer changes, pending musical
input, transport and local edits revoke the transaction. The actual editor,
main-loop UI, settings export, master polling and audio event dispatcher exclude
mutations while the arena is held. Playback is never stopped to admit a request.
All shared voices, resident effects and logical DAC/USB queues must already be
quiet; these counters do not establish physical inaudibility or timing.

The private simulation driver consumes a precise generation, operation, fixed
address, pointer and ordered callback ticket in a single existing IRQ interval.
It monotonically observes each own mutation without clearing the sticky capture
observer or suppressing unrelated changes. Native session saves select an actual
noncurrent shared spare and consume ordered sector/body/header tickets. Historical
eight-track project/autosave, settings, preset and sample writers remain closed.
The immutable completed-session identity is separate from permission for one
native save. A new note or PLAY request before the first mutation cancels that
operation, returns BUSY, and retains music, NOR, pending input and completed
identity. A subsequent save must independently pass a fresh quiet preflight and
obtain a new ordered ticket; cancellation observed during preflight cannot be
cleared at its end. The completed identity is independent of the obsolete
proposal-transfer deadline. READY accepts STATUS and explicit CANCEL only; old
STEP, RECEIVE or unknown operations refuse without modifying it. Explicit CANCEL
and whole-session failures also clear frontend/catalog/autosave readiness. USB,
observer, source and hardware changes revoke the whole session. After any mutation attempt, cancellation or uncertain readback latches
the frontend offline. No old operation ticket is restored.

A failed destructive transaction latches the native frontend offline; no legacy
fallback or false SAVED result is allowed.

Before releasing ownership, completed output is reread in bounded 256-byte steps
and bound to small private CRC/inventory metadata. That metadata survives controlled
owner release while edit exclusion remains active. The ordinary runtime loader
rereads actual flash and verifies selected inventory before adoption. No staged
pointer crosses release and no second project state or 40 KiB copy is allocated.
Observer serialization covers actual application writers, not arbitrary unobserved
external NOR mutation or hostile mutate-and-revert callbacks.

## Private command 79, version 1

All payload bytes are seven-bit. Unsigned 32-bit fields use five little-endian
seven-bit bytes with the highest byte at most 15. The existing INFO/CAPS layout
is unchanged; this command is not advertised as a production restore capability.
Four-track firmware retains its upstream unknown-command behavior; the strict
host rejects its response as unsupported.

| Operation | Exact request after command 79 |
| --- | --- |
| BEGIN 1 | version, op, capture token u32, mode (APPLY=0/RESUME=1), source (PERSISTED=0/CURRENT=1), host-retention claim=1 |
| RECEIVE 2 | version, op, generation u32, request sequence u32, ordered offset u32, count u14, canonical packed bytes (1–256) |
| STEP 3 | version, op, generation u32, next request sequence u32 |
| CANCEL 4 | version, op, generation u32, next request sequence u32 |
| STATUS 5 | version, op, generation u32, next request sequence u32 |

The response is version, status, phase, generation u32, accepted request sequence
u32, received extent u32 and observed own-operation count u32. BEGIN acknowledges
sequence zero; subsequent accepted requests are strictly increasing. Public frames
never supply flash addresses, callbacks, grant flags, IRQ permissions or receipt
metadata. Consent is a fresh calibrated physical SAVE edge followed by release;
the simulation bridge invokes that actual panel path, not a consent Boolean.

Final source pins, paired test results, reachable target memory/stack and complete
upstream verification must be recorded before this prototype is considered for a
reviewable firmware PR. No release, installation or device-writing instructions
are supplied here.
