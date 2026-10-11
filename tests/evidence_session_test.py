# SPDX-License-Identifier: GPL-3.0-only
"""Fresh evidence directories preserve previous files and refuse creation errors."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('evidence_session', Path(__file__).resolve().parents[1] / 'ci/evidence_session.py')
session = importlib.util.module_from_spec(spec)
spec.loader.exec_module(session)

class EvidenceSessionTests(unittest.TestCase):
    def test_repeated_sessions_preserve_previous_logs_and_reports(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / 'evidence'
            first = session.create_evidence_directory(root)
            (first / 'checks.log').write_bytes(b'previous failed check')
            (first / 'results.json').write_bytes(b'previous result')
            (root / 'historical.log').write_bytes(b'old root log')
            second = session.create_evidence_directory(root)
            self.assertNotEqual(first, second)
            self.assertEqual(list(second.iterdir()), [])
            self.assertEqual(first.parent, root)
            self.assertEqual(second.parent, root)
            self.assertEqual((first / 'checks.log').read_bytes(), b'previous failed check')
            self.assertEqual((first / 'results.json').read_bytes(), b'previous result')
            self.assertEqual((root / 'historical.log').read_bytes(), b'old root log')

    def test_existing_file_is_preserved_on_creation_error(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / 'evidence'
            root.write_bytes(b'preserve existing file')
            with self.assertRaises(FileExistsError):
                session.create_evidence_directory(root)
            self.assertEqual(root.read_bytes(), b'preserve existing file')

if __name__ == '__main__':
    unittest.main()
