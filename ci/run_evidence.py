#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build/test evidence, with explicit failure, dependency and hardware gates."""
import datetime
import hashlib
import importlib.metadata
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/ci-evidence'
OUT.mkdir(parents=True, exist_ok=True)
LOCK = json.loads((ROOT / 'ci/dependencies.json').read_text())
report = {'schema': 1, 'time_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
          'host': platform.platform(), 'checks': [], 'skips': [], 'artifacts': {},
          'dependency_lock': LOCK, 'hardware': {'status': 'SKIP', 'reason': 'No physical device qualification or flashing authorized'}}

def capture(args):
    try:
        proc = subprocess.run(args, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return {'exit_code': proc.returncode, 'output': proc.stdout.strip()}
    except OSError as error:
        return {'exit_code': None, 'output': str(error)}

report['source'] = capture(['git', 'rev-parse', 'HEAD'])
report['worktree_status'] = capture(['git', 'status', '--short'])
report['versions'] = {'python': sys.version, 'node': capture(['node', '--version']),
                      'host_compiler': capture([os.environ.get('CC', 'cc'), '--version'])}
for name in ['Pillow', 'fonttools']:
    try: report['versions'][name] = importlib.metadata.version(name)
    except importlib.metadata.PackageNotFoundError: report['versions'][name] = None
sdk = os.environ.get('AC79_SDK')
report['sdk_commit'] = capture(['git', '-C', sdk, 'rev-parse', 'HEAD']) if sdk else {'exit_code': None, 'output': 'AC79_SDK absent'}
tc = os.environ.get('JIELI_TOOLCHAIN')
if tc:
    if os.environ.get('JIELI_DOCKER') == '1':
        report['versions']['target_compiler'] = capture(['docker', 'run', '--rm', '--platform', 'linux/amd64', '-v', f'{Path(tc).resolve()}:/opt/jieli:ro', LOCK['container'], '/opt/jieli/pi32v2/bin/clang', '--version'])
    else: report['versions']['target_compiler'] = capture([str(Path(tc) / 'pi32v2/bin/clang'), '--version'])
report['container_inspect'] = capture(['docker', 'image', 'inspect', LOCK['container'], '--format', '{{json .RepoDigests}}'])
report['python_dependencies'] = capture([sys.executable, '-m', 'pip', 'freeze'])
report['host_packages'] = capture(['dpkg-query', '-W']) if platform.system() == 'Linux' else {'exit_code': None, 'output': 'Not Debian/Ubuntu: package inventory unavailable'}

reference_files = ['tests/golden.txt', 'tests/cpu_baseline.txt', 'tests/target_budget.txt']
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
references_before = {name: digest(ROOT / name) for name in reference_files}
failed = False
built = False

def run(name, args):
    global failed
    log = OUT / f'{name}.log'
    try:
        with log.open('w') as output:
            proc = subprocess.run(args, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
        code = proc.returncode
    except OSError as error:
        log.write_text(str(error) + '\n'); code = 127
    report['checks'].append({'name': name, 'status': 'PASS' if code == 0 else 'FAIL', 'exit_code': code, 'log': log.name})
    failed |= code != 0
    print(f'{name}: {"PASS" if code == 0 else "FAIL"}', flush=True)
    return code == 0

setup_ok = os.environ.get('D8_SETUP_OK', 'true') == 'true'
unsafe_updates = any(os.environ.get(key) not in (None, '', '0') for key in ['GOLDEN_UPDATE', 'BUDGET_UPDATE'])
pin_errors = []
if os.environ.get('GITHUB_ACTIONS') == 'true':
    for name, actual, expected in [
        ('Python', platform.python_version(), LOCK['python']),
        ('Pillow', report['versions']['Pillow'], LOCK['pillow']),
        ('fonttools', report['versions']['fonttools'], LOCK['fonttools']),
        ('Node', report['versions']['node']['output'], 'v' + LOCK['node']),
        ('SDK', report['sdk_commit']['output'], LOCK['sdk_commit']),
    ]:
        if actual != expected: pin_errors.append(name)
report['pin_errors'] = pin_errors
if not setup_ok or unsafe_updates or pin_errors:
    failed = True
    report['checks'].append({'name': 'preflight', 'status': 'FAIL', 'reason': 'Workflow dependency setup failed' if not setup_ok else ('Installed dependency differs from lock: ' + ', '.join(pin_errors) if pin_errors else 'Baseline rewriting flag refused')})
    report['skips'].append({'name': 'build-and-host', 'reason': 'Failed preflight'})
else:
    built = run('build', ['sh', 'build.sh'])
    if built:
        run('host-tests', ['sh', 'tests/run_tests.sh'])
    else:
        report['skips'].append({'name': 'host-tests', 'reason': 'Target build failed; generated files/package unavailable'})

for log in OUT.glob('*.log'):
    for line in log.read_text(errors='replace').splitlines():
        if line.startswith('== skip ') or 'CPU: no instruction counter' in line:
            report['skips'].append({'name': log.name, 'reason': line})
            if 'sanitizer runs' in line:
                failed = True
                report['checks'].append({'name': 'mandatory-sanitizers', 'status': 'FAIL', 'reason': line})
for name in reference_files + ['build/felucca.bin', 'build/felucca.fwsc', 'build/loader/ota.bin']:
    p = ROOT / name
    if p.exists() and (built or name in reference_files): report['artifacts'][name] = {'bytes': p.stat().st_size, 'sha256': digest(p)}
for name, before in references_before.items():
    if digest(ROOT / name) != before:
        failed = True
        report['checks'].append({'name': 'unchanged-reference', 'status': 'FAIL', 'path': name})
report['result'] = 'FAIL' if failed else 'PASS'
(OUT / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
summary = f'Host/package result: {report["result"]}\n\nHardware qualification: SKIP (not performed).\n\nExplicit optional/environment skips: {len(report["skips"])}. Details and dependency versions are in results.json; raw failures remain in the logs.\n'
(OUT / 'summary.md').write_text(summary)
if os.environ.get('GITHUB_STEP_SUMMARY'):
    with open(os.environ['GITHUB_STEP_SUMMARY'], 'a') as output: output.write(summary)
print(summary, flush=True)
sys.exit(1 if failed else 0)
