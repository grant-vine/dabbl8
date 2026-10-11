# Dependency-integrity change review

Snapshot: uncommitted five-file patch based on main 1d47ebee17c72b933008524b6cabbfb138da3e72 in the isolated dependency-integrity worktree. Adjacent SHA-256 manifest pins the reviewed files. Review was read-only; no builds, downloads, project edits or test execution. `git diff --check` passed.

## Result

No source correctness blocker identified in the core verifier at this snapshot. The concrete dependency-provenance gap identified in the prior audit is addressed: verified archive content is compared against every reused extracted entry, SDK populated tracked files are compared to HEAD blobs even when Git index flags hide changes, and run_evidence refuses compiler-version invocation and build on a verification error.

Sparse checkout handling is coherent: missing skip-worktree entries may be omitted only if not required; each of the actual three package SDK inputs is required to be tracked and populated. It does not treat a populated skip-worktree file as trusted. Binary Git-blob calculation includes object header, size, contents and executable type; symlink text and escape rules are checked. Existing user files are not repaired, reset or deleted by the verifiers.

## Required validation integration gap

At this snapshot, `tests/dependency_integrity_test.py` has no caller in `tests/run_tests.sh`, `.github/workflows` or the evidence drivers. A green hosted run would exercise the normal verifier on genuine dependencies but would not execute the new mutation/hidden-edit regression cases.

Recommendation: explicitly run the lightweight fixture tests before acquiring target dependencies, or add a named evidence-driver check. Validate that changing an existing compiler/library/SDK byte refuses before a compiler sentinel could execute. This is a test integration gap rather than proof of a broken guard.

## Useful test additions

- Existing SDK executable-mode and symlink changes, including external symlink escape; current archive tests cover analogous cases but SDK tests do not.
- Plain dirty tracked SDK files/staged changes. Existing SDK test checks wrong HEAD, untracked files and hidden index edits, but does not directly exercise an ordinary tracked dirty refusal.
- Archive path escape, duplicate entry and unsupported types. The verifier checks these paths; current tiny archive fixture only creates regular files, directories and an internal symlink.
- Integration-level evidence receipt on refusal: ensure failure emits a structured result and compiler is never invoked, rather than testing the helper in isolation.

These are targeted coverage suggestions; they do not require another full firmware build for each fixture.

## Non-blocking disk lifecycle caveat

`ci/run_browser_conversion.py:24–29` now creates a fresh full Emscripten tree per invocation and does not remove it afterward. This avoids trusting existing user/compiler/cache work, but repeated local runs can accumulate gigabytes of invocation-owned extraction folders. A `finally` cleanup limited to this newly created path after logs/results are retained would preserve existing SDK work. If the tree is deliberately retained as evidence, record that policy/path and provide a bounded cleanup mechanism. This is a local resource-management issue, not a compiler content-verification defect.

## Remaining scope

The change does not fix earlier-run logs retained in reused evidence directories, diagnostic package naming or native timing budgets; those are separate findings. No claim is made about hosted execution success, hardware timing or concurrent external edits after verification. The routine fresh-run and corrupt-reuse guards should be verified before merging.
