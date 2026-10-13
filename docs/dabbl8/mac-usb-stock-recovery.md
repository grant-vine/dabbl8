<!-- SPDX-License-Identifier: GPL-3.0-only -->
# FM-1 stock recovery over USB on an Apple Silicon Mac

On 13 October 2026, Dabbl8 development restored one M-VAVE FM-1 to its original
v15 firmware directly from a Mac, without an RP2040. A full flash readback
matched the restoration plan, and the owner confirmed the original interface,
sound and controls worked after a power cycle.

This is a tested engineering record and contribution reference. It is not a
general-purpose recovery utility or a claim that every unresponsive FM-1 is
recoverable directly from macOS. No recovery executable, vendor image, RAM
loader binary or private device backup is distributed by this document.

## Scope and prerequisites

The chip's ROM recovery mode was reachable. The running, damaged Dabbl8 build
still had a working diagnostic console; its existing `uboot yes` command
requested ROM entry through a RAM mailbox and reset. This was an intentional
restart. It was not an independent rescue from an application with no usable
console or ROM enumeration. A completely dead device may still require the
upstream hardware recovery route.

The device appeared as `4C4A:8057`, USB product `WL80UBOOT1.00`. Its mass-storage
interface used class/subclass/protocol `8/6/80` and configuration 1. The observed
bulk endpoints were IN `0x81` and OUT `0x01`; implementations must derive and
validate these from the actual descriptors, rather than assume these numbers.

| Tested environment | Version |
|---|---|
| Mac architecture | arm64 / Apple Silicon |
| macOS | 26.7.1, build 25G241 |
| Python | 3.14.8 |
| libusb | 1.0.30 |
| PyUSB | 1.3.1 |
| crcmod | 1.7 |
| Target | One FM-1, originally stock v15 |

## The Mac-specific access problem and resolution

The ROM presents a mass-storage interface. macOS attached
`IOUSBMassStorageInterfaceNub`, which held it exclusively. Cooperative
`DeviceSeize` at both device and interface level did not release that owner.
The interface initializer returned `0xe00002c9`; the kernel log identified the
exclusive mass-storage owner. The native SCSI route examined earlier was also
unavailable on this connection.

The successful sequence was:

1. Select exactly one matching ROM device and its directly associated interface;
   validate configuration, interface and endpoint descriptors.
2. With the owner's authorization and macOS administrator authentication, open
   the parent `IOUSBHostDevice` using Apple's `DeviceCapture` option. This
   terminates the target's existing device/interface clients and drivers; it
   is stronger than a cooperative close request.
3. Explicitly destroy that device object with `DeviceSurrender`. Apple's API
   documents that this avoids the ordinary captured-device destruction reset
   and driver rematching. It does **not** restore the terminated mass-storage
   driver. On this connection the ROM stayed enumerated and its interface was
   left without that driver.
4. Open the now-unowned interface through the existing libusb/PyUSB path. Check
   that no kernel driver is attached before claiming it. Use no auto-detach,
   explicit detach, reset, configuration change or retry.
5. Send one fixed, read-only 36-byte SCSI INQUIRY. Verify vendor `WL82`, product
   `UBOOT1.00`, revision `1.00`, and complete CBW/data/CSW lengths, tag, residue
   and status. Release the interface and close the transport.

Native `IOUSBHostInterface` access still failed after capture with a macOS
System Policy denial. A subsequent explicit `IOServiceAuthorize` call returned
`0xe00002c6` (`kIOReturnBadMessageID`); it did not qualify native interface I/O.
Those failed paths were stopped and surrendered. The working data path was
legacy libusb after the native capture/surrender, not a successful native SCSI
or native-pipe transfer.

The useful Mac contribution is this target-specific driver-handover sequence
and its observed cleanup result. The JieLi ROM protocol, flash loader and
application-only restoration policy come from the existing projects below.
No global driver unload, SIP change, persistent security-setting change or
RP2040 programming was used.

