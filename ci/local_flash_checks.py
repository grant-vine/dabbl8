# SPDX-License-Identifier: GPL-3.0-only
"""Offline host recipes for application flash, first-use and USB containment."""
from pathlib import Path
from typing import Final, Literal, TypedDict


class LocalStep(TypedDict):
    name: str
    argv: list[str]
    requires: list[str]


FIRST_USE_HEADERS: Final[tuple[str, ...]] = (
    'felucca_tables.h', 'felucca_samples.h', 'felucca_fm6.h',
    'ui_fonts.h', 'ui_icons.h', 'ui_keycaps.h', 'ui_palettes.h',
)


def flash_recipes(check: Literal['flash-driver', 'first-use', 'usb-controller'], cc: list[str],
                  out: Path) -> list[LocalStep]:
    """Assemble bounded optimized/sanitized commands without executing them."""
    # The existing offline runner supports the system Python 3.9; source
    # selection is a typed lookup rather than requiring structural matching.
    sources = {
        'flash-driver': ['tests/flash_driver_timeout_test.c'],
        'first-use': ['firmware/src/d8p1.c', 'firmware/src/d8pool.c',
                     'firmware/src/d8pool_mapped.c', 'tests/native_owner_initialize_test.c'],
        'usb-controller': ['tests/usb_sie_wait_test.c'],
    }[check]
    first_use = check == 'first-use'
    usb_controller = check == 'usb-controller'
    flags = ['-std=c11', '-Wall', '-Wextra']
    required = sources + ([] if usb_controller else ['firmware/hal/fm1_flash.h',
                                                  'firmware/hal/fm1_flash_trace.h'])
    if first_use:
        flags += ['-D_POSIX_C_SOURCE=200809L', '-w', '-DNPART=8',
                  '-DFELUCCA_FM4=0', '-DFELUCCA_SLICE=1',
                  '-Ibuild/gen', '-Ifirmware/src']
        required += [
                'tests/native_first_use_session_test.c', 'tests/native_local_save_test.c',
                'tests/native_readonly_boot_test.c', 'tests/native_migration_session_test.c',
                'tests/native_source_equivalence_test.c', 'tests/instrument_capture_test.c',
                'tests/native_autosave_session_test.c', 'tests/native_output_queue_test.c',
                'tests/d8p1_flash_runtime_test.c', 'tests/ui_test.c', 'tests/hostsim.c',
                'tests/native_queue_audio_fixture.h', 'tests/native_queue_usb_hal.h',
                'firmware/src/native_first_use.c', 'firmware/src/native_first_use_session.c',
                'firmware/src/native_first_use_diagnostic.h',
                'firmware/src/storage.c', 'firmware/src/upreset.c', 'firmware/src/up_fm6.c']
        required += ['build/gen/' + header for header in FIRST_USE_HEADERS]
    else:
        flags += ['-Wno-unused-function', '-Wno-int-to-pointer-cast']
    if usb_controller:
        flags += ['-Werror', '-Wno-missing-field-initializers']
        required += ['tests/native_queue_usb_hal.h', 'tests/uac_test.c',
                     'firmware/src/usb.c', 'firmware/src/main.c',
                     'firmware/src/identity.h', 'firmware/src/uac_fir.h']
    steps: list[LocalStep] = []
    for mode in ('optimized', 'sanitized'):
        tag = check + '-' + mode
        binary = str(out / tag)
        mode_flags = ['-O2'] if mode == 'optimized' else ['-O1', '-g',
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
        if mode == 'sanitized' and first_use:
            mode_flags += ['-fno-sanitize=signed-integer-overflow,shift']
        steps += [LocalStep(name=tag + '-compile',
                           argv=cc + flags + mode_flags + sources +
                               (['-lm'] if first_use or usb_controller else []) + ['-o', binary],
                           requires=required),
                  LocalStep(name=tag, argv=[binary], requires=[])]
        trace_fixtures = {
            'flash-driver': ['tests/flash_trace_test.c', 'tests/flash_trace_spi_watchdog_test.c',
                             'tests/time_handover_test.c'],
            'first-use': ['tests/flash_trace_reset_test.c'],
            'usb-controller': [],
        }[check]
        for fixture in trace_fixtures:
            trace_sources = sources[:-1] + [fixture] if first_use else [fixture]
            trace_tag = Path(fixture).stem + '-' + mode
            trace_binary = str(out / trace_tag)
            trace_required = required + [fixture, 'tests/flash_trace_fixture.h',
                                        'firmware/src/flash_trace_diagnostic.h']
            if fixture == 'tests/time_handover_test.c':
                trace_required += ['firmware/hal/fm1_time.h']
            steps += [LocalStep(name=trace_tag + '-compile',
                               argv=cc + flags + mode_flags + trace_sources +
                                   (['-lm'] if first_use else []) + ['-o', trace_binary],
                               requires=trace_required),
                      LocalStep(name=trace_tag, argv=[trace_binary], requires=[])]
    return steps



def save_consent_recipes(cc: list[str], out: Path) -> list[LocalStep]:
    """Check actual emulator disclosure pixels and fresh-process cancellation."""
    source = 'web/emu/save_consent_test.c'
    flags = ['-std=c11', '-D_POSIX_C_SOURCE=200809L', '-w', '-ffp-contract=off',
             '-DNPART=8', '-DFELUCCA_FM4=0', '-DFELUCCA_SLICE=1',
             '-Ibuild/gen', '-Ifirmware/src']
    required = [source, 'web/emu/felucca_web.c', 'web/emu/workflow_qa.h',
                'tests/hostsim.c', 'firmware/src/native_local_session.c',
                'firmware/src/native_local_session.h', 'firmware/src/native_first_use.c',
                'firmware/src/native_first_use_session.c', 'firmware/src/ui.c',
                'firmware/src/gfx.c'] + ['build/gen/' + header for header in FIRST_USE_HEADERS]
    steps: list[LocalStep] = []
    for sanitized in (False, True):
        mode = 'sanitized' if sanitized else 'optimized'
        tag = 'save-consent-' + mode
        binary = str(out / tag)
        mode_flags = ['-O1', '-g', '-fsanitize=address,undefined',
                      '-fno-sanitize-recover=all',
                      '-fno-sanitize=signed-integer-overflow,shift'] if sanitized else ['-O2']
        steps += [LocalStep(name=tag + '-compile',
                           argv=cc + flags + mode_flags + [source, '-lm', '-o', binary],
                           requires=required),
                  LocalStep(name=tag, argv=[binary, str(out)], requires=[])]
        steps += [LocalStep(name=tag + '-cancel-' + str(event),
                            argv=[binary, str(out), str(event)], requires=[])
                  for event in range(5)]
    return steps
