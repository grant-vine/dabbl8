# Dabbl8 software voice cards

Software voice cards are proposed engine profiles with a defined resource budget. Genre sound packs can supply presets and patterns without requiring a different firmware binary. Begin with a small curated catalogue and prebuilt qualified profiles; add custom builds after exclusion and compatibility are tested. Eight tracks initially share eight sounding voices.

Tracked in [software voice card issue #53](https://github.com/grant-vine/dabbl8/issues/53), supporting memory task #12 and website task #21.

## Research and resource model

The JV-1080 analogy concerns sound resources: Roland's expansion boards add waveform ROM and patches within the instrument's existing polyphony. [Roland TurboStart](https://cdn.roland.com/assets/media/pdf/JV1080ts.pdf). A Dabbl8 engine profile instead selects synthesis implementations and state capacity. Sound packs and performance limits remain separate choices.

At source `432e9d5ddab59669334c295068f1edb7854cd7b4`, actual eight-track target app probes measured:

| Configuration | App bytes | General RAM bytes | Pool bytes |
| --- | ---: | ---: | ---: |
| All 13 selectable engines | 451432 | 95716 | 334100 |
| Existing SLICE-off flag | 440032 | 90740 | 334100 |

Both passed link/image/RAM-text checks; SLICE-off functional and hardware qualification is not established. GRAIN sets the current shared union maximum at 7020 bytes, or 56160 bytes for eight slots. Target PHYS state is 4296 bytes, DRUM 1824, FM6 296, WHEEL 92 and SLICE reverse state 128. Removing smaller union members does not shrink a union while GRAIN remains. Conditional types without GRAIN but with PHYS could save 21792 pool bytes; a DRUM/FM6/WHEEL-only union could save 41568. These are arithmetic opportunities, not implemented exclusion builds.

The delay buffer uses 131072 bytes and the drawing/project arena 59520. Thus engine count is not a resource budget. Removing code affects app space, allocating state affects RAM/pool, and actual notes/effects/index building affect deadlines. A profile must declare and enforce qualified limits rather than advertise an arbitrary “up to X engines” rule. The all-engine probe already fits: reduced profiles are optional, not proven necessary for eight logical tracks.

## Implemented preflight foundation

`firmware/src/d8card.def` is a single stable registry of the 13 current engines. Retired DIGITAL ID 1 remains reserved. Names and IDs are never compacted when selecting a subset. Three dependency bits describe SAMPLE runtime, FM6 patch services and the shared heavy-state pool. They represent internal components, not extra visible engines or a promise of full future dependency closure.

`d8card_validate` rejects empty/unknown/reserved masks, unknown component bits and missing direct components. `d8card_preflight` validates a complete D8P1 file using the actual byte codec, derives the required engine mask from its eight encoded tracks, and reports missing engines. It never adopts state, allocates memory or writes flash. Invalid profiles/files, unsupported optional data, retired DIGITAL and output/input overlap refuse without publishing a report. Known compatible and known missing-engine outcomes publish required/missing masks; the input stays immutable.

Preflight checks this file's structural validity and encoded engine IDs only. It does not verify referenced-project availability, sample assets, native engine parameter policy, resource fit or timing. Callers must compose those checks with the existing reference/native validators before adoption.

DIGITAL refusal is deliberate at this new boundary. Historical archive conversion must run first, preserving original bytes and applying the existing migrations. This does not change historical import behaviour because the preflight is not integrated into those paths.

`tools/dabbl8_card.py` accepts a bounded JSON configuration and emits `card.json` plus `card.h` into a new directory. It expands direct component requirements and canonicalizes engine ordering. Duplicate keys/engines, unknown fields/names, malformed schemas and unsafe profile names refuse. Existing destinations refuse. Example:

```sh
python3 tools/dabbl8_card.py docs/dabbl8/cards/core.json /tmp/dabbl8-core-candidate
```

The outputs explicitly say candidate, no firmware generated, no runtime enforcement and no hardware qualification. The header macros are not consumed by the shipping build. Neither the registry nor preflight is included in `felucca.c`; they are freestanding preparation, not a selectable firmware card or a device capability advertisement. No engine is compiled out or remapped by this PR.

## Test case and implementation sequence

| Stage | Test case and acceptance | Implementation |
| --- | --- | --- |
| Preflight foundation | Exhaust every subset and dependency-bit combination; verify fixed IDs, deterministic CLI output, malformed profiles, existing-directory refusal, structural project validation, missing engines, every fixture truncation, unsupported records, immutable inputs and unchanged failure reports | Registry, bounded configuration generator and freestanding C project preflight |
| Runtime availability | Default behaviour and audio hashes remain unchanged; all UI/editor paths reject unavailable IDs; failed adoption leaves complete runtime/cache/patch state intact | Explicit engine availability mask, filtered browsing, safe startup defaults and preflight before native publication |
| Actual engine exclusion | Core and Breaks link; excluded symbols/state disappear; conditional union uses actual target ABI; image/RAM/pool reserves pass | Compile-time implementations/dependencies and conditional pool members; emit measured manifests |
| Performance limits | Stress permitted mixtures, unison, release/steal transitions, drums, reverse decode, grain-index work, effects and USB/MIDI; measure IRQ deadlines and stack high water physically | Admission limits based on evidence, not engine count; publish supported maxima |
| Cross-card persistence | Original projects and backups survive swaps; missing engines/assets refuse or require explicit separate conversion; corrupt records and power-cut recovery remain safe | Stable schema/IDs and approved common map; preserve reference meaning and original files |
| Curated release catalogue | Each artifact has source/configuration, hashes, licences, tests, memory evidence and recovery qualification | Pinned build matrix and manifest-backed website downloads in the separate website repository |
| Custom build service | Repeated identical configuration resolves to the same canonical build inputs; invalid requests never start builds; custom output cannot inherit unrelated qualification | Bounded backend queue, dependency expansion, pinned source/toolchain, cache and explicit experimental status |

Implemented tests exhaust 8192 masks across all eight component combinations in C, and all 8191 nonempty subsets through the Python resolver. The standalone C tests use strict ASan/UBSan without upstream DSP exclusions. Their synthetic projects include an explicitly normalized engine-1 test copy; committed fixtures are unchanged. Existing tests continue covering the codec's corruption/field policies and actual runtime adoption independently. No physical performance claim follows from these host tests.

Core proposes ANALOG/FM6/LOFI/DRUM; Breaks proposes ANALOG/FM6/SAMPLE/SLICE/DRUM. Dance, Chips and FM, Organic and Texture are additional research candidates. None is a known-to-work hardware combination. Keep the full-engine profile as a candidate while measuring reduced ones.

## Storage and website dependencies

Engine exclusion does not shrink the current fixed D8P1 steps, parameters or eight patch records automatically. Compression and the shared-spare storage proposal remain independent work under #7/#13. Cards must not silently change flash boundaries or reuse sample space. Runtime availability depends on the native/editor capability work under #11; physical limits remain under #12 and release/recovery under #14/#20.

The website should first map curated card and genre choices to qualified prebuilt artifacts. GitHub Actions supports a controlled build matrix. [Matrix documentation](https://docs.github.com/en/actions/how-tos/write-workflows/choose-what-workflows-do/run-job-variations). Later, a backend can queue declared workflow inputs; its repository credentials stay outside the browser. [Workflow dispatch API](https://docs.github.com/en/rest/actions/workflows#create-a-workflow-dispatch-event). No backend or production infrastructure is configured here.

Keep GPL-3.0-only and dependency notices. Each binary needs its exact corresponding source and configuration, not merely an upstream link. [GPL v3 section 6](https://www.gnu.org/licenses/gpl.en.html). Qualified profile downloads belong to the separate website task #21 after release evidence. No installation, release publication or website deployment is authorized by this foundation.
