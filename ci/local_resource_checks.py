# SPDX-License-Identifier: GPL-3.0-only
"""Bounded local resource recipes; no target compilation or device access."""
from collections.abc import Callable
from pathlib import Path
import shlex
import sys
from typing import Final, Literal
from local_flash_checks import FIRST_USE_HEADERS, LocalStep

ResourceCheck = Literal['resource-build', 'resource-capture', 'resource-console',
                        'heavy-pool-diagnostic', 'stack-watermark']
RESOURCE_MAP: Final[dict[ResourceCheck, set[str]]] = {
    'resource-build': {'tools/build.py', 'tests/resource_diagnostic_build_test.py'},
    'resource-capture': {'tools/dabbl8_resource_capture.py', 'tools/dabbl8_resource_report.py',
                         'tests/dabbl8_resource_capture_test.py'},
    'resource-console': {'firmware/src/resource_diagnostic_console.h', 'tests/resource_diagnostic_console_test.c'},
    'heavy-pool-diagnostic': {'firmware/src/heavy_pool_diagnostic.h', 'tests/heavy_pool_diagnostic_test.c'},
    'stack-watermark': {'firmware/crt0.S', 'firmware/hal/fm1_stack_watermark.h',
                        'tests/stack_watermark_test.c', 'tests/run_stack_watermark_checks.sh'},
}
GENERATED: Final[list[str]] = ['build/gen/' + name for name in FIRST_USE_HEADERS]
CORE_INPUTS: Final[list[str]] = ['firmware/src/core.h', 'firmware/src/track_limits.h',
    'firmware/src/identity.h', 'firmware/src/native_migration_session.h',
    'firmware/src/engine_build_profile.h']


def build_recipes(cc: list[str], out: Path) -> list[LocalStep]:
    """Drive actual CLI/functions with stopped compiler/tool fixture adapters."""
    return [LocalStep(name='resource-build-functions',
        argv=['env', 'D8_RESOURCE_BUILD_EVIDENCE_OUT=' + str(out / 'resource-build'),
              'CC=' + shlex.join(cc), sys.executable, 'tests/resource_diagnostic_build_test.py'],
        requires=['tools/build.py', 'tools/fm1pkg_make.py', 'tools/lz4blk.py',
                  'tools/d8identity.py', 'tests/resource_diagnostic_build_test.py'] + CORE_INPUTS)]


def console_recipes(cc: list[str], out: Path) -> list[LocalStep]:
    """Compile real formatter/parser fixtures for each effective profile."""
    steps: list[LocalStep] = []
    source = 'tests/resource_diagnostic_console_test.c'
    required = [source, 'firmware/src/console.c', 'firmware/src/resource_diagnostic_console.h',
        'firmware/src/heavy_pool_diagnostic.h', 'firmware/src/heavy_state.h',
        'firmware/src/audio_timing.h', 'firmware/src/audio_timing_console.h',
        'firmware/src/audio_clock_console.h',
        'firmware/src/native_first_use_diagnostic.h', 'firmware/src/flash_trace_diagnostic.h',
        'firmware/hal/fm1_stack_watermark.h', 'tests/hostsim.c'] + CORE_INPUTS + GENERATED
    flags = ['-std=c11', '-w', '-DNPART=8', '-DFELUCCA_FLASH=0', '-DFELUCCA_FM4=0', '-DFELUCCA_SLICE=1',
        '-DFELUCCA_CDC=1', '-DFELUCCA_UAC=0', '-DDABBL8_IDENTITY=1',
        '-DDABBL8_TIMING_DIAGNOSTIC=1', '-DD8_HEAVY_POOL_DIAGNOSTIC=1',
        '-DD8_STACK_WATERMARK_DIAGNOSTIC=1', '-Ibuild/gen', '-Ifirmware/src', '-Ifirmware/hal']
    for profile in (0, 1, 2):
        for sanitized in (False, True):
            mode = 'sanitized' if sanitized else 'optimized'
            tag = 'resource-console-' + str(profile) + '-' + mode
            binary = str(out / tag)
            mode_flags = ['-O1', '-g', '-fsanitize=address,undefined',
                          '-fno-sanitize-recover=all'] if sanitized else ['-O2']
            steps += [LocalStep(name=tag + '-compile', argv=cc + flags + mode_flags +
                ['-DD8BUILD_PROFILE=' + str(profile), source, '-lm', '-o', binary], requires=required),
                LocalStep(name=tag, argv=[binary], requires=[])]
    return steps


def capture_recipes(cc: list[str], out: Path) -> list[LocalStep]:
    """Reuse one optimized full-profile console binary for wire interop."""
    steps = console_recipes(cc, out)
    steps.append(LocalStep(name='resource-capture-scripted-transport',
        argv=['env', 'D8_RESOURCE_CONSOLE_BIN=' + str(out / 'resource-console-0-optimized'),
              sys.executable, 'tests/dabbl8_resource_capture_test.py'],
        requires=['tools/dabbl8_resource_capture.py', 'tools/dabbl8_timing_capture.py',
                  'tools/dabbl8_resource_report.py', 'tools/dabbl8_timing_report.py',
                  'tests/dabbl8_resource_capture_test.py']))
    return steps


def heavy_recipes(cc: list[str], out: Path) -> list[LocalStep]:
    """Check diagnostic admission/census behavior without DSP rendering."""
    source = 'tests/heavy_pool_diagnostic_test.c'
    required = [source, 'tests/hostsim.c', 'firmware/src/heavy_state.h',
                'firmware/src/heavy_pool_diagnostic.h'] + CORE_INPUTS + GENERATED
    steps: list[LocalStep] = []
    for sanitized in (False, True):
        mode = 'sanitized' if sanitized else 'optimized'
        tag = 'heavy-pool-diagnostic-' + mode
        binary = str(out / tag)
        flags = ['-std=c11', '-w', '-DNPART=8', '-DFELUCCA_FM4=0', '-DFELUCCA_SLICE=1',
                 '-DD8_HEAVY_POOL_DIAGNOSTIC=1', '-Ibuild/gen', '-Ifirmware/src']
        flags += ['-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all'] if sanitized else ['-O2']
        steps += [LocalStep(name=tag + '-compile', argv=cc + flags + [source, '-lm', '-o', binary], requires=required),
                  LocalStep(name=tag, argv=[binary], requires=[])]
    return steps


def stack_recipes(cc: list[str], out: Path) -> list[LocalStep]:
    """Force the existing fresh-output stack runner to its host-only route."""
    return [LocalStep(name='stack-watermark-host',
        argv=['env', 'D8_STACK_TARGET=0', 'CC=' + shlex.join(cc), 'sh',
              'tests/run_stack_watermark_checks.sh', str(out / 'stack-watermark')],
        requires=['tests/run_stack_watermark_checks.sh', 'tests/stack_watermark_test.c',
                  'firmware/hal/fm1_stack_watermark.h'])]


# Typed lookup preserves the existing runner's system Python 3.9 support.
GENERATORS: Final[dict[ResourceCheck, Callable[[list[str], Path], list[LocalStep]]]] = {
    'resource-build': build_recipes, 'resource-capture': capture_recipes,
    'resource-console': console_recipes, 'heavy-pool-diagnostic': heavy_recipes,
    'stack-watermark': stack_recipes,
}


def resource_recipes(check: ResourceCheck, cc: list[str], out: Path) -> list[LocalStep]:
    """Return only the explicitly selected offline resource checks."""
    return GENERATORS[check](cc, out)
