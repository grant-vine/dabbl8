# Local Work handoff

Use this in a local Work chat on the Mac when M0 starts:

> Work on https://github.com/grant-vine/dabbl8. Clone or open a clean checkout and fetch dabbl8/develop. Read AGENTS.md if present, DABBL8.md, docs/dabbl8/README.md, issue-index.md, desktop-first-build.md and the baseline build issue. Start with the baseline build issue only. Confirm my actual macOS, workspace, Docker and toolchain. Review the pinned v1.1.5 versus main v1.1.5.1 difference before changing the baseline. Set up isolated dependencies, record toolchain/SDK/container versions and hashes, compile unmodified firmware and run required upstream tests. Retain map, logs, hashes and explicit skips/failures. Do not install firmware, change loader or flash boundaries, modify golden hashes, publish a release or deploy the website in this first session. Report evidence and update the issue when the build gate is satisfied; propose the next small implementation PR.

The website repository is grant-vine/dabbl; it is not the firmware checkout. Never embed private user backups or credentials in source, issues or logs. Issue completion requires evidence, not just a successful command exit. A blocked environment should leave the build issue open with a concrete blocker and next action.
