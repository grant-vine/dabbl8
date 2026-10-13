#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Small offline affected-check planner. A selected PASS is not release qualification."""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
from local_flash_checks import flash_recipes, save_consent_recipes

ROOT = Path(__file__).resolve().parents[1]
GEN = ('felucca_tables.h', 'felucca_samples.h', 'felucca_fm6.h',
       'ui_fonts.h', 'ui_icons.h', 'ui_keycaps.h', 'ui_palettes.h')
MAP = {
    'storage': {'firmware/src/storage.c', 'tests/storage_test.c', 'tests/native_storage_gate_test.c'},
    'legacy-ui': {'tests/ui_test.c', 'tests/ui_legacy_v6_bounds_test.c'},
    'readonly-boot': {'firmware/src/project.c', 'firmware/src/project_native_flash.c',
                      'firmware/src/project_native_frontend.h', 'tests/native_readonly_boot_test.c'},
    'sampler': {'firmware/src/eng_sample.c', 'firmware/src/eng_slice.c',
                'firmware/src/eng_grain.c', 'tests/fuzz_smp.c'},
    'sample-upload': {'firmware/src/native_sample_session.c',
                      'tests/native_sample_session_test.c', 'tests/native_sample_client_test.mjs'},
    'own-project-client': {'web/d8projects.js', 'web/d8archive.js', 'web/d8midi.js',
        'web/d8projects-panel.js', 'web/d8companion.js', 'web/build-d8archive.sh',
        'tools/d8archive_inputs.py', 'tools/dabbl8_project_archive.c',
        'tests/native_project_client_bridge.c', 'tests/native_project_client_test.mjs'},
    'own-project-transfer': {'firmware/src/native_project_transfer.c',
        'tests/native_project_transfer_test.c'},
    'dry-sends': {'tests/native_dry_sends_test.c',
        'tools/native_dry_compare.py'},
    'fx-quiet': {'firmware/src/fx.c', 'tests/native_fx_stationary_quiet_test.c'},
    'timing-capture': {'tools/dabbl8_timing_capture.py', 'tests/dabbl8_timing_capture_test.py'},
    'flash-driver': {'firmware/hal/fm1_flash.h', 'tests/flash_driver_timeout_test.c'},
    'usb-controller': {'firmware/src/usb.c', 'tests/usb_sie_wait_test.c'},
    'save-consent': {'firmware/src/native_local_session.c', 'web/emu/save_consent_test.c'},
    'first-use': {'firmware/hal/fm1_flash.h', 'firmware/src/native_first_use.c',
        'firmware/src/native_first_use.h', 'firmware/src/native_first_use_session.c',
        'firmware/src/native_first_use_session.h', 'firmware/src/native_first_use_diagnostic.h',
        'tests/native_owner_initialize_test.c',
        'tests/native_first_use_session_test.c', 'tests/native_local_save_test.c',
        'tests/instrument_capture_test.c', 'tests/ui_test.c', 'tests/d8p1_flash_runtime_test.c'},
    'runner': {'ci/local_checks.py', 'ci/local_checks_test.py', 'ci/local_flash_checks.py', 'tests/run_tests.sh'},
}


def select(paths):
    checks, unknown = {'diff'}, []
    for name in paths:
        if Path(name).is_absolute() or '..' in Path(name).parts:
            raise ValueError('Paths must be relative to the repository: ' + name)
        matches = {k for k, files in MAP.items() if name in files}
        if name.startswith('.github/workflows/') and name.endswith(('.yml', '.yaml')):
            matches.add('runner')
        if name.endswith('.md') or name.startswith('docs/') and name.endswith(('.txt', '.json')):
            matches.add('docs')
        if not matches:
            unknown.append(name)
        checks.update(matches)
    return checks - {'docs'}, unknown


