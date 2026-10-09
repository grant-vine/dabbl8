# Successful hosted CI evidence and reporting correction

[Run37991665831](https://github.com/grant-vine/dabbl8/actions/runs/37991665831), job114027133684, completed successfully on source head68b700c38f8ee77b5f529d1b54b031c4f3ce3e30. Target package build, complete host suite, golden renders, historical four/eight-destination fixtures, D8M1, real eight-part voice tests, editor/backup checks, ASan/UBSan and all three fuzz tests PASS. Raw output ends ALL HOST TESTS PASSED. No golden/CPU/target baseline or loader hashes changed.

Artifact11645731155 (107972B) was downloaded through the authorized GitHub API and verified against zipSHA256 73c71a8b8fcaae0d871526d9401e6f3aa9fb0af91a02cbb7597a4a48a85bc704. The connector's temporary file URL returned403; direct GitHub download succeeded. No signed download URLs or credentials are preserved. Original artifact manifests/logs copied here remain unchanged.

The pinned Python3.14.8,Node26.11.0,Pillow12.3.0,fonttools4.66.1,Clang18 packages, SDKd179b4484759423312073f5fbb232501aa491047 and exact target toolchain/container pins were verified. Public-cache acquisition uses the identical container digest. Prior three setup failures and the exact-baseline acquisition fixes are recorded in ../2026-10-09-ci-preflight.

Image447940B,package610086B,RAM92116/98304B,pool331332/344064B. This Linux image differs by16B from the Mac's447924B image; raw generator output shows keycaps5442B versus Mac5429B. Do not claim bit-identical cross-platform packages. Generated graphics/library versions and asset hashes are added by the subsequent reporting revision to make such differences diagnosable. Both builds retain the same loader and golden audio hashes. No installable packages or vendor firmware are included in this evidence.

## All check omissions, including correction

The original structured manifest recorded only three omissions. Inspection of the complete raw log found thirteen actual skipped checks: nine missing Mac instruction-counter measurements (MOD,SLICER,PERFORM,REVERB,regression,PHYS,DRUM,NOISE,FM6), DaisySP comparison, vendor V15 restore, editor MENU UI, and Emscripten emulator. Their exact lines are in skip-parser-check.json. Expected behavior such as skipping retired presets or optional import blocks is not a skipped test.

The reporter has been corrected against this actual log; all13 omissions and9 missing-counter checks are detected, without misclassifying the tested preset/import behaviors. The original PASS manifest is not rewritten. This reporting correction still needs its own hosted artifact verification before #18 closes. Separate Mac CPU and stock-restore simulation evidence remains available from earlier tasks; no corresponding Linux/hardware measurement is invented.

Hardware qualification is SKIP/not performed. Source runtime still four tracks; real eight-part host allocation success does not resolve the target-memory failure recorded in PR29. No flash/install, loader/boundary changes, merge, release or deployment occurred.
