<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Eight-track emulator runtime composition

The eight-track wrapper includes the same native codec, pool, read-only boot and
ordinary autosave runtime definitions as the target. These use simulated NOR;
boot grants no writable native session. OTA and migration remain disabled.
This does not implement first-use formatting or qualify device writes/recovery.

With existing generated headers and locally installed compilers only:

```sh
cc -O2 -ffp-contract=off -std=gnu99 -w -DNPART=8 -Ibuild/gen -Ifirmware/src \
  web/emu/native_storage_check.c -lm -o build/emu/native_storage_check
build/emu/native_storage_check own
build/emu/native_storage_check unknown
```

The fixture manufactures synthetic Dabbl8 autosave data, checks restoration on
all eight tracks, read-only ownership, dormant autosave, and unchanged pool bytes.
A foreign header remains offline without writes. It never reads a device.

Local validation against main `63ce1b0` passed native and Emscripten 4.0.17
compilation with four/eight tracks. The existing emulator suite passed with four
tracks. With eight tracks its panel/audio checks and 88,200-frame bit-for-bit
native/WASM comparison passed, but **two user-preset write/retention
assertions failed**: eight-track legacy writes remain quarantined. That suite is
not reported as passing. This patch neither enables those writes nor replaces
its expected outcomes. Own user-preset persistence is an unfinished eight-track
feature, still required after first-use formatting; the clean-install policy
does not remove that requirement. Native save UI remains separate.
