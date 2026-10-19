# Retained hardware qualification — 19 October 2026

This is a public summary of completed, separately authorized checks on the
owner's FM-1. It contains no private project payloads, vendor firmware or raw RAM
dumps. No hardware was accessed to prepare this summary. Reuse these results;
do not request another restore/reboot to reconfirm the same software behavior.

The last verified installed image is **v0.2.2-timing / FM-1_8022**:

| Artifact | SHA-256 |
| --- | --- |
| Installed package | `b3e137b9d4e918d32f6933e0bc92139e96e6d347df1570a3cacce8834cc238ff` |
| Application | `9f8194cdc03320a4faa452a31f12f55ba01e8882b2dedb33ab3f705363b46ed8` |
| Incoming loader | `48638bed4882d8b4056be4fcc95e6e143b0b505ed3969eec0911b32ed1b1ae0e` |

The tested v022 working-source snapshot is preserved separately in `34c3b21`.
Exact input hashes/build flags and the corresponding-source archive, rather
than a Git commit or version string alone, identify the installed bytes. Later
Felucca backports and D8C1 clock additions have **not** been tested on hardware.
The frozen v023 package is not the installed image.

## Confirmed scope to reuse

- **Initial controls:** on v015, the owner confirmed the normal synth interface,
  sound through the built-in speaker, all four parameter knobs and individual
  selection/sound on tracks5–8. Headphones and complete physical TRS routing
  were not established by those checks.
- **Connected own-project transfer:** v018 and subsequent diagnostics completed
  owner-confirmed restores and companion logical readback. In the final v022
  session, fresh original backup, temporary setup, upgrade/cold readbacks and
  original restoration completed. Final COMMIT stage7, rc0, guards0, revoke0;
  connections closed. This qualifies those project archives, not an archive of
  settings, user presets or samples.
- **Stopped local save/recall on v020:** save to C completed; the owner loaded A
  as ANALOG and C as WHEEL. Exact readback showed only the expected selected-slot
  global/CRC changes; A/B/autosave remained unchanged. The initial overly strict
  host equality alarm was explained by PROJECT-page slot selection, not repaired
  by changing device data or deleting a failure record.
- **Three slots/shared autosave across reboot on v020:** A/B/C and autosave were
  byte-exact after a confirmed cold restart; the live project matched autosave.
  It was not required to equal saved C: saved C and shared autosave are distinct
  legitimate states. The original five logical roles were restored exactly.
- **Own-project upgrade/cold reopen on v022:** all five project roles matched
  exactly after upgrade and after a separately authorized, confirmed cold
  restart. The first cold guard refusal is retained. It was not silently retried
  or treated as a successful fresh boot.
- **Bounded full-pool workload on v022:** two18-second MIDI timing/resource
  passes, 6,300 recorded audio blocks, zero late publications/taint/allocation
  failures, peak eight owned heavy-state slots. All17 captured data roles were
  unchanged before/after. This is neither an all-preset survey nor a substitute
  for independent listening, clock/ISR calibration or full stack bounds.
- **Stack observations:** 5,472 user and1,188 supervisor bytes had surviving
  writes. These are lower bounds on observed written depth, not maximum-SP or
  complete interrupt/fault stack usage.
- **Stock rollback:** the historical Mac restore reached original v15 with
  owner-confirmed sound/controls and exact verified readback, preserving boot
  and non-application bytes. Its entry relied on a working console. Do not
  reinterpret it as unresponsive-application or RP2040 recovery.

Private authoritative receipts remain in the original source checkout:
`build/persistence-v020-20261018/completed-results.json`, its four named comparison
receipts, `build/hardware-v022-20261019/completed-results.json`, warm/cold readback
receipts and `final-commit-result.json`. Recovery evidence is described in
[the retained qualification audit](qualification-evidence-20261019.md).
Raw data stays private; hashes of the reviewed receipts are retained in the local
requirements-audit directory.

## Still unqualified

Do not close an entire issue from a passing substep. Full physical USB/TRS routing,
all applicable UI gestures, interrupted first-use/power-cut durability, own
settings/preset/sample upgrade durability, exact-candidate calibrated timing,
complete stack/ABI/nesting, independent recovery and a second tester remain
separate gates. USB audio is disabled in these candidates; neither USB audio
sample rate is qualified. No public download or release is approved.

Use [current RC status](release-candidate-status.md),
[clock measurement](clock-measurement.md) and
[independent recovery preparation](independent-recovery-preparation.md) for the
next work. No device action or restore prompt is armed by this document.
