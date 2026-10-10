# Native shared autosave session policy

This is a stopped/main-loop adapter for issue #13, based on PR #74 (`fd2c1a458190b21f82f9dca69e4b3eae0c86f93c`). It implements policy over the canonical eight-track signature and the existing shared native object 3 writer. It does not initialize or migrate storage. No production caller starts a session: an original-preserving migration coordinator must explicitly authorize a completed migration and establish the existing native frontend first. This is an implementation candidate, not device-qualified autosave or a release.

## Contract and startup

`d8p1_autosave_session_begin(completed_migration_authorized, clean_boot)` requires explicit authorization, online existing native ownership, and available flash. The synchronous APIs run in the main loop with interrupts enabled and without callback reentry. A false authorization or unbound/offline frontend refuses before storage access. The authorization parameter is a trusted caller contract, not cryptographic proof of migration.

On a clean boot with RESTORE LAST enabled, begin invokes the existing stopped restore adapter. Otherwise it leaves current RAM intact. Successful initialization baselines current canonical music without writing; a later edit is required to trigger autosave. This matches the inherited clean-boot/RESTORE LAST policy. Restored autosave retains its saved name and has no manual slot identity. Ordinary autosaves retain the current manual slot and name. Initialization can restore successfully and then fail during subsequent signature/readback verification: a failure does not promise rollback of that already completed restore. No session becomes ready on failure.

`end()` revokes only session readiness, never native storage ownership. `hold()` restarts the quiet interval and is called by the existing transfer hold path. The main-loop autosave route calls the new poll adapter only on an already-owned native frontend; historical four-track routing remains unchanged. Binding the frontend alone does not begin an autosave session.

## Scheduling and saved-state truth

The policy polls at 250 ms, waits 10 seconds after an observed change/hold or activity, enforces a 60-second gap after verified successful replacement, and waits 30 seconds before retrying a verified failure. All time intervals use unsigned subtraction through clock rollover. OFF disables work; OFF/ON restarts the quiet interval without clearing an established wear gap. Actual stopped transport, held notes, engine residents/tails, canvas borrowing and logical DAC/UAC queues are checked before signature and storage work. The existing automatic writer retains its late-access and IRQ-off driver guards.

A signature failure restarts quiet time and cannot mark invalid music clean. The canonical signature excludes only selected track and historical UI slot/name/load/save fields; track, pattern, sound, automation, effects, arrangement and stored-base state remain included. FNV32 is a practical change detector and has theoretical collision risk; it is not a cryptographic integrity proof.

Before an attempt, a verified read-only snapshot identifies current object 3. The writer captures actual state itself, so its committed capture may differ from the pre-save signature. After every attempt, even an OK result, the policy reads and canonicalizes the actual committed record and compares its sequence against the pre-attempt record. Only that persisted identity becomes clean. A live edit after capture remains dirty. A verified new sequence counts as a replacement across sequence rollover. The successful gap begins when replacement is verified, conservatively delaying retries after a late reconciliation.

A postcommit I/O error or late activity is uncertain, not a definitely failed write. Pending uncertainty blocks physical retries until a valid rescan resolves the current object set. Rescans are throttled to 30 seconds; persistent failure stays pending. A valid unchanged prior record resolves an unsuccessful attempt and permits the failure retry schedule. The public poll result can be OK for a no-op or verification and is not a new-write notification. Session `saved` starts as a RAM baseline, so that internal field alone is not proof of persistence.

## Read-only snapshot API

`d8p1_autosave_snapshot_pool` validates the complete native object set, loads and decodes object 3, normalizes canonical UI fields and publishes its FNV32 signature plus exact committed record only on success. Missing object 3 returns EMPTY. Null, arena-overlapping or mutually overlapping outputs refuse before pool access. No live music, cache, slot or name is adopted.

`d8p1_autosave_snapshot_flash` opens the existing stopped mapped backend and delegates to the pool API. Opening the mapping can scan storage before pool-level output validation; this is read-only. Mapped reads recheck stopped/canvas state, while the scheduler checks the full automatic activity guard before starting readback. Late audio activity during read-only verification cannot write or adopt live state; physical writes retain the stricter automatic guard.

## Verification and limits

See [retained evidence](evidence/2026-10-10-native-autosave-session/README.md). The new test uses actual runtime, canonical codec, native frontend, mapped driver and writer against virtual NOR, synthetic time, and the actual logical audio/UAC queue fixture. It covers all physical erase/program cuts, failed signatures, enable/hold/boot policy, rollover, late capture/live edits, persistent read failure, postcommit transport start and output alias refusal. Synthetic queue injection and cold fixture reset are test techniques, not proof of physical DMA drain or device timing.

The default image must retain pinned baseline SHA-256 `b69e74e98721ca1f1124edec114c824e315102c65e1f89bf437619d8f048387b`. The eight-track target remains within static memory limits. This does not qualify flash latency/endurance, physical DAC/UAC quiet, interrupt/caller stack, power cuts on hardware or eight-track audio timing. The inherited sanitizer exclusions remain explicit; this is not a whole-engine strict UBSan claim. No firmware was installed or flashed. The separate signature optimization is not included in this parent-based candidate.
