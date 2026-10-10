# Retained setup attempt notes

These notes transcribe observed tool results; they are not presented as raw compiler log files. Final freshly compiled source checks supersede earlier failed setup attempts without discarding their occurrence.

- Initial C compile failed with `fatal error: felucca_tables.h file not found`; generated inputs had not yet been copied into this new isolated worktree. Fixed by copying the parent's pinned generated inputs.
- Next link failed for `_ota_idle` and `_ota_now_ms`. Fixed by adding test HAL time/drain stubs; production helpers were unchanged.
- A later test-only assertion attempted nonexistent `panel.bias[0]`; compile failed with `no member named bias in panel_t`. Corrected to the actual `panel.dir[0]`. The same shell invocation subsequently printed the prior 55,614-check binary result; that stale-binary output is not counted as validation of the edited source. Fresh compilations under `set -e` subsequently passed 67,341 checks optimized and inherited SAN.
- Invoking the positional collector test with `--help` failed because it treats its first argument as a bridge executable path. Reran with the actual bridge path; no collector-source behavior was changed for this invocation error.
- Independent collector integration initially sent contiguous frame hex to the agreed space-separated bridge. Its raw failure is retained in `initial-hex-framing-failure.log`; the adapter was corrected, and final immutable normal/SAN runs each passed 287 checks.

Target builds and full-suite qualification are tracked separately; no setup failure here is labeled a target or physical-device failure.
