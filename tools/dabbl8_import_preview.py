#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Preserve a standalone legacy project and produce a non-installable preview."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def prepare(source, destination, importer):
    # Read once; the subprocess uses the preserved snapshot, never a changing input.
    original = Path(source).read_bytes()
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=False)
    backup = destination / "original.bin"
    backup.write_bytes(original)
    report = {
        "format": "dabbl8-import-report", "version": 1,
        "source_name": Path(source).name,
        "original_file": "original.bin", "original_bytes": len(original),
        "original_sha256": hashlib.sha256(original).hexdigest(),
        "accepted": False, "device_writes": False, "firmware_project": False,
        "warnings": [
            "This is an offline semantic preview, not a D8P1 project or firmware download.",
            "The original is retained byte for byte, including unknown or reserved data.",
            "Tracks 5-8 in the preview use explicit empty defaults; no historical padding is treated as music.",
            "The pinned FUN1..FUN9 importer maps parameter IDs, fills defaults and applies historical engine/step migrations.",
            "A semantic preview is not a lossless substitute for the original; reserved bytes and unsupported interpretations stay in original.bin.",
            "DIGITAL-to-FM6 and SAMPLE-PERC-to-DRUM conversions may remove incompatible engine automation; removed records are counted.",
            "No flash layout, project upload, backup restore or hardware behavior is qualified by this tool.",
        ],
    }
    try:
        result = subprocess.run([str(Path(importer).resolve()), str(backup.resolve())],
                                capture_output=True, text=True, check=False)
        report["importer_exit_code"] = result.returncode
        if result.stderr:
            report["diagnostics"] = result.stderr
        preview = json.loads(result.stdout)
        if preview.get("format") != "dabbl8-import-preview" or preview.get("version") != 1:
            raise ValueError("Unrecognized importer output")
        if result.returncode == 0 and preview.get("accepted") is True:
            report["accepted"] = True
            report["source_format"] = preview["source_format"]
            report["changes"] = preview["changes"]
            report["removed_motion_records"] = preview["removed_motion_records"]
            report["preview_file"] = "preview.json"
            text = json.dumps(preview, indent=2) + "\n"
            (destination / "preview.json").write_text(text)
            report["preview_sha256"] = hashlib.sha256(text.encode()).hexdigest()
        else:
            report["reason"] = preview.get("reason", "Importer refused the project")
    except (OSError, ValueError, KeyError) as error:
        report["reason"] = str(error)
    (destination / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path, help="New directory; existing paths are refused")
    parser.add_argument("--importer", type=Path, default=Path("build/host/dabbl8_import_preview"))
    args = parser.parse_args()
    report = prepare(args.source, args.destination, args.importer)
    print(json.dumps(report, indent=2))
    return 0 if report["accepted"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
