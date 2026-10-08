# Implementation backlog

GitHub Issues creation is currently blocked because Issues is disabled on this fork. Enable Settings > General > Features > Issues. No issue numbers or native dependency relationships have been created yet. This index and backlog.json preserve the complete issue bodies and dependency graph for loading.

| ID | Phase | Task | Blocked by |
|---|---|---|---|
| D8-001 | M0 | Reproduce pinned upstream build | None |
| D8-002 | M1 | Freeze historical project layouts | D8-001 |
| D8-003 | M1 | Version automation addresses | D8-002 |
| D8-004 | M1 | Define capability and protocol handshake | D8-003 |
| D8-005 | M1 | Model D8P1 maximum storage | D8-002 |
| D8-006 | M2 | Specify bounded voice allocation | D8-001 |
| D8-007 | M2 | Enable eight track sequencing and MIDI | D8-003, D8-004, D8-006, D8-005 |
| D8-008 | M2 | Build eight track mixer and selection | D8-007 |
| D8-009 | M2 | Update editor and backup conversion | D8-004, D8-005, D8-007 |
| D8-010 | M3 | Measure and bound engine state | D8-007 |
| D8-011 | M3 | Implement atomic multi-sector saves | D8-005, D8-010 |
| D8-012 | M3 | Prepare recovery and first target alpha | D8-009, D8-011, D8-008 |
| D8-013 | M4 | Add bounded performance arrangement | D8-012 |
| D8-014 | M4 | Evaluate weighted FM voice budget | D8-012 |
| D8-015 | M4 | Audit selected donor feature | D8-012 |
| D8-016 | M5 | Qualify and publish release | D8-008, D8-009, D8-011, D8-012, D8-013, D8-017, D8-018 |
| D8-017 | M0 | Establish CI and evidence reporting | D8-001 |
| D8-018 | M3 | Define fork identity and license provenance | D8-004, D8-012 |
| D8-019 | M5 | Prepare Dabbl8 information for dabbl.co.za | D8-016 |

## D8-001 Reproduce pinned upstream build

Phase: M0. Dependencies: None.

### Steps

- [ ] Confirm Mac model, macOS, Docker and compiler prerequisites.
- [ ] Compare pinned v1.1.5 with fork main v1.1.5.1; record whether to retain or deliberately update the implementation baseline.
- [ ] Record toolchain archive hash, compiler version, SDK commit and container digest.
- [ ] Compile the unchanged selected baseline and run the upstream test suite.
- [ ] Archive memory reports, test skips/failures, output hashes and build logs.

### Completion evidence

Record compiler and archive hash, SDK commit, container digest, map, binary hashes and full test results without flashing.

## D8-002 Freeze historical project layouts

Phase: M1. Dependencies: D8-001.

### Steps

- [ ] Inventory every historical project struct and serialized size.
- [ ] Replace historical track-count expressions with explicit legacy constants.
- [ ] Preserve old checksums, offsets and engine conversions.
- [ ] Add real legacy-format fixtures and round-trip/import checks.

### Completion evidence

Replace legacy NTRK dependence with explicit four-track constants; all historical fixture sizes and imports unchanged.

## D8-003 Version automation addresses

Phase: M1. Dependencies: D8-002.

### Steps

- [ ] Inventory motion place/param encoding and all consumers.
- [ ] Choose explicit track/step fields or a widened stable wire address.
- [ ] Version save and transport encodings without reusing old format IDs.
- [ ] Test all 512 addresses, boundary indices, locks and malformed inputs.

### Completion evidence

All 512 track/step addresses round-trip independently; locks and malformed records covered.

## D8-004 Define capability and protocol handshake

Phase: M1. Dependencies: D8-003.

### Steps

- [ ] Define protocol family, schema version, track count and feature limits.
- [ ] Keep stable parameter and engine IDs; define unsupported-feature errors.
- [ ] Specify editor negotiation and read-only fallback.
- [ ] Test both mismatched firmware/editor pairings and bounds.

### Completion evidence

Old editor/new firmware and new editor/old firmware fail safely or negotiate supported operations.

