# Integrated candidate verification, 10 October 2026

Functional candidate **6f73dba1576ad12bdc3fa30eac1509cd674dc0a5** was merged through PR #81 as main **baae18074f444b5681e04b45f3d0373aea9e101b**. Their complete trees match. This evidence documents that exact combined implementation, including the restored #62 UI checks, offline preview kit, project-set archive and read-only instrument capture. It is not a firmware release or hardware qualification.

The integration deliberately retains the upstream v1.1.5.1 screen-off/backlight wake fixes. The untouched upstream v1.1.5 reference **276f72a4e6ea8a12499a7a6819aadf3165126755** remains the historical baseline. The earlier baseline report explicitly selected carrying the wake fixes in a deliberate later update before hardware qualification. Neither an old baseline hash match nor a silent development-branch upgrade is claimed here.

## Existing successful CI

[Run 38086303575](https://github.com/grant-vine/dabbl8/actions/runs/38086303575) tested merge **5abbbee80427412f41175ff203d326631f0f0354**, whose complete blob tree equals the candidate. Its artifact ZIP SHA-256 **659a01d1914f55f47b59db635e7f2095a48fae659d225ccd94fe526b3f6b9b8c** matched GitHub's digest. The original run was observed to terminal success; no restart was used. [Independent audit comment](https://github.com/grant-vine/dabbl8/pull/81#issuecomment-6102304190).

Direct logs confirm these scopes in both normal and sanitizer phases:

- 46,720 actual banked UI frames, 1,280 bank-label assertions, and 64 held-owner track pairs covering 192 chord/solo/effect cases plus 64 drum cases.
- Instrument capture 67,341 and actual-C collector 287 checks; signature 13,774 and canonical equivalence 2,317.
- Resident tails 1,256,615, FX publication 227,895, output queues 249,563 and autonomous-TONE refusal 73 checks.
- Quiet activity 594,684, integration 130,295, autosave session 292,748, actual main-loop routes 210,648 and project-set archive 374 checks.

All 92 golden renders matched, with zero changed/gone renders or health failures. The separately built Emscripten 4.0.17 converter passed 218 actual C/WASM/native parity and independent ZIP checks using Node 26.11.0. The four preserved browser log hashes match its result record. This converter check is distinct from the omitted full browser emulator.

CI records **13 optional/environment omissions**: nine unavailable Linux instruction-counter/cost checks, the optional DaisySP comparison, vendor V15 restore fixture, one legacy editor MENU fixture, and full browser emulator. A zero CI host CPU-overrun count does not prove those omitted instruction budgets passed. The inherited whole-engine ASan/UBSan configuration excludes signed integer overflow, shift, bounds, object size and pointer overflow. The standalone archive sanitizer entry has no DSP exclusions.

## Fresh Mac and target checks

The isolated Mac full host suite passed in 696.77 seconds. Every checked source file, six baseline/build artifact pins and all 29 generated inputs stayed unchanged. Its explicit optional omissions were DaisySP, vendor V15 restore, the legacy editor MENU fixture and full browser emulator. The standalone offline preview kit passed 66 checks; its runner entry is absent from CI, so this result is separately attributed to the Mac. Actual converter parity passed 218 checks with the same pinned compiler and Node version.

| Pinned target candidate | Image | RAM data + BSS / 98,304 | Image pool / 344,064 | App SHA-256 |
|---|---:|---:|---:|---|
| Mac default four tracks | 447,928 B | 92,116 B | 331,332 B | `7e89ae7fd6815748419c60948ac69f0d368f92be8d729608f82218721153ec0b` |
| Mac native eight tracks | 461,312 B | 95,780 B | 334,312 B | `34091caeec7d9b2bdd06d65844e313fa4b23a6e8a7a78a8e6121de61e7ab11db` |

Both pinned target builds pass their image/section/RAM checks. Default target cost checking exits 0. Native checking explicitly exits 1: **UAC 570/504, ROOM 296/218, SPRING 176/126** remain above the unchanged static budgets. These are static instruction estimates, not physical deadline measurements. The target report records this qualification failure rather than converting it into a passing release gate. An initial packaging command lacked the SDK environment and failed; the explicit-SDK retry passed, with both attempts retained.

Linux CI's default app is 447,944 B, SHA-256 `ec1f7b960dd80d9644dd85f48f422e31bb6702d6eeb847a056980996a6c9a65d`. Its generated fonts/keycaps differ from the Mac inputs; cross-platform binary identity is not claimed. The unchanged loader hash is `d71d1b7c12adede951605e758a3580dacccedc427d49e1fceaf51decccc68a4b`. No golden audio hash or budget was rewritten.

## Remaining gates and evidence boundaries

The tests use simulated NOR, DMA/controller seams and host execution. Physical audio/output completion, worst-case deadlines, stack/interrupt behavior, USB transfers, recovery, power-loss behavior and safe production ownership remain unqualified. Native binding alone does not activate production migration or grant ownership. Post-boot capture is not authenticated provenance, a pre-upgrade dump, vendor recovery or a qualified restore path.

No firmware was flashed or installed, no loader/flash boundary was changed, and no production activation or release was performed for this evidence. Binaries, disassembly, vendor firmware and private musical originals are excluded. The raw logs and scoped summaries are under `ci/`, `mac/` and `target/`; `manifest.json` records each retained file's bytes and SHA-256. It intentionally excludes itself.
