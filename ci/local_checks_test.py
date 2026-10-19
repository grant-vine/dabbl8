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
    def test_arrangement_stress_uses_only_actual_host_slices(self) -> None:
        selected, unknown = checks.select(['tests/native_arrangement_stress_test.c'])
        self.assertEqual(selected, {'diff', 'arrangement-stress'})
        self.assertEqual(unknown, [])
        steps = checks.recipes(selected, ['clang'], Path('out'), 'HEAD')
        self.assertEqual(len(steps), 11)
        scenarios = ('internal-dry', 'internal-room', 'usb-spring', 'trs-loss-wrap')
        for index, mode in enumerate(('optimized', 'sanitized')):
            compile_step = steps[1 + index * 5]
            self.assertEqual(compile_step['name'], 'arrangement-' + mode + '-compile')
            self.assertIn('tests/fixtures/d8p1/minimal.d8p', compile_step['requires'])
            self.assertIn('tests/native_queue_audio_fixture.h', compile_step['requires'])
            self.assertIn('-DNPART=8', compile_step['argv'])
            self.assertIn('-DDABBL8_TIMING_DIAGNOSTIC=1', compile_step['argv'])
            self.assertEqual('-fsanitize=address,undefined' in compile_step['argv'], mode == 'sanitized')
            for offset, scenario in enumerate(scenarios, 2 + index * 5):
                self.assertEqual(steps[offset]['argv'], ['out/arrangement-' + mode, scenario])
        self.assertFalse(any(arg in ('--capture', '--flash', 'docker', 'tests/run_tests.sh')
                             for step in steps for arg in step['argv']))

    def test_startup_order_stays_local_and_covers_audio_and_reopen(self) -> None:
        selected, unknown = checks.select(['tests/audio_startup_order_test.py',
                                          'tests/native_boot_startup_order_test.py'])
        self.assertEqual(selected, {'diff', 'startup-order'})
        self.assertEqual(unknown, [])
        steps = checks.recipes(selected, ['clang'], Path('out'), 'HEAD')
        self.assertEqual([step['name'] for step in steps[:3]],
                         ['diff', 'startup-audio-order', 'startup-native-reopen'])
        self.assertEqual(len(steps), 19)
        for step in steps[1:3]:
            self.assertEqual(step['argv'][:2], ['env', 'CC=clang'])
        for uart in (0, 1):
            for uac in (0, 1):
                for sie in (0, 1):
                    label = f'startup-timer-uart{uart}-uac{uac}-sie{sie}'
                    compile_step = next(s for s in steps if s['name'] == label + '-compile')
                    self.assertEqual(compile_step['argv'][0], 'clang')
                    self.assertIn(f'-DFELUCCA_UART={uart}', compile_step['argv'])
                    self.assertIn(f'-DFELUCCA_UAC={uac}', compile_step['argv'])
                    self.assertIn(f'-DFELUCCA_SIE_IRQ_BUDGET={sie}', compile_step['argv'])
                    self.assertIn('tests/timer5_cadence_test.c', compile_step['requires'])
                    run_step = next(s for s in steps if s['name'] == label)
                    self.assertEqual(run_step['argv'], [str(Path('out') / label)])
        self.assertIn('firmware/hal/fm1_audio.h', steps[1]['requires'])
        self.assertIn('firmware/src/project_native_flash.c', steps[2]['requires'])
        self.assertFalse(any(arg in ('--capture', '--flash', 'docker', 'tests/run_tests.sh')
                             for step in steps for arg in step['argv']))

    def test_native_midi_is_offline_and_includes_direct_guard_and_snapshot(self) -> None:
        selected, unknown = checks.select(['tools/dabbl8_native_midi_guard.py',
                                          'tools/dabbl8_readonly_snapshot.py'])
        steps = checks.recipes(selected, ['clang'], Path('out'), 'HEAD')
        self.assertEqual(selected, {'diff', 'midi-native'})
        self.assertEqual(unknown, [])
        self.assertEqual([step['name'] for step in steps], ['diff', 'snapshot-fixture-compile',
            'native_midi_guard_test', 'native_midi_qualification_guard_test',
            'raw_coremidi_inventory_test', 'readonly_snapshot_test',
            'readonly_midi_adapter_test', 'instrument_capture_collector_test'])
        self.assertFalse(any(arg in ('--capture', '--virtual-only', 'docker', 'tests/run_tests.sh')
                             for step in steps for arg in step['argv']))

    def test_midi_eight_is_local_and_does_not_repeat_engine_matrix(self) -> None:
        # Given native read-only state and the new eight-channel workload.
        selected, unknown = checks.select(['tests/native_track_recording_query_test.c',
                                          'tools/dabbl8_eight_track_workload.py'])
        # When planning, then only the two native fixtures and fake-I/O schedule run.
        steps = checks.recipes(selected, ['clang'], Path('out'), 'HEAD')
        self.assertEqual(selected, {'diff', 'midi-eight'})
        self.assertEqual(unknown, [])
        self.assertEqual([step['name'] for step in steps], ['diff',
            'recording-query-compile', 'recording-query',
            'eight-channel-render-compile', 'eight-channel-render', 'eight-channel-schedule'])
        self.assertEqual(steps[4]['argv'], ['out/eight-channel-render'])
        self.assertFalse(any(path.endswith('.d8p') for step in steps
                             for path in step.get('requires', [])))
        self.assertFalse(any(arg in ('--capture', '--flash', 'docker', 'tests/run_tests.sh')
                             for step in steps for arg in step['argv']))

    def test_midi_renderer_plan_needs_no_private_demo(self) -> None:
        selected, unknown = checks.select(['tests/native_midi_project_fixture.h'])
        steps = checks.recipes(selected, ['clang'], Path('out'), 'HEAD')
        self.assertEqual(selected, {'diff', 'midi-eight', 'midi-drum'})
        self.assertEqual(unknown, [])
        for name in ('eight-channel-render', 'drum-channel-render'):
            step = next(step for step in steps if step['name'] == name)
            self.assertEqual(step['argv'], ['out/' + name])
        required = {path for step in steps for path in step.get('requires', [])}
        self.assertIn('tests/native_midi_project_fixture.h', required)
        self.assertFalse(any(path.endswith('.d8p') for path in required))
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            for path in required:
                target = repo / path
                target.parent.mkdir(parents=True, exist_ok=True)
                target.touch()
            plan = checks.make_plan(repo, ['tests/native_midi_project_fixture.h'], [],
                                    ['clang'], Path('out'), 'HEAD')
            self.assertEqual(plan['status'], 'PLAN')
            (repo / 'tests/native_midi_project_fixture.h').unlink()
            refused = checks.make_plan(repo, ['tests/native_midi_project_fixture.h'], [],
                                       ['clang'], Path('out'), 'HEAD')
            self.assertEqual(refused['status'], 'REFUSED')
            self.assertIn('tests/native_midi_project_fixture.h', str(refused['refusals']))



    def test_engine_fx_selects_only_source_bound_host_matrix(self) -> None:
        # Given the matrix fixture and the existing local compiler route.
        fixture = 'tests/engine_fx_matrix_test.c'
        selected, unknown = checks.select([fixture])
        # When planning its bounded recipe, without executing the renderer.
        steps = checks.recipes(selected, ['clang'], Path('out'), 'HEAD')
        # Then only the diff, sanitized host compile and matrix run are selected.
        self.assertEqual(selected, {'diff', 'engine-fx'})
        self.assertEqual(unknown, [])
        self.assertEqual([step['name'] for step in steps],
                         ['diff', 'engine-fx-compile', 'engine-fx-matrix'])
        compile_step = steps[1]
        for flag in ('-DNPART=8', '-DFELUCCA_FM4=0', '-DFELUCCA_SLICE=1',
                     '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                     '-fno-sanitize=signed-integer-overflow,shift'):
            self.assertIn(flag, compile_step['argv'])
        self.assertIn(fixture, compile_step['requires'])
        self.assertIn('tests/hostsim.c', compile_step['requires'])
        self.assertEqual(steps[2]['argv'], ['out/engine-fx-matrix'])
        self.assertEqual(self.plan([fixture])['status'], 'REFUSED')
        self.assertFalse(any(arg in ('--capture', '--flash', 'docker', 'tests/run_tests.sh')
                             for step in steps for arg in step['argv']))

    def test_shared_midi_core_selects_bounded_protocol_and_renderers(self) -> None:
        for path in ('tools/dabbl8_midi_workload.py', 'tests/dabbl8_midi_workload_test.py'):
            selected, unknown = checks.select([path])
            self.assertEqual(selected, {'diff', 'midi-workload', 'midi-eight'})
            self.assertEqual(unknown, [])
            steps = checks.recipes(selected, ['clang'], Path('out'), 'HEAD')
            self.assertEqual({step['name'] for step in steps}, {'diff',
                'recording-query-compile', 'recording-query', 'eight-channel-render-compile',
                'eight-channel-render', 'eight-channel-schedule', 'midi-workload-offline'})
            self.assertEqual(steps[-1]['argv'], [checks.sys.executable, 'tests/dabbl8_midi_workload_test.py'])
            self.assertEqual(steps[-1]['requires'], ['tools/dabbl8_midi_workload.py', 'tests/dabbl8_midi_workload_test.py'])

    def test_performance_buses_does_not_repeat_engine_matrix(self) -> None:
        # Given the existing banked performance fixture, when selecting checks.
        fixture = 'tests/eight_track_performance_test.c'
        selected, unknown = checks.select([fixture])
        steps = checks.recipes(selected, ['clang'], Path('out'), 'HEAD')
        # Then only its actual mixer/panel fixture is compiled and run locally.
        self.assertEqual(selected, {'diff', 'performance-buses'})
        self.assertEqual(unknown, [])
        self.assertEqual([step['name'] for step in steps],
                         ['diff', 'performance-buses-compile', 'performance-buses'])
        self.assertIn(fixture, steps[1]['argv'])
        self.assertIn('-fsanitize=address,undefined', steps[1]['argv'])
        self.assertEqual(steps[2]['argv'], ['out/performance-buses'])
        self.assertNotIn('engine-fx', str(steps))

    def test_performance_fx_retains_optimized_cost_and_no_demo_output(self) -> None:
        # Given the upstream performance fixture, when planning its native run.
        selected, unknown = checks.select(['tests/perform_test.c'])
        steps = checks.recipes(selected, ['clang'], Path('out'), 'HEAD')
        # Then cost assertions use the existing optimized recipe without demos.
        self.assertEqual(selected, {'diff', 'performance-fx'})
        self.assertEqual(unknown, [])
        self.assertEqual([step['name'] for step in steps],
                         ['diff', 'performance-fx-compile', 'performance-fx'])
        for flag in ('-O2', '-DNPART=8', '-DFELUCCA_FM4=0', '-DFELUCCA_SLICE=1'):
            self.assertIn(flag, steps[1]['argv'])
        self.assertFalse(any(arg.startswith('-fsanitize=') for arg in steps[1]['argv']))
        self.assertEqual(steps[2]['argv'], ['out/performance-fx'])

    def test_fingerprint_detects_dirty_transitive_and_generated_inputs(self):
        # Given real local Git source, including an untracked transitive header.
        with tempfile.TemporaryDirectory() as folder:
            repo = Path(folder)
            checks.subprocess.run(['git', 'init', '-q', str(repo)], check=True)
            header = repo / 'firmware/shared.h'
            header.parent.mkdir()
            header.write_text('original')
            generated = repo / 'build/gen/table.h'
            generated.parent.mkdir(parents=True)
            generated.write_text('generated')
            before = checks.input_hashes(repo, ['build/gen/table.h'])
            # When source changes without a commit, then its receipt changes.
            header.write_text('edited')
            changed = checks.input_hashes(repo, ['build/gen/table.h'])
            self.assertNotEqual(before, changed)
            generated.write_text('new generated')
            self.assertNotEqual(changed, checks.input_hashes(repo, ['build/gen/table.h']))
            generated.unlink()
            self.assertIn(('build/gen/table.h', 'MISSING'),
                          checks.input_hashes(repo, ['build/gen/table.h']))

    def plan(self, paths, explicit=()):
        return checks.make_plan(Path('/missing-local-inputs'), paths, explicit,
                                ['clang'], Path('build/local-checks/PLANNED'), 'HEAD')

    def test_source_mutation_during_successful_checks_refuses_pass(self):
        with tempfile.TemporaryDirectory() as folder:
            repo = Path(folder)
            checks.subprocess.run(['git', 'init', '-q', str(repo)], check=True)
            checks.subprocess.run(['git', '-c', 'user.name=Fixture', '-c',
                'user.email=fixture@example.invalid', 'commit', '--allow-empty',
                '-qm', 'Fixture'], cwd=repo, check=True)
            source = repo / 'README.md'
            source.write_text('before')

            def mutate(_argv, _repo, _env):
                source.write_text('changed during check')
                return 0, b'', b''

            with patch.object(checks, 'ROOT', repo), patch.object(checks, 'execute', mutate):
                with contextlib.redirect_stdout(io.StringIO()):
                    code = checks.main(['--paths', 'README.md', '--run'])
            report = json.loads(next((repo / 'build/local-checks').glob('run-*/results.json')).read_text())
            self.assertEqual(code, 1)
            self.assertEqual(report['status'], 'FAILED')
            self.assertFalse(report['source_inputs_stable'])
            self.assertEqual(report['changed_during_checks'], ['README.md'])

    def test_docs_only(self):
        selected, unknown = checks.select(['README.md', 'docs/x.json'])
        self.assertEqual(selected, {'diff'})
        self.assertFalse(unknown)
        self.assertEqual(len(self.plan(['README.md'])['commands']), 1)

    def test_resource_paths_select_only_bounded_new_recipes(self):
        # Given each newly owned resource input, when selecting affected checks.
        cases = {'tools/build.py': 'resource-build',
            'tests/resource_diagnostic_build_test.py': 'resource-build',
            'tools/dabbl8_resource_capture.py': 'resource-capture',
            'tools/dabbl8_resource_report.py': 'resource-capture',
            'tests/dabbl8_resource_capture_test.py': 'resource-capture',
            'firmware/src/resource_diagnostic_console.h': 'resource-console',
            'tests/resource_diagnostic_console_test.c': 'resource-console',
            'firmware/src/heavy_pool_diagnostic.h': 'heavy-pool-diagnostic',
            'firmware/hal/fm1_stack_watermark.h': 'stack-watermark'}
        for path, check in cases.items():
            with self.subTest(path=path):
                selected, unknown = checks.select([path])
                # Then no unrelated fixture or target/package workflow is selected.
                self.assertEqual(selected, {'diff', check})
                self.assertEqual(unknown, [])
                steps = checks.recipes(selected, ['clang'], Path('out'), 'HEAD')
                self.assertFalse(any('run_tests.sh' in s['argv'] or 'tools/build.py' in s['argv'] or
                    'docker' in s['argv'] for s in steps))

    def test_resource_capture_reuses_console_and_keeps_strict_sanitizers(self):
        # Given both new console/capture paths and a host compiler selection.
        selected, _ = checks.select(['firmware/src/resource_diagnostic_console.h',
                                    'tools/dabbl8_resource_capture.py'])
        # When the actual recipe planner assembles commands.
        steps = checks.recipes(selected, ['clang'], Path('out'), 'HEAD')
        # Then six profile/mode compiles occur once and interop uses their binary.
        compiles = [s for s in steps if s['name'].endswith('-compile')]
        self.assertEqual(len(compiles), 6)
        self.assertEqual(len({s['name'] for s in steps}), len(steps))
        for step in compiles:
            self.assertIn('-DD8_STACK_WATERMARK_DIAGNOSTIC=1', step['argv'])
            self.assertIn('-DD8_HEAVY_POOL_DIAGNOSTIC=1', step['argv'])
            self.assertIn('-DFELUCCA_UAC=0', step['argv'])
            self.assertFalse(any(arg.startswith('-fno-sanitize=') for arg in step['argv']))
            if '-sanitized-' in step['name']:
                self.assertIn('-fsanitize=address,undefined', step['argv'])
                self.assertIn('-fno-sanitize-recover=all', step['argv'])
        interop = steps[-1]
        self.assertEqual(interop['name'], 'resource-capture-scripted-transport')
        self.assertIn('D8_RESOURCE_CONSOLE_BIN=out/resource-console-0-optimized', interop['argv'])
        self.assertEqual(interop['argv'][-2:], [checks.sys.executable, 'tests/dabbl8_resource_capture_test.py'])

    def test_resource_missing_inputs_refuse_before_output_or_execution(self):
        # Given each new explicit recipe with every declared input available.
        for check in checks.RESOURCE_MAP:
            with self.subTest(check=check), tempfile.TemporaryDirectory() as folder:
                repo = Path(folder)
                steps = checks.recipes({'diff', check}, ['clang'], Path('out'), 'HEAD')
                required = {name for step in steps for name in step.get('requires', [])}
                for name in required:
                    file = repo / name
                    file.parent.mkdir(parents=True, exist_ok=True)
                    file.touch()
                self.assertEqual(checks.make_plan(repo, [], [check], ['clang'], Path('out'), 'HEAD')['status'], 'PLAN')
                # When any directly declared input is removed before a requested run.
                for missing in required:
                    (repo / missing).unlink()
                    with patch.object(checks, 'ROOT', repo), patch.object(checks.subprocess, 'run') as run:
                        with contextlib.redirect_stdout(io.StringIO()):
                            code = checks.main(['--paths', missing, '--check', check, '--run'])
                        # Then the plan refuses without adapters/output creation.
                        self.assertEqual(code, 2)
                        run.assert_not_called()
                        self.assertFalse((repo / 'build/local-checks').exists())
                    (repo / missing).touch()

    def test_resource_stack_route_is_host_only_and_evidence_child_is_fresh(self):
        # Given an explicit stack recipe, when planning its existing runner.
        steps = checks.recipes({'diff', 'stack-watermark'}, ['clang', '-Werror'], Path('out'), 'HEAD')
        # Then target mode is forcibly off and the child output is not precreated.
        argv = steps[-1]['argv']
        self.assertIn('D8_STACK_TARGET=0', argv)
        self.assertIn('CC=clang -Werror', argv)
        self.assertEqual(argv[-1], 'out/stack-watermark')
        self.assertNotIn('--target-plan', argv)

    def test_resource_python_and_stack_routes_refuse_missing_compiler(self):
        # Given their complete declared inputs but an unavailable chosen compiler.
        for check in ('resource-build', 'stack-watermark'):
            with tempfile.TemporaryDirectory() as folder:
                repo = Path(folder)
                for step in checks.recipes({'diff', check}, ['missing-compiler'], Path('out'), 'HEAD'):
                    for name in step.get('requires', []):
                        file = repo / name
                        file.parent.mkdir(parents=True, exist_ok=True)
                        file.touch()
                # When planning, then refusal occurs before execution/output.
                with patch.object(checks.shutil, 'which', return_value=None):
                    plan = checks.make_plan(repo, [], [check], ['missing-compiler'], Path('out'), 'HEAD')
                self.assertEqual(plan['status'], 'REFUSED')
                self.assertIn('Compiler unavailable: missing-compiler', plan['refusals'])

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
            self.assertEqual(len(p['commands']), 13 if name == 'legacy-ui' else 9)
            self.assertEqual(p['status'], 'REFUSED')  # no generated inputs

    def test_voice_allocation_recipe_is_local_and_bounded(self) -> None:
        selected, unknown = checks.select(['firmware/src/voice.c', 'tests/demo_voice_allocation_test.c'])
        self.assertEqual(selected, {'diff', 'voice-allocation'})
        self.assertFalse(unknown)
        plan = self.plan(['firmware/src/voice.c'])
        self.assertEqual(plan['status'], 'REFUSED')
        self.assertEqual(len(plan['commands']), 9)
        encoded = json.dumps(plan['commands'])
        self.assertEqual(encoded.count('-compile"'), 4)
        self.assertIn('tests/demo_voice_allocation_test.c', encoded)
        self.assertEqual(encoded.count('"-DNPART=4"'), 1)
        self.assertEqual(encoded.count('"-DNPART=8"'), 3)
        self.assertEqual(encoded.count('"-fsanitize=address,undefined"'), 4)

    def test_flash_driver_and_first_use_select_bounded_host_checks(self):
        cases = (
            ('firmware/hal/fm1_flash.h', ['diff', 'first-use', 'flash-driver']),
            ('tests/flash_driver_timeout_test.c', ['diff', 'flash-driver']),
            ('tests/native_owner_initialize_test.c', ['diff', 'first-use']),
            ('firmware/src/native_first_use_session.c', ['diff', 'first-use']),
            ('firmware/src/native_first_use_diagnostic.h', ['diff', 'first-use']),
            ('firmware/hal/fm1_flash_trace.h', ['diff', 'first-use', 'flash-driver']),
            ('tests/flash_trace_fixture.h', ['diff', 'first-use', 'flash-driver']),
            ('tests/flash_trace_test.c', ['diff', 'flash-driver']),
            ('tests/flash_trace_spi_watchdog_test.c', ['diff', 'flash-driver']),
            ('tests/flash_trace_reset_test.c', ['diff', 'first-use']),
            ('firmware/src/flash_trace_diagnostic.h', ['diff', 'first-use', 'flash-driver']),
        )
        for path, selected in cases:
            p = self.plan([path])
            self.assertEqual(p['checks'], selected)
            self.assertEqual(p['unmapped_paths'], [])
            compile_steps = [step for step in p['commands'] if step['name'].endswith('-compile')]
            self.assertEqual(len(compile_steps), 2 * sum({'first-use': 2, 'flash-driver': 3}[name]
                                                       for name in selected if name != 'diff'))
            for step in compile_steps:
                self.assertIn('firmware/hal/fm1_flash.h', step['requires'])
                self.assertFalse(any('build.py' in arg or 'run_tests.sh' in arg for arg in step['argv']))
                if step['name'].startswith('first-use-'):
                    self.assertIn('firmware/src/native_first_use_diagnostic.h', step['requires'])
                    for header in checks.GEN:
                        self.assertIn('build/gen/' + header, step['requires'])
            sanitized = [step for step in compile_steps if '-sanitized-' in step['name']]
            for step in sanitized:
                self.assertIn('-fsanitize=address,undefined', step['argv'])
                self.assertIn('-fno-sanitize-recover=all', step['argv'])
                exclusions = '-fno-sanitize=signed-integer-overflow,shift'
                self.assertEqual(exclusions in step['argv'],
                                 step['name'].startswith(('first-use-', 'flash_trace_reset_test-')))
            self.assertEqual(p['status'], 'REFUSED')

    def test_first_use_missing_scaffold_refuses_without_execution(self):
        with tempfile.TemporaryDirectory() as folder:
            repo = Path(folder)
            steps = checks.recipes({'diff', 'first-use'}, ['clang'], Path('out'), 'HEAD')
            for name in {name for step in steps for name in step.get('requires', [])}:
                file = repo / name
                file.parent.mkdir(parents=True, exist_ok=True)
                file.touch()
            with patch.object(checks.shutil, 'which', return_value='/usr/bin/clang'):
                plan = checks.make_plan(repo, ['tests/native_owner_initialize_test.c'], (),
                                        ['clang'], Path('out'), 'HEAD')
                self.assertEqual(plan['status'], 'PLAN')
            for missing in ('tests/instrument_capture_test.c', 'firmware/src/native_first_use_diagnostic.h'):
                (repo / missing).unlink()
                plan = checks.make_plan(repo, ['tests/native_owner_initialize_test.c'], (),
                                        ['clang'], Path('out'), 'HEAD')
                self.assertEqual(plan['status'], 'REFUSED')
                with patch.object(checks, 'ROOT', repo), patch.object(checks.subprocess, 'run') as run:
                    with contextlib.redirect_stdout(io.StringIO()):
                        code = checks.main(['--paths', 'tests/native_owner_initialize_test.c', '--run'])
                    self.assertEqual(code, 2)
                    run.assert_not_called()
                    self.assertFalse((repo / 'build/local-checks').exists())
                (repo / missing).touch()

    def test_usb_controller_selects_only_actual_standalone_fixture(self) -> None:
        for path in ('firmware/src/usb.c', 'tests/usb_sie_wait_test.c'):
            plan = self.plan([path])
            self.assertEqual(plan['checks'], ['diff', 'usb-controller'])
            self.assertEqual(plan['unmapped_paths'], [])
            self.assertEqual([step['name'] for step in plan['commands']],
                             ['diff', 'usb-controller-optimized-compile', 'usb-controller-optimized',
                              'usb-controller-sanitized-compile', 'usb-controller-sanitized'])
            for step in (plan['commands'][1], plan['commands'][3]):
                self.assertIn('tests/usb_sie_wait_test.c', step['argv'])
                self.assertIn('-lm', step['argv'])
                self.assertFalse(any('build/gen/' in name for name in step['requires']))
                self.assertFalse(any('run_tests.sh' in arg or 'build.py' in arg for arg in step['argv']))
            sanitized = plan['commands'][3]['argv']
            self.assertIn('-fsanitize=address,undefined', sanitized)
            self.assertIn('-fno-sanitize-recover=all', sanitized)
            self.assertFalse(any(arg.startswith('-fno-sanitize=') for arg in sanitized))
            self.assertEqual(plan['status'], 'REFUSED')
        selected, unknown = checks.select(['firmware/src/main.c', 'firmware/src/console.c'])
        self.assertEqual(selected, {'diff', 'first-use', 'startup-order'})
        self.assertEqual(unknown, [])

    def test_usb_controller_missing_input_refuses_without_execution(self) -> None:
        with tempfile.TemporaryDirectory() as folder:
            repo = Path(folder)
            required = checks.recipes({'diff', 'usb-controller'}, ['clang'], Path('out'), 'HEAD')[1]['requires']
            for name in required:
                file = repo / name
                file.parent.mkdir(parents=True, exist_ok=True)
                file.touch()
            with patch.object(checks.shutil, 'which', return_value='/usr/bin/clang'):
                plan = checks.make_plan(repo, ['firmware/src/usb.c'], (), ['clang'], Path('out'), 'HEAD')
                self.assertEqual(plan['status'], 'PLAN')
                for missing in required:
                    (repo / missing).unlink()
                    with patch.object(checks, 'ROOT', repo), patch.object(checks.subprocess, 'run') as run:
                        with contextlib.redirect_stdout(io.StringIO()):
                            code = checks.main(['--paths', 'firmware/src/usb.c', '--run'])
                        self.assertEqual(code, 2)
                        run.assert_not_called()
                        self.assertFalse((repo / 'build/local-checks').exists())
                    (repo / missing).touch()

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

    def test_save_consent_selects_actual_emulator_scenarios(self) -> None:
        # Given either disclosure source or its actual emulator regression.
        for path in ('firmware/src/native_local_session.c', 'web/emu/save_consent_test.c'):
            # When planning affected checks without executing anything.
            plan = self.plan([path])
            # Then only the bounded actual emulator scenarios are selected.
            self.assertEqual(plan['checks'], ['diff', 'save-consent'])
            compile_steps = [step for step in plan['commands'] if step['name'].endswith('-compile')]
            self.assertEqual(len(compile_steps), 2)
            runs = [step for step in plan['commands'] if step['name'].startswith('save-consent-')
                    and not step['name'].endswith('-compile')]
            self.assertEqual(len(runs), 12)
            for mode in ('optimized', 'sanitized'):
                binary = 'build/local-checks/PLANNED/save-consent-' + mode
                expected = [[binary, 'build/local-checks/PLANNED']]
                expected += [[binary, 'build/local-checks/PLANNED', str(event)] for event in range(5)]
                self.assertEqual([step['argv'] for step in runs if step['name'].startswith('save-consent-' + mode)], expected)
            for step in compile_steps:
                self.assertIn('web/emu/save_consent_test.c', step['argv'])
                self.assertNotIn('firmware/src/d8p1.c', step['argv'])  # wrapper already includes it
                self.assertIn('-DNPART=8', step['argv'])
                self.assertIn('-ffp-contract=off', step['argv'])
                for header in checks.GEN:
                    self.assertIn('build/gen/' + header, step['requires'])
            self.assertIn('-fsanitize=address,undefined', compile_steps[1]['argv'])
            self.assertIn('-fno-sanitize-recover=all', compile_steps[1]['argv'])
            self.assertIn('-fno-sanitize=signed-integer-overflow,shift', compile_steps[1]['argv'])
            self.assertFalse(any('run_tests.sh' in step['argv'] or 'build.py' in step['argv'] for step in plan['commands']))
            self.assertEqual(plan['status'], 'REFUSED')
        selected, unknown = checks.select(['firmware/src/fx.c'])
        self.assertNotIn('save-consent', selected)
        self.assertFalse(unknown)

    def test_save_consent_missing_inputs_refuse_before_execution(self) -> None:
        # Given all local inputs required by the actual emulator recipe.
        with tempfile.TemporaryDirectory() as folder:
            repo = Path(folder)
            steps = checks.recipes({'diff', 'save-consent'}, ['clang'], Path('out'), 'HEAD')
            required = steps[1]['requires']
            for name in required:
                file = repo / name
                file.parent.mkdir(parents=True, exist_ok=True)
                file.touch()
            self.assertEqual(checks.make_plan(repo, ['web/emu/save_consent_test.c'], (),
                                             ['clang'], Path('out'), 'HEAD')['status'], 'PLAN')
            for missing in required:
                with self.subTest(missing=missing):
                    # When any required input is absent from a requested run.
                    (repo / missing).unlink()
                    with patch.object(checks, 'ROOT', repo), patch.object(checks.subprocess, 'run') as run:
                        with contextlib.redirect_stdout(io.StringIO()):
                            code = checks.main(['--paths', 'web/emu/save_consent_test.c', '--run'])
                        # Then planning refuses before executing or creating output.
                        self.assertEqual(code, 2)
                        run.assert_not_called()
                        self.assertFalse((repo / 'build/local-checks').exists())
                    (repo / missing).touch()

    def test_sample_and_transfer_refuse_missing_pixel_fixture(self) -> None:
        shared = 'tests/native_consent_pixels.h'
        for check in ('sample-upload', 'own-project-transfer'):
            # Given all recipe inputs, including the shared real-pixel fixture.
            with tempfile.TemporaryDirectory() as folder:
                repo = Path(folder)
                steps = checks.recipes({'diff', check}, ['clang'], Path('out'), 'HEAD')
                required = {name for step in steps for name in step.get('requires', [])}
                for name in required | {shared}:
                    file = repo / name
                    file.parent.mkdir(parents=True, exist_ok=True)
                    file.touch()
                self.assertEqual(checks.make_plan(repo, [], [check], ['clang'], Path('out'), 'HEAD')['status'], 'PLAN')
                # When the shared included fixture disappears.
                (repo / shared).unlink()
                plan = checks.make_plan(repo, [], [check], ['clang'], Path('out'), 'HEAD')
                # Then refusal identifies the missing dependency without compiling.
                self.assertEqual(plan['status'], 'REFUSED')
                self.assertTrue(any(shared in refusal for refusal in plan['refusals']))
                with patch.object(checks, 'ROOT', repo), patch.object(checks.subprocess, 'run') as run:
                    with contextlib.redirect_stdout(io.StringIO()):
                        self.assertEqual(checks.main(['--paths', shared, '--check', check, '--run']), 2)
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
        p = self.plan(['firmware/src/unmapped-example.c'])
        self.assertEqual(p['status'], 'REFUSED')
        self.assertEqual(p['unmapped_paths'], ['firmware/src/unmapped-example.c'])

    def test_explicit_selection_retains_gap(self):
        p = self.plan(['firmware/src/unmapped-example.c'], ['docs'])
        self.assertEqual(p['status'], 'PLAN')
        self.assertEqual(p['selection'], 'explicit')
        self.assertEqual(p['unmapped_paths'], ['firmware/src/unmapped-example.c'])
        self.assertIn('not complete coverage', p['scope'])

    def test_paths_cannot_escape(self):
        for path in ['../firmware/src/unmapped-example.c', '/tmp/file.md']:
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
                self.assertEqual(checks.main(['--paths', 'firmware/src/unmapped-example.c', '--run']), 2)
            run.assert_not_called()


if __name__ == '__main__':
    unittest.main()