## D8-005 Model D8P1 maximum storage

Phase: M1. Dependencies: D8-002.

### Steps

- [ ] Calculate maximum project bytes for eight tracks, patches, motion, banks and scenes.
- [ ] Specify bounded D8P1 chunks, checksums and unknown-chunk behavior.
- [ ] Model flash areas, autosave and retained RAM separately.
- [ ] Document two-versus-four project and sample-slot alternatives.
- [ ] Record the selected map before implementation.

### Completion evidence

Bound all chunks; calculate worst case and publish flash/RAM tradeoffs before map approval.

## D8-006 Specify bounded voice allocation

Phase: M2. Dependencies: D8-001.

### Steps

- [ ] Document current protected-voice invariant and every no-room path.
- [ ] Choose bounded priorities for held notes, bass, drums, chords and unison.
- [ ] Keep the initial sounding budget at eight.
- [ ] Test eight held voices plus additional notes, sustain, release and cross-track stealing.

### Completion evidence

Eight held protected notes plus ninth event cannot exceed pool or enter an undefined fallback; musical stealing documented.

## D8-007 Enable eight track sequencing and MIDI

Phase: M2. Dependencies: D8-003, D8-004, D8-006, D8-005.

### Steps

- [ ] Audit track-index loops, arrays, masks and selected-track controls.
- [ ] Expand recording/playback, mutes, engine switching and command bounds.
- [ ] Route MIDI channels 1–8 consistently over USB and TRS input.
- [ ] Test tracks 5–8 independently and concurrently with 1–4.
- [ ] Preserve four knobs/project slots where they are unrelated to tracks.

### Completion evidence

Tracks 1–8 record/play/mute independently; USB and TRS routes validated; non-track fours unchanged.

## D8-008 Build eight track mixer and selection

Phase: M2. Dependencies: D8-007.

### Steps

- [ ] Prototype track selection and 1–4/5–8 knob banking.
- [ ] Show selected track, record destination, engine and mute states clearly.
- [ ] Resolve collisions with chord, drum and FX key layers.
- [ ] Render all screens and palettes; test clipping and gesture behavior.

### Completion evidence

Bank indicator and record destination always clear; UI clipping tests and all quick-layer conflicts reviewed.

## D8-009 Update editor and backup conversion

Phase: M2. Dependencies: D8-004, D8-005, D8-007.

### Steps

- [ ] Confirm whether to port the separate Felucca-WebApp or maintain a new companion editor.
- [ ] Implement capability-driven track arrays and count limits.
- [ ] Import legacy projects into tracks 1–4 with tracks 5–8 empty.
- [ ] Preserve original files and emit a migration report.
- [ ] Validate backup/restore without silently dropping unsupported data.

### Completion evidence

Track-count aware editing; lossless legacy import; original backup remains available.

## D8-010 Measure and bound engine state

Phase: M3. Dependencies: D8-007.

### Steps

- [ ] Capture baseline and eight-track linker/map differences.
- [ ] Measure engine pool and stack high-water usage.
- [ ] Pool expensive side-state by active engine where justified.
- [ ] Publish per-engine caps and maximum supported combinations.

### Completion evidence

Linker diff, pool/stack high-water report, active-state allocation and declared engine caps.

## D8-011 Implement atomic multi-sector saves

Phase: M3. Dependencies: D8-005, D8-010.

### Steps

- [ ] Implement only the approved storage map and bounded encoders.
- [ ] Preserve A/B validation and commit-last behavior across multiple sectors.
- [ ] Inject cuts after every erase/program operation.
- [ ] Test corrupt copies, length overflow, sequence wrap and full storage.
- [ ] Save while stopped until live saving is separately proven.

### Completion evidence

Power-cut tests at every program/erase boundary; last valid save recoverable; all writes within approved map.

## D8-012 Prepare recovery and first target alpha

Phase: M3. Dependencies: D8-009, D8-011, D8-008.

### Steps

