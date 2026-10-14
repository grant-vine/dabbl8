<!-- SPDX-License-Identifier: GPL-3.0-only -->
# FM-1 initialization evidence, 14 October 2026

The owner's FM-1 successfully installed and initialized the private Dabbl8
v0.1.5 diagnostic. This is a hardware initialization result, not release
qualification. Issues #9, #12, #13, #14 and #20 retain their remaining gates.

## Exact candidate

- Source: `76ea40ba7a673d93d5b59053457c98bbf6d75191`.
- Identity: `FM-1_8015`, `v0.1.5-timing`, eight tracks sharing eight voices,
  full engine profile, USB audio disabled, serial/MIDI retained, manual SAVE.
- Application: 522232 bytes; SHA-256
  `7ec81d270a757cef39c3ea1d42156e75abeacb16cadeb59e7e772fb0268fb22a`.
- Private diagnostic package: 610086 bytes; SHA-256
  `e257ba8a017959b7ffd45f0aec409249502cca4a7722420aff4f7bdc0bb374c9`.
- Felucca OTA loader: 6930 bytes; unchanged SHA-256
  `d71d1b7c12adede951605e758a3580dacccedc427d49e1fceaf51decccc68a4b`.

The existing Felucca CoreMIDI installer completed the update and verified the
new identity. Package parser/decryption/CRC and source correspondence checks
passed locally. No alternative loader or changed flash map was used. The
package remains local and is not offered as a public firmware download.

## Physical observations

Before SAVE, three read-only status samples span 60060 ms: uptime
12423/42453/72483 ms, boot counter 5, USB reset counter 1 and flash admission
flag 1. Counters remain unchanged; no initialization or RAM-driver failure is
recorded. The owner confirms steady first-use text.

After one physical SAVE press/release, status at uptime 310993 ms records:
`fi_status=4` COMPLETE, `fi_operation=6` READY, `fi_result=0`, `fi_guards=0`,
`fi_phase=0` IDLE, formatter phase 4, sector 78, verify offset 4096,
foundation count 4 and released workspace generation 0. Current/record boot
identities match. Boot counter 5 and USB reset counter 1 are unchanged.
The final RAM-driver record is a successful read with failure 0, not an
interrupted erase. The owner photo shows the normal ANALOG/ACID synth screen
at tempo 120 with parameter controls and HOME visible.

The completion path publishes COMPLETE only after approved sector erase and
readback, foundation commits, initial native record commit and successful
native bind/autosave readiness. This is firmware-recorded verification; no
independent whole-flash comparison was performed. The former third-erase
watchdog failure did not recur in this attempt, but its original cause is
still unproved. The retained reset reason belongs to the installation boot,
not a new initialization reset. `fi_elapsed_ms` is consent age sampled at an
operation, not a measured erase duration.

## Local evidence and limits

Delayed SAVE is reproduced on the previous source and passes after idle
confirmation renewal. Renewal requires unchanged visible disclosure, full
quiet, released controls and matching storage/USB/LCD/workspace guards. It
does not extend held or accepted operation deadlines. Exact manual diagnostic
native LCD fixtures pass 192576 checks in optimized and sanitized modes;
4096 idle polls advancing one second each send zero commands/data/pixels in
both themes and all five tested screen phases. Retained trace/reset fixtures
and selective local first-use checks pass. No hosted CI was run.

RAM data/BSS is 94628/98304 bytes; pool is 335688/344064, leaving 8376 bytes
against the existing 8192-byte reserve. Flash-off RAM code is unchanged from
the reviewed v0.1.4 trace build: 11272 bytes, 3654 instructions, no calls, all
541 decoded branches within RAM. These are linker/static observations, not
stack high-water or hardware timing clearance. SLICE +14% and audio interrupt
+49% still fail the unchanged static budgets; TIMER5 passes.

Remaining physical acceptance includes sound and controls, tracks 1–8 and
USB/TRS routing, three saved slots/shared autosave, own backup/restore,
stopped saves and reboot recall, interrupted-format recovery, render timing
and stack/pool high-water. USB audio sample-rate testing is not represented
by this UAC-disabled diagnostic. Independent dead-application recovery and a
second tester remain separate gates. Successful stock-v15 restoration is
documented in PR #177; vendor firmware/private backups remain outside Git.

Local logs, manifests, native fixtures and photo attribution are retained in
the owner's workspace. Their source-specific receipt is
`build/hardware-trial-v015/after-save-initialization-status.log`; package and
test receipts remain under `build/timing-diagnostics/run-276mn83g/`,
`build/app-only/full/run-dontvc4d/` and `build/save-long-wait/`. No vendor
firmware, private backup or credential is included in this document.
