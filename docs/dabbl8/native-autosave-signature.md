# Coherent native autosave change signature

`d8p1_signature_runtime(uint32_t *signature)` is an eight-track main-loop API for future autosave scheduling. It detects canonical native musical changes, including retained banks, scenes and chain rows. The inherited `autosave_sig()` hashes the historical chain configuration and does not include the native arrangement cache; it remains unchanged for the historical scheduler.

The helper uses the existing stopped, IRQ-guarded native capture and LCD DMA ownership fence. It borrows the existing staging arena, normalizes selected track and project-page `G_SLOT/G_NAME/G_LOAD/G_SAVE` controls, re-encodes the canonical native snapshot and applies FNV32 to that wire. It covers encoded track parameters/base automation values, engine requests/presets, steps, packed patches, globals, motion, name and active arrangement metadata. Inactive motion/arrangement storage and structure padding do not create spurious dirty changes. There is no retained project-sized cache or added RAM allocation.

The output must be caller-owned outside the borrowed arena. Null/arena aliases, active canvas, running/pending transport, a late start during LCD sync or invalid native state refuse without publishing a signature. The helper may overwrite staging after a late refusal, following the existing capture contract. It neither writes flash nor changes native ownership. Callers must serialize editor/drawing/project operations and enter from the main loop with interrupts enabled.

This inherits a 32-bit change-detector tradeoff: hash collisions are possible. It is not a replacement for format CRCs, validation or backup verification. Two complete encoding/CRC passes plus hashing cost main-loop time; frequent polling and the existing snapshot IRQ window require measured scheduling/device qualification. No scheduler invokes this API yet.

## Evidence

[Manifest and logs](evidence/2026-10-10-native-signature/manifest.json) record exact source, dependencies, target hashes, review and initial failure history. Optimized and inherited ASan/UBSan each pass 13774 checks and 3579 musical mutations: every track parameter and all stored note positions, representative packed-FM6/motion fields, all active bank/scene/row fields, project name/globals, selection/page-control exclusions, stale inactive content, stored automation bases and actual pending LCD pixel consumption. Null/alias/canvas/transport/late-start/legacy-chain refusals preserve caller output. These mutations are not a claim that every step field, FM6 byte or active motion field was individually varied. Host IRQ stubs do not establish physical interrupt behavior.

Adjacent native adoption/capture tests pass 8911 checks in both modes. Independent code review found no concrete omission or ownership defect, without independently rerunning tests. Sanitizers retain inherited DSP exclusions for signed overflow, shift, bounds, object size and pointer overflow. Full new-head host/browser CI is pending at evidence capture.

Pinned native app: 457156 bytes, SHA-256 `04161e9ab1981a8f76f62f6570d6b71d6c1c6dc851348f8f26a44e396b31b491`; RAM95732/98304, pool334164/344064, interrupt RAM text916instructions/no calls. This adds132imagebytes over PR#70 and no retained RAM/pool. Default four-track app remains byte-identical to unchanged Felucca v1.1.5 `276f72a4e6ea8a12499a7a6819aadf3165126755`; all six baseline app/package/loader/golden/CPU/target artifact hashes remain unchanged. No new audio path was changed and no fresh golden render run is claimed for this API addition.

Issue #13 still needs queued-output and physical timing gates, scheduler idle/wear/retry policy, session/RESTORE LAST behavior and original-preserving migration/boot ownership. No automatic write caller, firmware installation, loader/boundary change or release is enabled.

SPDX-License-Identifier: GPL-3.0-only. Preserve Felucca and dependency notices.
