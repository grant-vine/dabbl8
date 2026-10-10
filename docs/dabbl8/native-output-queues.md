# Native logical output queue gate

This extends the explicitly authorized native autosave adapter's refusal policy, without adding a scheduler or granting storage ownership. Stopped voices and empty effects histories can still leave sound in output buffers. Before every physical mutation, including inside the existing IRQ-off fence, the adapter now also checks the firmware's DAC and USB output state. This is logical buffer evidence, not proof of inaudibility or timing on the FM-1.

## Accounting and ownership

The audio ISR publishes two nonzero-half summaries only after a complete DMA half has been rendered. The existing final conversion loop collects the summary after USB-fixed DAC scaling and DAC-only metronome output. Both halves must be zero, including stale data in the half currently marked free. Cold initialization and the existing explicit audio_silence path update summaries after actually clearing the corresponding RAM. Eligibility is checked before entering erase: no new silence call is added and clearing audio is never used to manufacture eligibility.

The audio producer counts all 512 stored USB ring frames, including already-consumed frames, and all 32 unique stereo FIR history frames. The duplicated FIR view is accounted once. Store accounting follows the actual clamped/packed samples; existing re-prime history clearing resets its history count afterward. TIMER5 never decrements ring/history counts, avoiding a shared read-modify-write race when it nests inside audio rendering. Consumed nonzero ring cells become eligible only when natural zero production overwrites them.

TIMER5 summarizes the actual EP4 packet supplied to the hardware. A zero packet replacing an observed completed packet clears that summary; a successful existing FIFO reset also clears it. Stream/rate metadata changes alone do not cancel a previously queued packet. The retained underrun frame is checked independently, since an empty ring repeats it. Failed or unavailable SIE accesses set sticky uncertainty: a returned zero from a failed read cannot prove completion. The uncertainty persists until firmware restarts; no recovery shortcut is introduced.

The USB benchmark TONE build is always refused, since it generates output independently of musical activity. The normal four-track build has no new accounting or guard policy.

## Availability limits

All-slot counting is deliberately conservative. A stopped or disconnected USB reader may strand nonzero cells even though they are outside the active logical queue. Such cells, retained inactive resampler state, fixed-point musical histories, or sticky SIE uncertainty can defer autosave indefinitely. The caller must handle this as an unavailable save opportunity; never clear musical/output state to force a save.

No main-loop buffer scan, endpoint-index access, dynamic allocation, or new automatic caller is introduced. The main loop reads bounded summaries freshly under the existing IRQ/compiler memory barriers. Normal manual native saves retain their prior stopped-transport policy.

## Host evidence and open device gates

The new test imports actual audio.c, USB producer/packet/service logic and the native physical adapter. A simulated register-level USB HAL exercises the actual SIE wrappers and TxPktRdy transitions without MMIO; it is not a controller emulator. Host-only independent scans compare the published counters and complete DMA halves to actual buffers. Tests cover both USB rates, ring/FIR wrapping and natural drain, overrun drops, consumer nesting at producer publication boundaries, stale halves, click after USB tap, MASTER zero with fixed USB gain, retained underrun output, stalled packets, rate changes, actual FIFO reset and failed SIE operations. Seven queue states are injected before every physical mutation and after commit; valid committed autosaves remain recoverable after later refusal. Storage-only older fixtures explicitly assume idle queues rather than pretending to simulate DMA/USB.

Device qualification remains open: ALNK/codec pipeline latency beyond RAM, USB DMA/FIFO completion semantics, host capture buffering, actual erase/program worst-case duration, pending interrupts, and DAC/USB continuity under storage operations. Those gates must pass before a scheduler or release claim. No firmware was flashed, installed, released or deployed.

The default pinned target must remain byte-identical, native memory/disassembly evidence is retained, and actual native DAC/USB-tap samples are compared with exact parent a686c29a7acb298ed38ab85b486e7a7b6c7f2b5f without changing golden hashes. See evidence/2026-10-10-output-queues for exact results.

Parallel integration must retain PR #71's independent canonical signature helper and both standard/sanitizer runner entries; this branch starts at PR #70 and does not incorporate #71.

SPDX-License-Identifier: GPL-3.0-only.
