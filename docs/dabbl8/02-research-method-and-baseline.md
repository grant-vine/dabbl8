# Research method and baseline

The implementation baseline is Felucca tag v1.1.5 at commit 276f72a4e6ea8a12499a7a6819aadf3165126755. This replaces older cached web descriptions of version 1.0.5.2. The report’s source-code observations refer to that exact checkout. Repository snapshots, file hashes and source URLs are in evidence/source-index.json in the companion bundle.

Evidence levels used throughout: observed means read directly in source or primary documentation; advertised means described by a project’s maintainer but not tested here; proposed means a Dabbl8 design decision or target; unverified means it requires a build, measurement, hardware experiment or license review. A browser simulator is useful for controls and musical ideas, not proof of target CPU headroom or recovery.

Felucca already provides thirteen engines, four tracks, eight shared voices, a 64-step sequencer, drum lanes, chords, probability, ratchets, modulation, performance effects, user presets, projects, USB MIDI and stereo USB audio. Current source supports 44.1 or 48 kHz USB output to a host, unlike older documentation. The editor has moved to the separate Felucca-WebApp repository. Preserve working features before replacing them.

The audit read the core types, voice allocator, sequencer and motion encoding, project compatibility structures, storage implementation, flash boundaries, linker map, audio scheduler, engine side-state arrays, build/package tooling and licensing. Related repositories were inspected for architecture and feature candidates, not merged.

The source snapshot is reproducible but the toolchain baseline is not fully pinned: the upstream downloader fetches a moving toolchain archive, and the Docker image uses a moving tag. The desktop baseline must record the downloaded archive SHA-256, actual compiler version, SDK commit, container image digest, dependency versions, build logs and outputs. Do not call a release reproducible until two clean environments produce matching artifacts or understood differences.

Primary indexes: github.com/hugelton/Felucca; github.com/hugelton/Felucca-WebApp; www.groove-os.com; github.com/isod89/sloop-fm1; github.com/keremimo/melodee; github.com/charlesvestal/fm1-x0x; github.com/zednaked/jangada; github.com/charlesvestal/fm1-fomni; github.com/AL-255/FM-1-RE; github.com/kurogedelic/FM-1-transporter. These are live projects; refresh before implementation but retain this baseline for comparison.
