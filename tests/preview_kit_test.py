#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the real built offline preview kit without device access."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('kit', ROOT / 'tools/dabbl8_preview_kit.py')
kit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(kit)
checks = 0


def check(value, label):
    global checks
    assert value, label
    checks += 1


with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    tables = Path(sys.argv[1])
    one = tmp / 'one.zip'
    result = kit.build(one, tables)
    check(hashlib.sha256(one.read_bytes()).hexdigest() == result['sha256'], 'archive checksum')
    two = tmp / 'two.zip'
    kit.build(two, tables)
    check(one.read_bytes() == two.read_bytes(), 'identical local inputs produce identical archive')
    try:
        kit.build(one, tables)
    except FileExistsError:
        check(True, 'existing archive refused')
    else:
        raise AssertionError('existing archive overwritten')
    extracted = tmp / 'kit'
    with zipfile.ZipFile(one) as archive:
        check(archive.testzip() is None, 'ZIP integrity')
        archive.extractall(extracted)
    (extracted / 'bin/dabbl8_project_convert').chmod(0o755)
    manifest = json.loads((extracted / 'manifest.json').read_text())
    for name, record in manifest['files'].items():
        data = (extracted / name).read_bytes()
        check(len(data) == record['bytes'] and hashlib.sha256(data).hexdigest() == record['sha256'], 'manifest file hash')
    check(not manifest['device_writes'] and not manifest['firmware_package'], 'offline scope')
    check((extracted / 'source.tar').exists() and (extracted / 'LICENSE').exists(), 'source and notices included')
    subprocess.run([sys.executable, 'verify.py'], cwd=extracted, check=True)
    fixture = ROOT / 'tests/fixtures/projects'
    originals = sorted(p for p in fixture.glob('*.bin') if '.expected-fun9' not in p.name)
    check(len(originals) == 13, 'all frozen originals found')
    for index, source in enumerate(originals):
        destination = tmp / f'bundle-{index}'
        command = [sys.executable, str(extracted / 'convert.py'), str(source), str(destination)]
        for slot in range(4):
            command += ['--reference', f'{slot}={originals[0]}']
        p = subprocess.run(command, cwd=tmp, capture_output=True, text=True)
        check(p.returncode == 0, 'relocatable launcher accepts fixture: ' + p.stderr)
        report = json.loads((destination / 'report.json').read_text())
        check(report['accepted'] and (destination / 'original.bin').read_bytes() == source.read_bytes(), 'original retained')
        check(hashlib.sha256((destination / 'project.d8p').read_bytes()).hexdigest() == report['converted_sha256'], 'converted hash')
    bad = tmp / 'bad.bin'
    bad.write_bytes(b'unknown private test input')
    p = subprocess.run([sys.executable, str(extracted / 'convert.py'), str(bad), str(tmp / 'refused')], cwd=tmp, capture_output=True)
    check(p.returncode == 1, 'unknown input refused')
    check((tmp / 'refused/original.bin').read_bytes() == bad.read_bytes(), 'refusal original retained')
    check(not list((tmp / 'refused').glob('*.d8p')), 'no refused output')
    old = (tmp / 'refused/report.json').read_bytes()
    p = subprocess.run([sys.executable, str(extracted / 'convert.py'), str(bad), str(tmp / 'refused')], cwd=tmp, capture_output=True)
    check(p.returncode != 0 and (tmp / 'refused/report.json').read_bytes() == old, 'existing bundle preserved')
    (extracted / 'README.txt').write_text('tampered')
    p = subprocess.run([sys.executable, 'verify.py'], cwd=extracted, capture_output=True)
    check(p.returncode != 0 and b'Mismatch: README.txt' in p.stderr, 'kit tampering rejected')
print(f'PASS: {checks} offline preview kit checks')
