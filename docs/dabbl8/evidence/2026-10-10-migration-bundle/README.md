# Offline migration evidence — 2026-10-10

Parent firmware source: `50cf903e67cd7cad6642f02acc729f9aedb648b1` (draft PR #60). This change adds host tools/tests only; all inputs in these logs are synthetic. No user backup, vendor firmware, device operation, firmware build or physical migration was performed.

- Optimized: 121 checks, zero failures.
- Sanitized: 121 checks, zero failures.
- Actual existing C legacy importer/native checks plus actual shared-pool writer/reader, not a duplicated pool encoder.
- Complete 13-object archive and valid synthetic ADPCM sample, all decoded originals, fourth project and both raw autosave sectors retained; inputs unchanged.
- Distinct live/persisted content, modular wrap, identical-generation selection, payload-corruption fallback, proven-erased initialization and zero native projects.
- Explicit refusal for missing/truncated/oversized-length snapshot, committed foreign identity, divergent equal/half-range generation, corruption, fourth-slot/missing references, damaged candidate/fourth project, malformed sample, archive CRC and incomplete archive.
- Native pool headers/payloads, spare erased, hashes and deterministic bytes checked; initializer refuses bad masks/objects and existing output.

Compile/run from repository root:

```sh
clang -O2 -w -Ibuild/gen -Ifirmware/src firmware/src/d8p1.c tools/dabbl8_project_convert.c -lm -o /private/tmp/d8-convert
clang -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/src firmware/src/d8p1.c firmware/src/d8pool.c tools/dabbl8_pool_initialize.c -o /private/tmp/d8-initialize
python3 tests/migration_bundle_test.py /private/tmp/d8-convert /private/tmp/d8-initialize
clang -O1 -g -w -fsanitize=address,undefined -fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow -fno-sanitize-recover=undefined -Ibuild/gen -Ifirmware/src firmware/src/d8p1.c tools/dabbl8_project_convert.c -lm -o /private/tmp/d8-convert-san
clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=undefined -Ifirmware/src firmware/src/d8p1.c firmware/src/d8pool.c tools/dabbl8_pool_initialize.c -o /private/tmp/d8-initialize-san
ASAN_OPTIONS=detect_leaks=0 python3 tests/migration_bundle_test.py /private/tmp/d8-convert-san /private/tmp/d8-initialize-san
```

Standalone initializer/codec/pool has strict ASan/UBSan. Legacy converter includes upstream DSP and retains the established exclusions above; sanitizer success does not prove excluded classes safe. Generated binaries remain private. Mac environment inherited from the verified baseline: Apple clang 21, Python 3.14.8 isolated environment, Node 26.11.0. `git diff --check` and test-runner syntax validation passed. No full local suite was repeated because firmware/core/converter implementation and golden inputs are unchanged; the new cases are integrated into normal and sanitizer CI runs. Physical backend/migration/recovery and hardware timing/stack gates remain open.
