# Historical project byte fixtures

These immutable FUN1–FUN9 records and their expected FUN9 migrations were captured from unchanged Felucca v1.1.5 (`276f72a4e6ea8a12499a7a6819aadf3165126755`) before the historical-layout change. They contain synthetic test songs, not private backups or device recordings. `manifest.json` records sizes and SHA-256 values.

`capture_reference.c` documents the reference producer. Compile it against the pristine baseline tests, firmware and generated headers; do not regenerate expected bytes from changed production code. FUN1–FUN6 use the original historical structures; FUN7/8 use their original compact framing, parameter count and patch/name offsets. FUN9 uses the unchanged upstream serializer. Expected files are the unchanged importer followed by its FUN9 serializer. Variants cover DIGITAL-to-FM6, PHYS DRUM and SAMPLE PERC conversion, and compact records contain automation and a lock on track four, step 63.

The inventory is FUN1 688 bytes, FUN2 2552, FUN3 2584, FUN4 2680, FUN5 3352, FUN6 3388, FUN7 3388, FUN8 3584 and FUN9 3648. FUN2–6 track arrays start at offset 66; checksum offsets are size minus four. FUN6 chain offset is 3346. FUN7 name offset is 3372; FUN8 patches/name are 3056/3568; FUN9 patches/name are 3120/3632. FUN1 has one instrument; all later historical formats have four tracks.

`tests/legacy_fixture_test.c` checks immutable imports, exact canonical bytes, round trips and refusal of damaged/truncated records. It also compiles with an eight-track import destination to verify fixed disk layouts and loops. This isolated test does not enable an eight-track runtime. FUN9 serialization refuses expanded runtime state until the new project format exists; initialization of new tracks belongs to the eight-track runtime work.