## Restoration after read-only identification

After identification and separate owner authorization, a private adaptation
used the pinned `jl-uboot-tool` protocol classes over a checked USB Bulk-Only
Transport adapter. Its Windows/Linux OS opener was disabled; it received the
already-owned Mac transport. The preserved Windows recovery scripts and
FM-1 Transporter protocol supplied the reference sequence.

- Upload the externally obtained WL82 loader in 512-byte RAM blocks at
  `0x01C02000`; compare every block through ROM readback before jumping with
  argument 1. This loads and executes RAM code; it is not a read-only action.
- Check chip key `0x980F` and online device `{type: 3, id: 0x856014}` before
  reading or writing flash. These are expected hardware identifiers, not proof
  that a different device or loader is compatible.
- Read the complete 1 MiB flash twice and require exact agreement. Keep these
  emergency snapshots private even if there are no saved user projects.
- Validate the physical flash header/configuration and exact stock package.
  Derive a plan replacing only application bytes in `[0x4000, 0x93000)`.
  Preserve `[0, 0x4000)` and `[0x93000, 0x100000)` byte for byte.
- Keep one owned transport alive during review, using bounded loader identity
  queries. In this trial a digest-bound operator gate separated backup/planning
  from persistent writes; no flash command was permitted before that gate.
- Compare a fresh full read to the backup, erase/program only changed 4 KiB
  sectors, verify each sector, then read the entire flash again and require an
  exact match to the planned image. Stop on failure; do not retry an ambiguous
  write or erase.
- Close/release the transport. No loader reboot command was sent in this
  successful session. The owner unplugged USB and power-cycled the FM-1.

The recorded plan changed 143 application sectors. The complete final readback
matched the plan and protected regions matched the original snapshot. The owner
then confirmed stock boot, sound and control operation. This was application
restoration while preserving device data, not a full-chip erase or restoration
of previously erased user data.

The exact stock file was 699,956 bytes, SHA-256
`db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`.
The external RAM loader was 24,064 bytes, SHA-256
`d41da6126760c9d66660bcc0cac8d27d221806c5e369a8036921efe68dca5376`.
Hashes identify this trial's inputs; they are not a safety or redistribution
license certification. Obtain those files separately from their owners.

The private package-parser adaptation also needed two narrow compatibility
adjustments: accept the vendor's empty type-2 `info.log` with its normal CRC
check, and permit specific vendor metadata CRC exceptions only for the exact
stock-file SHA-256 above. The metadata transformation was not decoded. Generic
package integrity checks remained strict; this is not support for all vendor
packages or a reason to bypass their checks.

## Evidence and limits

The adjacent [sanitized result](evidence/2026-10-13-mac-stock-recovery.json)
records the tested environment, milestones and limitations without device
contents or personal machine paths. Full source, logs, backups and readback
comparisons remain in ignored local development evidence. Historical reports
that said “offline only” describe earlier stages; the stock restoration above
is the later physical result.

Local checks covered real adapter framing/guards, stop-on-error and cleanup
paths with fakes, and RAM-loader readback before execution. They do not qualify
macOS's internal cleanup during failed initialization, process death, power
loss, all USB topologies, other Mac/OS versions or an independent forced ROM
entry. Native capture's default cleanup can reset/rematch, so ordinary libusb
detach/reattach must not be described as harmless or equivalent to the tested
explicit surrender. Failed/unknown ownership needs reviewed containment.

The RP2040-Matrix owned by the developer was not used or qualified. Upstream
FM-1 Transporter recommends a Seeed XIAO RP2040 and labels other boards untested.
Recovery of a wholly unresponsive FM-1 remains a separate qualification task.

Normal Dabbl8 installation continues to use Felucca's existing `.fwsc`
packaging, OTA/MIDI loader and installer. This emergency method is not a
replacement for that established flashing workflow.

