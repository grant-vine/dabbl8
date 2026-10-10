# Offline browser project conversion

The development companion at `web/d8companion.html` can prepare a local legacy-project bundle without MIDI access. It uses the same C converter as the [offline CLI workflow](offline-project-conversion.md), compiled to WebAssembly. No JavaScript replacement importer is used.

Build the existing baseline tables first, then use the isolated, recorded Emscripten 4.0.17 SDK:

```sh
EM_CONFIG=/path/to/isolated/emsdk/.emscripten \
EMCC=/path/to/isolated/emsdk/upstream/emscripten/emcc \
sh web/build-d8converter.sh
python3 -m http.server 8768 --bind 127.0.0.1
```

Open `http://127.0.0.1:8768/web/d8companion.html` in Chrome. Choose a standalone FUN1–FUN9 file. Supply the actual saved-slot files if its chain refers to them: browser slots 1–4 correspond to original indices 0–3. Missing or invalid references refuse conversion rather than aliasing another slot. All supplied references must independently convert and verify.

The ZIP includes byte-exact original snapshots, SHA-256 hashes and `report.json`. Accepted bundles also include canonical D8P1 project data and every converted saved-slot reference. C decoding and canonical re-encoding check each output before publication. Tracks 1–4 retain canonical pinned-importer semantics; tracks 5–8 start empty with native defaults, and eight tracks share eight sounding voices. The report identifies engine migrations, removed incompatible automation and chain translation. Keep complete originals because conversion can normalize inactive/reserved data and omit incompatible automation.

Unsupported or damaged inputs produce an original-and-refusal-report bundle without converted files. A missing compiler module similarly keeps copied originals available. Each file is limited to 16 MiB; unreadable or larger files cannot be copied into a bundle and are explicitly refused. Keep independent source backups. Browser memory, downloads and file copying do not provide filesystem crash consistency or device recovery qualification.

Changing an input invalidates a pending result and revokes the old download URL. Page exit clears pending results and download URLs; conversion can resume when the page returns. Destroying the view disables its inputs and prevents late output. The optional `onSettled` callback supports callers that need notification when an attempted operation finishes; it does not establish acceptance, which remains in the report.

Generated modules live under ignored `build/d8converter/`; do not commit binaries or private user files. CI verifies the exact official Linux compiler archive from `ci/browser-conversion.json`, uses scoped `EM_CONFIG`/`EMCC`, builds this same C source and compares actual WASM/native outputs and reports across all thirteen frozen legacy originals. Independent Python ZIP checks verify CRCs, original bytes and all report hashes. The separate browser evidence includes compiler/archive/source/module hashes and explicit failures. This step does not alter the baseline environment's PATH or claim physical browser/hardware coverage from Node tests.

The companion does not upload D8P1 to a device. Device backup/restore, approved flash map, runtime adoption, target state/cache/stack allocation, persistence/power-cut recovery and arrangement playback remain unqualified. These bundles contain experimental project data, not firmware or installation packages. No website deployment or release is implied.
