# Experimental application-only builds

This route builds an application for software qualification. It does not build an
update loader or an installable firmware package, and it does not authorise flashing.
Curated profiles remain experimental: the current eight-track IRQ/UAC static budgets
fail (base IRQ37502/UAC504, inherited +10% acceptance limits41252.2/554.4), and physical timing, stack, restore/recovery and release qualification remain open.

Use the isolated Python dependencies and JieLi Linux pi32v2 toolchain described in
`BUILDING.md`. On this Mac the toolchain runs in Docker `linux/amd64`; select the
qualified toolchain and pinned image through `JIELI_TOOLCHAIN` and
`JIELI_DOCKER_IMAGE`. This application-only route does not read the AC79 SDK or
build/download dependencies. Keep the verified dependency receipts alongside results.

```sh
python tools/build.py --dabbl8 0.1.0 --app-only --engine-profile core
python tools/build.py --dabbl8 0.1.0 --app-only --engine-profile breaks
python tools/build.py --dabbl8 0.1.0 --app-only --engine-profile full
```

Core retains ANALOG, LOFI, DRUM and FM6 (`0x1409`). Breaks retains ANALOG, SAMPLE,
DRUM, FM6 and SLICE (`0x3411`). Both retain fourteen historical engine IDs/layouts
and share eight sounding voices. Unavailable sounds are refused before admission;
retained user records are preserved. These are fixed curated combinations, not an
arbitrary engine-card generator. Core and Breaks require `FELUCCA_FM4=0` and
`FELUCCA_SLICE=1` (the defaults); SLICE's historical slot remains in Core metadata.

Each invocation creates a fresh `build/app-only/<profile>/run-*/` directory. It
contains the application, ELF, disassembly, generated headers, symbols, memory
report and static-budget logs. Generated sample WAVs/cache also stay inside that
run. Normal `build/` application and loader outputs are not overwritten.
`app-only-manifest.json` records the exact public source-input hashes, Git identity
when available (working-file hashes are authoritative), configuration/compiler flags,
toolchain file hashes, compiler version, resolved container identity, generated inputs,
artifact hashes, memory checks and static-budget outcome. SDK use is explicitly false.
The inherited menu displays compiler `__DATE__`, so matching source and flags alone
cannot promise identical binaries across dates. Run timestamps, selected time-environment
values and a compiler clock probe are recorded; generated/artifact hashes bind the actual
run. Docker tools do not forward host time-environment variables. The original Python generator dependencies must still be installed as documented.

A zero process exit means the application was built and passed link/image/memory
and source-integrity checks. It does **not** mean timing or release qualification:
check `qualification_passed` and `static_budget` in the manifest. Known static
failures are retained without modifying budgets; reduced builds also report missing
renderers that the unchanged full-profile checker expects. A failed generation,
compiler, link or integrity step leaves a manifest with `phase: failed` when output
initialisation has completed. Invalid arguments, configuration and missing toolchain/container dependencies
are refused before creating output. Missing Python generator libraries fail during
generation and leave the failed manifest.

`--app-only` requires `--dabbl8`; it cannot combine with `--release`. Diagnostic app-only builds require CDC enabled. The CLI profile must agree with `DABBL8_ENGINE_PROFILE` when
both are supplied. Invalid flag values and budget/golden-update requests are refused.
Normal reduced-profile packaging is still refused before SDK, generation, loader or
package work. Existing FULL compilation and packaging routes are preserved.

Run the focused CLI regression with `python tests/build_app_only_test.py`. Its early
refusals execute the real CLI; positive orchestration uses controlled tool fixtures to
assert that loader/package/SDK paths cannot run. Actual pinned vendor app builds are
separate evidence. This document offers no firmware download or installation procedure.

Diagnostic applications can be selected explicitly with `--timing-diagnostic`:

```sh
python tools/build.py --dabbl8 0.1.0 --app-only --engine-profile core --timing-diagnostic
python tools/build.py --dabbl8 0.1.0 --app-only --engine-profile breaks --timing-diagnostic
```

These remain applications, not installable packages. They use a `-timing` version,
retain the selected profile and stable IDs, and record `timing_diagnostic: true`,
`timing_report_schema: D8T2` and `stack_qualified: false`. Collection starts stopped.
The existing paused console start/stop/report controls and aligned-window limits
remain unchanged. The build manifest records fixed linked user/supervisor stack
windows. Those allocations do not establish stack use, interrupt nesting headroom,
or high-water measurements. The unchanged image/RAM/pool and static-budget checks
still run; failed static budgets do not become passed physical deadlines.

Pair diagnostic-on/off applications with the same profile, explicit version and
flags before attributing measured code/RAM overhead. `--dabbl8 0.1.0-timing`
without diagnostics gives the same version identity as `--dabbl8 0.1.0
--timing-diagnostic`; output directories remain separate. Physical timer calibration,
external elapsed/reset evidence, stress mixtures, interrupt nesting, transport state,
stack high water and recovery remain separate gates. Retain the manifest with each
D8T2 report: the wire frame does not identify the selected engine profile itself.
