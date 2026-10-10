#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build a local, host-specific offline converter kit; never download or flash."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import platform
import subprocess
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
README = '''Dabbl8 offline project converter — developer preview

This kit prepares experimental D8P1 project data from standalone FUN1–FUN9
files using Felucca's pinned importer. It is NOT firmware, an installer or a
qualified device restore tool. It has no MIDI/device connection. Tracks 5–8
start empty; eight tracks share eight sounding voices. Arrangement playback
is not implemented by this tool. Keep independent backups of every original.

Requirements: Python 3.10 or newer and the host OS/architecture recorded in
manifest.json. The native executable is host-specific and is unsigned.

From this extracted directory:
  python3 convert.py /path/to/project.bin /path/to/NEW-bundle
  python3 convert.py /path/to/project.bin /path/to/NEW-bundle --reference 0=/path/to/slot0.bin

The destination must be new. Saved-slot indices are historical 0–3; they do
not declare the new device's three saved slots. Supply ALL actual referenced
legacy files. Missing/corrupt references refuse conversion; slot 3 is never
silently mapped to a new slot. This kit does not resolve device migration.

Read report.json and require accepted:true. Even accepted conversion can
normalize inactive/reserved bytes and remove incompatible engine automation.
Every readable original is retained byte for byte with SHA-256. Refusal keeps
originals and a report, without converted outputs. Incomplete copying/report
writing is not a verified backup. Existing destination directories are refused.

Run python3 verify.py before conversion. This checks every listed kit file's
size/hash, not publisher identity or hardware safety. manifest.json records
source commit, compiler/platform, generated-table hashes and smoke checks.
A whole-archive checksum accompanies the ZIP. Exact repeatability applies
only to packaging identical inputs; compiler/OS differences may change binaries.

Complete corresponding source and original notices: source.tar, LICENSE,
LICENSING.md and LICENSES/. Source includes the converter and build tools.
Extract source.tar separately, follow BUILDING.md and the pinned setup in
docs/dabbl8/desktop-first-build.md to generate build/gen, then rebuild:
  cc -O2 -w -Ibuild/gen -Ifirmware/src firmware/src/d8p1.c tools/dabbl8_project_convert.c -lm -o dabbl8_project_convert
Private projects, vendor firmware and device backups are not included.
Felucca copyright (C) 2026 Leo Kuroshita, Hügelton Instruments.
GPL-3.0-only; dependency licenses remain in force. This is a local developer
artifact. Publishing a release requires separate review and authorization.
'''
LAUNCHER = '''#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
import sys
from dabbl8_project_convert import main
if not any(arg == "--converter" or arg.startswith("--converter=") for arg in sys.argv):
    sys.argv.extend(["--converter", str(Path(__file__).resolve().parent / "bin/dabbl8_project_convert")])
raise SystemExit(main())
'''
VERIFY = '''#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import hashlib, json
from pathlib import Path
root = Path(__file__).resolve().parent
manifest = json.loads((root / "manifest.json").read_text())
for name, record in manifest["files"].items():
    path = Path(name)
    if path.is_absolute() or ".." in path.parts:
        raise SystemExit("Invalid manifest path")
    data = (root / path).read_bytes()
    if len(data) != record["bytes"] or hashlib.sha256(data).hexdigest() != record["sha256"]:
        raise SystemExit("Mismatch: " + name)
print("PASS: all listed kit files match manifest; publisher identity is not verified")
'''


def sha(data):
    return hashlib.sha256(data).hexdigest()


def build(destination, tables, compiler='cc', revision='HEAD'):
    destination = Path(destination)
    if destination.exists():
        raise FileExistsError(destination)
    commit = subprocess.check_output(['git', 'rev-parse', '--verify', revision + '^{commit}'], cwd=ROOT, text=True).strip()
    source = subprocess.check_output(['git', 'archive', '--format=tar', commit], cwd=ROOT)
    version = subprocess.check_output([compiler, '--version'], text=True)
    generated = {p.name: p.read_bytes() for p in sorted(Path(tables).glob('*.h'))}
    if 'felucca_tables.h' not in generated:
        raise ValueError('Missing generated baseline tables; run the baseline build first')
    with tempfile.TemporaryDirectory(prefix='dabbl8-preview-') as tmp:
        root = Path(tmp)
        with tarfile.open(fileobj=io.BytesIO(source)) as archive:
            archive.extractall(root, filter='data')
        gen = root / 'build/gen'
        gen.mkdir(parents=True)
        for name, data in generated.items():
            (gen / name).write_bytes(data)
        executable = root / 'dabbl8_project_convert'
        command = [compiler, '-O2', '-w', '-Ibuild/gen', '-Ifirmware/src', 'firmware/src/d8p1.c', 'tools/dabbl8_project_convert.c', '-lm', '-o', str(executable)]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stderr)
        smoke = subprocess.run([str(executable), 'check', str(root / 'tests/fixtures/d8p1/minimal.d8p'), '15'], capture_output=True, text=True, check=True)
        checked = json.loads(smoke.stdout)
        if checked.get('accepted') is not True:
            raise ValueError('Native converter smoke check refused the immutable fixture')
        files = {'README.txt': README.encode(), 'convert.py': LAUNCHER.encode(), 'verify.py': VERIFY.encode(),
                 'dabbl8_project_convert.py': (root / 'tools/dabbl8_project_convert.py').read_bytes(),
                 'bin/dabbl8_project_convert': executable.read_bytes(), 'source.tar': source,
                 'LICENSE': (root / 'LICENSE').read_bytes(), 'LICENSING.md': (root / 'LICENSING.md').read_bytes()}
        for path in sorted((root / 'LICENSES').rglob('*')):
            if path.is_file():
                files[str(path.relative_to(root))] = path.read_bytes()
        manifest = {'format': 'dabbl8-offline-preview-kit', 'version': 1, 'source_commit': commit,
                    'host': {'system': platform.system(), 'machine': platform.machine()},
                    'compiler_version': version, 'compile_flags': command[1:-2],
                    'generated_tables': {n: {'bytes': len(d), 'sha256': sha(d)} for n, d in generated.items()},
                    'smoke_check': checked, 'firmware_package': False, 'device_writes': False,
                    'files': {n: {'bytes': len(d), 'sha256': sha(d)} for n, d in sorted(files.items())}}
        files['manifest.json'] = (json.dumps(manifest, indent=2, sort_keys=True) + '\n').encode()
        output = io.BytesIO()
        with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            for name, data in sorted(files.items()):
                entry = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
                entry.create_system = 3
                entry.external_attr = (0o100755 if name == 'bin/dabbl8_project_convert' else 0o100644) << 16
                entry.compress_type = zipfile.ZIP_DEFLATED
                archive.writestr(entry, data)
        data = output.getvalue()
        # Exclusive creation also refuses a destination created while compiling.
        with destination.open('xb') as out:
            out.write(data)
        return {'archive': str(destination), 'bytes': len(data), 'sha256': sha(data), 'source_commit': commit}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=Path, help='New ZIP path outside Git')
    parser.add_argument('--tables', required=True, type=Path, help='Audited baseline build/gen directory')
    parser.add_argument('--compiler', default='cc')
    parser.add_argument('--revision', default='HEAD', help='Exact committed source to package (default HEAD)')
    args = parser.parse_args()
    print(json.dumps(build(args.destination, args.tables, args.compiler, args.revision), indent=2))


if __name__ == '__main__':
    main()
