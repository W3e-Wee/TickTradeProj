"""Non-destructive repository readiness command tests."""
import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
import project


class ReadinessTests(unittest.TestCase):
    def test_pi_and_manual_gaps_do_not_fail_source_readiness(self):
        output = io.StringIO()
        with patch.object(project, 'build_all', side_effect=AssertionError('must not build')), \
                contextlib.redirect_stdout(output):
            status = project.readiness()
        self.assertEqual(status, 0)
        report = output.getvalue()
        self.assertIn('[PI EVIDENCE REQUIRED] Pi evidence tree:', report)
        self.assertIn('[MANUAL ITEM] PMCOAReportGroup05.pdf', report)
        self.assertIn('read-only; no build, benchmark, or evidence changes', report)

    def test_missing_required_source_layout_returns_nonzero(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'tests').mkdir()
            (root / 'tests' / 'test_placeholder.py').write_text('', encoding='utf-8')
            output = io.StringIO()
            with patch.object(project, 'ROOT', root), patch.object(project, 'BUILD_ROOT', root / 'build'), \
                    contextlib.redirect_stdout(output):
                status = project.readiness()
        self.assertEqual(status, 1)
        self.assertIn('[MISSING] Required source, build, and documentation files:', output.getvalue())
        self.assertIn('Source/layout result: BLOCKED', output.getvalue())


if __name__ == '__main__':
    unittest.main()
