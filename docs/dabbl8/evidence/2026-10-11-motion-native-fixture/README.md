<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Native motion fixture qualification

The original `motion_test` used live `NTRK` to fill historical four-track
FUN6/FUN9 structures. With eight tracks, the FUN6 loop overran its stack object;
the unsanitized fixture also expected quarantined native saves to succeed.

This test-only repair uses `PROJ_LEGACY_NTRK` for historical layouts. The original
four-track cases and assertions remain. Native cases assert the real
`MIGRATION REQUIRED` save refusal and unchanged music, transport, motion, effects,
project mirrors and workspace before independently constructing and validating
historical inputs for subsequent read/playback checks. These inputs represent
only the historical first four tracks; they are not native eight-track saves.

The actual eight-track lifecycle checks recording, overwrite isolation, locks,
playback, base restoration, bypass and clearing, together with literal D8M1 track
addresses/wire fields, decoder validation and unchanged outputs on refusal.
The runner executes both track counts with all FM4/SLICE combinations, normally
and with the inherited DSP sanitizer exclusions.

Fresh measured-source results: all **16 configurations pass** (123 assertions per
four-track run and 161 per eight-track run). All **eight original four-track
controls pass with byte-identical output**. Address-alias, paired codec-alias and
refused-save mutation controls fail in both normal and sanitizer builds. The
re-expanded historical loop aborts under ASan; its unsanitized run passes despite
the overflow. That non-detection and the original failures remain in the sealed
local evidence, alongside initial fixture setup failures.

`qualification.json` binds the exact source and generated headers. Execution used
parent `a01ac4bb990f20e14246230c9f869e680235f529`; delivery uses
`406de63335eb55248db21005efd17261b167b03a`, whose only intervening changes are
three planning documents. Retained qualification inputs are identical, so no new
execution is claimed for that documentation-only parent change.

This is host fixture qualification. The whole test suite/CI, vendor builds,
physical storage, audio deadlines and device behavior were not qualified by this
change. Native write guards, firmware sources, historical production layouts,
loader/flash boundaries, golden hashes and timing budgets are unchanged.
