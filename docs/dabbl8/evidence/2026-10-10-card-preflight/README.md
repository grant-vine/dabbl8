# Candidate card preflight evidence — 2026-10-10

Source base: `432e9d5ddab59669334c295068f1edb7854cd7b4` (PR #52), with the source changes hashed in [manifest.json](manifest.json). These are the first standalone preflight/configuration changes for [issue #53](https://github.com/grant-vine/dabbl8/issues/53), not a qualified reduced-engine firmware variant.

## Results

- Test-first compilation failed because the implementation/header did not yet exist; preserved in `test-first-red.log`.
- C optimized and strict ASan/UBSan: **79,881 checks each**, exit 0. Includes every subset/component combination, current stable identities, malformed/truncated project refusal, immutable source and output alias checks.
- Candidate CLI: **16,407 checks**, exit 0; all 8,191 nonempty subsets, configuration refusal and deterministic output.
- Pinned JieLi compiler accepts the freestanding preflight object.
- Default four-track app-only rebuild: **447,924 bytes**, SHA-256 `b69e74e98721ca1f1124edec114c824e315102c65e1f89bf437619d8f048387b`, byte-identical to the preserved default build. RAM `.data + .bss`: **92,116 / 98,304 bytes**; pool **331,332 / 344,064 bytes**; `.ram_text`: **916 instructions, no calls**.
- Full local upstream suite: **ALL HOST TESTS PASSED**, exit 0. All 92 golden renders unchanged, with no render-health/voice-limit failures, CPU overs or crashes. Package, loader, golden audio hashes and CPU/target budgets remained unchanged; see `unchanged-artifacts.json`.

## Scope and omissions

`d8card.c` is not linked into the shipping app. Generated candidate macros are not consumed by the build. These tests establish configuration and project-engine preflight, not runtime enforcement, actual engine exclusion, pool reduction, project reference/asset validity or hardware timing.

The suite explicitly omits optional DaisySP comparison, official V15 restore (vendor firmware remains outside Git), embedded MENU editor settings and the browser emulator (`emcc` absent from this environment). **Hardware tests, firmware installation and recovery are SKIPPED**; no device was flashed. Synthetic fixtures do not replace real cross-card user-project qualification.

## Environment and reproduction

Mac M1 Pro / 32 GB, macOS 26.7.1; Apple clang 21, Python 3.14.8, Pillow 12.3.0, fonttools 4.66.1, Node 26.11.0. Python uses the isolated `.local-baseline/venv`.

JieLi toolchain archive `20250324.1` SHA-256: `f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958`; compiler clang 4.0.1. AC79 SDK: `d179b4484759423312073f5fbb232501aa491047`. Docker linux/amd64: `debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587`.

[verification-command.py.txt](verification-command.py.txt) records the actual local commands and isolated paths; [checks.json](checks.json) records exit codes. Logs are retained alongside this file. The artifact equality assertion compares hashes before and after the suite. Source hashes identify the exact tested implementation independently of subsequent documentation edits or the Git commit containing this evidence.
