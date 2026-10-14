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
- Use the approved eight-track emulator at commit
  `04ba2469bed14847de51adb471d74159d2ee3614` as the behavioural reference for fixes;
  follow `web/emu/NATIVE8.md`'s baseline section. Preserve reference evidence,
  write candidate outputs separately, and qualify target timing/hardware independently.
- Preserve explicit failures and skips. A cost-cancelled hosted job is not proof of
  either a code failure or a pass. Host checks do not establish device safety.
- Keep local logs and generated evidence outside tracked source, under `build/`
  or `.local-baseline/`. Never commit vendor firmware, credentials or private backups.
- Do not rewrite golden audio hashes or budgets to force a pass. Preserve licenses.

## Install and data compatibility policy

- First installation is a clean reset of the previous firmware's user data.
  Clearly disclose that projects, autosave, settings, user presets and user samples
  are reset. Sample-slot capacity remains; previous sample contents do not.
- Do not implement or qualify migration from vendor, Felucca or other firmware
  as a release prerequisite. Existing converters are historical/developer tools.
- Preserve recognized Dabbl8 data on normal reboots and firmware upgrades. Support
  project/version migration only for Dabbl8's own formats when needed. Unknown or
  damaged Dabbl8 formats must not be silently treated as a fresh installation.
- Make first-use formatting restartable after power loss within the approved
  user-state map. Do not alter loader, OTA, calibration or flash boundaries.
- A clean-install policy is not permission to flash a device. Installation and
  physical erase/write operations still require a separate owner instruction.
