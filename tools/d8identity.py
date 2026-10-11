# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Dabbl8 contributors
"""Coordinated experimental identity. This grants no release or write authority."""
import re
from dataclasses import dataclass

@dataclass(frozen=True)
class Identity:
    version: str
    product: str
    package: str

def dabbl8_identity(value):
    # Separate numeric family from upstream FM-1_9XY; retain FM-1 loader model.
    m = re.fullmatch(r"([0-9])\.([0-9])(?:\.([0-9]))?(-[a-z0-9]{1,8})?", value)
    if not m:
        raise ValueError("Dabbl8 version: X.Y[.Z][-suffix], single digits, lowercase suffix of 1..8 letters/digits")
    version = f"{m[1]}.{m[2]}.{m[3] or '0'}{m[4] or ''}"
    return Identity("v" + version, "FM-1_8" + m[1] + m[2] + (m[3] or '0'), "dabbl8-" + version + ".fwsc")
