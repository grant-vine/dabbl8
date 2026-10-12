# Clean first use and own-format persistence

Dabbl8's first installation starts with fresh user storage. It does not migrate
projects, settings, presets or samples from vendor firmware or another firmware.
The actual first-boot screen discloses **ERASE ALL USER DATA** and requires a
physical SAVE press and release before initialization starts. Installation must
not be described as already having erased data before that boot operation ran.

The formatter uses only the existing approved user-state sectors: 0x97000 through
0xDFFFF, 0xE5000 through 0xE6FFF, and 0xFC000 through 0xFEFFF (78 sectors,
319,488 bytes). The loader, OTA staging, linker layout and flash boundary code
are unchanged. Hardware recovery and the physical device's contents remain
verification gates; this is not an installation recommendation or download.

All those sectors are erased and verified before the initial canonical native
project 0 commits. Three manual projects, one shared autosave and three user
sample slots retain their planned storage capacity; existing sample contents
are erased on first use. The shared autosave may initially be absent. Its idle
scheduler baselines current music without writing, then creates an autosave
following a musical change and the existing quiet/idle policy.

Recognized committed Dabbl8 records, including future or damaged payloads,
prevent first-use formatting. Own upgrades use existing native admission and
preserve their data. Recognizable torn/damaged headers require an explicitly
disclosed reset decision; an ordinary reboot never silently formats them.
Absent autosave is accepted only after the full native residue scan proves
absence. A corrupt autosave refuses initialization of its scheduler baseline.

Only the private initializer's exact sector/page ticket is enabled. Generic
legacy, settings, sample and remote full-project writes remain quarantined.
Own preset/settings/sample persistence requires separate bounded writer work.

## Local evidence

The enabled native8 app was built once from qualified source `e9052b5`, with
`D8_FIRST_USE_WRITE_ENABLED=1`. Changing both fallback definitions to 1 and
removing that override yielded a byte-identical vendor preprocessed translation
unit (SHA-256 `824896a18152b9143e8fb3e11fbec28534d43a203f7fe7f48734e60d7c5a0030`),
including the qualified compiler date, `Oct 12 2026`. That proves the enabled
build can be reused for this default change; it does not prove device behavior.

- App: 493,652 bytes; SHA-256
  `1e0b01659d53289020b7da08424af39c9be27ade7411619e93e48e36ff36bf77`.
- Main RAM: 96,068 / 98,304 bytes; 2,236 bytes free.
- Pool: 335,512 / 344,064 bytes; 8,552 bytes free, only 360 bytes above the
  existing 8 KiB reserve.
- Actual panel, first-use ticket, sparse autosave and existing/corrupt autosave
  checks: 87,258 checks, zero failures, both normal and full ASan/UBSan.
- Largest new outlined own frame: `d8fu_step`, 300 bytes. The build also inlines
  first-use code into `fm1_main` (1,024-byte own frame) and `ui_input` (712-byte
  own frame). These are individual allocations, **not observed nested peaks**.
- Existing static-budget failures remain explicit: SLICE 517, audio interrupt
  55,591 and USB resampler 562. These estimates are not measured CPU cycles.

The earlier freestanding `string.h` compile failure and its corrected retry are
retained locally. No hosted CI, package generation, release or device operation
was performed for this change. Physical first-use, interrupted-write recovery,
restore, reboot retention and audio timing acceptance remain open.

The failed/refused screen currently retains the SAVE instruction, although
those states refuse input and cannot reset automatically. Reboot/read-only
recovery messaging should be corrected before hardware testing; do not infer
that pressing SAVE on an error screen restarts initialization.
