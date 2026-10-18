<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Local affected checks

`midi-eight` checks all 256 recording masks through the actual editor and runs
bounded eight-channel notes through the host renderer. `midi-drum` covers all
eight drum lanes; `midi-workload` covers read-only preflight and uncertain-send
cleanup with fake ports and a deterministic clock. These recipes open no MIDI
ports and need no private demo or device snapshot: C fixtures capture their
normal startup project in memory. Both renderer fixtures retain optional D8P1
file input and contract output for explicit local investigations. A shared
fixture change selects both renderers; shared scheduling changes select both
Python schedules. These checks do not qualify physical timing or saved-data
durability. Generated headers must already be available locally.

The `engine-fx` recipe runs 84 actual host-renderer scenarios: 13 enabled engines
plus a mixed eight-track layout, each dry or with chorus, delay, ROOM, SPRING or
combined effects. It checks audio/sends/wet output, shared ownership and release;
it does not qualify device deadlines, all presets or every effect combination.
`performance-buses` and `performance-fx` are separate bounded local recipes for
the existing banked mixer and performance-effect fixtures. Use their selected
checks to avoid repeating the engine matrix; host cost assertions are not
physical FM-1 measurements. Existing cached inputs and sanitizer scopes apply.

`--check voice-allocation` runs the generated eight-track demo, allocator budget
cases and four/eight-track overload controls locally under the existing sanitizer
recipe. It checks released-tail reclamation without changing allocation policy.
It opens no MIDI ports, downloads nothing and builds no firmware package. See
`docs/dabbl8/voice-allocation.md` for counter semantics and measurement limits.

Native MIDI guard and read-only snapshot changes select `midi-native`. It runs
one virtual-NOR fixture and offline guard, inventory, release and deadline tests.
It opens no physical or software MIDI ports and downloads nothing. Native
software-loopback qualification is a separate explicit operation; do not attach
it to hosted CI or run it automatically from the selector.

GitHub stores source; local checks are the default. Inspect a plan first:

```sh
python3 ci/local_checks.py --base origin/main
python3 ci/local_checks.py --base origin/main --run
```

Without `--base`, compare the current worktree to local `HEAD`, including staged,
unstaged and untracked files. Nothing is fetched. Use `--paths PATH...` to state
changed paths explicitly. The default prints `PLAN`; it does not execute checks
or create output directories. Unknown mappings and missing inputs return
`REFUSED` with exit 2. They are never treated as passing or silently skipped.

The small selector covers documentation (diff check), its own runner (shell
syntax and selector tests), historical UI fixtures (four/eight tracks), legacy
storage quarantine, read-only native boot, and focused source-local SAMPLE/GRAIN/SLICE range and render fixtures.
C checks use local compiler/generated headers only. Storage runs both the actual
historical torn-write tests and native legacy-write quarantine. Legacy/boot/storage checks
use strict ASan/UBSan; sampler range/render checks retain only the inherited signed-overflow/shift
exclusions, with bounds/object/pointer checks active. They require the actual
`sample_source_range_test.c` and `slice_render_source_range_test.c` fixtures; if
these are absent, selection refuses rather than substituting weaker fuzzing. FM4 is off and
SLICE is on for these selected checks; this is not the full configuration matrix.

For an unmapped area, select a reviewed scope explicitly, for example:

```sh
python3 ci/local_checks.py --paths docs/dabbl8/README.md --check docs --run
python3 ci/local_checks.py --paths firmware/src/audio.c --check sampler
```

Explicit selection retains unmapped paths in the report; it does not establish
coverage of them. Add/review an actual recipe when a change needs a different
check. Do not use a documentation check to qualify firmware. `--cc 'clang-18'`
selects a compiler; `CC` is also supported without shell evaluation.

Execution writes commands, exits, stdout/stderr, source HEAD and available
header hashes under ignored `build/local-checks/run-*/results.json`. Each command has a 120-second timeout (exit 124 with partial logs). It stops on
the first failure. `SELECTED_CHECKS_PASS` means only those recorded checks passed.
No dependencies are downloaded, headers generated, vendor build/package created,
firmware installed or hosted workflow triggered. Update-golden/budget overrides
are refused. Broad qualification remains an explicit, separate upstream
`sh tests/run_tests.sh` invocation with its existing prerequisites; there is no
full-suite fallback or `--full` flag in this small runner.

Native sample replacement has a separate `--check sample-upload` recipe: the
actual eight-track virtual-NOR handler with bounded ASan/UBSan (only inherited
DSP signed-overflow/shift exclusions), plus actual browser helper fake-reply
tests. It requires the existing exact HAL-derived
`build/gen/d8_flash_store_ranges_test.h`; missing input refuses without
generation/download. Select this scope explicitly for shared editor/coordinator
changes after reviewing their affected paths. It does not establish other editor
features, vendor memory/timing or physical uploads.

Effects resident quiet has a separate `--check fx-quiet` recipe, selected by
`firmware/src/fx.c` or `tests/native_fx_stationary_quiet_test.c`. It compiles only
the focused eight-track ROOM/AP2/SPRING invariant fixture and its three pool
sources, with FM4 off and SLICE on. ASan/UBSan are fatal; only the existing DSP
signed-overflow/shift exclusions remain. All seven generated headers and the
actual included fixture sources must already exist, otherwise selection refuses.
This fixture does not include the sample-upload ranges header. There is no
header generation, download or full-suite fallback. This scope establishes
resident-state proof controls; it does not qualify DSP changes generally,
ISR timing, memory use or release readiness.

Whole own-project transfer has a separate `--check own-project-transfer` recipe:
actual command-81 handler, UI confirmation, retained virtual NOR and runtime
adoption, with the existing inherited DSP sanitizer exclusions only. It uses
existing generated inputs and HAL range header; it never downloads dependencies,
opens MIDI ports, builds vendor packages or runs hosted CI. Pure pool tombstone
checks are a separate dependency proof. No selected check establishes physical
backup/restore, target budgets or release readiness.

The `own-project-client` check exercises the actual own-project command handler
with simulated MIDI/virtual NOR and the actual native/WASM archive codec. Build
the browser codec explicitly using `web/build-d8archive.sh` and the existing
pinned Emscripten 4.0.17 first. Its source/artifact identity is checked; the runner
refuses missing/stale inputs and never downloads or builds them automatically.

The `dry-sends` check compares the real mixer and state trace against immutable
local parent8105e2a. It archives only local Git source (no fetch), compiles both
against identical cached headers and fatal sanitizers, and refuses differing or
empty traces. The optional cost probe records host measurements without changing
timing budgets or claiming target cycles.

Diagnostic console capture has a `--check timing-capture` recipe mapped only to
the host collector and its scripted transport test. It runs the bounded fake
transport suite, never `--capture`, serial enumeration, MIDI or firmware builds.
Missing collector/parser/test inputs refuse before execution. Shared report-parser
edits remain unmapped and require separately reviewed broader parser coverage.
