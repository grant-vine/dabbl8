#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Dabbl8 contributors
import pathlib, sys, subprocess
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from d8identity import dabbl8_identity
import fm1_install as installer
n = 0
for version, canonical, product in [("0.1", "v0.1.0", "FM-1_8010"), ("0.1-dev", "v0.1.0-dev", "FM-1_8010"), ("1.2.3-rc1", "v1.2.3-rc1", "FM-1_8123"), ("9.9.9-abcdefgh", "v9.9.9-abcdefgh", "FM-1_8999")]:
    i = dabbl8_identity(version)
    assert (i.version, i.product) == (canonical, product)
    assert len("DABBL8 " + i.version) <= 24
    assert installer.model_of(i.product) == installer.model_of("ota-FM-1_900")
    assert i.package == "dabbl8-" + canonical[1:] + ".fwsc"
    n += 5
products = {dabbl8_identity(f"{a}.{b}.{c}").product for a in range(10) for b in range(10) for c in range(10)}
assert len(products) == 1000 and not any(p.startswith("FM-1_9") for p in products)
n += 2
for bad in ["", "0", "0.1.0.1", "10.1", "0.10", "0.1.10", "0.1-", "0.1-ABCDEFGHI", "0.1-abcdefghi", "0.1-rc-1", "0.1/../../", "0.1\n", "0.1 DEV", "０.１"]:
    try: dabbl8_identity(bad)
    except ValueError: n += 1
    else: raise AssertionError(bad)
for args in [("--dabbl8", "0.1-dev", "--release", "0.1"), ("--dabbl8", "0.1-abcdefghi")]:
    p = subprocess.run([sys.executable, "tools/build.py", *args], text=True, capture_output=True)
    assert p.returncode == 2 and "error:" in p.stderr and "JIELI_TOOLCHAIN" not in p.stderr and "AC79_SDK" not in p.stderr
    n += 1
print(f"Dabbl8 identity: {n} checks passed; parser/model/namespace only, no device or release")
