<!-- SPDX-License-Identifier: GPL-3.0-only -->
The UI test harness copied eight canonical tracks into the historical four-track FUN6 structure. This repair uses `PROJ_LEGACY_NTRK` in a shared fixture helper. For native eight-track tests, RAM capture prepares the fixture because legacy-sector saving is quarantined. Production code and layouts are unchanged.

On main `752e8deb21395d79b00df139d1d48d6ed58fd138`, all 16 focused configurations passed: four/eight tracks, FM4 off/on, SLICE off/on, normal and strict ASan/UBSan. Each four-track run checks 797 conditions; each eight-track run checks 805. Complete four-track UI stdout matches the unchanged parent with FM4 off/on. Existing FM6 regression invocations remain in the runner.

Historical evidence separately retains the original native8 array-bounds failure and compiled negative controls for an eight-track historical copy and an omitted fourth track. These controls were not rerun on this current-main composition. This qualification covers the shared copy helper and actual importer; it does not establish a passing full eight-track UI suite, whole CI run, firmware build or hardware operation.
