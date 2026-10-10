# Dabbl8 plan

The numbered documents retain the original source-research and planning snapshot. GitHub Issues are authoritative for progress. The unchanged upstream baseline and subsequent host/target build evidence are recorded separately; draft implementation PRs do not establish a hardware-qualified eight-track release. No device flashing has been performed in this work. All 19 implementation tasks and roadmap issue #2 have been created.

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
