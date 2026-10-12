<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Dabbl branding masters

`dabbl-wordmark.svg` is the owner's supplied original, preserved byte-for-byte
for future website use. Do not overwrite it when revising firmware branding.
`dabbl8-wordmark.svg` is a separate derivative: the original five letter paths
are unchanged and an independently drawn outlined digit 8 is appended.
Both masters remain vector artwork with transparent backgrounds.

`dabbl8-painted-wordmark.svg` is the current firmware master. The original five
letter paths are retained, with only the approved painted8 concept traced into
separate paths using local Potrace. The previous outlined8 master remains.

The firmware uses a static160 ×50, 4-bit alpha mask in flash (4000 bytes),
blended through the existing theme ramp. No extra persistent RAM, animation,
startup wait or runtime SVG renderer is introduced. The centred version, smaller "Based on Felucca" and
"Hügelton + community" credits remain below it; full upstream attribution remains
in About/licensing. The mask centre is the display centre (120,120). Unbranded Felucca
builds retain the original splash.

Explicit offline regeneration with existing local `rsvg-convert` and Pillow:
`python3 tools/gen_dabbl8_wordmark.py`. The generated header is tracked, so normal
firmware builds do not need these tools or download anything.

Original SVG SHA-256: `098e083f263ca4ba4b3b77a125284d35fac9025c044061e3321edcbff6a8dc62`.
