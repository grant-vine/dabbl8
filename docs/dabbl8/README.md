# Dabbl8 plan

For the current evaluated binary, exact package/source review and remaining gates,
start with the [RC review guide](rc-review-guide.md) and
[current status](release-candidate-status.md). Older bundle references below are
historical and are not requests to repeat tests or flash.

The consolidated private v0.2.3 diagnostic package, exact source/checksum record
and combined test kit are frozen locally. The last verified installed firmware
is v0.2.2. See [RC readiness](release-candidate-status.md), the [current handoff](local-work-handoff.md)
and [safe hardware test plan](rc-hardware-test-plan.md). No new device session,
public release or flash/data-change authorization follows from this preparation.

[Engine resource limits](engine-resource-limits.md) separates source-defined
voice/pool caps from measured physical combinations. Retained v022 observations
reach eight heavy slots; current-source combined renderer and collector checks
pass locally. Full timing/stack and maximum target combinations remain open.

The numbered documents retain the original source-research and planning snapshot. GitHub Issues are authoritative for progress. The unchanged upstream baseline and subsequent host/target build evidence are recorded separately. The owner's FM-1 passed bounded v0.1.8 eight-track playback, own-project restore/readback and saved-project reboot checks on 15 October 2026. See [current RC readiness and exact evidence](release-candidate-status.md). A hardware-qualified eight-track release remains pending. All 19 implementation tasks and roadmap issue #2 have been created.

## Current data scope — 14 October 2026

First installation is a disclosed clean reset of the previous firmware's projects,
autosave, settings, user presets and user samples within the approved user-state
map. Dabbl8's own projects and supported upgrades must preserve their data. Only
Dabbl8 format migrations are in scope when needed; external firmware import and
original-retaining migration are not release prerequisites. Earlier migration
documents and converter previews below are historical developer work, not the
current install contract. Disclosed first-use formatting completed on the owner's
FM-1. Later retained v020/v022 evidence includes own-project restore, all populated
project slots/shared autosave, stopped save/recall, cold retention and own-upgrade
readback. Other own data and interrupted-format recovery remain open where not
qualified. The policy itself grants
no device operation. See [RC readiness](release-candidate-status.md) for current
coverage and remaining physical measurement gates, and the dated
[software acceptance map](software-acceptance-status.md) for historical coverage. The
[private resource diagnostic package handoff](resource-diagnostic-package.md)
records the implemented observation capability and future separately authorized
hardware phases; it grants no installation or capture permission.

## Reading order

1. [Project brief](01-project-brief.md) and [source blockers](04-eight-track-source-blockers.md).
2. [Memory budget](05-memory-and-flash-budget.md) and [audio architecture](06-audio-and-voice-architecture.md).
3. [Migration/recovery](07-project-migration-and-recovery.md) and [feature priorities](08-interface-and-feature-priorities.md).
4. [Roadmap](09-implementation-roadmap.md), [issue index](issue-index.md) and [decisions](decisions.md).
5. [Desktop first build](desktop-first-build.md) and [local Work handoff](local-work-handoff.md).

The numbered research documents are the original 8 October audit snapshot. Any “no fork/issue created” statements in that historical snapshot describe research time, not present repository status. Current repositories are grant-vine/dabbl8 for firmware and grant-vine/dabbl for dabbl.co.za. Evidence is pinned in evidence/source-index.json.

M0–M5 are phase identifiers used in issue titles and bodies. They are not configured GitHub milestone objects. Dependency links are explicit planning gates; they do not enforce merge or close operations automatically.

Initial scope preserves eight shared sounding voices while adding eight logical tracks. Weighted budgets, new engines and multichannel USB are experiments or later work. Do not advertise them as delivered. Source planning material is GPL-3.0-only; original dependency notices remain in force.

## Project conversion preparation

The [staged D8P1 adapter](d8p1-project-state.md) uses explicit project fields and preserves the pinned legacy importer. The [offline conversion workflow](offline-project-conversion.md) saves original snapshots and migration reports, validates complete saved-slot reference bundles, and emits proposed project data. Neither adds qualified device upload, runtime adoption or flash persistence.

The [browser conversion workflow](browser-project-conversion.md) uses the same C importer and produces a local original/report/project-data ZIP without MIDI access. Its device upload and recovery gates remain open.

The [stopped native state API](d8p1-runtime-state.md) loads/captures actual eight-track runtime state and retains arrangement metadata outside the display arena. Device transfer, persistence, playback and physical qualification remain open.

- [Relative multi-sector save engine](multisector-record-engine.md): simulated A/B record preparation; approved map and physical integration remain gated.

- [Software voice cards](software-voice-cards.md): resource research, test matrix and standalone profile/project preflight; runtime exclusion and released variants remain gated.

- [Selected three-project shared storage](three-project-shared-storage.md): three projects and one shared autosave, all samples retained, commit-last write spare; replaces the per-object A/B proposal for initial integration.

- [Shared storage pool implementation and remaining migration gates](shared-storage-pool.md)

- [Real eight-track runtime through shared-pool persistence](runtime-shared-persistence.md)

- [Offline converter developer-preview kit](offline-preview-kit.md): local host-specific tooling package, complete corresponding source and original-preserving conversion; device migration and release gates remain open.
- [Original-preserving full-backup and persisted-autosave migration proposal](migration-bundle.md)

- [Guarded native runtime connection to the existing flash driver](guarded-flash-backend.md)

- [Native three-project names and stored-snapshot rename preparation](native-project-catalog.md)

- [Native staged save-as naming and commit-only publication](native-save-as.md)

- [Three-project native menu, cached inventory and legacy-writer refusal](native-project-menu.md)

- [First-device evidence plan](target-evidence-plan.md): original retention, separate
  install authorisation, timing/late-counter observation limits and remaining
  physical recovery/release acceptance gates.