## Sources, credits and reuse

The recovery protocol and preservation policy were existing work. Dabbl8's
local contribution was the Mac adapter, ownership/cleanup investigation,
targeted tests and this one-device physical restoration record.

1. **Felucca — Leo Kuroshita / Hügelton Instruments.** The inherited firmware
   and normal installer are GPL-3.0-only. The audited reference is
   [`276f72a4e6ea8a12499a7a6819aadf3165126755`](https://github.com/hugelton/Felucca/tree/276f72a4e6ea8a12499a7a6819aadf3165126755).
   Its [recovery discussion](https://github.com/hugelton/Felucca/issues/61#issuecomment-6018813157)
   was an investigation lead; current [installation guidance](https://github.com/hugelton/Felucca/wiki/Installation)
   describes the normal USB installer.
2. **FM-1 Transporter — kurogedelic.** MIT reference for ROM entry, BOT command
   framing, loader use, flash layout and protected application-only writes.
   Pinned [protocol documentation](https://github.com/kurogedelic/FM-1-transporter/blob/a632d923203170e3a08565f4056dbb3fd3d65ca5/docs/PROTOCOL.md)
   also credits `fm-1-research-lab`; see the pinned
   [README](https://github.com/kurogedelic/FM-1-transporter/blob/a632d923203170e3a08565f4056dbb3fd3d65ca5/README.md).
3. **flipper-fm1-recovery — merthsoft.** The Windows backup/restore host flow,
   package parser and preservation-aware planner were inspected at
   [`df88b862a4794855f792fed529629a255cba0a0e`](https://github.com/merthsoft/flipper-fm1-recovery/tree/df88b862a4794855f792fed529629a255cba0a0e),
   Unlicense. No Flipper was used in the Mac restoration.
4. **jl-uboot-tool — Andrey Grigoryev / kagaimiq.** MIT protocol implementation,
   pinned to [`adb3f18889e88ac512ce0a3c4d8cc3d3cb30696a`](https://github.com/kagaimiq/jl-uboot-tool/tree/adb3f18889e88ac512ce0a3c4d8cc3d3cb30696a).
   Preserve its notices. The underlying precompiled loader's provenance remains
   unestablished here; its presence in that repository does not relicense it.
5. **Apple.** Primary API contracts for
   [DeviceCapture](https://developer.apple.com/documentation/iousbhost/iousbhostobjectinitoptions/devicecapture),
   [DeviceSurrender](https://developer.apple.com/documentation/iousbhost/iousbhostobjectdestroyoptions),
   [explicit destruction options](https://developer.apple.com/documentation/iousbhost/iousbhostobject/destroy(options:))
   and [IOServiceAuthorize](https://developer.apple.com/documentation/iokit/1514533-ioserviceauthorize).
   The installed Command Line Tools SDK headers were checked against these
   contracts. Apple supplied the APIs; it has not validated this FM-1 method.
6. **libusb and PyUSB.** The installed libusb 1.0.30 Darwin backend's
   [capture/reenumeration implementation](https://github.com/libusb/libusb/blob/v1.0.30/libusb/os/darwin_usb.c)
   informed the decision to avoid automatic detach/reattach. libusb retains
   LGPL-2.1-or-later and PyUSB retains BSD-3-Clause; see the pinned
   [PyUSB tutorial](https://github.com/pyusb/pyusb/blob/v1.3.1/docs/tutorial.rst).
7. **M-VAVE.** Original FM-1 hardware and stock v15 firmware. The firmware is
   obtained separately and is not redistributed or relicensed by Dabbl8.

This documentation is GPL-3.0-only under the repository's license; dependencies
and reference projects keep their own terms. The result can inform an eventual
Felucca or recovery-project contribution. A reusable helper still needs portable
device selection, reviewed privilege/cleanup behavior, failure tests and
explicit operating instructions before it is offered as an installable tool.
