# Architecture decisions

| Decision | Status | Rationale or next evidence |
|---|---|---|
| Felucca fork; preserve upstream history and licenses | Accepted | Existing hardware, audio, tests and updater foundation |
| Eight logical tracks separate from sounding voices | Accepted direction | Sequencing capacity is not polyphony |
| Start with shared eight-voice budget | Provisional | Profile before expanding voices |
| Keep OTA/loader and boot boundaries unchanged initially | Accepted | Minimize recovery changes |
| New automation and project formats | Required | Existing addresses and payload cannot represent eight tracks |
| D8P1 chunked bounded encoding | Proposed | Maximum-size model and migration tests first |
| Three projects, shared autosave, three samples | Accepted design, 2026-10-10 | Five existing two-sector blocks; one shared write spare, no per-object A/B; [decision and migration gates](three-project-shared-storage.md). Physical implementation remains gated |
| Track selection and bank gesture | Open | Prototype around four knobs and existing shortcuts |
| Companion editor repository strategy | Open | Inspect Felucca-WebApp licensing/build/protocol before choosing |
| Baseline upgrade from v1.1.5 to v1.1.5.1 | Open | Review delta and record baseline build evidence |
| Website lives in grant-vine/dabbl | Accepted | Public project information on dabbl.co.za |

Record future decisions with date, issue/PR, options, measured evidence and consequences. A proposal becomes accepted only when its relevant issue records the choice. Do not silently amend historical evidence to match later changes.
