# SPDX-License-Identifier: GPL-3.0-only
"""Tiny controlled archives/checkouts exercise refusal without tool execution."""
import contextlib
import hashlib
import io
import json
import runpy
import shutil
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest import mock

spec = importlib.util.spec_from_file_location('integrity', Path(__file__).resolve().parents[1] / 'ci/dependency_integrity.py')
integrity = importlib.util.module_from_spec(spec)
spec.loader.exec_module(integrity)
session_spec = importlib.util.spec_from_file_location('evidence_session', Path(__file__).resolve().parents[1] / 'ci/evidence_session.py')
evidence_session = importlib.util.module_from_spec(session_spec)
session_spec.loader.exec_module(evidence_session)

class IntegrityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def archive(self):
        original = self.root / 'original/toolchain'
        (original / 'bin').mkdir(parents=True)
        (original / 'lib').mkdir()
        (original / 'bin/compiler').write_bytes(b'pinned compiler')
        (original / 'bin/compiler').chmod(0o755)
        (original / 'lib/runtime').write_bytes(b'pinned library')
        (original / 'bin/link').symlink_to('compiler')
        archive = self.root / 'tools.tar.xz'
        with tarfile.open(archive, 'w:xz') as output:
            output.add(original, arcname='toolchain')
        extraction = self.root / 'extraction'
        with tarfile.open(archive) as source:
            source.extractall(extraction, filter='data')
        return archive, extraction / 'toolchain', hashlib.sha256(archive.read_bytes()).hexdigest()

    def test_complete_archive_acceptance(self):
        archive, tree, digest = self.archive()
        receipt = integrity.verify_archive_tree(archive, tree, 'toolchain', digest)
        self.assertEqual(receipt['regular_files'], 2)
        self.assertEqual(receipt['symlinks'], 1)
        self.assertEqual(receipt['extra_entries'], 0)

    def test_archive_digest_refusal(self):
        archive, tree, digest = self.archive()
        archive.write_bytes(archive.read_bytes() + b'changed')
        with self.assertRaisesRegex(ValueError, 'SHA-256'):
            integrity.verify_archive_tree(archive, tree, 'toolchain', digest)

    def test_each_reused_member_refused_and_preserved(self):
        mutations = ['compiler', 'library', 'symlink', 'extra', 'executable', 'missing']
        for mutation in mutations:
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory(dir=self.root) as isolated:
                previous = self.root
                self.root = Path(isolated)
                try:
                    archive, tree, digest = self.archive()
                    if mutation == 'compiler': (tree / 'bin/compiler').write_bytes(b'pinned compileX')
                    if mutation == 'library': (tree / 'lib/runtime').write_bytes(b'pinned librarX')
                    if mutation == 'symlink':
                        (tree / 'bin/link').unlink(); (tree / 'bin/link').symlink_to('../lib/runtime')
                    if mutation == 'extra': (tree / 'lib/untracked').write_bytes(b'extra dependency')
                    if mutation == 'executable': (tree / 'bin/compiler').chmod(0o644)
                    if mutation == 'missing': (tree / 'lib/runtime').unlink()
                    before = {str(p.relative_to(tree)): (os.readlink(p) if p.is_symlink() else p.read_bytes())
                              for p in tree.rglob('*') if p.is_symlink() or p.is_file()}
                    with self.assertRaises(ValueError):
                        integrity.verify_archive_tree(archive, tree, 'toolchain', digest)
                    after = {str(p.relative_to(tree)): (os.readlink(p) if p.is_symlink() else p.read_bytes())
                             for p in tree.rglob('*') if p.is_symlink() or p.is_file()}
                    self.assertEqual(before, after)
                finally:
                    self.root = previous

    def sdk(self):
        sdk = self.root / 'sdk'
        sdk.mkdir()
        def git(*args):
            return subprocess.check_output(['git', '-C', str(sdk), *args], stderr=subprocess.STDOUT)
        git('init', '-q')
        (sdk / 'input').write_bytes(b'pinned SDK input')
        git('add', 'input')
        git('-c', 'user.name=Fixture', '-c', 'user.email=fixture@example.invalid',
            '-c', 'commit.gpgsign=false', '-c', 'core.hooksPath=/dev/null', 'commit', '-qm', 'fixture')
        return sdk, git('rev-parse', 'HEAD').decode().strip(), git

    def driver(self, name):
        repo = self.root / 'driver'
        (repo / 'ci').mkdir(parents=True)
        (repo / 'tests').mkdir()
        source = Path(__file__).resolve().parents[1]
        shutil.copyfile(source / 'ci' / name, repo / 'ci' / name)
        shutil.copyfile(source / 'ci/dependencies.json', repo / 'ci/dependencies.json')
        for name in ('golden.txt', 'cpu_baseline.txt', 'target_budget.txt'):
            (repo / 'tests' / name).write_text('unchanged reference\n')
        return repo

    def test_evidence_refusal_never_executes_target_compiler_or_build(self):
        repo = self.driver('run_evidence.py')
        previous = repo / 'build/ci-evidence'
        previous.mkdir(parents=True)
        stale = previous / 'host-tests.log'
        stale.write_text('== skip stale sanitizer runs\n')
        calls = []
        def capture(args, **kwargs):
            calls.append(args)
            return subprocess.CompletedProcess(args, 0, stdout='fixture')
        env = {'JIELI_TOOLCHAIN': str(self.root / 'target'),
               'AC79_SDK': str(self.root / 'sdk'), 'GITHUB_ACTIONS': 'false',
               'GITHUB_STEP_SUMMARY': '', 'GOLDEN_UPDATE': '0', 'BUDGET_UPDATE': '0', 'CC': 'clang-18'}
        with mock.patch.dict(sys.modules, dependency_integrity=integrity, evidence_session=evidence_session), \
             mock.patch.dict(os.environ, env), \
             mock.patch('platform.platform', return_value='fixture host'), \
             mock.patch.object(integrity, 'verify_archive_tree', side_effect=ValueError('altered compiler fixture')), \
             mock.patch.object(integrity, 'verify_sdk') as sdk_check, \
             mock.patch.object(subprocess, 'run', side_effect=capture), \
             contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaises(SystemExit) as exit_status:
                runpy.run_path(str(repo / 'ci/run_evidence.py'), run_name='__main__')
        self.assertEqual(exit_status.exception.code, 1)
        sdk_check.assert_not_called()
        self.assertFalse(any(str(self.root / 'target') in str(arg) or arg == 'build.sh' for args in calls for arg in args))
        reports = list((repo / 'build/ci-evidence').glob('run-*/results.json'))
        self.assertEqual(len(reports), 1)
        report = json.loads(reports[0].read_text())
        self.assertEqual(stale.read_text(), '== skip stale sanitizer runs\n')
        self.assertEqual(report['log_hashes'], {})
        self.assertFalse(any('stale' in item.get('reason', '') for item in report['skips']))
        self.assertIn(['clang-18', '--version'], calls)
        self.assertEqual(report['result'], 'FAIL')
        self.assertEqual(report['pin_errors'], ['Dependency content'])
        self.assertEqual(report['dependency_integrity']['error'], 'altered compiler fixture')
        self.assertNotIn('target_compiler', report['versions'])
        self.assertEqual(report['skips'][0]['reason'], 'Failed preflight')

    def test_browser_driver_extracts_fresh_and_preserves_previous_sdk(self):
        repo = self.driver('run_browser_conversion.py')
        (repo / 'build/gen').mkdir(parents=True)
        (repo / 'build/gen/felucca_tables.h').write_text('fixture')
        deps = repo / '.local-baseline/ci/browser-deps'
        deps.mkdir(parents=True)
        old = deps / 'sdk/install/emscripten'
        old.mkdir(parents=True)
        (old / 'emcc').write_text('preserve altered previous SDK')
        original = self.root / 'archive/install/emscripten'
        original.mkdir(parents=True)
        (original / 'emscripten-version.txt').write_text('"4.0.17-git"\n')
        (original / 'emcc').write_text('pinned compiler fixture')
        archive = deps / 'emscripten-4.0.17.tar.xz'
        with tarfile.open(archive, 'w:xz') as output:
            output.add(original.parent, arcname='install')
        lock = {'linux_archive_sha256': integrity.file_digest(archive),
                'linux_archive_bytes': archive.stat().st_size, 'emscripten_version': '4.0.17'}
        (repo / 'ci/browser-conversion.json').write_text(json.dumps(lock))
        previous = repo / 'build/browser-conversion-evidence'
        previous.mkdir(parents=True)
        stale = previous / 'browser-parity.log'
        stale.write_text('previous successful parity must remain historical\n')
        first_evidence = None
        first_bytes = None
        for invocation in range(2):
            def failed_build(args, **kwargs):
                kwargs['stdout'].write('fixture invocation ' + str(invocation) + '\n')
                return subprocess.CompletedProcess(args, 7)
            before = set(previous.glob('run-*/results.json'))
            # Stop after extraction/version reporting, before actual compilation.
            with mock.patch.dict(sys.modules, evidence_session=evidence_session), \
                 mock.patch('platform.system', return_value='Linux'), \
                 mock.patch('platform.machine', return_value='x86_64'), \
                 mock.patch('shutil.which', return_value='/fixture/node'), \
                 mock.patch.object(subprocess, 'check_output', return_value='fixture version'), \
                 mock.patch.object(subprocess, 'run', side_effect=failed_build), \
                 contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(SystemExit) as exit_status:
                    runpy.run_path(str(repo / 'ci/run_browser_conversion.py'), run_name='__main__')
            self.assertEqual(exit_status.exception.code, 1)
            current = set(previous.glob('run-*/results.json')) - before
            self.assertEqual(len(current), 1)
            self.assertEqual(len(list(previous.glob('run-*/results.json'))), invocation + 1)
            current_report = current.pop()
            report = json.loads(current_report.read_text())
            if first_evidence is None:
                first_evidence = current_report.parent
                first_bytes = {p.name: p.read_bytes() for p in first_evidence.iterdir()}
            else:
                self.assertEqual({p.name: p.read_bytes() for p in first_evidence.iterdir()}, first_bytes)
            self.assertEqual((current_report.parent / 'wasm-build.log').read_text(), 'fixture invocation ' + str(invocation) + '\n')
            self.assertEqual(stale.read_text(), 'previous successful parity must remain historical\n')
            self.assertEqual(set(report['log_hashes']), {'wasm-build.log'})
            self.assertFalse(report['sdk_extraction']['reused_tree'])
            self.assertIn('wasm-build failed', report['reason'])
            trees = list(deps.glob('sdk-verified-*'))
            self.assertEqual(len(trees), invocation + 1)
            for tree in trees:
                self.assertEqual((tree / 'install/emscripten/emcc').read_text(), 'pinned compiler fixture')
            self.assertEqual((old / 'emcc').read_text(), 'preserve altered previous SDK')

    def test_sdk_acceptance(self):
        sdk, commit, git = self.sdk()
        self.assertEqual(integrity.verify_sdk(sdk, commit)['populated_tracked_entries_verified'], 1)

    def test_sparse_omission_cannot_hide_required_input(self):
        sdk, commit, git = self.sdk()
        git('update-index', '--skip-worktree', 'input')
        (sdk / 'input').unlink()
        receipt = integrity.verify_sdk(sdk, commit)
        self.assertEqual(receipt['sparse_entries_omitted'], 1)
        with self.assertRaisesRegex(ValueError, 'missing'):
            integrity.verify_sdk(sdk, commit, required_paths=('input',))
        with self.assertRaisesRegex(ValueError, 'not tracked'):
            integrity.verify_sdk(sdk, commit, required_paths=('other',))

    def test_sdk_dirty_commit_untracked_and_hidden_edits(self):
        sdk, commit, git = self.sdk()
        with self.assertRaisesRegex(ValueError, 'commit'):
            integrity.verify_sdk(sdk, '0' * 40)
        (sdk / 'untracked').write_bytes(b'preserve me')
        with self.assertRaisesRegex(ValueError, 'working tree'):
            integrity.verify_sdk(sdk, commit)
        self.assertEqual((sdk / 'untracked').read_bytes(), b'preserve me')
        (sdk / 'untracked').unlink()
        for flag in ['--assume-unchanged', '--skip-worktree']:
            git('update-index', flag, 'input')
            (sdk / 'input').write_bytes(b'changed SDK input')
            self.assertFalse(git('status', '--porcelain'))
            with self.assertRaisesRegex(ValueError, 'tracked content'):
                integrity.verify_sdk(sdk, commit)
            self.assertEqual((sdk / 'input').read_bytes(), b'changed SDK input')
            (sdk / 'input').write_bytes(b'pinned SDK input')
            git('update-index', '--no-assume-unchanged', '--no-skip-worktree', 'input')

if __name__ == '__main__':
    unittest.main()
