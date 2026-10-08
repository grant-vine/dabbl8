# Memory and flash budget

Observed storage geometry: 4096-byte flash sectors, a 256-byte payload offset and a 3840-byte maximum single-sector payload. The current FUN9 project serializes to 3648 bytes. Eight tracks of step data alone require 8 × 64 × 9 = 4608 bytes, before parameters, FM6 patches, automation, chain data or a checksum. The existing project payload cannot hold an eight-track version.

Current project storage spans 0x97000–0x9EFFF, eight sectors or 32 KiB, implementing four projects with one-sector A/B copies. A two-sector copy would provide about 7936 payload bytes with one 256-byte header region. A/B would then require four sectors, or 16 KiB per project: only two projects fit the current 32 KiB area. This is a budgeting illustration, not an approved final map. Autosave, additional banks and scenes need their own allocation.

Other observed regions include sample data at 0xA0000–0xDBFFF, three 80 KiB user slots; preset sectors around 0xDC000–0xDFFFF; OTA at 0xE0000–0xE4FFF; autosave at 0xE5000–0xE6FFF; settings at 0xFC000–0xFDFFF. FM6 resources use additional locations. Preserve the complete map and overflow checks. The 1 MiB flash address wraps above its capacity; invalid writes must be rejected, not masked.

The linker partitions include 24 KiB RAM text, 96 KiB general RAM and a 336 KiB engine pool. These are partition capacities, not free space. Retained NOINIT capacity is about 15 KiB; four current project objects already consume most of it. Four doubled project objects cannot simply stay there. Stack guards, loader areas and reserved boot information are not spare memory.

FM6, grain, physical and slice engines allocate track-indexed side state. Some multiply tracks by per-engine polyphony or by NVOICE. Generate a baseline map and then a diff after each change. Prefer a bounded active-engine-state pool and fewer retained project copies if needed; avoid allocating the largest engine’s full state for all eight tracks. This refactor is a measurement-led second step, not a prerequisite to an untested wholesale rewrite.

Initial budget decision: preserve sample regions and boot/update regions; design a compact new project format; evaluate two retained projects versus four projects with reduced sample capacity. Present the tradeoff before changing the map. Multiple banks must reference stored patterns instead of copying a complete project for every scene. Compression may help typical songs but cannot replace a provable maximum-size calculation.
