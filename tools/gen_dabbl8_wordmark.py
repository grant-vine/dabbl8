#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Explicit offline branding generation; not a firmware build dependency.
Requires local rsvg-convert and Pillow. Supersample then encode existing 4-bit alpha.
"""
import io, subprocess
from pathlib import Path
from PIL import Image
root = Path(__file__).resolve().parents[1]
svg = root / 'assets/branding/dabbl8-wordmark.svg'
png = subprocess.check_output(['rsvg-convert', '-w', '2560', '-h', '576', str(svg)])
a = Image.open(io.BytesIO(png)).getchannel('A').resize((160, 36), Image.Resampling.BOX)
n = [(v * 15 + 127) // 255 for v in a.getdata()]
b = [(n[i] << 4) | n[i+1] for i in range(0, len(n), 2)]
text = '/* SPDX-License-Identifier: GPL-3.0-only */\n/* Generated offline from assets/branding/dabbl8-wordmark.svg. */\nstatic const uint8_t D8_WORDMARK_ALPHA[2880] = {\n'
text += '\n'.join('    ' + ','.join('0x%02x' % x for x in b[i:i+24]) + ',' for i in range(0,len(b),24))
(root / 'firmware/src/dabbl8_wordmark.h').write_text(text + '\n};\n')
