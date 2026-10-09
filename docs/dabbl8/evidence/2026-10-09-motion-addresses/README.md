# D8-003: versioned automation addresses

Runtime addresses are widened to 16 bits; historical FUN7–FUN9 motion records remain explicit 260-byte encodings. The [format specification](../../motion-address-format.md) inventories consumers and defines D8M1 (explicit track/step, versioned section) and new SysEx command 74 (schema 1). Current live saves remain frozen FUN9, runtime track count remains four, and capability negotiation / D8P1 storage integration are later gates (#6, #7, #13).

Source is stacked on issue #4 commit `3eab2483a85ae228a73abb0acc8d19cf202b94c8`, targeting its branch for review. The unchanged audited upstream baseline remains `276f72a4e6ea8a12499a7a6819aadf3165126755`. Same [pinned baseline environment](https://github.com/grant-vine/dabbl8/tree/fc4822e700b8d9f0c1aab8d52ca793eaa3cfac69/docs/dabbl8/evidence/2026-10-08-baseline): Python 3.14.8, JieLi pi32v2 clang 4.0.1, SDK d179b4484759423312073f5fbb232501aa491047, linux/amd64 Debian image pinned by digest.

- Target build PASS; complete host test suite PASS.
- All 512 addresses round-trip independently as automation and locks in 64-record pool batches. Unknown version, reserved bytes, counts, truncations, track/step boundaries, invalid parameters/values and duplicate identities refused; failed codecs preserve destination/output.
- Real editor handler schema-prefix, unsupported-schema and unavailable-track tests PASS. Historical command replies remain unchanged. Old browser editor tests PASS.
- Immutable legacy fixtures remain byte-exact: 13 cases, four-track and isolated eight-track destinations PASS. Separate legacy destination and exhaustive D8M1 ASan/UBSan checks PASS, using upstream sanitizer exclusions.
- Golden audio, CPU baseline and target budget files unchanged; recorded hashes in `results.json`.
- Application 447,736 bytes; package 610,086 bytes. General RAM 92,116 / 98,304 bytes (6,188 free), pool 331,332 / 344,064 bytes (12,732 free). Increase relative to issue #4: 160 image bytes, 896 general RAM bytes and 128 pool bytes. Loader hash unchanged.

Optional full-suite skips: DaisySP floating reference, official V15 restore filename absent, Emscripten browser emulator. The separate stock restore simulation PASS used the official 699,956-byte vendor file (SHA-256 `db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`) retained outside Git. The simulated device returned to FM-1_015 and a modified vendor file was refused before MIDI. The full suite's original skip remains in its log; the additional `stock-restore-simulation.log` resolves that optional host check. Host simulation does not establish physical recovery. No flash/install, loader code or flash-boundary changes occurred. This does not qualify eight-track playback on hardware.

Raw logs, focused checks, sanitizer checks, output sizes/hashes and whitespace validation are preserved here. No firmware binaries or vendor firmware are committed.
