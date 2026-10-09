# CI preflight validation

This evidence exercises expected failures in the real CI reporting driver, not a hosted build. Workflow YAML parses successfully; Python scripts parse successfully. A failed setup records FAIL/preflight and an explicit build/test skip, with no stale binary/package hashes. A GOLDEN_UPDATE=1 attempt is refused with exit 1 before build/test; the actual golden file hash remains unchanged. Assertions inspecting both manifests pass.

The included manifests record those deliberate negative cases. They are not failed firmware builds or passing CI runs. Local compiler/dependency inventory differs from the intended hosted Linux job and is recorded honestly. The previously verified full target/host/real eight-track checks are in ../2026-10-09-voice-budget. No duplicate firmware regression was necessary for this reporting-only change.

Official action release tag commits were resolved directly from their repositories: checkout v4.3.1 34e114876b0b11c390a56381ad16ebd13914f8d5; setup-python v6.0.0 e797f83bcb11b83ae66e0230d6156d7c80228e7c; setup-node v6.0.0 2028fbc5c25fe9cf00d9f06a71cc4710d4507903; upload-artifact v4.6.2 ea165f8d65b6e75b540449e92b4886f43607fa02. Ubuntu compiler package version was checked against the official Noble package listing.

Issue #18 remains open until concrete hosted setup, package, regression and artifact evidence exists. No hardware qualification, dependency download success or hosted-run success is claimed by this local check.

## First hosted run and fixed download provenance

[Run 37990600752](https://github.com/grant-vine/dabbl8/actions/runs/37990600752), job114023468679, failed toolchain hash verification before SDK extraction/build. Python3.14.8,Node26.11.0,pinnedClang18packages,Pillow12.3.0/fonttools4.66.1 setup succeeded; failure/skip manifest and artifact11644288540 were uploaded. This demonstrates the refusal/reporting path, not a passing firmware build. Full job log is preserved locally in .local-baseline/issue18/hosted-first-job.log.

The moving vendor endpoint now redirects to jieli-linux-toolchains-20260730.1.tar.xz. The audited archive remains available at https://jl-update.oss-cn-shenzhen.aliyuncs.com/jieli-linux-toolchains-20250324.1.tar.xz; independently downloading it produced the original SHA-256 f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958 (26,009,040 bytes). CI now uses that version-specific URL. No compiler/archive pin or baseline was upgraded. The audited archive also passes Python's data extraction filter. A subsequent hosted run must still prove package/regression/artifact success.
