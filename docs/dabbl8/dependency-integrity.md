# Pinned dependency content integrity

The build previously checked the downloaded compiler archive and SDK HEAD,
but could reuse changed extracted compiler files, SDK blobs hidden from Git
status, or an existing browser compiler/cache. This weakens baseline provenance.

The new read-only verifier compares every extracted JieLi file, link and
executable bit to the SHA-256-pinned archive and rejects extra entries. It checks
SDK HEAD, ordinary working-tree status and direct Git blob content for every
populated tracked path, including assume-unchanged and skip-worktree edits.
Explicit sparse omissions are accepted only outside the required build inputs:
`uboot.boot`, `cfg_tool.bin`, and `cfg/eq_cfg_hw.bin`. It never resets, repairs,
or deletes existing dependency work. Target compiler execution and firmware
builds are blocked after a content-integrity refusal.

Browser conversion now extracts each hash-and-size-verified compiler archive
into a fresh invocation-owned directory instead of trusting an existing SDK.
Its reviewed release metadata normalization remains unchanged. These directories
are currently retained, including failures; repeated local invocations consume
additional disk space. This change does not isolate the existing output/log
folders; stale-log reuse remains a separate audit finding.

## Validation on the Mac

Eight controlled regression tests pass with isolated Python 3.14.8. They cover
compiler/library/link/permission/missing/extra mutations, wrong archive digest,
wrong SDK commit, dirty/untracked SDK work, hidden tracked edits, required-file
sparse omissions, pre-execution refusal, and two browser-driver invocations
using separate fresh trees while preserving a changed previous SDK. Driver
fixtures intentionally stop before real compilation. Tests run before dependency
acquisition in CI and also in the host test runner.

The actual existing pinned JieLi archive/extraction passes: 599 regular files,
44 links, 694 child entries, no extras; archive SHA-256
`f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958`.
The actual sparse SDK passes at
`d179b4484759423312073f5fbb232501aa491047`: all five populated tracked entries
match their blobs, including all three required build files; 15,253 explicit
non-required sparse omissions are reported. This is not a verification of
omitted SDK file contents on disk.

The initial strict SDK check refused those intentional sparse omissions;
the verifier was revised to require actual build inputs and verify all populated
tracked paths. One regression fixture initially failed because its subprocess
mock conflicted with macOS platform detection; the fixture now isolates platform
metadata, and the failed log is retained. The reviewer also caught an assertion
that confused the hosted `clang-18` metadata call with target compiler execution;
it now checks the target path specifically, asserts the host capture still occurs,
and all eight tests pass with `CC=clang-18`. No target tool or device was executed
by these regression tests. No dependency pins, firmware sources, loader, flash
layout, golden hashes, test budgets, or release packages change in this patch.

Raw test results, actual-input receipts, and the independent read-only source
review are under [evidence](evidence/2026-10-11-dependency-integrity/).
The independent review predates the two driver fixtures and CI wiring it
requested; its remaining extraction-retention caveat is documented above.
