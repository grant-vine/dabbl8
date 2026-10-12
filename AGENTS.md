# Dabbl8 verification and cost policy

- GitHub is for source storage and review for now. The owner requires local CI only.
- Do not enable GitHub Actions, add hosted workflow triggers, dispatch hosted jobs,
  or require a green hosted check before merging. Re-enabling hosted CI or release
  builds requires a separate owner instruction.
- Select local checks for the actual changed code. Use `ci/local_checks.py` to
  inspect the plan before executing it; see `ci/README.md`.
- Do not automatically download dependencies, build firmware packages, or run the
  complete suite for every change. Use existing isolated local dependencies.
- Reuse evidence when the relevant source and configuration are unchanged. Run
  additional checks for changed inputs, a failure, or an uncovered requirement.
- Preserve explicit failures and skips. A cost-cancelled hosted job is not proof of
  either a code failure or a pass. Host checks do not establish device safety.
- Keep local logs and generated evidence outside tracked source, under `build/`
  or `.local-baseline/`. Never commit vendor firmware, credentials or private backups.
- Do not rewrite golden audio hashes or budgets to force a pass. Preserve licenses.
