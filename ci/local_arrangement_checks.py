# SPDX-License-Identifier: GPL-3.0-only
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
"""Actual host ISR/arrangement/FX/UI combinations; no device adapter."""
from pathlib import Path
from typing import Final

from local_flash_checks import LocalStep

ARRANGEMENT_MAP: Final = {
    'arrangement-stress': {'tests/native_arrangement_stress_test.c'},
    'arrangement-proposal': {'tests/native_arrangement_proposal_test.c'},
}
SCENARIOS: Final = ('internal-dry', 'internal-room', 'usb-spring', 'trs-loss-wrap')
GENERATED: Final = ('felucca_tables.h', 'felucca_samples.h', 'felucca_fm6.h',
                    'ui_fonts.h', 'ui_icons.h', 'ui_keycaps.h', 'ui_palettes.h')


def arrangement_recipes(check: str, cc: list[str], out: Path) -> list[LocalStep]:
    """Plan isolated processes for natural scene changes, fill and clock loss."""
    if check not in ARRANGEMENT_MAP:
        raise ValueError('Unknown arrangement scope: ' + check)
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
