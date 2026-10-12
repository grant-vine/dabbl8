<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Dabbl branding masters

`dabbl-wordmark.svg` is the owner's supplied original, preserved byte-for-byte
for future website use. Do not overwrite it when revising firmware branding.
`dabbl8-wordmark.svg` is a separate derivative: the original five letter paths
are unchanged and an independently drawn outlined digit 8 is appended.
Both masters remain vector artwork with transparent backgrounds.

The firmware uses a static 160 × 36, 4-bit alpha mask in flash (2880 bytes),
blended through the existing theme ramp. No extra persistent RAM, animation,
startup wait or runtime SVG renderer is introduced. The version and inherited
Hügelton Instruments/community credits remain below it. Unbranded Felucca
builds retain the original splash.

Explicit offline regeneration with existing local `rsvg-convert` and Pillow:
`python3 tools/gen_dabbl8_wordmark.py`. The generated header is tracked, so normal
firmware builds do not need these tools or download anything.

Original SVG SHA-256: `098e083f263ca4ba4b3b77a125284d35fac9025c044061e3321edcbff6a8dc62`.
