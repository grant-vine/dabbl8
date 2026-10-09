# Eight-track target memory preflight (not qualification)

The tested allocation commit 981262a91b924db3aa2ad0a481d8dc7da75bd5c3 was checked out in an isolated detached worktree. The pinned baseline Python, SDK, JieLi compiler and linux/amd64 Debian image were reused. The only override was appending -DNPART=8 to imported tools.build.CFLAGS in the running Python process. No source/loader/linker/map changes were made; probe working-tree status is clean. The main target and its four-track build outputs remain untouched.

Target compilation generated an object but linking FAILED. The linker reports RAM overflow 34,436 B and POOL overflow 106,960 B, including overlap with protected regions. No valid eight-track image or package was generated. Existing capacities MUST NOT be widened to silence this failure.

| Resource | Verified four-track build | Eight-track probe requirement | Capacity | Probe overflow |
|---|---:|---:|---:|---:|
| General RAM | 92,116 | 132,740 | 98,304 | 34,436 |
| Engine pool | 331,332 | 451,024 | 344,064 | 106,960 |
| Legacy retained section | 14,796 | 14,796 | 15,696 | 0 |

This is static compile/link accounting, not stack/pool runtime high-water or target deadline evidence. Unchanged legacy retained storage fits because it still uses its frozen four-track layout; it is not eight-track project persistence. Main four-track binary SHA-256 still matches the completed issue #8 evidence: 651b8654c092df70289cd159bb7462d8fce0699772e24d0be80ed9093c563d6f.

The object symbol comparison identifies the largest growth: phys_slot +51,552 pool bytes, sl_buf +32,768, gr_p +28,080, drum_kit +7,296; chain +11,424 general RAM, fm6_note +7,104, trk +7,088, proj_scratch +4,128, slc_rbuf +4,096 and drw_v +2,944. Full symbol-growth JSON and raw section/symbol tables are retained. Candidate pooling/reuse needs an actual design and validation, not removal of engines to make this probe green.

The toolchain has no common/bin/nm; the first symbol-dump attempt failed with exit127 and is retained. Bundled common/bin/objdump -t succeeded instead. The first comparison script expected an extra symbol-kind column and emitted an empty result; it was corrected to parse the actual object format, and symbol-growth.json now contains real differences. No source/firmware regression changed for this evidence-only work.

Reproduce in an isolated checkout with the pinned env: python -c "import sys;sys.path.insert(0,'tools');import build;build.CFLAGS.append('-DNPART=8');sys.exit(build.main())". It is expected to fail linking at this commit. Dump build/felucca.o sections/symbols with the bundled common/bin/objdump -h/-t inside the same container. Do not flash these objects or change linker boundaries.

Issue #9 cannot claim a usable eight-track target from the passing host tests. Preparatory #12 memory work must make the target fit before runtime enablement can complete; the final #12 acceptance still depends on #9 and runtime/hardware measurements. Storage-map selection (#7) also remains pending. These planning dependencies are recorded as gates, not completed or silently bypassed. No task is closed by this preflight; no release/hardware claim is made.