def discover(repo, base):
    def git(*args):
        return subprocess.check_output(['git', *args], cwd=repo)
    git('rev-parse', '--verify', base + '^{commit}')  # never fetch or silently change the base
    changed = git('diff', '--no-renames', '--name-only', '-z', base, '--').split(b'\0')
    changed += git('ls-files', '--others', '--exclude-standard', '-z').split(b'\0')
    return sorted({os.fsdecode(p) for p in changed if p})


def recipes(checks, cc, out, base):
    steps = [{'name': 'diff', 'argv': ['git', 'diff', '--check', base, '--']}]
    pool = ['firmware/src/d8p1.c', 'firmware/src/d8pool.c', 'firmware/src/d8pool_mapped.c']
    for name in sorted(checks - {'diff'}):
        if name == 'runner':
            steps += [{'name': 'shell', 'argv': ['sh', '-n', 'tests/run_tests.sh']},
                      {'name': 'selector', 'argv': [sys.executable, 'ci/local_checks_test.py']}]
            continue
        if name == 'timing-capture':
            steps.append({'name': 'timing-capture-scripted-transport',
                'argv': [sys.executable, 'tests/dabbl8_timing_capture_test.py'],
                'requires': ['tools/dabbl8_timing_capture.py',
                    'tools/dabbl8_timing_report.py', 'tests/dabbl8_timing_capture_test.py']})
            continue
        if name == 'save-consent':
            steps += save_consent_recipes(cc, out)
            continue
        if name in ('flash-driver', 'first-use', 'usb-controller'):
            steps += flash_recipes(name, cc, out)
            continue
        if name == 'dry-sends':
            steps.append({'name': 'dry-send-parent-equivalence',
                'argv': [sys.executable, 'tools/native_dry_compare.py', '--cc', ' '.join(cc),
                         '--out', str(out / 'dry-send-equivalence')],
                'requires': ['tools/native_dry_compare.py', 'tests/native_dry_sends_test.c'] +
                    ['build/gen/' + p for p in GEN]})
            continue
        if name == 'fx-quiet':
            # Exact qualified focused command; these defines make its existing
            # fixture/core defaults explicit without expanding the matrix.
            binary = str(out / 'fx-quiet-8')
            flags = ['-O1', '-g', '-w', '-Ibuild/gen', '-Ifirmware/src',
                     '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                     '-fno-sanitize=signed-integer-overflow,shift',
                     '-DNPART=8', '-DFELUCCA_FM4=0', '-DFELUCCA_SLICE=1']
            sources = pool + ['tests/native_fx_stationary_quiet_test.c']
            steps += [{'name': 'fx-quiet-8-compile',
                       'argv': cc + flags + ['-o', binary] + sources + ['-lm'],
                       'requires': sources + ['firmware/src/fx.c',
                           'tests/d8p1_flash_runtime_test.c', 'tests/ui_test.c',
                           'tests/hostsim.c'] + ['build/gen/' + p for p in GEN]},
                      {'name': 'fx-quiet-8', 'argv': [binary]}]
            continue
        if name == 'storage':
            jobs = [('historical', None, 'tests/storage_test.c'),
                    ('native', None, 'tests/native_storage_gate_test.c')]
        elif name == 'legacy-ui':
            jobs = [(str(n), n, 'tests/ui_legacy_v6_bounds_test.c') for n in (4, 8)]
        elif name == 'own-project-client':
            adapter = str(out / 'own-project-archive')
            bridge = str(out / 'own-project-client-bridge')
            archive_sources = ['firmware/src/d8p1.c', 'firmware/src/d8pool.c',
                               'tools/dabbl8_project_archive.c']
            bridge_sources = pool + ['tests/native_project_client_bridge.c']
            steps += [
                {'name': name + '-codec-identity',
                 'argv': [sys.executable, 'tools/d8archive_inputs.py'],
                 'requires': ['build/d8archive/inputs.json',
                    'build/d8archive/dabbl8_project_archive.mjs',
                    'build/d8archive/dabbl8_project_archive.wasm']},
                {'name': name + '-archive-compile',
                 'argv': cc + ['-std=c11', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-Ifirmware/src'] + archive_sources + ['-o', adapter],
                 'requires': archive_sources},
                {'name': name + '-bridge-compile',
                 'argv': cc + ['-std=c11', '-D_POSIX_C_SOURCE=200809L', '-O1', '-g', '-w',
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-DNPART=8', '-DFELUCCA_FM4=0', '-DFELUCCA_SLICE=1',
                    '-fno-sanitize=signed-integer-overflow,shift', '-Ibuild/gen', '-Ifirmware/src'] +
                    bridge_sources + ['-lm', '-o', bridge],
                 'requires': bridge_sources + ['build/gen/' + p for p in GEN] +
                    ['build/gen/d8_flash_store_ranges_test.h']},
                {'name': name, 'argv': ['node', 'tests/native_project_client_test.mjs',
                    bridge, adapter, 'build/d8archive/dabbl8_project_archive.mjs'],
                 'requires': ['tests/native_project_client_test.mjs']},
            ]
            continue
        elif name == 'own-project-transfer':
            jobs = [('8', 8, 'tests/native_project_transfer_test.c')]
        elif name == 'sample-upload':
            jobs = [('8', 8, 'tests/native_sample_session_test.c')]
        elif name in ('sampler', 'sample-upload'):
            jobs = [(f'{n}-{fixture}', n, 'tests/' + fixture + '.c') for n in (4, 8)
                    for fixture in ('sample_source_range_test', 'slice_render_source_range_test')]
        else:
            jobs = [('8', None, 'tests/native_readonly_boot_test.c')]
        for suffix, tracks, fixture in jobs:
            tag = name + '-' + suffix
            binary = str(out / tag)
            flags = ['-std=c11', '-D_POSIX_C_SOURCE=200809L', '-O1', '-g', '-w',
                     '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
            sources = pool + [fixture] if name in ('readonly-boot', 'sample-upload', 'own-project-transfer') else [fixture]
            inputs = [] if name == 'storage' else list(GEN)
            args = [str(out / (tag + '.trace'))] if fixture.endswith('slice_render_source_range_test.c') else []
            if tracks:
                flags += [f'-DNPART={tracks}']
            if name != 'storage':
                flags += ['-DFELUCCA_FM4=0', '-DFELUCCA_SLICE=1']
            if name in ('sampler', 'sample-upload', 'own-project-transfer'):
                # Same bounded range/render recipes: only inherited DSP arithmetic exclusions.
                flags += ['-fno-sanitize=signed-integer-overflow,shift']
            flags += ['-Ibuild/gen', '-Ifirmware/src']
            steps += [{'name': tag + '-compile', 'argv': cc + flags + sources + ['-lm', '-o', binary],
                       'requires': sources + ['build/gen/' + p for p in inputs] +
                       (['build/gen/d8_flash_store_ranges_test.h', 'tests/native_consent_pixels.h']
                        if name in ('sample-upload', 'own-project-transfer') else [])},
                      {'name': tag, 'argv': [binary, *args]}]
    if 'sample-upload' in checks:
        steps.append({'name': 'sample-upload-browser',
                      'argv': ['node', 'tests/native_sample_client_test.mjs'],
                      'requires': ['tests/native_sample_client_test.mjs', 'web/editor.html']})
    return steps


def make_plan(repo, paths, explicit, cc, out, base):
    checks, unknown = select(paths)
    refusal = []
    if explicit:
        checks = {'diff'} | set(explicit) - {'docs'}
    elif unknown:
        refusal.append('Unmapped changes need explicit --check selection: ' + ', '.join(unknown))
    steps = recipes(checks, cc, out, base) if paths or explicit else []
    missing = sorted({p for s in steps for p in s.get('requires', []) if not (repo / p).is_file()})
    if missing:
        refusal.append('Missing local inputs (no automatic generation/download): ' + ', '.join(missing))
    if any(s['name'].endswith('-compile') for s in steps) and not shutil.which(cc[0]):
        refusal.append('Compiler unavailable: ' + cc[0])
    for name in ('GOLDEN_UPDATE', 'BUDGET_UPDATE'):
        if os.environ.get(name):
            refusal.append('Refused inherited ' + name)
    return {'status': 'REFUSED' if refusal else 'PLAN' if steps else 'NO_CHANGES',
            'base': base, 'paths': paths, 'selection': 'explicit' if explicit else 'affected',
            'unmapped_paths': unknown, 'checks': sorted(checks), 'commands': steps, 'refusals': refusal,
            'scope': 'Selected local host checks only; not complete coverage, vendor, timing or release qualification.'}


def execute(argv, repo, env):
    try:
        q = subprocess.run(argv, cwd=repo, env=env, capture_output=True, timeout=120)
        return q.returncode, q.stdout, q.stderr
    except subprocess.TimeoutExpired as exc:
        return 124, exc.stdout or b'', (exc.stderr or b'') + b'\nLocal check timed out after 120 seconds.\n'
    except OSError as exc:
        return 127, b'', str(exc).encode()


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--base', default='HEAD', help='Local commit/ref to diff; never fetched')
    ap.add_argument('--paths', nargs='+', help='Explicit repository-relative changed paths')
    ap.add_argument('--check', action='append', choices=['docs', *MAP], default=[],
                    help='Explicit bounded scope for unmapped changes; repeatable')
    ap.add_argument('--cc', default=os.environ.get('CC', 'clang'), help='Compiler command; no shell evaluation')
    ap.add_argument('--run', action='store_true', help='Execute the plan locally; default prints it only')
    args = ap.parse_args(argv)
    try:
        cc = shlex.split(args.cc)
        if not cc:
            raise ValueError('Empty compiler command')
        paths = args.paths if args.paths is not None else discover(ROOT, args.base)
        plan = make_plan(ROOT, paths, args.check, cc, Path('build/local-checks/PLANNED'), args.base)
    except (ValueError, subprocess.CalledProcessError) as exc:
        print(json.dumps({'status': 'REFUSED', 'reason': str(exc)}))
        return 2
    if not args.run or plan['refusals'] or not plan['commands']:
        print(json.dumps(plan, indent=2))
        return 2 if plan['refusals'] else 0
    folder = ROOT / 'build/local-checks'
    folder.mkdir(parents=True, exist_ok=True)
    out = Path(tempfile.mkdtemp(prefix='run-', dir=folder))
    plan['commands'] = recipes(set(plan['checks']), cc, out, args.base)
    plan['source_head'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    plan['generated_inputs'] = {n: hashlib.sha256((ROOT / 'build/gen' / n).read_bytes()).hexdigest()
                                for n in GEN if (ROOT / 'build/gen' / n).is_file()}
    plan['results'] = []
    env = {**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0:halt_on_error=1', 'UBSAN_OPTIONS': 'halt_on_error=1'}
    for step in plan['commands']:
        code, stdout, stderr = execute(step['argv'], ROOT, env)
        (out / (step['name'] + '.stdout')).write_bytes(stdout)
        (out / (step['name'] + '.stderr')).write_bytes(stderr)
        plan['results'].append({'name': step['name'], 'argv': step['argv'], 'exit': code})
        if code:
            break
    plan['status'] = 'FAILED' if code else 'SELECTED_CHECKS_PASS'
    for name in paths:
        if name.endswith('.py') and (ROOT / name).is_file():
            try:
                ast.parse((ROOT / name).read_bytes(), filename=name)
            except SyntaxError as exc:
                plan['status'] = 'FAILED'
                plan['python_syntax_error'] = str(exc)
    (out / 'results.json').write_text(json.dumps(plan, indent=2) + '\n')
    print(json.dumps({'status': plan['status'], 'report': str(out / 'results.json'), 'scope': plan['scope']}))
    return 0 if plan['status'] == 'SELECTED_CHECKS_PASS' else 1


if __name__ == '__main__':
    sys.exit(main())
