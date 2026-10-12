"""Rival execution, compact P&L, evidence validation, and race tests."""
#!/usr/bin/env python3
import contextlib
import io
import subprocess
import sys
import tempfile
import tracemalloc
import unittest
from unittest import mock
from decimal import Decimal
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
import project
from rival import (REACTION_SWEEP_US, Signal, compact_simulate, read_replay_signals,
                   simulate, stream_replay_pair)


class RivalSimulationTests(unittest.TestCase):
    def test_faster_equal_and_slower_reaction_price_rules(self):
        signals = [Signal(1, 1, 10.0, 0.9), Signal(3, -1, 12.0, 1.0),
                   Signal(5, 1, 11.0, 1.1), Signal(7, -1, 9.0, 1.1)]
        prices = [10.0, 10.5, 12.0, 11.5, 11.0, 10.0, 9.0, 8.0]
        result = simulate(signals, prices, 1.0)
        self.assertEqual([row['decision'] for row in result.decisions], [
            'filled_at_trigger_price', 'filled_at_next_trade_price',
            'filled_at_next_trade_price', 'filled_at_next_trade_price'])
        self.assertEqual(result.trigger_price_count, 1)
        self.assertEqual(result.next_price_count, 3)
        self.assertEqual(result.pnl, Decimal('-0.5'))

    def test_position_transitions_duplicates_and_open_position(self):
        signals = [Signal(1, -1, 9, 0), Signal(2, 1, 10, 0),
                   Signal(3, 1, 8, 0), Signal(4, -1, 11, 0),
                   Signal(5, -1, 12, 0), Signal(6, 1, 7, 0)]
        prices = [9, 10, 8, 11, 12, 7, 8]
        result = simulate(signals, prices, 1)
        self.assertEqual(result.ignored_duplicate_count, 3)
        self.assertEqual(result.buy_count, 2)
        self.assertEqual(result.sell_count, 1)
        self.assertEqual(result.holding, 1)
        self.assertEqual(result.open_position_price, 7)
        self.assertEqual(result.pnl, -6)

    def test_final_signal_unfilled_and_no_signals_are_valid(self):
        final = simulate([Signal(2, 1, 11, 0.1)], [10, 11], 1)
        self.assertEqual(final.unfilled_orders, 1)
        self.assertEqual(final.pnl, 0)
        self.assertEqual(final.holding, 0)
        empty = simulate([], [10], 1)
        self.assertEqual(empty.pnl, 0)
        self.assertEqual(empty.decisions, [])

    def test_sweep_has_documented_endpoints_and_one_microsecond(self):
        self.assertEqual(REACTION_SWEEP_US[0], 0.1)
        self.assertIn(1.0, REACTION_SWEEP_US)
        self.assertEqual(REACTION_SWEEP_US[-1], 100.0)
        rows = project.reaction_sweep([], [10])
        self.assertEqual([row['rival_reaction_us'] for row in rows], list(REACTION_SWEEP_US))
        self.assertTrue(all(row['pnl'] == 0 for row in rows))

    def test_money_lost_is_fast_minus_baseline(self):
        base = simulate([Signal(1, 1, 10, 2)], [10, 9], 1)
        fast = simulate([Signal(1, 1, 10, 0.5)], [10, 9], 1)
        self.assertEqual(fast.pnl - base.pnl, -1)
        base = simulate([Signal(1, 1, 10, 2), Signal(2, -1, 8, 2)], [10, 8, 7], 1)
        fast = simulate([Signal(1, 1, 10, 0.5), Signal(2, -1, 8, 2)], [10, 8, 7], 1)
        self.assertEqual(fast.pnl - base.pnl, -2)


