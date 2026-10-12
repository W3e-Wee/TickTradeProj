"""Workflow checks for benchmark, perf, and assembly evidence handling."""
import contextlib
import csv
import hashlib
import json
import io
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
import project


class PerformanceWorkflowTests(unittest.TestCase):
    def test_python_comparison_smoke_is_separate_and_final_label_needs_five(self):
        smoke, smoke_class, smoke_kind = project.python_comparison_profile(1, 'pi-smoke-test20')
        self.assertTrue(smoke)
        self.assertEqual('SMOKE / NON-FINAL', smoke_class)
        self.assertEqual('python-comparison-smoke', smoke_kind)
        final, final_class, final_kind = project.python_comparison_profile(5, 'pi-c-python-numpy-final')
        self.assertFalse(final)
        self.assertEqual('FINAL-COMPARISON ELIGIBLE', final_class)
        self.assertEqual('python-comparisons', final_kind)
        with self.assertRaises(ValueError):
            project.python_comparison_profile(1, 'pi-final-controlled')
        with tempfile.TemporaryDirectory() as temp, patch.object(project, 'ROOT', Path(temp)), \
                patch.object(project, 'on_pi', return_value=False):
            smoke_path = project.unique_evidence_dir(smoke_kind, 'pi-smoke-test20')
            final_path = project.unique_evidence_dir(final_kind, 'pi-c-python-numpy-final')
        self.assertNotEqual(smoke_path, final_path)
        self.assertIn('python-comparison-smoke-runs', str(smoke_path))
        self.assertIn('python-comparisons-runs', str(final_path))

    def test_metadata_captures_governors_and_unique_result_roots(self):
        with tempfile.TemporaryDirectory() as temp:
            with patch.object(project, 'ROOT', Path(temp)), patch.object(
                    project, 'on_pi', return_value=True), patch.object(
                    project, 'cpu_governors', return_value={'cpu0': 'performance', 'cpu1': 'powersave'}):
                first = project.unique_evidence_dir('performance', 'pi-final-performance')
                second = project.unique_evidence_dir('performance', 'pi-final-performance')
                self.assertNotEqual(first, second)
                self.assertEqual(first.relative_to(Path(temp)).parts[:2], ('results', 'pi'))
                self.assertEqual(project.cpu_governors(), {'cpu0': 'performance', 'cpu1': 'powersave'})

    def test_non_pi_evidence_never_routes_to_pi_tree(self):
        with tempfile.TemporaryDirectory() as temp:
            with patch.object(project, 'ROOT', Path(temp)), patch.object(project, 'on_pi', return_value=False):
                result = project.unique_evidence_dir('profile', 'windows')
            self.assertIn(('results', 'laptop'), tuple(zip(result.parts, result.parts[1:])))
            self.assertNotIn('pi', result.parts)

    def test_profile_reports_missing_perf(self):
        with patch.object(project, 'on_pi', return_value=True), patch.object(
                project, 'find_executable', return_value=None):
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                status = project.profile_versions('test20', [project.FROZEN_V0], 1, None, 'pi-test')
        self.assertEqual(status, 2)
        self.assertIn('command -v perf', output.getvalue())

    def test_final_controlled_rejected_off_pi_before_creating_evidence(self):
        with tempfile.TemporaryDirectory() as temp:
            dataset = Path(temp) / 'stress_full.csv'
            dataset.write_text('trade_id,price,qty,quoteQty,time,isBuyerMaker,isBestMatch\n')
            with patch.object(project, 'on_pi', return_value=False), patch.object(
                    project, 'choose_dataset', return_value=dataset), patch.object(
                    project, 'dataset_facts', return_value={'sha256': '0' * 64, 'bytes': dataset.stat().st_size, 'trades': 0}):
                output = io.StringIO()
                with contextlib.redirect_stdout(output):
                    status = project.benchmark_dataset('stress_full', 5, 'workflow-test',
                                                       [project.FROZEN_V0], '3', True)
        self.assertEqual(status, 2)
        self.assertIn('native Linux AArch64 Pi', output.getvalue())

    def test_perf_summary_ipc_and_unsupported_counters(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'perf.txt'
            path.write_text('1000,,cycles,\n2500,,instructions,\n<not supported>,,cache-misses,\n')
            values, notes = project.parse_perf_stat(path)
        self.assertEqual(values['cycles'], 1000)
        self.assertEqual(values['instructions'], 2500)
        self.assertIsNone(values['cache-misses'])
        self.assertIn('cache-misses: <not supported>', notes)
        self.assertEqual(values['instructions'] / values['cycles'], 2.5)

    def test_perf_parser_accepts_real_suffixes_empty_fields_and_ipc_precision(self):
        raw_runs = (
            '21473072346,,cycles:u,9142844394,100.00,,\n'
            '56257739741,,instructions:u,9142844394,100.00,2.62,insn per cycle\n'
            '11554316,,cache-misses:u,9142844394,100.00,,\n',
            '17134892642,,cycles:u,7341961457,100.00,,\n'
            '46623697573,,instructions:u,7341961457,100.00,2.72,insn per cycle\n'
            '8851511,,cache-misses:u,7341961457,100.00,,\n',
        )
        with tempfile.TemporaryDirectory() as temp:
            parsed = []
            for index, raw in enumerate(raw_runs):
                path = Path(temp) / f'perf{index}.txt'
                path.write_text(raw, encoding='utf-8')
                parsed.append(project.parse_perf_stat(path))
        for (values, notes), expected in zip(parsed, (
                {'cycles': 21473072346, 'instructions': 56257739741, 'cache-misses': 11554316},
                {'cycles': 17134892642, 'instructions': 46623697573, 'cache-misses': 8851511})):
            self.assertEqual(values, expected)
            self.assertEqual(notes, [])
            self.assertAlmostEqual(values['instructions'] / values['cycles'],
                                   expected['instructions'] / expected['cycles'], places=14)

    def test_perf_parser_handles_unsupported_permission_and_malformed_rows(self):
        raw = (
            '<not supported>,,cycles:u,0,0.00,,\n'
            '<not counted>,,instructions:u,0,0.00,,\n'
            'Error: No permission to enable cache-misses event.\n'
            'garbage,,instructions:u,100,100.00,,\n'
            '1,234,,cycles:u,100,100.00,,\n'
            'malformed row\n'
        )
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'perf.txt'
            path.write_text(raw, encoding='utf-8')
            values, notes = project.parse_perf_stat(path)
        self.assertEqual(values, {'cycles': None, 'instructions': None, 'cache-misses': None})
        joined = '\n'.join(notes).lower()
        self.assertIn('cycles: <not supported>', joined)
        self.assertIn('instructions: <not counted>', joined)
        self.assertIn('permission', joined)
        self.assertIn('malformed', joined)
        self.assertIn('garbage', joined)
        self.assertIn('check locale/delimiter', joined)

    def test_refresh_perf_summary_reuses_raw_files_and_writes_normalized_values(self):
        with tempfile.TemporaryDirectory() as temp:
            profile_dir = Path(temp) / 'profile-run'
            profile_dir.mkdir()
            (profile_dir / 'profile_manifest.json').write_text(json.dumps({
                'host_class': 'Pi', 'governor_performance': True, 'taskset_used': True,
                'final_controlled': True, 'status': 'CONTROLLED',
            }), encoding='utf-8')
            raw = profile_dir / 'v1-run1.perf.txt'
            raw_content = '21473072346,,cycles:u,1,100.00,,\n56257739741,,instructions:u,1,100.00,,\n'
            raw.write_text(raw_content, encoding='utf-8')
            status = project.refresh_perf_summary(profile_dir)
            summary = (profile_dir / 'perf_summary.csv').read_text(encoding='utf-8')
            markdown = (profile_dir / 'perf_summary.md').read_text(encoding='utf-8')
            self.assertEqual(raw.read_text(encoding='utf-8'), raw_content)
        self.assertEqual(status, 0)
        self.assertIn('21473072346', summary)
        self.assertIn('56257739741', summary)
        self.assertIn('2.619', summary)
        self.assertIn('normalized from the preserved raw', markdown)

    def test_annotate_assembly_references_actual_lines_and_preserves_input(self):
        with tempfile.TemporaryDirectory() as temp:
            run_dir = Path(temp) / 'run'
            asm_dir = run_dir / 'assembly'
            asm_dir.mkdir(parents=True)
            assembly = asm_dir / 'replay_engine.s'
            text = (
                '.text\n.type replay_main, %function\nreplay_main:\n'
                '    stp x29, x30, [sp, -16]!\n'
                '    bl read_trade\n'
                '    ldr x0, [x1]\n'
                '    fadd d0, d0, d1\n'
                '    cbz x0, .Ldone\n'
                '.Ldone:\n    ldp x29, x30, [sp], 16\n    ret\n.size replay_main, .-replay_main\n'
            )
            assembly.write_text(text, encoding='utf-8')
            digest = hashlib.sha256(assembly.read_bytes()).hexdigest()
            manifest = {
                'version_id': 'replay-engine', 'output_file': str(assembly.resolve()),
                'output_sha256': digest, 'compiler_path': 'gcc', 'compiler_version': 'test gcc',
                'flags': ['-O2'], 'host_os': 'Linux', 'architecture': 'aarch64',
            }
            (run_dir / 'assembly_manifest.json').write_text(json.dumps(manifest), encoding='utf-8')
            status = project.annotate_assembly(assembly, 'replay-engine')
            annotation = (run_dir / 'annotation.md').read_text(encoding='utf-8')
            self.assertEqual(hashlib.sha256(assembly.read_bytes()).hexdigest(), digest)
        self.assertEqual(status, 0)
        self.assertIn('Line 4: `stp x29, x30, [sp, -16]!`', annotation)
        self.assertIn('Line 5: `bl read_trade`', annotation)
        self.assertIn('Floating-point work', annotation)
        self.assertIn('Stack setup / restore', annotation)
        self.assertIn('read_trade (actual call line).', annotation)
        self.assertIn('No clock_gettime, nanosleep, or clock_nanosleep call line was observed.', annotation)
        self.assertIn('Code and report annotation checklist', annotation)
        self.assertIn('have not been manually reviewed', annotation)

    def test_source_hash_change_invalidates_and_rebuild_refreshes_metadata(self):
        compiler = project.find_executable(['gcc', 'gcc.exe'])
        self.assertIsNotNone(compiler)
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / 'sample.c'
            executable = root / 'sample.exe'
            spec = {
                'id': 'unit_version', 'profile': 'release', 'source': 'sample.c',
                'input_files': ['sample.c'], 'compile_sources': ['sample.c'],
                'exe': executable, 'flags': ['-std=c11', '-O2'], 'selection': 'unit fixture',
            }
            with patch.object(project, 'ROOT', root):
                source.write_text('int main(void) { return 0; }\n', encoding='utf-8')
                self.assertEqual(project.compile_version_spec(spec, compiler), 0)
                first = json.loads(executable.with_suffix('.exe.json').read_text(encoding='utf-8'))
                self.assertTrue(project.build_metadata_matches(spec, first))
                source.write_text('int main(void) { return 1; }\n', encoding='utf-8')
                self.assertFalse(project.build_metadata_matches(spec, first))
                self.assertEqual(project.compile_version_spec(spec, compiler), 0)
                refreshed = json.loads(executable.with_suffix('.exe.json').read_text(encoding='utf-8'))
                self.assertTrue(project.build_metadata_matches(spec, refreshed))
                self.assertNotEqual(first['source_sha256'], refreshed['source_sha256'])

    def test_clean_build_outputs_have_valid_v0_v1_and_variant_metadata(self):
        specs = project.version_specs()
        self.assertEqual(specs[0]['flags'], project.profile_build_flags('debug'))
        self.assertEqual(specs[1]['flags'], project.profile_build_flags('release'))
        for spec in specs:
            with self.subTest(version=spec['id']):
                self.assertTrue(spec['exe'].is_file(), f"missing {spec['exe']}; run build-all")
                metadata = json.loads(spec['metadata'].read_text(encoding='utf-8'))
                self.assertTrue(project.build_metadata_matches(spec, metadata))
        dataset = project.choose_dataset('test20')
        expected_rows, expected_bars = project.read_test20_oracle()
        for spec in [*specs[:2], specs[-1]]:
            with self.subTest(baseline_verification=spec['id']), contextlib.redirect_stdout(io.StringIO()):
                self.assertTrue(project.verify_version(spec, dataset, expected_rows, expected_bars))
        replay_specs = project.replay_specs()
        self.assertEqual(len(replay_specs), 11)
        soa_spec = next(item for item in specs if item['id'] == 'v10_manual_running_double_soa_O2')
        self.assertIn('storage=exact two-pass malloc Struct-of-Arrays', soa_spec['selection'])
        soa_replay = next(item for item in replay_specs if item['id'] == soa_spec['id'])
        self.assertIn('replay-compatible', soa_replay['selection'])
        self.assertIn('offline SoA arrays are not used', soa_replay['selection'])
        for spec in replay_specs:
            with self.subTest(replay_version=spec['id']):
                self.assertTrue(spec['exe'].is_file(), f"missing {spec['exe']}; run build-all")
                metadata = json.loads(spec['metadata'].read_text(encoding='utf-8'))
                self.assertTrue(project.build_metadata_matches(spec, metadata))
        self.assertTrue((project.BUILD_ROOT / 'tests' / ('replay_support_tests.exe' if project.os.name == 'nt' else 'replay_support_tests')).is_file())

    def test_make_targets_delegate_to_canonical_build_driver(self):
        makefile = (ROOT / 'Makefile').read_text(encoding='utf-8')
        self.assertIn('scripts/project.py build-all', makefile)
        self.assertIn('scripts/project.py verify --all-versions', makefile)
        self.assertIn('scripts/project.py clean-build', makefile)
        self.assertIn('scripts/project.py build-replay', makefile)
        self.assertIn('scripts/project.py build-soa', makefile)

    def test_controlled_aos_soa_report_retains_slower_result_and_memory(self):
        specs_by_id = {item['id']: item for item in project.version_specs()}
        selected = [specs_by_id[name] for name in (
            project.FROZEN_V0,
            'v9_manual_running_double_exact_malloc_trade_array_O2',
            'v10_manual_running_double_soa_O2')]
        values = {
            project.FROZEN_V0: (400, 300, 128, 4096),
            'v9_manual_running_double_exact_malloc_trade_array_O2': (100, 60, 384, 8192),
            'v10_manual_running_double_soa_O2': (125, 75, 224, 7168),
        }
        rows = []
        for version_id, (end_ns, compute_ns, logical_bytes, rss) in values.items():
            rows.append({
                'version_id': version_id, 'end_to_end_ns': end_ns, 'compute_ns': compute_ns,
                'counting_ns': 10, 'loading_ns': 20,
                'process_peak_memory_bytes': rss, 'process_memory_metric': 'ru_maxrss',
                'logical_allocated_bytes': logical_bytes, 'logical_used_bytes': logical_bytes,
                'host_class': 'test', 'governor_performance': True, 'taskset_used': True,
                'final_controlled': True, 'run_status': 'CONTROLLED', 'preliminary_reason': '',
            })
        with tempfile.TemporaryDirectory() as temp, patch.object(project, 'locate_gnuplot', return_value=None):
            report_root = Path(temp) / 'run'
            project.build_benchmark_reports(rows, selected,
                                            {'trades': 10, 'bytes': 100, 'sha256': 'a' * 64},
                                            Path(temp) / 'stress_full.csv',
                                            report_root, 'CONTROLLED', '')
            with (report_root / 'controlled_comparisons.csv').open(newline='', encoding='utf-8') as handle:
                comparisons = list(csv.DictReader(handle))
            with (report_root / 'memory_comparison.csv').open(newline='', encoding='utf-8') as handle:
                memory = list(csv.DictReader(handle))
            controlled_md = (report_root / 'controlled_comparisons.md').read_text(encoding='utf-8')
            speedup_data = (report_root / 'speedup_ladder.dat').read_text(encoding='utf-8')
        soa_pair = next(row for row in comparisons if row['comparison'] == 'storage_exact_malloc_aos_to_soa')
        self.assertAlmostEqual(float(soa_pair['right_speedup_vs_left']), 0.8)
        self.assertEqual(soa_pair['left_logical_allocated_bytes'], '384')
        self.assertEqual(soa_pair['right_logical_allocated_bytes'], '224')
        self.assertEqual(soa_pair['left_median_process_peak_memory_bytes'], '8192')
        self.assertEqual(soa_pair['right_median_process_peak_memory_bytes'], '7168')
        self.assertIn('0.800000x', controlled_md)
        self.assertIn('slower results remain visible', controlled_md)
        self.assertIn('exact malloc SoA', speedup_data)

    def test_assembly_manifest_matches_actual_compiler_output(self):
        status = project.generate_assembly(project.version_specs()[1]['id'], 'workflow-test')
        self.assertEqual(status, 0)
        host_result = 'pi' if project.on_pi() else 'laptop'
        evidence_root = Path(project.ROOT) / 'results' / host_result / 'assembly-runs' / 'workflow-test'
        manifests = sorted(evidence_root.glob('*/assembly_manifest.json'))
        self.assertTrue(manifests)
        manifest = json.loads(manifests[-1].read_text(encoding='utf-8'))
        output = Path(manifest['output_file'])
        self.assertIn('-S', manifest['command'])
        self.assertTrue(output.is_file())
        self.assertGreater(output.stat().st_size, 0)
        self.assertEqual(hashlib.sha256(output.read_bytes()).hexdigest(), manifest['output_sha256'])
        self.assertEqual(manifest['version_id'], project.version_specs()[1]['id'])
        self.assertIn('src/market_baseline.c', manifest['source_hashes'])

    def test_documented_workflow_files_exist(self):
        docs = ROOT / 'docs'
        self.assertTrue((docs / 'codebase_map.md').is_file())
        self.assertTrue((docs / 'performance_methodology.md').is_file())
        self.assertIn('generate_assembly', project.__dict__)


if __name__ == '__main__':
    unittest.main(verbosity=2)