- [ ] Prepare verified backups, known-good packages and documented recovery equipment.
- [ ] Document device revision and host enumeration before changing hardware.
- [ ] Use a separate install session after owner authorization.
- [ ] Measure p95/p99/max render time and late blocks under MIDI/UI/USB stress.
- [ ] Test reboot, project recall and both USB sample rates on the actual Mac.

### Completion evidence

Verified backups and known-good rollback/recovery prepared; separate authorization before flashing; target timing and USB logs retained.

## D8-013 Add bounded performance arrangement

Phase: M4. Dependencies: D8-012.

### Steps

- [ ] Choose bounded pattern-change, scene and fill behavior.
- [ ] Implement bar-quantized switching before increasing scene depth.
- [ ] Store scene references rather than complete duplicated projects.
- [ ] Measure memory and timing after each feature.
- [ ] Run original performance songs and focused undo tests.

### Completion evidence

Quantized changes, scenes/fills/microtiming added incrementally; maximum size and timing tests pass.

## D8-014 Evaluate weighted FM voice budget

Phase: M4. Dependencies: D8-012.

### Steps

- [ ] Study Melodee cost units without assuming its target numbers transfer.
- [ ] Profile FM6 and other engine combinations on real hardware.
- [ ] Define weighted costs and engine caps with measured headroom.
- [ ] Accept expanded polyphony only when stressed maximum deadlines pass.

### Completion evidence

Hardware-calibrated cost units; no polyphony promise without max-block and stress-test evidence.

## D8-015 Audit selected donor feature

Phase: M4. Dependencies: D8-012.

### Steps

- [ ] Choose one concrete musical gap that upstream does not already solve.
- [ ] Record donor repository, commit, exact files and transitive licenses.
- [ ] Port the smallest useful component and retain attribution.
- [ ] Compare musical behavior and memory/CPU delta.
- [ ] Defer candidates that threaten first-release reliability.

### Completion evidence

Exact donor SHA/files/license/transitive dependencies recorded; test and profiling delta reviewed.

## D8-016 Qualify and publish release

Phase: M5. Dependencies: D8-008, D8-009, D8-011, D8-012, D8-013, D8-017, D8-018.

### Steps

- [ ] Verify source/binary correspondence, migration and recovery documentation.
- [ ] Publish tested firmware/editor/host compatibility and engine limits.
- [ ] Have a second tester reproduce install, import and rollback.
- [ ] Package notices, build manifest, source snapshot and checksums.
- [ ] Release only features whose required issue gates are complete.

### Completion evidence

Second tester reproduces install/import/rollback; source matches binary; notices, hashes, limits and compatibility matrix published.

## D8-017 Establish CI and evidence reporting

Phase: M0. Dependencies: D8-001.

### Steps

- [ ] Inventory host/toolchain/generated-file dependencies.
- [ ] Add narrow read-only CI jobs with pinned actions and dependency versions.
- [ ] Capture serialization, audio and editor regressions with artifacts.
- [ ] Separate simulator/host success from hardware qualification.

### Completion evidence

Pinned host checks and package build produce explicit pass/fail/skip results; hardware tests remain a separate gate.

## D8-018 Define fork identity and license provenance

Phase: M3. Dependencies: D8-004, D8-012.

### Steps

- [ ] Inventory release identity overrides and updater checks.
- [ ] Specify coordinated Dabbl8 package and protocol naming.
- [ ] Retain compatible loader behavior and regression coverage.
- [ ] Maintain a donor/file license ledger and exclude vendor firmware.

### Completion evidence

Package, USB strings, updater and editor identify Dabbl8 consistently; GPL and file-level notices are preserved; no unsupported VID/PID claim.

## D8-019 Prepare Dabbl8 information for dabbl.co.za

Phase: M5. Dependencies: D8-016.

### Steps

- [ ] Inspect grant-vine/dabbl conventions and deployment process before editing.
- [ ] Choose a project page path; do not assume /dabbl8 already exists.
- [ ] Prepare overview, requirements, install/recovery links and release status.
- [ ] Review through a website-repository PR; publication is a separate requested action.

### Completion evidence

Reviewed public information in grant-vine/dabbl links to the correct source/releases and states tested capabilities and limits.
