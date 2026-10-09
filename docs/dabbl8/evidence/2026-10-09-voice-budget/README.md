# D8-006: bounded voice allocation evidence

Base: capability PR #25, commit c71f8ddf9e9623e5bcc2ba81e5309c6704e08394. The audited upstream baseline remains v1.1.5 276f72a4e6ea8a12499a7a6819aadf3165126755. The pinned environment from baseline evidence is reused; no upstream upgrade.

## Checks

Target build and full host suite PASS. The default target remains four tracks, with original golden renders/CPU/target references unchanged. Image 447,924 B (+16); general RAM 92,116 / 98,304 B and pool 331,332 / 344,064 B unchanged. Package 610,086 B; loader 6,930 B and unchanged hash. Full binary/package and reference hashes are in results.json. No binaries are committed here.

The real eight-track allocator/renderer/MIDI sustain test PASS with NPART=8 at O2 and ASan/UBSan O1. It checks eight protected notes plus a ninth, release/chord/bass/unison/drum priority, sustain ownership after stealing, completed releases, 10,000 bursts and every enabled engine/voice mode. It verifies active counts and summed actual track_render engine calls never exceed eight. The full runner's first new test pass predates the all-engine extension; final dedicated O2/O1 logs contain the extension and prove it separately.

See ../../voice-allocation.md for musical policy and every no-room path. Immediate retirement in expanded builds bounds actual rendering without an overlapping steal tail. It may produce a click: this does not claim release sound quality or target ISR deadlines. Target eight-track runtime/memory enablement is separate #9/#12 work. Per-track destinations do not grant eight voices per track.

The first focused test failed an expectation that a protected bass would be stolen while a non-bass chord tone was available. The allocator correctly selected the higher-priority chord tone; the assertion was corrected. Its burst assertion also depended on the earlier failure counter; this now has its own result. initial-focused.log preserves those failures. Later focused, full and sanitizer results pass.

Optional skips remain explicit: DaisySP reference, repo-root V15 filename and Emscripten. The separate verified vendor V15 simulated restore is recorded in issue #5 evidence, not asserted as hardware recovery. Host ASan/UBSan uses the upstream exclusions for DSP arithmetic/XIP rebasing; no new sanitizer exclusions were added. No flash/install, loader/boundary changes, release, merge or deployment occurred.
