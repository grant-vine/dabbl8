# Private owner-authorised first-use diagnostic

`D8_OWNER_INITIALIZE=1` is an opt-in build override for the owner's empty FM-1,
authorised on 13 October 2026 after the physical SAVE prompt did not advance.
It is accepted only for branded Dabbl8 timing diagnostics. Ordinary builds keep
the existing physical SAVE confirmation, including deliberate use while offline.
This is not a public release option.

Automatic owner initialization requires an established USB configuration:
`usb.up`, configuration 1 and a bus that is not suspended. While USB is
unconfigured or unavailable the screen shows `WAITING FOR USB` and no formatting
operations are admitted. `usb.up == 0` cannot prove that the cable is disconnected;
it can also mean an SIE fault. There is no automatic offline fallback or fixed
enumeration delay. After configuration becomes available, a fresh displayed
snapshot is required before entering the existing formatter.

The override replaces only the initial SAVE press. Epoch, USB configuration,
reset and availability snapshots, header, LCD, workspace, immutable payload,
quiet-state, deadline, erase/program tickets, readback and flash-boundary checks
remain in force. A later USB or lifetime change stops the operation; it cannot
refresh a running format's consent. The screen states `OWNER-AUTHORIZED RESET`
when eligible and visibly discloses `ERASE ALL USER DATA` and the affected data.
Prior firmware projects, settings, presets and sample contents are erased within
the existing approved user-state map.

Recognized Dabbl8 storage is preserved on reboot/upgrades. Uncertain or refused
storage never enters automatic initialization. A failed initialization remains
stopped; this override does not bypass its failure state or authorize a retry.
Physical input, audio, timing, recovery and release qualification remain separate
gates. The original SAVE failure is not fixed by this workaround.

Private timing builds retain a 76-byte diagnostic in `.noinit`. Read-only CDC
`status` emits `fi_*` fields. `fi_boot` identifies the current boot;
`fi_record_boot` identifies the boot that captured the operation. A mismatch
means the operation record belongs to an earlier boot. `fi_status` distinguishes
empty (0), active (1), an operation that has not returned (2), failed (3) and
completed (4). A prior completed record is not a new initialization failure.
The current boot's reset reason/source and watchdog register accompany retained
records; they do not establish why a physical reset occurred.

The record includes the raw owned callback result, helper result, operation and
address, pre-cleanup session/formatter phase, sector, verification offset,
foundation index, generation, expected/current epoch and USB reset/configuration
snapshots, elapsed time and a guard mask. A raw result of -32768 means the owned
operation has not returned or was not entered. The in-flight record is published
before entering the read/erase/program callback, so a failstop can retain its
breadcrumb. Foundation commits retain their aggregate storage-helper result;
that helper's readback error does not distinguish every underlying SPI read
from a payload mismatch. Retention is across SRAM-preserving resets, not a
promise that diagnostic data survives loss of power.

Guard bits: revoked `0x1`, phase `0x2`, workspace `0x4`, epoch `0x8`, USB reset
`0x10`, configuration `0x20`, deadline `0x40`, quiet state `0x80`, USB availability
or owner-bus suspension `0x100`, write ticket `0x200`, staging `0x400`, and
non-erased verification bytes `0x800`. Operation codes are staging (0), engine
step (1), read (2), erase (3), program (4), foundation (5), and ready binding (6).

Local evidence: `native_owner_initialize_test.c` exercises enumeration waiting,
fresh-display gating, ambiguous offline waiting, complete virtual-NOR
initialization without a button, own/torn/future preservation, stopped failure,
raw I/O versus verification mismatch, lifetime reason masks, retained failed and
in-flight records, actual completion reload effects, and visible disclosure in
light/dark LCD pixels. Ordinary panel consent and foundation cut/reboot tests
remain separate focused checks. No hosted CI, loader changes or golden/budget
changes are involved. Host passes do not establish physical initialization,
watchdog, recovery, stack or audio timing safety.
