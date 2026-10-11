# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Dabbl8 contributors
"""Real Git/CLI refusals using tracked notice fixtures; no private-file scan."""
from pathlib import Path
import json
import subprocess
import sys
import tempfile

repo = Path(__file__).resolve().parents[1]
tool = repo / 'tools/license_inventory.py'
reference = json.loads((repo / 'tools/license_notice_baseline.json').read_text())
checks = 0


def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args], stderr=subprocess.PIPE)


with tempfile.TemporaryDirectory(prefix='d8-notice-test-') as directory:
    fixture = Path(directory) / 'source'
    fixture.mkdir()
    git(fixture, 'init', '-q')
    git(fixture, 'config', 'user.email', 'test@example.invalid')
    git(fixture, 'config', 'user.name', 'Notice fixture')
    paths = set(reference['notice_asset_pins']) | set(reference['port_first_comment_sha256']) | {'firmware/src/eng_phys.c'}
    for path in paths:
        target = fixture / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(git(repo, 'show', 'HEAD:' + path))
    own = fixture / 'tools/own.py'
    own.parent.mkdir(exist_ok=True)
    own.write_text('# SPDX-License-Identifier: GPL-3.0-only\n# Copyright (C) 2026 Dabbl8 contributors\npass\n')
    git(fixture, 'add', '.')
    git(fixture, 'commit', '-qm', 'Exact tracked notices and own-file declaration')
    initial = git(fixture, 'rev-parse', 'HEAD').decode().strip()

    def run(expected, reason=None, extra=()):
        global checks
        result = subprocess.run([sys.executable, str(tool), '--repo', str(fixture), *extra], text=True, capture_output=True)
        assert result.returncode == expected, (result.returncode, result.stdout, result.stderr)
        if reason:
            assert reason in result.stdout, result.stdout
        checks += 1
        return result

    def mutation(path, transform, reason):
        git(fixture, 'reset', '--hard', initial)
        target = fixture / path
        target.write_bytes(transform(target.read_bytes()))
        git(fixture, 'add', path)
        git(fixture, 'commit', '-qm', reason)
        run(1, reason)

    run(0)
    # Untracked malformed code is not ingested; the scanned source is immutable Git.
    (fixture / 'tools/private-untracked.py').write_text('pass\n')
    run(0)
    (fixture / 'tools/private-untracked.py').unlink()
    def drop_copyright(data):
        lines = data.splitlines(keepends=True)
        required = next(line for line in lines if b'Copyright' in line)
        return b''.join(line for line in lines if line != required)
    mutation('firmware/src/eng_phys.c', drop_copyright, 'lost_upstream_notice')
    mutation('tools/own.py', lambda data: data.replace(b'# SPDX-License-Identifier: GPL-3.0-only\n', b''), 'missing_comment_header_spdx')
    for identifier in (b'GPL-3.0-or-later', b'MIT', b'LicenseRef-Proprietary'):
        mutation('tools/own.py', lambda data, identifier=identifier: data.replace(b'GPL-3.0-only', identifier), 'unexpected_spdx_identifier')
    mutation('tools/own.py', lambda data: data.replace(b'# SPDX-License-Identifier: GPL-3.0-only\n', b'license = "# SPDX-License-Identifier: GPL-3.0-only"\n'), 'missing_comment_header_spdx')
    mutation('tools/own.py', lambda data: data.replace(b'# SPDX-License-Identifier: GPL-3.0-only\n', b'# SPDX-License-Identifier:\nlicense = "SPDX-License-Identifier: GPL-3.0-only"\n'), 'missing_comment_header_spdx')
    mutation('tools/own.py', lambda data: data.replace(b'# SPDX-License-Identifier: GPL-3.0-only\n', b'# SPDX-License-Identifier: GPL-3.0-only\n# SPDX-License-Identifier: GPL-3.0-only\n'), 'unexpected_spdx_identifier')
    mutation('LICENSE', lambda data: b'X' + data[1:], 'changed_notice_or_asset')
    mutation('firmware/src/phys_dsp.c', lambda data: data.replace(b'*/', b'\n*/', 1), 'changed_port_notice_header')
    git(fixture, 'reset', '--hard', initial)
    licensing = fixture / 'LICENSING.md'
    licensing.write_bytes(licensing.read_bytes() + b'\nAdditional source-ledger link.\n')
    git(fixture, 'add', 'LICENSING.md')
    git(fixture, 'commit', '-qm', 'Append-only licensing documentation')
    run(0)
    output = Path(directory) / 'inventory.json'
    run(0, extra=('--output', str(output)))
    before = output.read_bytes()
    run(2, extra=('--output', str(output)))
    assert output.read_bytes() == before
    checks += 1
    # Clone only the latest commit: validation needs no upstream history/network.
    shallow = Path(directory) / 'shallow'
    subprocess.run(['git', 'clone', '-q', '--depth=1', fixture.as_uri(), str(shallow)], check=True)
    assert git(shallow, 'rev-list', '--count', 'HEAD').strip() == b'1'
    result = subprocess.run([sys.executable, str(tool), '--repo', str(shallow)], text=True, capture_output=True)
    assert result.returncode == 0, (result.stdout, result.stderr)
    checks += 1
print(f'license inventory: {checks} checks PASS; real immutable Git CLI, notice-loss/header SPDX/identifier/body-spoof/asset/header negatives and shallow-history validation')
