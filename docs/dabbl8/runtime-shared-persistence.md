# Runtime shared-pool persistence

`d8p1_pool_runtime.c` connects the real eight-track runtime capture/adoption API to the selected shared storage engine. It is compiled into the experimental eight-track translation unit; the default four-track app remains unchanged. The backend is still a callback interface, not a physical driver. No project menu, editor command, autosave scheduler or first-time migration calls these entry points yet. Keep issues #11/#13/#14 open.

## Entry points and preflight

- `d8p1_save_pool(backend, object)` captures coherent stopped live music into the existing display/staging arena, then commits through the shared pool. Objects 0–2 are projects and 3 is autosave. Manual current-slot identity changes only after success; autosave capture preserves it. A postcommit read error can leave a new record: rescan before retry.
- `d8p1_load_pool(backend, object)` validates the stored set, stages the selected wire and adopts it through the existing guarded runtime API. Successful project recall publishes its slot. Autosave recall uses `PROJ_NO_SLOT`. Failed operations preserve live tracks, engine patches, arrangement cache, transport, UI/undo and current slot; pending start requests remain pending.
- `d8p1_restore_pool_autosave(backend, allowed)` performs no access when the caller's boot/recovery/RESTORE LAST policy disallows restoration. It is an explicit restore entry point, not an automatic boot hook or autosave scheduler.

All return `D8POOL_*` status codes. Caller must provide already explicitly migrated native storage, synchronous main-loop execution with interrupts enabled, immutable backend state and serialized drawing/editor/backup operations. Backend callbacks cannot reenter drawing/runtime code or alias the staging arena.

The controller refuses blank storage instead of silently initializing it. Legacy, unknown/future and ambiguous metadata prohibit use. It validates the CRC, bounded D8P1 parser and native project decoder for **every current object**, so CRC-valid unknown optional payloads also prohibit adoption/overwrite. The available reference mask derives from the actual validated stored project set; a damaged/missing slot is not counted. A newly saved project may reference its own new identity. Other references need an existing validated project. Historical fourth-project references are representable offline but refused by this three-project persistent controller, without remapping or dropping their original data.

Preflight uses the existing arena and no persistent allocation. It fences outstanding LCD pixels before staging, then checks transport again. A wrapper combines the actual transport/drawing state with the backend's stopped policy before every erase/program. A late start preserves all committed predecessors and is not cancelled. These checks do not establish physical flash stall/IRQ timing; the backend must implement actual safe hardware access.

## Evidence and scope

`tests/d8p1_pool_runtime_test.c` uses the real eight-track UI/runtime, asynchronous LCD ownership and guarded virtual NOR. It exercises complete music save/recall for all four logical objects, explicit autosave boot policy, separate current-slot semantics, arrangement retention through drawing, every save-operation and late-start boundary, legacy/blank/future/unknown-payload refusal, fourth references, missing corrupted references and busy/read-error preservation.

Optimized and upstream ASan/UBSan configurations each pass 93016 checks, including 30 mutation cuts and 29 late-start cuts. Runtime sanitizers retain the documented upstream DSP exclusions; separate codec/pool tests still use strict full address/undefined-behavior checks. No original fixture or golden is changed. Initial synthetic test incorrectly used the note-count field as pitch; structural encoding correctly refused it. That failure log and the correction are retained.

Actual JieLi links retain the native save/load/restore and pool inventory symbols. The eight-track app is 453728 bytes, RAM 95716/98304 and pool 334100/344064; `.ram_text` stays 916 instructions with no calls. Default app is byte-identical at 447924 bytes, RAM 92116/98304 and pool 331332/344064. Static fit is not target deadline or stack qualification. Controller prologues expose 116-byte save and 100-byte load frames, plus a 36-byte preflight frame; parser/CRC/pool/backend/caller/IRQ descendants remain to be bounded and physically measured.

Exact source hashes, target memory/symbol/disassembly excerpts, artifact hashes, full regression logs, test commands and explicit omissions are retained in `evidence/2026-10-10-runtime-pool/`. Experimental eight-track images remain local outside Git and are not installable release offers.

## Next integration steps

1. Review an explicit migration/export workflow preserving four historical projects, autosave, settings, presets and samples before any destructive operation; fourth references must be refused or separately reviewed with originals retained. Prepare a native initial record only after that workflow, never through automatic mounting/erasure of legacy bytes.
2. Bind the existing noncontiguous approved blocks through a qualified backend. Prevent legacy writers from concurrently using the repurposed project/autosave allocations. Keep loader and boundaries unchanged.
3. Replace current four-slot cache/menu/editor paths with three native project identities, bounded previews/transfers and truthful capability reporting. Connect actual boot and quiet-time autosave policy; capture/storage errors must not publish false success.
4. Complete full call-chain stack, physical timing, power-loss, backup/recovery and second-tester evidence before releases or installation claims. A separate instruction is required before flashing, merging, publishing or deploying.

SPDX-License-Identifier: GPL-3.0-only. Preserve Felucca and dependency credits/notices.
