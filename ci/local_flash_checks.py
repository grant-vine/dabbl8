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
    required = sources + ([] if usb_controller else ['firmware/hal/fm1_flash.h'])
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
    return steps
