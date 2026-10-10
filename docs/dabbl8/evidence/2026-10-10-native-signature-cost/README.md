# Native signature cost evidence

See [scope, measured costs and target tradeoffs](../../native-signature-cost.md). Executable source commit `5874d17` is based on exact PR #71 head `268981e672ae41ee12d96996a14b9f7e60437104`. Tests are focused host/runtime validation, not a scheduler or installed firmware.

The equivalence test adds 2317 checks and the unchanged signature/runtime tests pass 13774/8911 checks in optimized and inherited sanitizer modes. Raw parent/optimized costs have 72 batches each, all case signatures match, and full-count/stored-base cases pass. Default target instruction budgets pass; native parent and optimized reports identically fail existing reverb budgets. No thresholds/goldens were changed. The initial mutable-baseline comparator failure is retained and explained in the scope document.

Private raw probes, pinned-parent source archive and target ELF/disassembly are retained outside Git under `.local-baseline/native-signature-cost-opt-evidence`. The original earlier cost audit under `.local-baseline/native-signature-cost` remains intact. Public source hashes and retained evidence hashes are in manifest.json. Full new-head CI, physical timing/stack, MIDI/backup/recovery and scheduler gates remain unqualified.

SPDX-License-Identifier: GPL-3.0-only.
