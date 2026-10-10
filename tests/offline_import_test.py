#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Real legacy fixtures, loss reporting and preservation/refusal boundaries."""
import hashlib
import importlib.util
import json
import random
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("preview", ROOT / "tools/dabbl8_import_preview.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
exe = Path(sys.argv[1]).resolve()
fixtures = ROOT / "tests/fixtures/projects"
checks = 0


def check(condition, label):
    global checks
    if not condition:
        raise AssertionError(label)
    checks += 1


def inspect(path):
    p = subprocess.run([str(exe), str(path)], capture_output=True, text=True)
    return p.returncode, json.loads(p.stdout)


with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    originals = sorted(p for p in fixtures.glob("*.bin") if ".expected-fun9" not in p.name)
    check(len(originals) == 13, "frozen fixture coverage")
    for original in originals:
        dest = tmp / original.stem
        before = original.read_bytes()
        report = module.prepare(original, dest, exe)
        check(report["accepted"], original.stem + " accepted")
        check(original.read_bytes() == before == (dest / "original.bin").read_bytes(), "original retained")
        check(report["original_sha256"] == hashlib.sha256(before).hexdigest(), "original hash")
        preview = json.loads((dest / "preview.json").read_text())
        rc, reference = inspect(original.with_name(original.stem + ".expected-fun9.bin"))
        check(rc == 0, "baseline canonical reference accepted")
        for key in ["globals", "selected_track", "parts", "phys", "name_bytes", "tracks", "chain", "motion"]:
            check((preview[key][:4] == reference[key][:4]) if key == "tracks" else (preview[key] == reference[key]), original.stem + " canonical semantic equality " + key)
        check(preview["track_count"] == 8 and len(preview["tracks"]) == 8, "eight tracks")
        check(all(len(t["params"]) == 99 and len(t["steps"]) == 64 and len(t["fm6"]) == 128 for t in preview["tracks"]), "complete arrays")
        check(all(s["n"] == 0 and s["hit"] == 0 and s["time"] == 2 for t in preview["tracks"][4:] for s in t["steps"]), "upper tracks empty rests")
        check(all(e["place"] < 256 for e in preview["motion"]["events"]), "legacy automation stays tracks 1-4")
        check(report["firmware_project"] is False and report["device_writes"] is False, "preview only")
        try:
            module.prepare(original, dest, exe)
        except FileExistsError:
            pass
        else:
            raise AssertionError("existing output overwritten")
        check((dest / "original.bin").read_bytes() == before, "existing backup untouched")
        # Every damaged record must be refused without losing its original.
        changed = bytearray(before); changed[100] ^= 1
        damaged = tmp / "damaged.bin"; damaged.write_bytes(changed)
        failure = module.prepare(damaged, tmp / (original.stem + "-damaged"), exe)
        check(not failure["accepted"], "corrupt checksum refused")
        check((tmp / (original.stem + "-damaged") / "original.bin").read_bytes() == changed, "corrupt original preserved")
        check(not (tmp / (original.stem + "-damaged") / "preview.json").exists(), "no corrupt preview")
    digital = json.loads((tmp / "fun9-digital/report.json").read_text())
    check(bool(digital["changes"]), "DIGITAL engine migration reported")
    phys = json.loads((tmp / "fun4-phys-drum/report.json").read_text())
    check(bool(phys["changes"]), "PHYS DRUM engine migration reported")
    # A synthetic copy, never a changed fixture: DIGITAL automation drops while
    # a track-4 step-64 signed lock remains at its original address.
    changed = bytearray((fixtures / "fun9-digital.bin").read_bytes())
    motion_off = 68 + 4 * (99 + 2 + 64 * 9) + 36
    changed[motion_off:motion_off + 260] = bytes(260)
    changed[motion_off:motion_off + 4] = bytes([2, 1, 0, 0])
    changed[motion_off + 4:motion_off + 8] = bytes([1, 91, 64, 0])
    changed[motion_off + 8:motion_off + 12] = bytes([255, 0x87, 224, 255])
    fnv = 2166136261
    for b in changed[:-4]:
        fnv = ((fnv ^ b) * 16777619) & 0xffffffff
    changed[-4:] = fnv.to_bytes(4, "little")
    path = tmp / "migration.bin"; path.write_bytes(changed)
    dest = tmp / "migration"
    report = module.prepare(path, dest, exe)
    check(report["accepted"] and report["removed_motion_records"] == 1, "removed DIGITAL automation reported")
    preview = json.loads((dest / "preview.json").read_text())
    check(preview["motion"]["events"] == [{"place": 255, "param": 0x87, "value": -32}], "unrelated signed lock retained")
    check((dest / "original.bin").read_bytes() == changed, "dropped automation remains recoverable in original")
    # Future format, short header, padded record, oversize and random bytes.
    good = (fixtures / "fun9.bin").read_bytes()
    future = bytearray(good); future[:4] = (0x46554E41).to_bytes(4, "little")
    rng = random.Random(11)
    refused = [b"", b"FUN", good[:7], good[:-1], good + b"\0", bytes(future), b"x" * 3649]
    refused += [rng.randbytes(n) for n in [8, 688, 2552, 2584, 2680, 3352, 3388, 3584, 3648]]
    for i, data in enumerate(refused):
        path = tmp / "refused.bin"; path.write_bytes(data)
        dest = tmp / ("refused-" + str(i))
        report = module.prepare(path, dest, exe)
        check(not report["accepted"] and (dest / "original.bin").read_bytes() == data, "unsupported original preserved")
        check(not (dest / "preview.json").exists(), "unsupported preview absent")
    missing = module.prepare(fixtures / "fun9.bin", tmp / "missing", tmp / "no-importer")
    check(not missing["accepted"] and (tmp / "missing/original.bin").read_bytes() == good, "missing executable retains original and report")
print(f"offline import: {checks} checks passed; 13 pristine fixtures; original preservation and explicit refusal")
