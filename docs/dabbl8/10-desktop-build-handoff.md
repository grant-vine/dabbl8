# Desktop build handoff

Target environment: Apple Silicon Mac. Confirm the intended desktop, macOS version, Docker availability and device revision before executing setup. No desktop session is being controlled in this research turn. Keep the checkout under a Docker-shareable user folder and isolate Python dependencies in a virtual environment.

After choosing the GitHub owner and visibility, create a normal fork of hugelton/Felucca, clone that fork, add the original as upstream, and make a dabbl8/develop branch starting at the pinned SHA. Keep the original history and license. Do not rename package/USB identity before establishing the unmodified baseline. A future owner/repository URL must replace placeholders in the companion desktop guide.

Prerequisites from upstream: Git; Python 3 with Pillow and fontTools; Docker Desktop with linux/amd64 support; JieLi pi32v2 compiler; AC79 SDK tag AC79NN_SDK_V1.2.1_2023-12-13. Node is optional for web tests and Emscripten for the emulator. Installer dependencies mido and python-rtmidi are needed later, not for the first non-flashing baseline.

The upstream toolchain downloader streams a moving archive from pkgman.jieliapp.com. Review it before running; prefer downloading, hashing and retaining the exact archive. The package builder checks the expected SDK boot/config files. Record the SDK commit as well as the tag. License texts must accompany packages containing those SDK files.

Run ./build.sh, then tests/run_tests.sh with the same SDK environment. Tests use generated build outputs, so do not assume they all run before the build. Preserve build/felucca.bin, build/loader/ota.bin, build/felucca.fwsc, linker reports and test logs privately as baseline evidence. The first goal is a successful build, not an installation. Do not update golden audio hashes to hide regressions.

Optional simulator follow-up: web/emu/build.sh and a localhost server serving build/emu. The simulator does not emulate USB audio/MIDI-out/update transport, TRS MIDI or user sample slots. Use it for sequencing/UI, not a USB or hardware qualification claim.

macOS qualification includes USB SERIAL on/off, reconnect, sleep/wake, stereo capture at both supported sample rates, MIDI clock, device selection and coexistence with the studio’s other USB MIDI gear. Old issue reports and older macOS workarounds require retesting on the actual host. No install commands are included in the first-build checklist to prevent an accidental flash.
