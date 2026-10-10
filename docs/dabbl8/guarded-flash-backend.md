# Guarded existing-driver connection

The eight-track build now has an adapter from native runtime persistence to the existing NOR driver. It uses the five allocations selected in issue #7, without changing the loader, driver, application/staging boundaries, settings, preset or sample allocations. This prepares issue #13; it does not execute migration or qualify an install.

`d8pool_mapped` translates virtual blocks 0–3 to `0x97000`, `0x99000`, `0x9B000`, `0x9D000`, and block 4 to `0xE5000`. Each block is 8192 bytes. Reads crossing block boundaries split before calling the physical callback; they cannot read the gap before the fifth block. Overflow-safe range subtraction, sector-aligned erases and bounded single-page programs prohibit access outside this set.

Opening a session requires both explicit `native_authorized` policy and a successful complete inventory containing at least one native current record. Authorization is **not inferred from recognizing a header**: the caller must already own a separately approved, completed native migration. Every open, including refusal, revokes the preceding session. Blank, legacy, committed foreign/future, ambiguous or unreadable inventories do not authorize initialization. Close revokes all callbacks. The caller owns a synchronous serialized session; callbacks must not reenter or replace it. Current native project semantics/references are validated by the existing runtime controller before capture or adoption.

The eight-track `FELUCCA_FLASH` adapter exposes `d8p1_save_flash`, `d8p1_load_flash` and policy-controlled `d8p1_restore_flash_autosave`. It checks the existing JEDEC-ready flag and actual transport/display state and delegates to the unchanged `st_read`, `st_erase` and `st_prog` hooks. The runtime capture supplies immutable RAM payloads; direct mapped-backend callers must honor the physical driver's RAM-source requirement. A rejected/failed save does not publish a new current slot. A postcommit error can still leave a valid new record; rescan before retry.

No current boot, menu, editor or autosave caller supplies migration authorization. These entry points do not mount legacy data, create a blank pool, erase a fourth original project or grant ownership after finding one native record. Original capture/provenance, complete qualified restore and an interruption-safe migration executor remain required. The [offline migration proposal](migration-bundle.md) supplies retained originals and verified proposed pool bytes, not a device-write authorization.

## Transport and interrupt contract

Each physical erase/program callback disables interrupts and rechecks actual stopped state before entering the existing driver, closing the interval in which a TIMER5 event could request PLAY after a preceding check. Erases cover one sector; programs cover at most one page with no page crossing. The next mutation must pass a fresh stopped check.

Upstream's `irq_save` returns zero and `irq_restore` unconditionally re-enables interrupts. They do not preserve nested interrupt state. Call these APIs only from the serialized **main loop with interrupts enabled**, never from an IRQ or an enclosing critical section. The underlying single-sector/page operation may re-enable interrupts before the wrapper returns, after that operation's mutation is complete. This is not evidence for general nested-critical-section correctness, target latency or live saving. No upstream IRQ helper was changed.

Concurrent editor/backup/update operations and display/workspace ownership must be serialized by the eventual integration. Unconverted legacy project/autosave paths must not write the newly owned pool after native migration. No integration policy or destructive migration command is provided by this adapter.

## Verification and remaining integration

[Evidence](evidence/2026-10-10-mapped-backend/README.md) records actual native runtime round trips through simulated physical hooks and standalone mapping/session checks over a guarded 1MiB NOR model. Every erase/program cut, a PLAY event at each write critical-section entry, inventory read errors, invalid ranges/pages, unauthorized/blank/legacy sessions and rotating writes are covered. Protected settings, presets, all samples, loader/application/staging and other regions remain byte-identical in that model. Host tests are not actual SPI NOR, DMA, power-cut or audio timing qualification.

The linked eight-track image is 454616 bytes, SHA-256 `6156abeb3c64ba3c83343acd69f7b1e7380778526df171b93cc03661684679d1`. RAM remains 95716/98304 bytes and pool remains 334100/344064 bytes. `.ram_text` remains 916 instructions with no calls. The default four-track application remains byte-identical to the pinned baseline. The new adapter adds 888 bytes to the prior eight-track image and no retained state allocation. Local adapter/session prologues still do not establish a complete callback/caller/IRQ stack bound.

Next work is three-slot project menus/cache/transfer, stopped autosave scheduling and boot recall with an explicit migration ownership policy, then authorized physical migration/recovery and device qualification. Issue #13 and its #12 prerequisite remain open. No firmware was installed or offered as a qualified download; no loader/boundary/golden changes, merge or release was performed.
