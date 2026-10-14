# Dabbl8 development plan

An open-source eight-track firmware project for the M-VAVE FM-1, based on Felucca. An eight-track development prototype exists; a hardware-qualified release remains pending.

- Firmware and releases: [grant-vine/dabbl8](https://github.com/grant-vine/dabbl8).
- Public information: [dabbl.co.za](https://dabbl.co.za), maintained in grant-vine/dabbl.
- Implementation plan: [docs/dabbl8](docs/dabbl8/README.md).
- Work queue: [roadmap issue #2](https://github.com/grant-vine/dabbl8/issues/2), [implementation issues](https://github.com/grant-vine/dabbl8/issues) and [issue index](docs/dabbl8/issue-index.md).

The audit baseline is Felucca v1.1.5, commit `276f72a4e6ea8a12499a7a6819aadf3165126755`. Fork main was observed at `ac6bfcbd76ee3de79eb461719e8d123b5512aa3c` with a v1.1.5.1 README. Review that delta deliberately in the first build issue; do not call a new baseline tested simply because it is newer.

Keep upstream credits, LICENSE and LICENSING.md. Baseline and development builds have retained local evidence. The owner's FM-1 installed and initialized the private v0.1.5 diagnostic on 14 October 2026, with speaker sound, all four knobs and individual keyboard sound on tracks 5–8 confirmed. See [source-specific hardware evidence](docs/dabbl8/hardware-initialization-20261014.md); release qualification remains pending. The inherited README describes upstream Felucca, not achieved Dabbl8 capabilities.

## Current install policy — 14 October 2026

The required first-install behavior is to reset previous firmware projects, autosave, settings,
user presets and user samples within the approved user-state map. Installation
guidance must disclose this before flashing. First-use formatting is part of
initial boot; it is not claimed complete merely because the application was
flashed. Loader, OTA, calibration and physical boundaries remain unchanged.

The disclosed first-use reset completed on the owner's FM-1 with approved-sector
readback and native readiness recorded by the firmware. Interrupted-format
recovery, saved-project/reboot retention and own-project backup/restore remain
unqualified. See the [software acceptance map](docs/dabbl8/software-acceptance-status.md)
for the remaining implementation and measurement gates.

Dabbl8 manages its own project versions and firmware upgrades. Recognized Dabbl8
data must survive ordinary upgrades and reboot; unsupported or damaged Dabbl8
formats require an explicit recovery/reset decision, not automatic destruction.
Migration from vendor, Felucca or other firmware is not a release requirement.
Existing legacy-conversion work remains available as historical developer tooling.

See AGENTS.md for local-only verification and the authoritative GitHub roadmap for
current acceptance gates. Do not repeat completed baseline/layout work or resume
external-data migration work from older planning snapshots.
