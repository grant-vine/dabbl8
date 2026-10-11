# Resampler channel-array bounds

The USB48 FIR previously formed a pointer into `rs.l` and read right-channel
samples with `x[2 * RS_H]`. Left and right histories are distinct arrays in a
struct. Their current adjacency does not permit indexing from one array into the
other under C's array-pointer rules ([WG14 N1570, 6.5.6p8](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)).

Use a pointer to each channel's own history and advance both together. Coefficients,
phase order, Q30 sums, rounding/clamping, history layout and ring/queue publication
remain unchanged. The existing static assertion requires FIR length at most RS_H;
starting index is 0..RS_H-1, so the largest dereferenced index is at most
2*RS_H-2, within each 2*RS_H-element array. This removes a source-language undefined
operation; it does not establish that the current device previously malfunctioned.

On current main `fbadbca894419c9626306eb773f38ae22baed52d`, normal and inherited DSP
sanitizer tests each cover 31,200 exact frozen-parent cases per normal/TONE executable
(all 160 phases, block lengths 0..64 and three histories). Counts are 2,246,436 and
2,277,636 checks, zero failures. Existing standalone USB audio tests also pass with
CDC on/off, normally and under strict ASan/UBSan, including independent floating
resampling/response/SNR and packet/rate tests. The native DSP suites retain their
existing sanitizer exclusions; the frozen historical reference is unchanged.

Pinned vendor app-only builds at four and eight tracks have byte-identical
application, ELF and disassembly to main. Four-track app: 447,952 bytes, SHA-256
`a3a87f6fe62d427bfb0f7ebc8c3f5905717e8fa636c641bd94886240331ae096`,
RAM 92,116/98,304, pool 331,332/344,064. Eight-track app: 480,284 bytes, SHA-256
`36b2b0000e6d5457219b29242a1601c734059aab39acc263779ffa51b270b24d`,
RAM 96,068/98,304, pool 335,240/344,064. Identical retained generated headers are
used in those comparisons; no loader/package generation or device installation
is included. Four-track static budgets pass. Eight-track IRQ/UAC static budgets
still fail at 55,767/555 against 37,502/504 with unchanged thresholds. No timing or
memory improvement is claimed. Future compiler changes still require verification.

[Raw evidence and hash manifest](evidence/2026-10-11-uac-array-bounds/) retain
host logs, target/compiler provenance, memory results and unchanged failures.
Complete target products/source snapshots remain in the ignored local baseline
folder. Golden hashes, loader and flash boundaries are untouched. Hardware timing,
recovery and release gates remain open.
