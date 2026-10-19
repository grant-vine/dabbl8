# SPDX-License-Identifier: GPL-3.0-only
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
# How to run: python3 ci/local_checks.py --check clock-capture (plan only).
"""Offline D8C1 formatter, calibration math and collector refusal checks."""
from pathlib import Path
import shlex
import sys
from typing import Final

from local_flash_checks import LocalStep

CLOCK_MAP: Final = {'clock-capture': {
    'firmware/src/audio_clock_console.h', 'tools/dabbl8_clock_bounds.py',
    'tools/dabbl8_clock_capture.py', 'tests/dabbl8_clock_test.py',
}}


def clock_recipes(cc: list[str], out: Path) -> list[LocalStep]:
    """Run only injected transports and actual C console slices; no native ports."""
    return [LocalStep(name='clock-console', argv=['env', 'CC=' + shlex.join(cc),
        'SANITIZE=1', 'D8_TIMING_CONSOLE_OUT=' + str(out / 'clock-console'),
        sys.executable, 'tests/audio_timing_console_test.py'], requires=[
            'tests/audio_timing_console_test.py', 'firmware/src/console.c',
            'firmware/src/audio_clock_console.h', 'firmware/src/audio_timing_console.h',
            'firmware/src/audio_timing.h', 'firmware/src/resource_diagnostic_console.h',
            'firmware/src/native_first_use_diagnostic.h', 'firmware/src/flash_trace_diagnostic.h',
            'firmware/hal/fm1_stack_watermark.h', 'firmware/hal/fm1_time.h',
            'tools/dabbl8_clock_bounds.py']),
        LocalStep(name='clock-bounds-collector', argv=[sys.executable, 'tests/dabbl8_clock_test.py'],
            requires=['tests/dabbl8_clock_test.py', 'tools/dabbl8_clock_bounds.py',
                      'tools/dabbl8_clock_capture.py', 'tools/dabbl8_timing_capture.py',
                      'tools/dabbl8_timing_report.py'])]
