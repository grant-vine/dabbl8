# D8-001: unchanged Felucca baseline, 8 October 2026

Issue: [#3](https://github.com/grant-vine/dabbl8/issues/3). Baseline gate **passed**, with the three optional skips below. No firmware was installed or flashed.

## Source decision

Built pristine Felucca v1.1.5, `276f72a4e6ea8a12499a7a6819aadf3165126755`, in a detached worktree. The planning checkout remains based on that commit (`cc31a43737b3f06fb13f3088d1e0b95c272ed8aa`); no upgrade was made.

Compared fork main `ac6bfcbd76ee3de79eb461719e8d123b5512aa3c` ([full delta](main-delta.log)): v1.1.5.1 keeps the backlight enabled because PA2 also enables the key matrix, fixing failure to wake after screen sleep; changes the sleep default to NEVER and stored encoding; adds the screen setting to the menu redraw signature; updates version, tests and docs. The later commit adds an AI disclaimer. Retain the audited baseline for measurement; carry the wake fix in a deliberate later update before hardware qualification.

## Environment and provenance

MacBook Pro MacBookPro18,3, M1 Pro, 10 cores, 32 GB; macOS 26.7.1 (25G241). Python 3.14.8 in a project-local venv, Pillow 12.3.0, fontTools 4.66.1, pip 26.2.1, Node 26.11.0. Pillow's native library versions are in [pillow-features.log](pillow-features.log). Host tests use Apple clang 21.0.0 (clang-2100.1.1.101), arm64.

JieLi archive from `https://pkgman.jieliapp.com/s/linux-toolchain`, extracted directory `jieli-linux-toolchains-20250324.1`, SHA-256 `f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958`. Compiler clang 4.0.1, confirmed with `-target pi32v2` ([output](target-compiler.log)). Archive retained locally.

SDK: `https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git`, tag `AC79NN_SDK_V1.2.1_2023-12-13`, annotated tag object `b02c8c9354c077238ccbf764e36eff53782e1c16`, peeled commit `d179b4484759423312073f5fbb232501aa491047`. Sparse checkout includes only the three package inputs, LICENSE and README. All three input SHA-256 values match upstream's expected values ([hashes](sdk-sha256.log)).

Docker Desktop 4.92.0, Engine 29.8.0, native daemon linux/arm64; compiler containers explicitly linux/amd64. Image pinned through upstream's `JIELI_DOCKER_IMAGE` override to `debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587`. Local amd64 image ID `sha256:d4c45f66efc59f89b0597d343a9bd72808493c0eacecde5285961743252cee23`; both identities recorded in [docker-image.log](docker-image.log).

## Build, tests and memory

Unchanged `sh build.sh`: exit 0 ([full log](build.log)). Unchanged `sh tests/run_tests.sh` with the same venv, SDK and compiler image environment: exit 0, `ALL HOST TESTS PASSED` ([full log](tests.log)). This includes golden audio, CPU regression, target disassembly budget, UI/font rendering, historical imports, storage, USB/MIDI, simulated loader/installer, ASan/UBSan, fixed-seed fuzzing and Node web tests. `baseline-status-before.log` and `baseline-status-after.log` are empty: source, golden hashes and budgets stayed unchanged.

Optional skips: PHYS float reference (no DaisySP); official V15 restore simulation (no vendor firmware); browser emulator (no emcc). No mandatory suite failed. The simulated installer opens no real MIDI device. Host checks establish no target runtime, USB interoperability or recovery performance.

| Region | Used bytes | Capacity bytes | Remaining bytes |
| --- | ---: | ---: | ---: |
| General RAM, data through BSS end including alignment | 91,220 | 98,304 | 7,084 |
| Engine/buffer pool | 331,204 | 344,064 | 12,860 |
| RAM text | 2,924 | 24,576 | 21,652 |
| Retained NOINIT | 14,796 | 15,696 | 900 |

App: 447,576 bytes. Loader raw image: 8,216 bytes; wrapped OTA loader: 6,930 bytes. Package: 610,086 bytes, unchanged development identity `FM-1_900`. All output SHA-256 values and sizes are in [manifest.json](manifest.json). Detailed [ELF sections](sections.log), [symbols](symbols.log) and [linker map](baseline.map) are preserved. The map is an additional link of the original objects with the unchanged linker script; the original ELF, binary, disassembly and package remain in the local baseline worktree.

## Setup recovery and reproduction

Docker was installed but stopped and was started. Two full SDK clones failed with connection reset/incomplete transfer. A filtered, no-checkout clone succeeded; sparse checkout materialized the required SDK files. An initial build attempted before those files were materialized exited 1 and is preserved locally in `.local-baseline/attempt-01-missing-sdk/`. These are setup failures, resolved before the final successful build and tests.

Local workspace: `~/Code/dabbl8`; pristine source, dependencies, archive, venv, full build outputs, logs and evidence collector are under `.local-baseline/`, excluded through `.git/info/exclude`. Source and SDK instructions were reviewed before downloading dependencies. No installer, release option or golden/budget-update option was used.

To repeat, create a detached worktree at the exact baseline commit, use Python 3.14.8 and [requirements.txt](requirements.txt), obtain and verify the archive hash, check out the exact SDK tag/commit and verify its three hashes. Set `JIELI_TOOLCHAIN`, `AC79_SDK`, `JIELI_DOCKER_IMAGE` to the recorded values and `PYTHON`/`PATH` to the isolated venv, then run `sh build.sh` and `sh tests/run_tests.sh`. Keep optional dependencies and skips explicit. This is one clean environment, not proof of matching artifacts from two independent environments.

Next implementation gate: [#4](https://github.com/grant-vine/dabbl8/issues/4), freeze FUN1–FUN9's historical four-track layouts and imports before introducing a new eight-track format. Loader and flash boundaries remain unchanged. GPL-3.0-only and dependency notices remain in place; no firmware binaries, SDK inputs, vendor firmware, private backups or credentials are committed here.
