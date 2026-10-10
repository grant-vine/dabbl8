# Implementation roadmap

M0 Evidence baseline. Pin upstream and editor commits, archive the audit and license inventory, establish a minimal set of original reference songs. Exit: a clean upstream build and test run on the desktop, with map, hashes and logs. No hardware changes are required for this gate.

M1 Compatibility foundations. Freeze historical four-track structures; implement explicit motion addressing and capability negotiation; add 512-address, routing and serialization tests. Design and validate D8P1 in host tests before changing live flash storage. Exit: all legacy fixtures still import correctly and malformed inputs fail safely.

M2 Eight tracks in host and browser. Expand sequencing, mixer, mute masks, routing and editor arrays; define voice stealing at eight protected tracks; instrument memory and timing. Exit: every track can record and play independently; switching, locks, mutes and chords do not alias; no old protocol consumer silently misinterprets data.

M3 Persistence and first hardware alpha. Approve the measured memory/flash map, implement bounded state and multi-sector atomic storage, test power cuts, prepare backups and recovery. Hardware install is an explicitly separate decision. Exit: an eight-track reference project survives reboot/save interruption; target deadlines and USB compatibility pass documented limits.

M4 Performance beta. Add quantized changes, bounded scenes, fills or microtiming in dependency order. Expand one expressive engine only after the core budget passes. Exit: representative live sets complete without late blocks, unexpected data loss, stuck notes or confusing record targets; known issues and maximum combinations are published.

M5 Public release. Publish GPL source and exact binary correspondence, license notices, hashes, migration documentation, limits, tested hardware/host matrix and recovery instructions. Version the firmware, editor and data schema independently. Exit: a second tester reproduces installation, project import, rollback and the core reference performance.

Do not assign calendar promises yet. The first desktop build will expose toolchain and baseline compatibility costs; the first target profile will determine engine and storage scope. Keep milestones as gated batches of small commits. Every optional feature competes with reliability, RAM, flash and realtime headroom.
