# SPDX-License-Identifier: GPL-3.0-only
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
"""Actual host ISR/arrangement/FX/UI combinations; no device adapter."""
from pathlib import Path
import sys
from typing import Final

from local_flash_checks import LocalStep

ARRANGEMENT_MAP: Final = {
    'arrangement-stress': {'tests/native_arrangement_stress_test.c'},
    'arrangement-proposal': {'tests/native_arrangement_proposal_test.c'},
    'arrangement-status': {'firmware/src/arrangement_diagnostic_console.h',
                           'tools/dabbl8_arrangement_state.py', 'tests/arrangement_console_test.c',
                           'tests/arrangement_state_test.py'},
    'arrangement-capture': {'tools/dabbl8_arrangement_capture.py', 'tests/arrangement_capture_test.py'},
    'arrangement-readback': {'tools/dabbl8_arrangement_readback.py', 'tests/arrangement_readback_test.py'},
}
SCENARIOS: Final = ('internal-dry', 'internal-room', 'usb-spring', 'trs-loss-wrap')
GENERATED: Final = ('felucca_tables.h', 'felucca_samples.h', 'felucca_fm6.h',
                    'ui_fonts.h', 'ui_icons.h', 'ui_keycaps.h', 'ui_palettes.h')


def arrangement_recipes(check: str, cc: list[str], out: Path) -> list[LocalStep]:
    """Plan isolated processes for natural scene changes, fill and clock loss."""
    if check not in ARRANGEMENT_MAP:
        raise ValueError('Unknown arrangement scope: ' + check)
    if check == 'arrangement-status':
        return status_recipes(cc, out)
    if check == 'arrangement-capture':
        return [LocalStep(name='arrangement-capture-offline',
            argv=[sys.executable, 'tests/arrangement_capture_test.py'], requires=[
                'tests/arrangement_capture_test.py', 'tools/dabbl8_arrangement_capture.py',
                'tools/dabbl8_arrangement_state.py', 'tools/dabbl8_timing_capture.py',
                'tools/dabbl8_resource_capture.py', 'tools/dabbl8_timing_report.py',
                'tools/dabbl8_resource_report.py'])]
    if check == 'arrangement-readback':
        binary = str(out / 'arrangement-native-archive')
        sources = ['tools/dabbl8_project_archive.c', 'firmware/src/d8p1.c', 'firmware/src/d8pool.c']
        required = [*sources, 'firmware/src/d8p1.h', 'firmware/src/d8pool.h',
            'tools/dabbl8_arrangement_readback.py', 'tests/arrangement_readback_test.py',
            'tools/dabbl8_project_archive.py', 'tools/dabbl8_instrument_capture.py',
            'tests/fixtures/d8p1/minimal.d8p']
        return [LocalStep(name='arrangement-readback-codec-compile', argv=cc+[
            '-std=c11', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
            '-Ifirmware/src', *sources, '-o', binary], requires=required),
            LocalStep(name='arrangement-readback-offline',
                argv=[sys.executable, 'tests/arrangement_readback_test.py', binary], requires=required)]
    proposal = check == 'arrangement-proposal'
    fixture = ('tests/native_arrangement_proposal_test.c' if proposal else
               'tests/native_arrangement_stress_test.c')
    sources = [fixture, 'firmware/src/d8p1.c', 'firmware/src/d8pool.c']
    required = sources + [
        'tests/native_arrangement_test.c', 'tests/d8p1_pool_runtime_test.c',
        'tests/ui_test.c', 'tests/hostsim.c', 'tests/native_queue_audio_fixture.h',
        'tests/fixtures/d8p1/minimal.d8p', 'firmware/src/audio.c',
        'firmware/src/d8arr_prepare.c', 'firmware/src/song_chain.c',
        'firmware/src/audio_timing.h', 'firmware/src/heavy_pool_diagnostic.h',
        *['build/gen/' + name for name in GENERATED],
    ]
    flags = ['-std=c11', '-w', '-DNPART=8', '-DFELUCCA_FM4=0',
             '-DFELUCCA_SLICE=1', '-DFELUCCA_UAC=0', '-DDABBL8_IDENTITY=1',
             '-DDABBL8_TIMING_DIAGNOSTIC=1', '-DD8_HEAVY_POOL_DIAGNOSTIC=1',
             '-Ibuild/gen', '-Ifirmware/src']
    steps: list[LocalStep] = []
    for mode in ('optimized', 'sanitized'):
        tag = ('arrangement-proposal-' if proposal else 'arrangement-') + mode
        binary = str(out / tag)
        mode_flags = ['-O2'] if mode == 'optimized' else [
            '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
            '-fno-sanitize=signed-integer-overflow,shift',
        ]
        steps.append(LocalStep(name=tag + '-compile',
            argv=cc + flags + mode_flags + sources + ['-lm', '-o', binary],
            requires=required))
        if proposal:
            steps.append(LocalStep(name=tag, argv=[binary, str(out / (tag + '.d8p'))], requires=[]))
            continue
        for scenario in SCENARIOS:
            steps.append(LocalStep(name='arrangement-' + mode + '-' + scenario,
                                   argv=[binary, scenario], requires=[]))
    return steps


def status_recipes(cc: list[str], out: Path) -> list[LocalStep]:
    """Compile actual status slice on/off; decode it with the real host parser."""
    sources = ['tests/arrangement_console_test.c', 'firmware/src/d8p1.c', 'firmware/src/d8pool.c']
    required = sources + ['tests/native_arrangement_test.c', 'tests/d8p1_pool_runtime_test.c',
        'tests/ui_test.c', 'tests/hostsim.c', 'tests/fixtures/d8p1/minimal.d8p',
        'firmware/src/arrangement_diagnostic_console.h', 'firmware/src/song_chain.c',
        'firmware/src/d8arr_prepare.c', 'tests/arrangement_state_test.py',
        'tools/dabbl8_arrangement_state.py', *['build/gen/' + name for name in GENERATED]]
    flags = ['-std=c11', '-w', '-DNPART=8', '-DFELUCCA_FM4=0', '-DFELUCCA_SLICE=1',
             '-Ibuild/gen', '-Ifirmware/src']
    steps: list[LocalStep] = []
    for mode in ('optimized', 'sanitized'):
        binaries: list[str] = []
        for enabled in (1, 0):
            tag = f'arrangement-status-{mode}-{enabled}'
            binary = str(out / tag)
            binaries.append(binary)
            mode_flags = ['-O2'] if mode == 'optimized' else ['-O1', '-g',
                '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                '-fno-sanitize=signed-integer-overflow,shift']
            steps.append(LocalStep(name=tag + '-compile',
                argv=cc + flags + mode_flags + [f'-DDABBL8_TIMING_DIAGNOSTIC={enabled}'] +
                     sources + ['-lm', '-o', binary], requires=required))
        steps.append(LocalStep(name='arrangement-state-' + mode,
            argv=[sys.executable, 'tests/arrangement_state_test.py', *binaries], requires=required))
    return steps
