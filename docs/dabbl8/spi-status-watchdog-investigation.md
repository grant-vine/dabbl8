<!-- SPDX-License-Identifier: GPL-3.0-only -->
# v025 watchdog investigation and v026 diagnostic correction

Continued by the [v027 three-advisor boundary review](commit-watchdog-boundary-review.md).
It fixes a distinct reproduced timer contract defect and preserves this v026
package/evidence. The physical COMMIT watchdog cause remains unresolved.

The20 October v025 original-project restore reached STORE, then rebooted.
The authorized read-only capture succeeded and closed its console. Retained
import boot1/current boot2, P3 reset0x04,9 completed primitives and10 feed
receipts establish an unfinished COMMIT followed by a watchdog reset.
The RAM driver identifies PROGRAM at0x9D800; its last recorded SPI TX wait11
is in WIP phase10, command0x05, MMIO0x29, polls1, SR1=0x03. No deliberate
failstop, guard/revocation or P33 timeout was recorded. Audio-late counters
were zero. Feed receipts do not independently prove hardware acceptance.

## Correction and its limit

A local actual-HAL reproduction produces the exact TX11/MMIO29/polls1/SR1=03
signature after delayed TX and RX/status transactions have completed. Unsampled
iterations did not retire that wait; unchanged SR1 did not publish a new state.
The earlier claim that the tenth program itself stopped returning is too
strong: its outer completion is absent, but the trace cannot locate the final
instruction. The physical watchdog cause remains unresolved.

`firmware/hal/fm1_flash_trace.h` now retires a published TX/RX wait after a
completed WIP SR1 transaction, including an unchanged SR1 byte. Phase, received
status/MMIO and zero active SPI polls/wait timestamps agree. Retirement adds
no MMIO read; the previous clock sample remains unchanged. Immediately
ready unsampled transactions do not repeatedly publish the retired record.
Genuinely unfinished TX/RX waits retain their existing diagnostics.

No SPI traffic, readiness rule, watchdog feed/deadline, loader, flash boundary,
storage format, engine or effect changed. **This fixes diagnostic correctness;
it does not establish a repair for the physical watchdog reset.**

## Verification and reuse

- Eight permanent cases fail with the old trace and pass with the correction:
  erase/program, TX/RX and wrap combinations. Program uses a256-byte page at
  the captured0x9D800 address in virtual hardware. Four genuine stuck-wait
  cases continue to pass per mode.
- The fixture models PCLR acknowledgement, delayed completion and stale PND.
  Removing PCLR in an isolated copied HAL is now rejected in all eight cases.
  Its independent CS-based observation cutoff is not calibrated silicon timing.
- The local recipe passes21 optimized/ASan/UBSan steps: flash HAL160 checks,
  trace75,12 SPI cases, owner initialization5554 and reset-retention401 per mode.
  Only the existing first-use DSP arithmetic sanitizer exclusions apply.
- Exact v026 target:12216 RAM-code bytes,3960 instructions, no calls and direct
  branches confined to RAM. Static memory/budget passes. This is not dynamic
  address, stack, timing or recovery qualification.
- The new package passes30 OTA/loader/native-pool consumer commands with
  corruption controls; the repinned read-only helper passes28 fake-console cases.
- Prior84 engine/effects and42 emulator scenarios are reused. All firmware
  build inputs except the trace header match v025. The separate host overlay
  retains its67-test/seven-clock-group evidence. Do not repeat those matrices.

Evidence, commands, hashes, initial setup failures and two advisor reports:
`build/rc-spi-watchdog-20261020/`. Physical incident:
`build/hardware-v025-20261020-074222/restore-failure-capture-1/`.

## Private package and remaining gate

`build/rc-v026-20261020/dabbl8-0.2.6-timing-diagnostic.fwsc`,610106 bytes,
SHA-256 `5f9fbfebbf41e0b70f03df91755acf2fb77e6fbbde9984cbf08f1307228bd4e9`.
Application529008B; RAM94796/98304 (3508 free); pool335696/344064 (8368 free).
Full0x3ffd engine mask; eight tracks share eight voices; USB audio off.
The unchanged6950B loader hash is
`d6a2726256681ce39ccc61a4089df885ed32b387d6d26393056b4c69c6d9229a`.
Packaging and installer preserve Felucca's flow.

Static audit found no XIP call/escaping direct branch in the RAM driver.
Sparse local SDK sources could not establish different SPI ordering. A fault
while XIP is off, peripheral/bus stall and hardware feed acceptance remain
hypotheses. No speculative protocol change, retry, longer watchdog or unsafe
XIP return was introduced.

Installed firmware remains v025; originals/readback remain unverified. The
private original archive is preserved and excluded from the package. No device
access, restore or flashing is authorized now. v026 is an uninstalled diagnostic,
not RC or a qualified restore repair. Timing/stack, independent recovery and
release approval remain open.
