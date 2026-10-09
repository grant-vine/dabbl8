# D8-017: CI and evidence reporting

The draft `dabbl8-evidence.yml` workflow runs only package builds and host/simulator checks on pull requests or manual workflow dispatch. Repository token permissions are `contents: read`; checkout does not persist credentials. It contains no device, publishing, push, release, merge or deployment step. Changes still need review. Successful automation does not establish hardware performance or recovery.

## Dependency inventory

`ci/dependencies.json` records Python 3.14.8, Pillow 12.3.0, fontTools 4.66.1, Node 26.11.0, the exact JieLi archive SHA-256, SDK tag/peeled commit, and linux/amd64 container digest used by the verified Mac baseline. Python packages install into an isolated job-local virtual environment. Dependency downloads never include stock/vendor firmware.

The hosted runner is Ubuntu 24.04, with host Clang 18.1.3 packages pinned to `1:18.1.3-1` (the [official Noble package](https://packages.ubuntu.com/noble/clang-18)). This differs from the Mac's Apple Clang 21; host compatibility and timing claims must be scoped accordingly. Runner image/system libraries can receive updates; actual compiler and OS-package versions are captured, so this is not a claim of a fully bit-identical hosted OS. Target compilation uses the pinned JieLi compiler inside the immutable Debian image on linux/amd64. The job requests the exact digest through [Google's public Docker Hub cache](https://docs.cloud.google.com/artifact-registry/docs/pull-cached-dockerhub-images). Both cache address and original Docker Hub address are recorded in the lock. The locally verified cached manifest has the identical SHA-256, so this changes acquisition location, not compiler/container contents; no daemon configuration changes are needed. Cache availability is not guaranteed, and an unavailable image remains a setup failure.

Official checkout/setup-python/setup-node/upload-artifact actions are pinned to immutable commits, resolved from their official release tags. `ci/bootstrap.py` verifies the downloaded archive before extraction and refuses an unexpected hash or SDK commit. The toolchain URL names the audited vendor archive directly. The initial hosted attempt used the upstream moving redirect and correctly refused its changed archive. The version-specific URL was then independently downloaded and matched the original SHA-256; compiler/version pins remain unchanged. A changed archive at either address must fail instead of silently upgrading. Actions/setup services and package availability can also fail; dependency acquisition failures are visible and must be resolved without widening pins casually.

Generated tables, patches, UI fonts/icons, samples, menus and keycaps are produced by the ordinary reviewed `build.sh` path. The standard runner checks serialization, historical fixtures, motion, protocol/editor pairing, audio goldens, voice budgets, UI, simulated persistence/recovery and sanitizers/fuzzing. Target disassembly budgets remain separate from host timing. No golden, CPU or target-budget update flags are allowed by the evidence driver; reference-file hashes are compared before and after.

## Evidence and qualification

`ci/run_evidence.py` records each command's exit status, raw logs, actual dependency versions, SDK commit, compiler/container identity, source/worktree status, reference and package hashes, and explicit skips in `build/ci-evidence/results.json`. A failed target build skips host tests with a reason. A failed workflow setup produces a failed preflight and skip, not an apparent pass. Required sanitizer skips fail the job.

Artifacts contain review evidence and UI reports, not stock firmware, private backups, secrets or installable release downloads. Package files are built and hashed locally in the job but not uploaded as downloads. Evidence expires after 14 days; a qualification PR must preserve relevant text manifests/logs in the repository. Failed job setup before the driver can start remains authoritative in GitHub job-step logs.

Optional upstream skips are explicitly retained: DaisySP comparison, stock V15 filename and Emscripten emulator. Linux lacks the Mac `proc_pid_rusage` instruction counter, so the host CPU budget is explicitly unmeasured there. The Mac baseline and change-specific Mac checks retain that separate evidence. Stock restore simulation has already passed locally with the hash-verified vendor file outside Git; CI does not acquire or upload that file.

Hardware qualification is always recorded as SKIP/not performed. USB/TRS physical routing, target ISR deadline/stack/pool high-water, audible steal quality, recovery on real hardware and release installation need their own evidence and authorization. A green host/package result cannot close those gates.

## Current verification status

Hosted run37991665831 passed dependency setup, package build, complete host regressions and artifact upload; downloaded raw results were inspected and preserved in evidence/2026-10-09-ci-hosted. Inspection also found omissions missing from the original structured skip list; the corrected reporter and graphics-library/asset provenance require verification in a subsequent hosted artifact. Issue #18 remains open until that verification. Linux and Mac package bytes differ, so no cross-platform bit-identity is claimed. See the issue for live run evidence.
