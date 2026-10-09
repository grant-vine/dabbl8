# D8-002: frozen historical project layouts

Baseline: unchanged Felucca v1.1.5 `276f72a4e6ea8a12499a7a6819aadf3165126755`. This change starts from planning branch `cc31a43737b3f06fb13f3088d1e0b95c272ed8aa3`, not the later fork main. Environment and pinned dependency identities are in [baseline evidence](https://github.com/grant-vine/dabbl8/tree/fc4822e700b8d9f0c1aab8d52ca793eaa3cfac69/docs/dabbl8/evidence/2026-10-08-baseline); the same Python environment, JieLi compiler, SDK and linux/amd64 image were used.

FUN1 has one instrument. FUN2–FUN9 store four tracks regardless of the live track count. Explicit four-track constants now govern historical arrays, import loops and compact offsets. FUN9 serialization rejects an expanded destination without writing bytes until a new format is implemented. The runtime remains four tracks.

The [fixture inventory](../../../../tests/fixtures/projects/README.md) records sizes and offsets. There are 13 immutable synthetic legacy-format records and 13 expected FUN9 migrations, captured with unchanged upstream code before this change. They are genuine historical byte formats, not hardware recordings. The committed reference producer and hash manifest document provenance. Tests cover DIGITAL-to-FM6, PHYS DRUM, SAMPLE PERC, track-four step-63 automation/locks, byte-exact canonical serialization, re-import, checksum corruption and truncated records. No expected audio hashes were changed.

## Results

- Target build: PASS, `sh build.sh`.
- Full upstream host suite plus fixture checks: PASS, `sh tests/run_tests.sh`.
- Four-track and isolated eight-track import destination: all 13 records PASS each, including separate AddressSanitizer/UndefinedBehaviorSanitizer runs using upstream sanitizer exclusions.
- General RAM: 91,220 / 98,304 bytes (7,084 free). Pool: 331,204 / 344,064 bytes (12,860 free). Image: 447,576 bytes; package: 610,086 bytes. Same sizes as the audited baseline; changed application/package hashes are recorded in `results.json`.
- Loader output hash remains recorded without loader source changes. `git diff --check`: PASS.

Explicit optional skips: DaisySP floating-point reference (checkout absent), vendor V15 restore test (expected repo-root filename absent), browser emulator (Emscripten absent). The official vendor file is held outside Git in Downloads; its acquisition does not demonstrate actual device recovery. Editor MENU settings are intentionally unsupported in the upstream test. No flashing, installation or hardware qualification occurred. This isolated eight-track destination test does not establish eight-track runtime behavior.

Raw build, full test, sanitizer and whitespace logs plus output sizes/hashes are preserved alongside this report. Firmware binaries and vendor firmware are excluded.
