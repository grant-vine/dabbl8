# SPDX-License-Identifier: GPL-3.0-only
"""Read-only content verification of pinned build inputs; never repairs user work."""
import hashlib
import os
from pathlib import Path, PurePosixPath
import stat
import subprocess
import tarfile


def file_digest(path, algorithm='sha256'):
    digest = hashlib.new(algorithm)
    with Path(path).open('rb') as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def verify_archive_tree(archive, root, prefix, expected_sha256):
    """Check every archived member and refuse extra entries in the extracted root."""
    archive, root = Path(archive), Path(root).resolve()
    if file_digest(archive) != expected_sha256:
        raise ValueError('Pinned toolchain archive SHA-256 mismatch')
    prefix = PurePosixPath(prefix)
    expected = set()
    files = links = 0
    # Stream once: repeated seeks in xz would repeatedly decompress the archive.
    with tarfile.open(archive, 'r|*') as source:
        for member in source:
            name = PurePosixPath(member.name)
            if name.is_absolute() or '..' in name.parts:
                raise ValueError('Unsafe archive member path')
            try:
                relative = name.relative_to(prefix)
            except ValueError:
                raise ValueError('Archive member outside pinned toolchain directory') from None
            if not relative.parts:
                if not member.isdir() or not root.is_dir():
                    raise ValueError('Toolchain root missing or wrong type')
                continue
            key = relative.as_posix()
            if key in expected:
                raise ValueError('Duplicate archive member: ' + key)
            expected.add(key)
            actual = root.joinpath(*relative.parts)
            try:
                actual.resolve().relative_to(root)
            except ValueError:
                raise ValueError('Extracted member escapes toolchain root: ' + key) from None
            if member.isdir():
                if actual.is_symlink() or not actual.is_dir():
                    raise ValueError('Toolchain directory differs: ' + key)
            elif member.issym():
                if not actual.is_symlink() or os.readlink(actual) != member.linkname:
                    raise ValueError('Toolchain symlink differs: ' + key)
                links += 1
            elif member.isfile():
                if actual.is_symlink() or not actual.is_file() or actual.stat().st_size != member.size:
                    raise ValueError('Toolchain file missing or wrong type/size: ' + key)
                archived = source.extractfile(member)
                digest = hashlib.sha256()
                for chunk in iter(lambda: archived.read(1024 * 1024), b''):
                    digest.update(chunk)
                if file_digest(actual) != digest.hexdigest():
                    raise ValueError('Toolchain file content differs: ' + key)
                if bool(actual.stat().st_mode & stat.S_IXUSR) != bool(member.mode & stat.S_IXUSR):
                    raise ValueError('Toolchain executable permission differs: ' + key)
                files += 1
            else:
                raise ValueError('Unsupported pinned archive member type: ' + key)
    observed = {entry.relative_to(root).as_posix() for entry in root.rglob('*')}
    if observed != expected:
        raise ValueError('Extracted toolchain contains missing or unarchived entries')
    return {'archive_sha256': expected_sha256, 'regular_files': files, 'symlinks': links,
            'entries': len(expected), 'extra_entries': 0}


SDK_BUILD_INPUTS = (
    "cpu/wl82/tools/uboot.boot", "cpu/wl82/tools/cfg_tool.bin",
    "cpu/wl82/tools/cfg/eq_cfg_hw.bin",
)


def verify_sdk(root, expected_commit, required_paths=()):
    """Check HEAD, status and every populated tracked blob against HEAD.

    Direct blob checks detect edits hidden by assume-unchanged/skip-worktree.
    Explicit sparse omissions are allowed only for non-required build inputs.
    No checkout, reset, deletion, execution of SDK files, or content logging.
    """
    root = Path(root).resolve()
    def git(*args):
        return subprocess.check_output(['git', '-C', str(root), *args],
                                       env={**os.environ, 'GIT_OPTIONAL_LOCKS': '0'})
    commit = git('rev-parse', 'HEAD').decode().strip()
    if commit != expected_commit:
        raise ValueError('SDK commit differs from lock; existing work preserved')
    if git('status', '--porcelain', '--untracked-files=all'):
        raise ValueError('SDK working tree differs from lock; existing work preserved')
    algorithm = git('rev-parse', '--show-object-format').decode().strip()
    count = omitted = 0
    required = set(required_paths)
    tracked = set()
    sparse = {os.fsdecode(row[2:]) for row in git('ls-files', '-v', '-z').split(b'\0')
              if row and row[:1].upper() == b'S'}
    for row in git('ls-tree', '-rz', 'HEAD').split(b'\0'):
        if not row:
            continue
        metadata, raw_name = row.split(b'\t', 1)
        mode, kind, oid = metadata.decode().split()
        name = os.fsdecode(raw_name)
        path = root / name
        tracked.add(name)
        if not path.exists() and not path.is_symlink() and name in sparse and name not in required:
            omitted += 1
            continue
        try:
            path.resolve().relative_to(root)
        except ValueError:
            raise ValueError('SDK tracked path escapes checkout') from None
        if mode == '160000':
            verify_sdk(path, oid)
            count += 1
            continue
        if kind != 'blob':
            raise ValueError('Unsupported SDK tracked entry')
        digest = hashlib.new(algorithm)
        if mode == '120000':
            if not path.is_symlink():
                raise ValueError('SDK tracked symlink differs; existing work preserved')
            data = os.fsencode(os.readlink(path))
            digest.update(b'blob ' + str(len(data)).encode() + b'\0' + data)
        else:
            if path.is_symlink() or not path.is_file():
                raise ValueError('SDK tracked file missing or wrong type; existing work preserved')
            if bool(path.stat().st_mode & stat.S_IXUSR) != (mode == '100755'):
                raise ValueError('SDK executable permission differs; existing work preserved')
            size = path.stat().st_size
            digest.update(b'blob ' + str(size).encode() + b'\0')
            with path.open('rb') as source:
                for chunk in iter(lambda: source.read(1024 * 1024), b''):
                    digest.update(chunk)
        if digest.hexdigest() != oid:
            raise ValueError('SDK tracked content differs from pinned commit; existing work preserved')
        count += 1
    if not required.issubset(tracked):
        raise ValueError('Required SDK build input is not tracked by pinned commit')
    return {'commit': commit, 'populated_tracked_entries_verified': count,
            'sparse_entries_omitted': omitted, 'required_inputs': sorted(required),
            'worktree_clean': True}
