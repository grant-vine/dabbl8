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
