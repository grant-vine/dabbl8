# GitHub and licensing plan

Dabbl8 remains GPL-3.0-only as a Felucca derivative. Preserve upstream notices, history and LICENSES. Retain file-level MIT, Apache-2.0, OFL and CC0 provenance where applicable. Felucca’s own generated drum material is GPL, not universally CC0. Independently audit each borrowed fork file and its transitive dependencies before copying. This is a release engineering plan, not a legal opinion.

Publish source that corresponds to every distributed binary, including patches, build scripts and required notices. Track the three SDK files embedded by upstream packaging; distribute their Apache notices as required. Keep vendor firmware out of the repository and releases. User-supplied stock firmware may be verified locally for rollback without uploading or republishing it.

Repository layout should retain upstream firmware/, tools/, tests/, web/, assets/ and LICENSES/. Add docs/architecture.md, roadmap.md, compatibility.md, recovery.md, release-checklist.md, decisions/ and benchmark fixtures. Use an upstream-tracking branch and small topic branches. Credit donor commits in PR descriptions and a provenance ledger. Offer generic bug fixes upstream without making Dabbl8 depend on acceptance.

CI proposal: host parser/serialization tests; legacy fixtures; all-track addressing; audio regression and bounded allocator tests; editor protocol tests; emulator build when its dependencies are available; firmware package build with pinned toolchain/SDK; license and secret checks. GitHub Actions are proposed additions, not an assumed working upstream pipeline. Target hardware profiling remains a manual gate or an explicitly managed test rig.

Release assets should contain firmware, checksums, corresponding source snapshot, build manifest, license bundle, schema compatibility notes and recovery/migration instructions. Use clear alpha/beta/stable labels. Pin third-party CI actions and container images; minimize workflow permissions; never expose tokens in logs or publish private user backups. No contributor or account credentials are needed for this research pack.

Fork identity requires coordinated changes to product name, package ID generation, USB strings, updater recognition, editor protocol family and URLs. tools/build.py can override an environment-only identity change for release builds. Do not invent an assigned USB VID/PID or accidentally impersonate upstream releases. Keep the loader compatible until the update path has explicit coverage.

Before publication, confirm the owner/repository name, public release intent, license inventory, tested board/host matrix, support expectations and donation/branding policy. The companion backlog is an importable planning seed; it has not created GitHub issues or a repository.
