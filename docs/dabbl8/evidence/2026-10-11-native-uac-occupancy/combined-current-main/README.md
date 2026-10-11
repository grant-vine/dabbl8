# USB occupancy integration with current main

This evidence measures functional merge `d450d49e42dda5f8cc345c5be9d212c2dbb4eef6`, whose parents are original PR102 head `45273e017378229887826650cbadedcb2e71c599` and verified main `832df855adb1f33c8517ebc5b8e8b11e4fb84c2c` (PR101). The original parent evidence remains a separate scope. No original full-suite or CI result qualifies this new composition.

All four retained SRC history modes pass: 31,200 cases per mode, 2,246,436 normal checks and 2,277,636 TONE checks, each optimized and under the inherited DSP sanitizer exclusions. Exact frozen reference function and full-buffer/history oracles are retained. Actual queue checks pass in optimized and inherited sanitizer modes.

Actual connected migration checks pass 103,371,616 checks in normal and strict ASan/UBSan modes, shipping quarantine passes 1,214,600 checks in both modes, and actual connected collector/session checks pass 781 in both modes with freshly built source-matched helper binaries. Actual C INFO discovery passes 95 checks in normal and inherited DSP sanitizer modes. Strict connected/helper checks have no sanitizer exclusions; inherited DSP checks retain the documented signed overflow, shift, bounds, object-size and pointer-overflow exclusions.

Fresh pinned targets pass build/link/MMIO checks. Default four-track image is 447,928 bytes, SHA-256 `3ced057d74785ecd0cc2ccc8f274a1c9e94caa4a6b1203c846014a890d4720fc`, unchanged from actual current main and distinct from the preserved pristine v1.1.5 baseline. Native image is 478,716 bytes, SHA-256 `32e3eceaa74a4329e17fed0b7812e1d99c37c0e7e2430648f6e4d1886c9a6baa`, with 95,812 bytes RAM and 335,240 bytes pool.

Default static budget passes. Native static budget still fails two original limits: IRQ cost 55,487 versus 37,502, and USB48 cost 555 versus 504 (555 exceeds the exact 554.4 allowance). ROOM 233/218 and SPRING 135/126 pass their original 10% allowances. No threshold or golden rewrite. These are static proxies, not measured device deadlines.

A fresh whole published-tree CI is required before merge. No new local full-suite run is claimed for this composition; parent full-suite and audio/benchmark evidence retains its exact original scope. No physical USB FIFO, codec completion, deadlines, recovery, device migration, firmware installation or release qualification is claimed. Shipping mutation authority remains closed.
