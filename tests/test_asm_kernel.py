import contextlib
import io
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
import project


class AssemblyKernelTests(unittest.TestCase):
    def test_driver_builds_and_runs_native_or_portable_correctness_suite(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            result = project.asm_kernel('test')
        self.assertEqual(result, 0)
        self.assertIn('assembly kernel correctness: PASS', output.getvalue())
        if not project.on_pi():
            self.assertIn('portable C fallback', output.getvalue())


if __name__ == '__main__':
    unittest.main()
