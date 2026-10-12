#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Extract the unchanged target storage-range declarations for host spies.

The target header emits used pi32v2 routines and cannot be included by a Mac
compiler. This fixture input contains its actual declarations, never a TRUE
permission replacement. It authorizes no device operation.
"""
import hashlib
import json
import re
from pathlib import Path
import sys


def generate(output):
    source = Path(__file__).resolve().parents[1] / "firmware/hal/fm1_flash.h"
    raw = source.read_bytes()
    text = raw.decode("utf-8")
    first, last = "#define FL_DATA_LO", "/* Where the RAM driver"
    if text.count(first) != 1 or text.count(last) != 1:
        raise ValueError("storage declaration boundaries changed; review required")
    declarations = text[text.index(first):text.index(last)]
    for name in ("FL_DATA_LO", "FL_DATA_HI", "FL_GLOB_LO", "FL_GLOB_HI",
                 "FL_OTA_LO", "FL_OTA_HI", "FL_AUTO_LO", "FL_AUTO_HI", "FL_IN", "FL_STORE_OK"):
        if len(re.findall(r"^#define " + name + r"(?:\s|\()", declarations, re.M)) != 1:
            raise ValueError("missing/duplicate actual storage declaration: " + name)
    notice = text[:text.index("/* FM-1 SPI NOR")]
    header = (notice + "/* Test-only exact HAL declaration extraction; no native primitives. */\n" + declarations).encode()
    output.mkdir(parents=True, exist_ok=True)
    (output / "d8_flash_store_ranges_test.h").write_bytes(header)
    report = {"source": "firmware/hal/fm1_flash.h", "source_sha256": hashlib.sha256(raw).hexdigest(),
              "extracted_declarations_sha256": hashlib.sha256(declarations.encode()).hexdigest(),
              "header_sha256": hashlib.sha256(header).hexdigest(), "native_operations": False}
    (output / "d8_flash_store_ranges_test.json").write_text(json.dumps(report, indent=2) + "\n")
    print("PASS exact target HAL storage-range extraction; no native operations")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: native_flash_ranges_test.py OUTPUT_DIRECTORY")
    generate(Path(sys.argv[1]))
