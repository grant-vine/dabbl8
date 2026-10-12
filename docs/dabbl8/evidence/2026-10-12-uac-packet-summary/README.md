# Native USB packet summary

Issue #12 / #56. At eight tracks, the USB consumer built a packet and then
read every packet word again to decide whether the queued packet was silent.
The consumer now combines that local reduction with the existing ring-copy
loop, or includes the repeated last frame once on underrun. It publishes
`uq.packet` at the same point after packet generation. The standalone packet
helper and four-track path retain their existing behavior.

The maintained actual-C oracle compares packet bytes, boundary sentinels,
service state, ring contents, indices and counters with a frozen parent
packet generator. Each of the four fresh normal/TONE and inherited sanitizer
runner configurations passes 74,880 cases and 673,955 checks. Earlier focused
qualification passed 14 native executables and four standalone historical UAC
executables; four compiled mutants were detected. These scopes are separate.

The retained pinned FULL 4/8 application-only comparisons used exact prototype
`75976d35bbfb5874b036e1c7e66cf010101bbd12` against `e5d64a5`. Delivery is based on
`561f7d7`, whose only intervening change is the CI job timeout. All 211 relevant
firmware, build-tool, verification-helper and generated-header inputs are
byte-identical. The measured recipe imports `build_app` with `-DNPART=4/8`;
it is not the public `--dabbl8` CLI or a final package build. This is bounded
reuse of the recorded October 12 build recipe,
not a new delivery build or a promise independent of build date/configuration.
Four-track binary, ELF and disassembly are identical. The native application,
RAM and pool sizes are unchanged. The USB service shrinks from 500 to 492 bytes.

The service saves two additional registers: 36 to 44 bytes, with no local stack
reservation. Its caller and IRQ-entry allocations are unchanged. A nested
service call therefore adds eight bytes to the live audio/interrupt stack;
physical stack high-water and timing qualification remain required. The
packet-copy loop adds one listed instruction per copied frame and removes the
separate five-instruction reduction loop. That removes a redundant packet read
pass; it is not a cycle count or a measured deadline improvement.

Native IRQ and UAC static scores remain **55,767** and **562**, above their
unchanged limits of **41,252.2** and **554.4**. No budget or golden hash changed.
This qualification does not cover reduced-profile target builds, physical USB,
audio deadlines, recovery or a release. No firmware was installed or flashed.
Raw source, tests, failed controls, dependency pins, build logs and disassemblies
are retained locally; the compact [qualification receipt](qualification.json)
records their hashes and exact scope.
