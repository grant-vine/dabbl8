#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Fetch verified build dependencies only; never contact a device."""
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import shutil
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
LOCK = json.loads((ROOT / 'ci/dependencies.json').read_text())
DEST = ROOT / '.local-baseline/ci/deps'
DEST.mkdir(parents=True, exist_ok=True)
archive = DEST / 'linux-toolchain.tar.xz'
if not archive.exists():
    subprocess.run(['curl', '-fL', '--retry', '3', LOCK['toolchain_url'], '-o', str(archive)], check=True)
actual_hash = hashlib.sha256(archive.read_bytes()).hexdigest()
if actual_hash != LOCK['toolchain_archive_sha256']:
    raise SystemExit(f'Toolchain archive changed: refusing unreviewed upgrade; actual SHA-256 {actual_hash}')
toolchain = DEST / LOCK['toolchain_directory']
if not toolchain.exists():
    with tarfile.open(archive) as source:
        source.extractall(DEST, filter='data')
if not (toolchain / 'pi32v2/bin/clang').is_file():
    raise SystemExit('Verified archive did not contain expected compiler')
sdk = DEST / 'ac79-sdk'
def fetch_sdk(destination):
    # Only freshly-created staging directories are removed. Existing SDK work
    # is never reset/deleted; every successful fetch still needs the exact commit.
    for attempt in range(3):
        staging = Path(tempfile.mkdtemp(prefix='sdk-fetch-', dir=DEST))
        try:
            subprocess.run(['git', 'clone', '--depth', '1', '--branch', LOCK['sdk_tag'], LOCK['sdk_url'], str(staging)], check=True, timeout=300)
            fetched = subprocess.check_output(['git', '-C', str(staging), 'rev-parse', 'HEAD'], text=True).strip()
            if fetched != LOCK['sdk_commit']:
                raise SystemExit('SDK commit changed: refusing unreviewed upgrade')
            staging.rename(destination)
            return
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
            shutil.rmtree(staging)
            if attempt == 2:
                raise
            print(f'SDK network attempt {attempt + 1} failed; retrying unchanged commit', flush=True)
            time.sleep(2 * (attempt + 1))

if not sdk.exists():
    fetch_sdk(sdk)
commit = subprocess.check_output(['git', '-C', str(sdk), 'rev-parse', 'HEAD'], text=True).strip()
if commit != LOCK['sdk_commit']:
    raise SystemExit('SDK commit changed: refusing unreviewed upgrade')
subprocess.run(['docker', 'pull', '--platform', 'linux/amd64', LOCK['container']], check=True)
# The workflow owns GITHUB_ENV. No credentials or user-controlled text is written.
import os
if os.environ.get('GITHUB_ENV'):
    with open(os.environ['GITHUB_ENV'], 'a') as output:
        output.write(f'JIELI_TOOLCHAIN={toolchain}\nAC79_SDK={sdk}\nJIELI_DOCKER=1\nJIELI_DOCKER_IMAGE={LOCK["container"]}\n')
print('Verified toolchain archive, SDK commit and immutable container digest')
