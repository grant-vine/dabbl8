#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Selector/refusal tests; never execute firmware, downloads or hosted jobs."""
import contextlib
import io
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import local_checks as checks


class SelectionTest(unittest.TestCase):
    def plan(self, paths, explicit=()):
        return checks.make_plan(Path('/missing-local-inputs'), paths, explicit,
                                ['clang'], Path('build/local-checks/PLANNED'), 'HEAD')

    def test_docs_only(self):
        selected, unknown = checks.select(['README.md', 'docs/x.json'])
        self.assertEqual(selected, {'diff'})
        self.assertFalse(unknown)
        self.assertEqual(len(self.plan(['README.md'])['commands']), 1)

    def test_timing_capture_has_only_scripted_transport_recipe(self):
        for path in ('tools/dabbl8_timing_capture.py', 'tests/dabbl8_timing_capture_test.py'):
            p = self.plan([path])
            self.assertEqual(p['checks'], ['diff', 'timing-capture'])
            self.assertEqual([step['name'] for step in p['commands']], ['diff', 'timing-capture-scripted-transport'])
            self.assertEqual(p['commands'][1]['argv'][1:], ['tests/dabbl8_timing_capture_test.py'])
            self.assertEqual(p['status'], 'REFUSED')  # inputs absent, never substituted
            self.assertFalse(any('--capture' in step['argv'] or 'run_tests.sh' in step['argv'] for step in p['commands']))
        # Shared report-parser edits need separate broader coverage; no silent narrow qualification.
        selected, unknown = checks.select(['tools/dabbl8_timing_report.py'])
        self.assertEqual(selected, {'diff'})
        self.assertEqual(unknown, ['tools/dabbl8_timing_report.py'])

    def test_timing_capture_missing_input_refuses_before_execution(self):
        with tempfile.TemporaryDirectory() as folder:
            repo = Path(folder)
            for name in ('tools/dabbl8_timing_capture.py', 'tools/dabbl8_timing_report.py', 'tests/dabbl8_timing_capture_test.py'):
                path = repo/name; path.parent.mkdir(parents=True, exist_ok=True); path.touch()
            p = checks.make_plan(repo, ['tools/dabbl8_timing_capture.py'], (), ['clang'], Path('out'), 'HEAD')
            self.assertEqual(p['status'], 'PLAN')
            (repo/'tools/dabbl8_timing_report.py').unlink()
            with patch.object(checks, 'ROOT', repo), patch.object(checks.subprocess, 'run') as run:
                with contextlib.redirect_stdout(io.StringIO()):
                    self.assertEqual(checks.main(['--paths', 'tools/dabbl8_timing_capture.py', '--run']), 2)
                run.assert_not_called()
                self.assertFalse((repo/'build/local-checks').exists())

    def test_known_storage(self):
        p = self.plan(['firmware/src/storage.c'])
        self.assertEqual(p['checks'], ['diff', 'storage'])
        self.assertTrue(any('tests/native_storage_gate_test.c' in x['argv'] for x in p['commands']))
        self.assertTrue(any('tests/storage_test.c' in x['argv'] for x in p['commands']))
        self.assertNotIn('build/gen/felucca_tables.h', p['commands'][1]['requires'])

    def test_known_legacy_and_sampler(self):
        for path, name in [('tests/ui_test.c', 'legacy-ui'), ('firmware/src/eng_slice.c', 'sampler')]:
            p = self.plan([path])
            self.assertIn(name, p['checks'])
            self.assertEqual(len(p['commands']), 5 if name == 'legacy-ui' else 9)
            self.assertEqual(p['status'], 'REFUSED')  # no generated inputs

    def test_fx_quiet_selects_only_focused_recipe(self):
        for path in ('firmware/src/fx.c', 'tests/native_fx_stationary_quiet_test.c'):
            p = self.plan([path])
            self.assertEqual(p['checks'], ['diff', 'fx-quiet'])
            self.assertEqual([s['name'] for s in p['commands']],
                             ['diff', 'fx-quiet-8-compile', 'fx-quiet-8'])
            compile_step = p['commands'][1]
            for arg in ('-DNPART=8', '-DFELUCCA_FM4=0', '-DFELUCCA_SLICE=1',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-fno-sanitize=signed-integer-overflow,shift'):
                self.assertIn(arg, compile_step['argv'])
            self.assertIn('tests/native_fx_stationary_quiet_test.c', compile_step['argv'])
            for header in checks.GEN:
                self.assertIn('build/gen/' + header, compile_step['requires'])
            # This fixture does not include the sample upload range header.
            self.assertNotIn('build/gen/d8_flash_store_ranges_test.h', compile_step['requires'])
            self.assertEqual(p['status'], 'REFUSED')
            self.assertFalse(any('run_tests.sh' in s['argv'] for s in p['commands']))

    def test_fx_quiet_inputs_are_required_without_execution(self):
        with tempfile.TemporaryDirectory() as folder:
            repo = Path(folder)
            steps = checks.recipes({'diff', 'fx-quiet'}, ['clang'], Path('out'), 'HEAD')
            required = steps[1]['requires']
            for name in required:
                file = repo / name
                file.parent.mkdir(parents=True, exist_ok=True)
                file.touch()
            def plan():
                return checks.make_plan(repo, ['firmware/src/fx.c'], (),
                                        ['clang'], Path('out'), 'HEAD')
            with patch.object(checks.shutil, 'which', return_value='/usr/bin/clang'):
                self.assertEqual(plan()['status'], 'PLAN')
                for name in required:
                    file = repo / name
                    file.unlink()
                    p = plan()
                    self.assertEqual(p['status'], 'REFUSED')
                    self.assertTrue(any(name in refusal for refusal in p['refusals']))
                    file.touch()
            with patch.object(checks, 'ROOT', repo), patch.object(checks.subprocess, 'run') as run:
                (repo / 'build/gen/felucca_tables.h').unlink()
                with contextlib.redirect_stdout(io.StringIO()):
                    self.assertEqual(checks.main(['--paths', 'firmware/src/fx.c', '--run']), 2)
                run.assert_not_called()
                self.assertFalse((repo / 'build/local-checks').exists())

    def test_deleted_workflow_selects_local_runner(self):
        selected, unknown = checks.select(['.github/workflows/old.yml'])
        self.assertIn('runner', selected)
        self.assertFalse(unknown)

    def test_timeout_keeps_partial_logs(self):
        import subprocess
        with patch.object(checks.subprocess, 'run', side_effect=subprocess.TimeoutExpired(
                ['slow'], 120, output=b'partial output', stderr=b'partial error')):
            code, stdout, stderr = checks.execute(['slow'], Path('.'), {})
        self.assertEqual(code, 124)
        self.assertEqual(stdout, b'partial output')
        self.assertIn(b'partial error', stderr)
        self.assertIn(b'timed out', stderr)

    def test_missing_inputs_refuse(self):
        p = self.plan(['tests/native_readonly_boot_test.c'])
        self.assertEqual(p['status'], 'REFUSED')
        self.assertTrue(any('Missing local inputs' in x for x in p['refusals']))

    def test_unknown_refuses(self):
        p = self.plan(['firmware/src/audio.c'])
        self.assertEqual(p['status'], 'REFUSED')
        self.assertEqual(p['unmapped_paths'], ['firmware/src/audio.c'])

    def test_explicit_selection_retains_gap(self):
        p = self.plan(['firmware/src/audio.c'], ['docs'])
        self.assertEqual(p['status'], 'PLAN')
        self.assertEqual(p['selection'], 'explicit')
        self.assertEqual(p['unmapped_paths'], ['firmware/src/audio.c'])
        self.assertIn('not complete coverage', p['scope'])

    def test_paths_cannot_escape(self):
        for path in ['../firmware/src/audio.c', '/tmp/file.md']:
            with self.assertRaises(ValueError):
                checks.select([path])

    def test_no_changes_is_not_pass(self):
        self.assertEqual(self.plan([])['status'], 'NO_CHANGES')

    def test_update_override_refuses(self):
        with patch.dict(os.environ, {'GOLDEN_UPDATE': '1'}):
            self.assertEqual(self.plan(['README.md'])['status'], 'REFUSED')

    def test_dry_plan_has_no_execution_or_outputs(self):
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(checks, 'ROOT', Path(folder)), patch.object(checks.subprocess, 'run') as run:
                out = io.StringIO()
                with contextlib.redirect_stdout(out):
                    code = checks.main(['--paths', 'README.md'])
                self.assertEqual(code, 0)
                self.assertEqual(json.loads(out.getvalue())['status'], 'PLAN')
                run.assert_not_called()
                self.assertEqual(list(Path(folder).iterdir()), [])

    def test_unknown_run_never_executes(self):
        with patch.object(checks.subprocess, 'run') as run:
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(checks.main(['--paths', 'firmware/src/audio.c', '--run']), 2)
            run.assert_not_called()


if __name__ == '__main__':
    unittest.main()
