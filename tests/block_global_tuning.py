#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Eight-track tuning differential receipt; no golden/budget rewrite mode.

The expected trace was measured from unchanged984f4802ba3e284cd9cd08f4c3606d363877eb49.
Requires the repository's generated build/gen headers. Run directly, optionally
--control /path/to/clean984f to independently reproduce the frozen receipt.
Inherited sanitizer exclusions match the existing host-test policy; this is not
strict whole-engine UBSan qualification or a hardware timing test.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

BASE = "984f4802ba3e284cd9cd08f4c3606d363877eb49"
EXPECTED = ("cases=42016 checks=211114 failures=0 silent=0 unadmitted=0 "
            "arrangements=6 audible_arrangements=6 audio_state_trace=aeb1daebac64c4a8\n")
SAN = ["-O1", "-g", "-fsanitize=address,undefined",
       "-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow",
       "-fno-sanitize-recover=undefined", "-w"]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--control", type=Path, help="Unchanged checkout at frozen BASE")
    parser.add_argument("--evidence-dir", type=Path, help="Retain executables and verbatim logs")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    sources = [("candidate", repo)]
    if args.control:
        control = args.control.resolve()
        head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=control, text=True).strip()
        if head != BASE or subprocess.check_output(["git", "diff", "HEAD", "--", "firmware/src"], cwd=control):
            parser.error("control must have unchanged firmware source at " + BASE)
        sources.insert(0, ("frozen", control))
    for _, source in sources:
        if not (source / "build/gen").is_dir():
            parser.error("generate build/gen headers first in " + str(source))
    with tempfile.TemporaryDirectory(prefix="dabbl8-tuning-") as temporary:
        evidence = args.evidence_dir.resolve() if args.evidence_dir else Path(temporary)
        evidence.mkdir(parents=True, exist_ok=True)
        for mode, flags in [("normal", ["-O2", "-w"]), ("inherited-san", SAN)]:
            for label, source in sources:
                binary = evidence / (mode + "-" + label)
                cmd = [os.environ.get("CC", "clang"), *flags, "-I" + str(source / "build/gen"),
                       "-I" + str(source / "firmware/src"),
                       "-DD8_TUNING_HOSTSIM=" + json.dumps(str(source / "tests/hostsim.c"))]
                if label == "candidate":
                    cmd.append("-DD8_TUNING_OPTIMIZED=1")
                cmd += [str(repo / "tests/block_global_tuning.c"), "-lm", "-o", str(binary)]
                compiled = subprocess.run(cmd, cwd=source, text=True, capture_output=True)
                (evidence / (mode + "-" + label + "-compile.log")).write_text(
                    repr(cmd) + "\n" + compiled.stdout + compiled.stderr + "\nexit=" + str(compiled.returncode) + "\n")
                if compiled.returncode:
                    raise SystemExit(compiled.stderr)
                tested = subprocess.run([str(binary)], cwd=source, text=True, capture_output=True,
                                        env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                (evidence / (mode + "-" + label + ".log")).write_text(
                    tested.stdout + tested.stderr + "\nexit=" + str(tested.returncode) + "\n")
                print(mode, label, tested.stdout.strip(), flush=True)
                if tested.returncode or tested.stdout != EXPECTED or tested.stderr:
                    raise SystemExit("FAIL: frozen receipt mismatch or runtime diagnostic; inspect " + str(evidence))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
