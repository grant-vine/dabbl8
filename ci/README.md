<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Local affected checks

`--check midi-eight` runs the actual command-27 recording-state query (all 256
recording masks), actual eight-channel USB MIDI renderer on the retained EIGHT
DEMO fixture, and guarded schedule/refusal tests. The renderer proves peak eight
voices/two heavy slots and unchanged project bytes after matched note cleanup;
it does not qualify all eight heavy slots or physical timing. Pure workload
checks separately refuse armed recording before notes and during playback.
The private native adapter remains a distinct, default-plan-only tool; no device
port is opened by this local recipe. Reuse `run-gutjktqg` for unchanged inputs.

`--check engine-fx` compiles and runs the actual host renderer for 84 scenarios:
13 enabled engines and one mixed eight-track layout, each with dry, chorus,
delay, ROOM, SPRING and combined distortion/chorus/delay/ROOM. The fixture uses
the first factory preset, short releases and four overlapping note bursts.
Reverse SLICE uses the built-in BREAK. It checks sound, requested sends and wet
output, eight shared voices, live pool ownership and note cleanup. Heavy layouts
reach all eight pool slots. It does not qualify every preset, performance FX,
effect-tail silence, target timing, stack headroom or audio quality. There is no
device or storage adapter. Inspect the plan before adding `--run`; retain the
source-bound result rather than rerunning an unchanged matrix.

`--check performance-buses` runs the existing eight-track banked solo/mute
panel and dry/stereo/send isolation fixture with the same sanitizer settings.
It covers cold first activation, all solo masks and release. This is separate
from `engine-fx`, so checking performance controls does not repeat the matrix.
It does not run the upstream instruction-cost benchmark or qualify device timing.

`--check performance-fx` runs the existing performance-effects fixture in its
optimized eight-track configuration, including its unchanged instruction-cost
assertions. It covers repeat/reverse, tape stop, freeze, filters, crush, throw,
harmonizer and native key/buffer boundaries. No demo files or device access are
requested. This recipe intentionally follows the existing unsanitized cost run;
sanitizer overhead is not comparable to its cost limits. If the host instruction
counter is unavailable, retain the fixture's explicit skip. Host counts and its
inherited device-cost estimates do not qualify FM-1 timing.

`--check midi-workload` runs only the pure offline MIDI schedule/protocol tests:
identity/routing/track refusal before notes, bounded note-on/off scheduling,
partial-send cleanup and health/cleanup failure controls. It imports no native
MIDI library and never opens a serial or MIDI port. Actual-device capture remains
a separately authorized operation; this check does not establish hardware timing.

Voice-allocation investigation uses `python3 ci/local_checks.py --check voice-allocation --run`: the actual project loader, sequencer and DSP run the eight-track demo and two allocation controls, followed by allocator stress and four/eight-track overload fixtures. It uses ASan/UBSan with the existing DSP arithmetic exclusions. No MIDI, device, download or package-build operations are included. Detailed counter interpretation is in `docs/dabbl8/voice-allocation.md`.

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

New execution receipts include SHA-256 `source_inputs` for tracked/nonignored
working-tree files, required ignored inputs and generated headers, including
uncommitted and transitive source changes. Source changes observed between the
two fingerprints fail the run even if commands exit zero. Generated output under
`build/` and `.local-baseline/` is excluded unless explicitly required. Compare
relevant input hashes and the recorded commands/configuration before reusing a
result; unchanged Git HEAD alone is insufficient. This is an input receipt, not
coverage of every hashed file, toolchain attestation or detection of a transient
edit that is reverted between fingerprints. Older receipts are not retroactively
source-bound by this change.

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

## Native MIDI guard and snapshot checks

Changes to the native guard, read-only MIDI bridge or snapshot runner select
`midi-native`. This builds one virtual-NOR fixture and runs injected guard,
inventory and 17-role snapshot/release/deadline tests. It never imports native
MIDI or opens a device. Actual Mac software-loopback qualification is a separate,
explicit local operation; neither the affected selector nor hosted CI runs it.

Startup ordering has a focused `--check startup-order` recipe. It executes the
actual HAL/main startup against simulated MMIO/clock, native read-only autosave
adoption against the actual C storage fixture, and all eight UART/UAC/SIE TIMER5
configurations under ASan/UBSan. It requires cached generated inputs and uses the
selected local compiler. The tests cover DMA start after IRQ enable, external
MIDI admission after autosave adoption, and unchanged steady-state timer cadence.
They do not attribute a retained hardware late counter, establish physical DMA
behavior or qualify target deadlines. Reuse unchanged engine/effects evidence;
this recipe does not rerun that matrix or access a device.
