# Project migration and recovery

Proposed new format D8P1 has explicit version, track count, bounded lengths, stable engine and parameter IDs, sequence chunks, per-track patch data and checksums. Use a documented wire encoding, not raw C structs. Unknown optional chunks may be skipped only with validated lengths; unsupported required capabilities must fail safely. Keep limits available in the editor’s capability handshake.

Legacy import maps tracks 1–4 to the same identities and creates empty tracks 5–8. Preserve older engine conversion rules. Conversion should produce a new save rather than overwrite the only original. Backward export is a separate optional feature: an eight-track project cannot generally be represented by old firmware. Provide a desktop backup/conversion tool and a readable migration report for unsupported settings.

Storage A/B currently commits the header last, using sequence and CRC validation to retain a prior valid copy after interruption. A multi-sector replacement must preserve that property across erase/program boundaries. Simulate power cuts after every write operation, malformed lengths, bad checksums, sequence wrap, exhausted space and all invalid copies. Never claim A/B project storage is dual-bank firmware recovery.

Flash erases silence or interrupt the audio path in the existing implementation. Save while stopped for the initial release. Advertise background saving only after real playback tests prove it safe. Keep OTA support enabled and the loader unchanged through the first architecture milestones; do not widen application boundaries or relocate boot data speculatively.

Before a first hardware install: export user projects and presets, preserve the original official firmware obtained by the owner, verify a known-good rollback package, record device identity and host enumeration, and prepare the documented recovery route. Software rollback depends on a working or reachable device. A black screen is not automatically recoverable through the same high-level installer.

Felucca documents WL80UBOOT/4C4A:8057 as a recovery observation. FM-1 Transporter uses a Seeed XIAO RP2040 to access flash over the USB lines and can make a full backup. Its wiring and supported ranges must be followed exactly; its documentation warns against connecting VBUS. Do not improvise a recovery procedure from this summary. Prefer a second development FM-1 before experimenting with storage or update machinery. Sources: Felucca README and https://github.com/kurogedelic/FM-1-transporter.
