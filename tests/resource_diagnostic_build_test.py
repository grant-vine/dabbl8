#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
# How to run: python3 tests/resource_diagnostic_build_test.py (local, no downloads).
"""Exercise actual builder entry points with stopped compiler/tool adapters."""
from __future__ import annotations

import contextlib
from collections.abc import Mapping
import importlib
import io
import json
import os
from pathlib import Path
import shutil
import shlex
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
from typing import Final

ROOT: Final = Path(__file__).resolve().parents[1]
EVIDENCE = Path(os.environ["D8_RESOURCE_BUILD_EVIDENCE_OUT"]) if "D8_RESOURCE_BUILD_EVIDENCE_OUT" in os.environ else None
if EVIDENCE is not None:
    EVIDENCE.mkdir()
sys.path.insert(0, str(ROOT / "tools"))
import build as builder  # noqa: E402


class CompilerStopped(RuntimeError):
    """Stop the real compiler command assembly before invoking any tool."""


def record(name: str, data: str) -> None:
    """Retain observed fixture outputs when the local recipe requests evidence."""
    if EVIDENCE is not None:
        (EVIDENCE / name).write_text(data)


class ResourceBuildTest(unittest.TestCase):
    def setUp(self) -> None:
        self.b = importlib.reload(builder)
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve()
        self.b.SRC = self.root
        self.b.FW = self.root / "firmware"
        self.b.OUT = self.root / "build"
        self.b.GEN = self.b.OUT / "gen"
        self.b.LDR = self.b.OUT / "loader"
        clean = {k: v for k, v in os.environ.items()
                 if not k.startswith(("FELUCCA_", "DABBL8_", "D8_")) and
                 k not in ("GOLDEN_UPDATE", "BUDGET_UPDATE")}
        environment = patch.dict(os.environ, {**clean, "FELUCCA_SIZE": "0"}, clear=True)
        environment.start()
        self.addCleanup(environment.stop)

    def test_ordinary_metadata_keeps_resource_capability_unavailable(self) -> None:
        # Given an ordinary build, when reading its actual configuration metadata.
        receipt = self.b.resource_diagnostic_metadata()
        # Then there is no added startup flag or collection capability.
        self.assertFalse(receipt["resource_diagnostic"])
        self.assertIsNone(receipt["resource_report_schema"])
        self.assertIsNone(receipt["resource_collection_initially_enabled"])
        self.assertEqual(receipt["startup_flags"], [])

    def test_uac_metadata_matches_explicit_app_flag(self) -> None:
        # Given resource mode with UAC explicitly disabled.
        self.b.RESOURCE_DIAGNOSTIC = self.b.TIMING_DIAGNOSTIC = True
        self.b.DABBL8_VERSION = "v0.1.0-timing"
        with patch.dict(os.environ, {"FELUCCA_UAC": "0"}):
            # When reading real metadata and C flag selection.
            receipt, flags = self.b.resource_diagnostic_metadata(), self.b.app_flags()
        # Then the collector-facing boolean matches the actual C override.
        self.assertFalse(receipt["resource_uac_enabled"])
        self.assertIn("-DFELUCCA_UAC=0", flags)

    def test_effective_mask_matches_actual_core_preprocessing(self) -> None:
        # Given full-profile switches and both curated profile selections.
        self.b.DABBL8_VERSION = "v0.1.0-timing"
        cases = [("full", fm4, slice_) for fm4 in (0, 1) for slice_ in (0, 1)]
        cases += [("core", 0, 1), ("breaks", 0, 1)]
        for profile, fm4, slice_ in cases:
            with self.subTest(profile=profile, fm4=fm4, slice=slice_), \
                 patch.dict(os.environ, {"DABBL8_ENGINE_PROFILE": profile,
                    "FELUCCA_FM4": str(fm4), "FELUCCA_SLICE": str(slice_)}):
                # When preprocessing the actual C availability contract.
                text = '#include "core.h"\n#if D8ENGINE_EFFECTIVE_MASK != ' + str(self.b.app_engine_mask()) + 'u\n#error mask mismatch\n#endif\n'
                result = subprocess.run([*shlex.split(os.environ.get("CC", "clang")), "-E", "-x", "c", "-I" + str(ROOT / "firmware/src"),
                    "-DNPART=8", f"-DFELUCCA_FM4={fm4}", f"-DFELUCCA_SLICE={slice_}",
                    *self.b.engine_profile_flags(), "-"], input=text, text=True, capture_output=True)
                # Then the Python manifest mask matches the compiled C mask.
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_actual_application_calls_forward_matching_flags_when_enabled(self) -> None:
        # Given the supported private eight-track timing configuration.
        self.b.DABBL8_VERSION = "v0.1.0-timing"
        self.b.TIMING_DIAGNOSTIC = True
        for enabled in (False, True):
            with self.subTest(enabled=enabled):
                self.b.RESOURCE_DIAGNOSTIC = enabled
                # When the actual build function assembles compiler calls.
                with patch.object(self.b, "tc_all", side_effect=CompilerStopped) as compile_calls:
                    with self.assertRaises(CompilerStopped):
                        self.b.build_app()
                commands = compile_calls.call_args.args
                record(f"compiler-calls-resource-{int(enabled)}.json",
                    json.dumps([[str(arg) for arg in command] for command in commands], indent=2))
                startup, main = commands[0], commands[3]
                # Then startup gets exactly its own flag, and C gets both.
                self.assertEqual(startup, ("cc", *(["-DD8_STACK_WATERMARK_DIAGNOSTIC=1"] if enabled else []),
                    "-c", self.b.FW / "crt0.S", "-o", self.b.OUT / "crt0.o"))
                self.assertIn("-DNPART=8", main)
                for flag in ("-DD8_HEAVY_POOL_DIAGNOSTIC=1", "-DD8_STACK_WATERMARK_DIAGNOSTIC=1"):
                    self.assertEqual(flag in main, enabled)
                self.assertNotIn("-DD8_OWNER_INITIALIZE=1", main)
                for command in commands[1:3]:
                    self.assertFalse(any(str(arg).startswith("-DD8_") for arg in command))

    def test_loader_calls_ignore_resource_mode(self) -> None:
        # Given enabled resource state, when the actual loader assembles calls.
        self.b.RESOURCE_DIAGNOSTIC = True
        with patch.object(self.b, "tc_all", side_effect=CompilerStopped) as compile_calls:
            with self.assertRaises(CompilerStopped):
                self.b.build_loader()
        # Then loader compiler arguments remain exactly their inherited forms.
        self.assertEqual(compile_calls.call_args.args, (
            ("cc", "-c", self.b.FW / "loader/crt0_ldr.S", "-o", self.b.LDR / "crt0_ldr.o"),
            ("cc", *self.b.CFLAGS, "-Ifirmware/hal", "-Ifirmware/src", "-c",
             self.b.FW / "loader/loader.c", "-o", self.b.LDR / "loader.o")))
        record("loader-compiler-calls.json", json.dumps(
            [[str(arg) for arg in command] for command in compile_calls.call_args.args], indent=2))

    def test_real_cli_refuses_before_dependency_or_output_work(self) -> None:
        # Given a copied real CLI, missing dependencies and a preserved sentinel.
        tools = self.root / "tools"
        tools.mkdir()
        for name in ("build.py", "fm1pkg_make.py", "lz4blk.py", "d8identity.py"):
            shutil.copyfile(ROOT / "tools" / name, tools / name)
        self.b.OUT.mkdir()
        sentinel = self.b.OUT / "preserve"
        sentinel.write_bytes(b"ordinary output")
        cases = ([], ["--dabbl8", "0.1"], ["--timing-diagnostic"],
                 ["--dabbl8", "0.1", "--timing-diagnostic", "--release", "0.1"])
        for args in cases:
            with self.subTest(args=args):
                # When requesting an unsupported resource configuration.
                result = subprocess.run([sys.executable, str(tools / "build.py"),
                    "--resource-diagnostic", *args], capture_output=True,
                    env={**os.environ, "JIELI_TOOLCHAIN": "/missing", "AC79_SDK": "/missing"})
                # Then argparse rejects before dependency errors or new output.
                self.assertEqual(result.returncode, 2)
                record(f"refused-profile-{cases.index(args)}.stderr", result.stderr.decode())
                self.assertEqual(list(self.b.OUT.iterdir()), [sentinel])
                self.assertEqual(sentinel.read_bytes(), b"ordinary output")
        for name, value, expected_exit in (("FELUCCA_CDC", "0", 2), ("FELUCCA_CDC", "banana", 2),
                ("FELUCCA_UAC", "banana", 2), ("FELUCCA_FM4", "banana", 2),
                ("DABBL8_ENGINE_PROFILE", "unknown", 1)):
            with self.subTest(name=name, value=value):
                result = subprocess.run([sys.executable, str(tools / "build.py"),
                    "--resource-diagnostic", "--dabbl8", "0.1", "--timing-diagnostic"],
                    capture_output=True, env={**os.environ, name: value,
                        "JIELI_TOOLCHAIN": "/missing", "AC79_SDK": "/missing"})
                self.assertEqual(result.returncode, expected_exit)
                record(f"refused-{name}-{value}.stderr", result.stderr.decode())
                self.assertEqual(list(self.b.OUT.iterdir()), [sentinel])

    def test_invalid_main_never_calls_dependency_adapters(self) -> None:
        # Given a forbidden four-track resource request and dependency tripwires.
        with patch.object(sys, "argv", ["build.py", "--resource-diagnostic", "--timing-diagnostic"]), \
             patch.object(self.b, "toolchain") as toolchain, \
             patch.object(self.b, "generate") as generate, \
             patch.object(self.b, "build_loader") as loader, \
             patch.object(self.b.subprocess, "check_output") as container, \
             patch.object(self.b.fm1pkg_make, "sdk_file") as sdk, \
             contextlib.redirect_stderr(io.StringIO()):
            # When the actual parser/entry function runs.
            with self.assertRaises(SystemExit) as result:
                self.b.main()
        # Then rejection occurs before any external or filesystem adapter.
        self.assertEqual(result.exception.code, 2)
        for adapter in (toolchain, generate, loader, container, sdk):
            adapter.assert_not_called()
        self.assertFalse(self.b.OUT.exists())

    def test_actual_manifest_metadata_matches_effective_profiles(self) -> None:
        # Given each accepted app-only profile and private timing-kit full mode.
        for app_only, profile in ((True, "full"), (True, "core"), (True, "breaks"), (False, "full")):
            with self.subTest(app_only=app_only, profile=profile):
                self.b.OUT = self.root / "build"
                self.b.GEN, self.b.LDR = self.b.OUT / "gen", self.b.OUT / "loader"
                compiler = self.root / "tool/pi32v2/bin/clang"
                compiler.parent.mkdir(parents=True, exist_ok=True)
                compiler.write_bytes(b"compiler fixture")
                def generate() -> None:
                    self.b.GEN.mkdir(parents=True)
                    (self.b.GEN / "fixture.h").write_bytes(b"generated fixture")
                def loader() -> bytes:
                    self.b.LDR.mkdir(parents=True)
                    (self.b.LDR / "ota.bin").write_bytes(b"loader fixture")
                    return b"loader fixture"
                def app() -> tuple[bytes, str, str, str]:
                    (self.b.OUT / "felucca.bin").write_bytes(b"app fixture")
                    (self.b.OUT / "felucca.dis").write_text("fixture")
                    symbols = "\n".join(f"{value:08x} *ABS* 0 {name}" for name, value in
                        (("_ustack_lo", 0x1C74100), ("_ustack_top", 0x1C7A000),
                         ("_sstack_lo", 0x1C7A100), ("_sstack_top", 0x1C7C000)))
                    return b"app fixture", symbols, "fixture", "fixture"
                def external(args: list[str | Path], **options: bool | Mapping[str, str]) -> subprocess.CompletedProcess[str] | subprocess.CompletedProcess[bytes]:
                    if options.get("text"):
                        return subprocess.CompletedProcess(args, 1, "", "")
                    return subprocess.CompletedProcess(args, 0, b"budget fixture", b"")
                args = ["build.py", "--dabbl8", "0.1", "--timing-diagnostic", "--resource-diagnostic"]
                if app_only:
                    args.append("--app-only")
                with patch.object(sys, "argv", args), \
                     patch.dict(os.environ, {"DABBL8_ENGINE_PROFILE": profile, "JIELI_DOCKER": "0"}), \
                     patch.object(self.b, "toolchain", return_value=compiler.parents[2]), \
                     patch.object(self.b, "tc", return_value="compiler fixture"), \
                     patch.object(self.b, "generate", side_effect=generate), \
                     patch.object(self.b, "build_loader", side_effect=loader) as loader_call, \
                     patch.object(self.b, "build_app", side_effect=app), \
                     patch.object(self.b, "check", return_value=([], [])), \
                     patch.object(self.b, "mmio_check", return_value=[]), \
                     patch.object(self.b.subprocess, "run", side_effect=external), \
                     patch.object(self.b.fm1pkg_make, "sdk_file", return_value=b"SDK fixture"), \
                     patch.object(self.b.fm1pkg_make, "ufw", return_value=b"package fixture"), \
                     patch.object(self.b.fm1pkg_make, "flash_image", return_value=b"flash fixture"), \
                     contextlib.redirect_stdout(io.StringIO()):
                    # When real main writes its manifest around mocked tool execution.
                    self.b.main()
                path = self.b.OUT / ("app-only-manifest.json" if app_only else "diagnostic-manifest.json")
                receipt = json.loads(path.read_text())
                record(f"{'app-only' if app_only else 'timing-kit'}-{profile}.json", path.read_text())
                # Then recorded capabilities/profile match actual selected flags.
                self.assertTrue(receipt["resource_diagnostic"])
                self.assertEqual(receipt["resource_report_schema"], "D8R1")
                self.assertFalse(receipt["resource_collection_initially_enabled"])
                self.assertEqual(receipt["startup_flags"], ["-DD8_STACK_WATERMARK_DIAGNOSTIC=1"])
                self.assertEqual(receipt["resource_engine_profile"], {"full": 0, "core": 1, "breaks": 2}[profile])
                self.assertEqual(receipt["resource_engine_mask"], {"full": 0x3FFD, "core": 0x1409, "breaks": 0x3411}[profile])
                self.assertEqual(receipt["engine_profile"], profile)
                self.assertTrue(receipt["resource_uac_enabled"])
                self.assertFalse(receipt["hardware_qualified"])
                self.assertFalse(receipt["flashing_authorized"])
                self.assertNotIn("-DD8_OWNER_INITIALIZE=1", receipt["compiler_flags"])
                self.assertIn("-DD8_HEAVY_POOL_DIAGNOSTIC=1", receipt["compiler_flags"])
                self.assertIn("-DD8_STACK_WATERMARK_DIAGNOSTIC=1", receipt["compiler_flags"])
                self.assertEqual(loader_call.call_count, int(not app_only))


if __name__ == "__main__":
    unittest.main(verbosity=2)