class RivalCommandTests(unittest.TestCase):
    def test_streaming_compact_analysis_matches_legacy_small_fixture(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            dataset = root / 'prices.csv'
            dataset.write_text('1,10.00\n2,11.00\n3,12.00\n4,13.00\n', encoding='utf-8')
            header = 'trade,timestamp_us,primary_short_sma,primary_long_sma,primary_decision,shadow_short_sma,shadow_long_sma,shadow_decision,effective_decision,policy\n'
            decisions = header + ''.join(f'{i},0,0,0,0,0,0,0,{side},primary\n'
                                         for i, side in enumerate((1, 0, -1, 0), 1))
            latency = 'trade,scheduled_release_ns,processing_start_ns,decision_ns,processing_duration_ns,release_to_decision_latency_ns\n' + ''.join(
                f'{i},0,0,0,{duration},0\n' for i, duration in enumerate((500, 0, 1500, 0), 1))
            bdec, fdec = root / 'bdec.csv', root / 'fdec.csv'
            blat, flat = root / 'blat.csv', root / 'flat.csv'
            for path, content in ((bdec, decisions), (fdec, decisions),
                                  (blat, latency), (flat, latency.replace('500', '250'))):
                path.write_text(content, encoding='utf-8')
            records, _, ignored = stream_replay_pair(dataset, bdec, blat, fdec, flat,
                                                      root / 'progress.log', progress_interval=2)
            old_signals, prices, _ = read_replay_signals(dataset, bdec, blat)
            old_result = simulate(old_signals, prices, 1.0)
            compact_result = compact_simulate(records, 'baseline', 1.0, ignored)
            self.assertEqual(compact_result['pnl'], old_result.pnl)
            self.assertEqual(compact_result['buy_count'], old_result.buy_count)
            self.assertEqual(compact_result['sell_count'], old_result.sell_count)
            self.assertEqual(compact_result['next_price_count'], old_result.next_price_count)
            self.assertEqual(compact_result['ignored_duplicate_count'], old_result.ignored_duplicate_count)

    def test_streaming_compact_memory_does_not_scale_with_all_rows(self):
        row_count = 100_000
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            dataset = root / 'prices.csv'
            bdec, fdec = root / 'bdec.csv', root / 'fdec.csv'
            blat, flat = root / 'blat.csv', root / 'flat.csv'
            with dataset.open('w', encoding='utf-8') as handle:
                for _ in range(row_count):
                    handle.write('1,10.00\n')
            dhead = 'trade,timestamp_us,effective_decision\n'
            lhead = 'trade,processing_duration_ns\n'
            with bdec.open('w', encoding='utf-8') as bd, fdec.open('w', encoding='utf-8') as fd, \
                 blat.open('w', encoding='utf-8') as bl, flat.open('w', encoding='utf-8') as fl:
                bd.write(dhead); fd.write(dhead); bl.write(lhead); fl.write(lhead)
                for ordinal in range(1, row_count + 1):
                    bd.write(f'{ordinal},0,0\n'); fd.write(f'{ordinal},0,0\n')
                    bl.write(f'{ordinal},0,0\n'); fl.write(f'{ordinal},0,0\n')
            input_bytes = sum(path.stat().st_size for path in (dataset, bdec, fdec, blat, flat))
            tracemalloc.start()
            try:
                with contextlib.redirect_stderr(io.StringIO()):
                    records, processed, ignored = stream_replay_pair(
                        dataset, bdec, blat, fdec, flat, root / 'progress.log',
                        progress_interval=25_000)
                _, peak = tracemalloc.get_traced_memory()
            finally:
                tracemalloc.stop()
            self.assertEqual(processed, row_count)
            self.assertEqual(records, [])
            self.assertEqual(ignored, 0)
            self.assertLess(peak, min(8 * 1024 * 1024, input_bytes // 2))

    def test_incomplete_compact_output_is_not_valid(self):
        with tempfile.TemporaryDirectory() as temp:
            run_dir = Path(temp)
            (run_dir / 'run_manifest.json').write_text('{"status":"in_progress"}', encoding='utf-8')
            self.assertFalse(project.rival_run_is_complete(run_dir))
            (run_dir / 'run_manifest.json').write_text('{"status":"complete"}', encoding='utf-8')
            for name in ('baseline_summary.json', 'fast_summary.json', 'rival_decisions.csv',
                         'pnl_summary.json', 'reaction_sweep.csv', 'reaction_sweep.svg',
                         'compact_progress.log'):
                (run_dir / name).write_text('ok', encoding='utf-8')
            self.assertTrue(project.rival_run_is_complete(run_dir))

    def test_baseline_and_fast_replay_signal_sequence_matches(self):
        self.assertTrue((ROOT / 'build/replay/v0_baseline_strtok_running_double_heap_O0.exe').exists(),
                        'Run `python3 scripts/project.py build --profile release` first')
        with tempfile.TemporaryDirectory() as temp:
            sequences = []
            for label, version in (('base', 'v0_baseline_strtok_running_double_heap_O0'),
                                   ('fast', 'v5_manual_digit_parser_running_double_stream_O2')):
                exe = ROOT / 'build' / 'replay' / f'{version}.exe'
                decision = Path(temp) / f'{label}.csv'
                latency = Path(temp) / f'{label}_latency.csv'
                command = [str(exe), str(ROOT / 'test20 - Copy (6).csv'), '--version-id', version,
                           '--speed', '1000', '--limit', '20', '--output', 'off',
                           '--decision-csv', str(decision), '--latency-csv', str(latency)]
                result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, check=False)
                self.assertEqual(result.returncode, 0, result.stderr)
                signals, _, sequence = read_replay_signals(ROOT / 'test20 - Copy (6).csv', decision, latency)
                sequences.append(sequence)
                self.assertEqual([(s.trade, s.side) for s in signals], [(7, 1), (10, -1)])
            self.assertEqual(sequences[0], sequences[1])

    def test_rival_command_writes_summary_decisions_sweep_and_graph(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / 'rival-run'
            args = SimpleNamespace(
                dataset='test20', baseline_version='v0_baseline_strtok_running_double_heap_O0',
                fast_version='v5_manual_digit_parser_running_double_stream_O2', speed=1000,
                short=None, long=None, duration_seconds=None, limit=20,
                signal_policy='primary', rival_reaction_us=1.0, sweep=True,
                compact_evidence=True,
                output_dir=str(output))
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(project.run_rival_command(args), 0)
            self.assertTrue((output / 'run_manifest.json').is_file())
            self.assertTrue((output / 'rival_decisions.csv').is_file())
            self.assertTrue((output / 'pnl_summary.json').is_file())
            self.assertTrue((output / 'reaction_sweep.csv').is_file())
            self.assertTrue((output / 'reaction_sweep.svg').is_file())
            self.assertTrue(project.rival_run_is_complete(output))

    def test_race_command_runs_both_versions_and_writes_live_log(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / 'race-run'
            args = SimpleNamespace(
                dataset='test20', baseline_version='v0_baseline_strtok_running_double_heap_O0',
                fast_version='v5_manual_digit_parser_running_double_stream_O2', speed=1000,
                short=None, long=None, duration_seconds=None, limit=20,
                signal_policy='primary', rival_reaction_us=1.0,
                hardware_led=False, log_only=True, output_dir=str(output))
            result_output = io.StringIO()
            with contextlib.redirect_stdout(result_output):
                result = project.run_race_command(args)
            self.assertEqual(result, 0, result_output.getvalue())
            self.assertIn('Race completed', result_output.getvalue())
            self.assertTrue((output / 'race_live.csv').is_file())
            self.assertTrue((output / 'race_summary.json').is_file())
            self.assertTrue((output / 'run_manifest.json').is_file())

    def test_race_cli_rejects_invalid_speed(self):
        result = subprocess.run([sys.executable, str(ROOT / 'scripts/project.py'), 'race', '--speed', '50'],
                                cwd=ROOT, capture_output=True, text=True, check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('invalid choice', result.stderr.lower())

    def test_fast_default_is_selected_from_eligible_benchmark_evidence(self):
        old_root = project.ROOT
        try:
            with tempfile.TemporaryDirectory() as temp:
                project.ROOT = Path(temp)
                self.assertTrue((project.PROJECT_SOURCE_ROOT / 'config' / 'platform_flags.mk').is_file())
                expected_flags = ['-D_POSIX_C_SOURCE=200809L'] if sys.platform.startswith('linux') else []
                self.assertEqual(project.platform_build_flags(), expected_flags)
                evidence = project.ROOT / 'results' / 'pi' / 'benchmark_medians.csv'
                evidence.parent.mkdir(parents=True)
                evidence.write_text(
                    'version_id,median_end_to_end_ns,status\n'
                    'v0_baseline_strtok_running_double_heap_O0,100,ELIGIBLE\n'
                    'v1_baseline_strtok_running_double_heap_O2,50,ELIGIBLE\n'
                    'v5_manual_digit_parser_running_double_stream_O2,25,FAIL\n',
                    encoding='utf-8')
                selected, source = project.select_fast_version_from_pi_evidence()
                self.assertEqual(selected, 'v1_baseline_strtok_running_double_heap_O2')
                self.assertEqual(Path(source), evidence)
        finally:
            project.ROOT = old_root

    def test_fast_default_without_evidence_or_with_invalid_evidence(self):
        old_root = project.ROOT
        try:
            with tempfile.TemporaryDirectory() as temp:
                project.ROOT = Path(temp)
                selected, message = project.select_fast_version_from_pi_evidence()
                self.assertIsNone(selected)
                self.assertIn('No eligible', message)

                evidence = project.ROOT / 'results' / 'pi' / 'benchmark_medians.csv'
                evidence.parent.mkdir(parents=True)
                evidence.write_text(
                    'version_id,median_end_to_end_ns,status\n'
                    'unknown_version,1,ELIGIBLE\n'
                    'v0_baseline_strtok_running_double_heap_O0,0,ELIGIBLE\n'
                    'v1_baseline_strtok_running_double_heap_O2,2,FAIL\n',
                    encoding='utf-8')
                selected, message = project.select_fast_version_from_pi_evidence()
                self.assertIsNone(selected)
                self.assertIn('No eligible', message)
        finally:
            project.ROOT = old_root

    def test_explicit_fast_version_bypasses_evidence_selection(self):
        args = SimpleNamespace(dataset='test20', fast_version='explicit_version',
                               baseline_version='baseline_version')
        with mock.patch.object(project, 'choose_dataset', return_value=ROOT / 'test20 - Copy (6).csv'), \
             mock.patch.object(project, 'select_fast_version_from_pi_evidence',
                               side_effect=AssertionError('evidence selection should be bypassed')), \
             mock.patch.object(project, 'replay_metadata_for',
                               return_value=(None, None, 'explicit version validation reached')):
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                result = project.run_rival_command(args)
        self.assertEqual(result, 3)
        self.assertIn('explicit version validation reached', output.getvalue())

    def test_missing_race_executable_never_reports_completion(self):
        old_build_root = project.BUILD_ROOT
        try:
            with tempfile.TemporaryDirectory() as temp:
                project.BUILD_ROOT = Path(temp)
                args = SimpleNamespace(
                    dataset='test20', fast_version='v5_manual_digit_parser_running_double_stream_O2',
                    baseline_version='v0_baseline_strtok_running_double_heap_O0',
                    hardware_led=False, speed=1000, output_dir=None)
                output = io.StringIO()
                with contextlib.redirect_stdout(output):
                    result = project.run_race_command(args)
                self.assertNotEqual(result, 0)
                self.assertNotIn('Race completed', output.getvalue())
                self.assertIn('missing', output.getvalue().lower())
        finally:
            project.BUILD_ROOT = old_build_root


if __name__ == '__main__':
    unittest.main(verbosity=2)
