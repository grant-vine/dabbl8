#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Inventory immutable Git blobs; no filesystem dependencies or donor-rights inference."""
import hashlib
import json
from pathlib import Path
import subprocess

SOURCE = "b2466bf9bc1c675ed8b8f667d37af8f5e4e4b377"
UPSTREAM = "276f72a4e6ea8a12499a7a6819aadf3165126755"
ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent / "inventory.json"

def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT)

def entries(commit):
    rows = {}
    for item in git("ls-tree", "-rz", commit).split(b"\0"):
        if not item:
            continue
        meta, path = item.split(b"\t", 1)
        mode, kind, oid = meta.decode().split()
        if kind == "blob":
            rows[path.decode()] = (mode, oid)
    return rows

def blob(oid):
    return git("cat-file", "blob", oid)

def sha(data):
    return hashlib.sha256(data).hexdigest()

current, old = entries(SOURCE), entries(UPSTREAM)
records, errors = [], []
suffixes = {".c", ".h", ".py", ".sh", ".js", ".mjs", ".html", ".css"}
for path, (mode, oid) in sorted(current.items()):
    if path != "build.sh" and not (path.startswith(("firmware/", "tools/", "tests/", "web/")) and Path(path).suffix in suffixes):
        continue
    data = blob(oid)
    text = data[:8192].decode("utf-8", errors="replace")
    declarations = [line.strip() for line in text.splitlines() if "SPDX-License-Identifier:" in line]
    prior = old.get(path)
    state = "new" if prior is None else "unchanged" if prior[1] == oid else "modified"
    if prior:
        before = blob(prior[1])[:8192].decode("utf-8", errors="replace")
        for line in before.splitlines():
            if "SPDX-License-Identifier:" in line or "copyright" in line.lower():
                if line.strip() not in text:
                    errors.append({"path": path, "missing_notice_line": line.strip()})
    records.append({"path": path, "git_mode": mode, "git_blob": oid, "sha256": sha(data), "upstream_blob": prior[1] if prior else None, "relative_to_upstream": state, "literal_spdx_lines": declarations})

# Explicit non-code license/attribution/asset identities, not inferred SPDX.
notices = ["LICENSE", "LICENSING.md", "assets/samples-cc0/ATTRIBUTION.txt", "assets/fonts/OFL.txt", "assets/fonts/InterTight[wght].ttf", "web/fukiai.ttf", "web/FUKIAI-LICENSE.txt", "web/emu/fonts/OFL.txt", "web/emu/fonts/DotGothic16-subset.woff"]
notices += [path for path in sorted(current) if path.startswith("LICENSES/")]
assets = {path: {"git_blob": current[path][1], "sha256": sha(blob(current[path][1]))} for path in notices}
for path in ("firmware/src/phys_dsp.c", "firmware/src/phys_symp.c", "firmware/src/fm6_core.c"):
    before = blob(old[path][1]).split(b"*/", 1)[0] + b"*/"
    assert blob(current[path][1]).startswith(before), path
for path in ["LICENSE"] + [path for path in current if path.startswith("LICENSES/")]:
    assert current[path][1] == old[path][1], path
assert not errors, errors
result = {"source_commit": SOURCE, "upstream_commit": UPSTREAM, "scope": "Immutable tracked code blobs and explicit notice/asset pins; no generated headers, external SDK or release bundle audit", "summary": {"code_files": len(records), "new_files": sum(row["relative_to_upstream"] == "new" for row in records), "modified_files": sum(row["relative_to_upstream"] == "modified" for row in records), "without_literal_spdx": [row["path"] for row in records if not row["literal_spdx_lines"]], "upstream_notice_lines_preserved": True, "three_port_headers_byte_preserved": True, "license_text_blobs_unchanged": True}, "notice_asset_pins": assets, "code_files": records, "release_qualification": "NOT_ESTABLISHED"}
OUT.write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps(result["summary"], indent=2))
