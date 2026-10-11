#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Dabbl8 contributors
"""Actual CLI early refusal and app-only orchestration; no vendor compiler required."""
import hashlib
import importlib.util
import json
import os
import shutil
from pathlib import Path
import subprocess
import sys
import tempfile
from unittest.mock import patch

SRC = Path(__file__).resolve().parents[1]
def module():
    spec = importlib.util.spec_from_file_location('app_build', SRC / 'tools/build.py')
    b = importlib.util.module_from_spec(spec); spec.loader.exec_module(b); return b

def main():
    checks = 0
    b = module()
    for malformed in ('', '01c7a000 *ABS* 00000000 _ustack_lo\n01c74100 *ABS* 00000000 _ustack_top'):
        try: b.app_stack_windows(malformed)
        except SystemExit: pass
        else: raise AssertionError('missing/reversed stack bounds must refuse')
        checks += 1
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        cli = root/'cli'; shutil.copytree(SRC/'tools', cli/'tools', ignore=shutil.ignore_patterns('__pycache__'))
        (cli/'build').mkdir(); (cli/'build/sentinel').write_bytes(b'CLI preserve')
        sentinel = root / 'build/sentinel'; sentinel.parent.mkdir(); sentinel.write_bytes(b'preserve')
        clean = {k:v for k,v in os.environ.items() if not k.startswith(('FELUCCA_', 'DABBL8_')) and k not in ('BUDGET_UPDATE','GOLDEN_UPDATE')}
        clean['JIELI_TOOLCHAIN'] = '/deliberately/missing/toolchain'
        cases = [(['--app-only'], {}), (['--app-only','--dabbl8','0.1','--release','0.1'], {}),
                 (['--app-only','--dabbl8','0.1','--timing-diagnostic'], {'FELUCCA_CDC':'0'}),
                 (['--dabbl8','0.1','--engine-profile','core'], {}),
                 (['--dabbl8','0.1','--engine-profile','breaks','--timing-diagnostic'], {}),
                 (['--app-only','--dabbl8','0.1'], {'DABBL8_ENGINE_PROFILE':'bad'}),
                 (['--app-only','--dabbl8','0.1','--engine-profile','core'], {'FELUCCA_FM4':'1'}),
                 (['--app-only','--dabbl8','0.1','--engine-profile','breaks'], {'FELUCCA_SLICE':'0'}),
                 (['--app-only','--dabbl8','bad'], {}),
                 (['--app-only','--dabbl8','0.1','--engine-profile','core'], {'DABBL8_ENGINE_PROFILE':'breaks'}),
                 (['--app-only','--dabbl8','0.1'], {'FELUCCA_USB_LAYOUT':'9'}),
                 (['--app-only','--dabbl8','0.1'], {'FELUCCA_SIZE':'garbage'}),
                 (['--app-only','--dabbl8','0.1'], {'BUDGET_UPDATE':'1'})]
        # Real CLI can refuse before any dependencies or output; snapshot its own build directory too.
        before = {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (cli/'build').rglob('*') if p.is_file()}
        expected_errors = [b'--app-only requires', b'--app-only requires', b'--timing-diagnostic needs',
                           b'Curated engine profile packaging is disabled', b'Curated engine profile packaging is disabled',
                           b'Unsupported DABBL8_ENGINE_PROFILE', b'Curated profiles require FELUCCA_FM4',
                           b'Curated profiles require FELUCCA_FM4', b'Dabbl8 version:',
                           b'conflicts with DABBL8_ENGINE_PROFILE', b'FELUCCA_USB_LAYOUT must', b'FELUCCA_SIZE must',
                           b'cannot update budgets or goldens']
        before_directories = {str(p) for p in (cli/'build').rglob('*') if p.is_dir()}
        for (args, extra), expected in zip(cases, expected_errors):
            q = subprocess.run([sys.executable, cli/'tools/build.py', *args], env={**clean, **extra}, capture_output=True)
            assert q.returncode and expected in q.stderr, (args,q.stderr)
            checks += 1
        q = subprocess.run([sys.executable, cli/'tools/build.py', '--app-only', '--dabbl8', '0.1'], env=clean, capture_output=True)
        assert q.returncode and b'JIELI_TOOLCHAIN must' in q.stderr; checks += 1
        after = {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (cli/'build').rglob('*') if p.is_file()}
        assert before == after and before_directories == {str(p) for p in (cli/'build').rglob('*') if p.is_dir()}; checks += 1
        for profile, fault, diagnostic in [('full', None, False), ('core', None, False), ('breaks', None, False), ('full', None, True), ('core', None, True), ('breaks', None, True), ('core', 'generation', True), ('core', 'source_mutation', True)]:
            b = module(); b.SRC = root; b.FW=root/'firmware'; b.OUT=root/'build'; b.GEN=b.OUT/'gen'; b.LDR=b.OUT/'loader'
            tool = root/'tool/pi32v2/bin'; tool.mkdir(parents=True,exist_ok=True); (tool/'clang').write_bytes(b'compiler fixture')
            def gen():
                b.GEN.mkdir(parents=True); (b.GEN/'input.h').write_text('generated'); assert b.APP_ONLY
                if fault == 'generation': raise SystemExit('forced generator failure')
                if fault == 'source_mutation':
                    (root/'tools').mkdir(exist_ok=True); (root/'tools/mutant.py').write_text('unexpected source mutation')
            def app():
                assert 'app-only' in b.OUT.parts and b.OUT.parent.name == profile
                for name in ('felucca.bin','felucca.elf','felucca.dis'): (b.OUT/name).write_bytes(b'app')
                return b'app','\n'.join(f'{value:08x}         *ABS* 00000000 {name}' for name,value in [('_ustack_lo',0x01c74100),('_ustack_top',0x01c7a000),('_sstack_lo',0x01c7a100),('_sstack_top',0x01c7c000)]),'dis','ramtext'
            with patch.dict(os.environ,{**clean,'DABBL8_ENGINE_PROFILE':profile,'JIELI_DOCKER':'0'},clear=True), \
                 patch.object(sys,'argv',['build.py','--app-only','--dabbl8','0.1']+(['--timing-diagnostic'] if diagnostic else [])), \
                 patch.object(b,'toolchain',return_value=tool.parents[1]), patch.object(b,'tc',return_value='compiler version'), \
                 patch.object(b,'generate',side_effect=gen),patch.object(b,'build_app',side_effect=app), \
                 patch.object(b,'build_loader',side_effect=AssertionError('loader must not run')), \
                 patch.object(b.fm1pkg_make,'sdk_file',side_effect=AssertionError('SDK must not be read')), \
                 patch.object(b.fm1pkg_make,'ufw',side_effect=AssertionError('package must not run')), \
                 patch.object(b,'check',return_value=([],['memory fixture'])),patch.object(b,'mmio_check',return_value=[]):
                if fault:
                    try: b.main()
                    except SystemExit: pass
                    else: raise AssertionError('failure must propagate')
                else: assert b.main()==0
            manifest=json.loads((b.OUT/'app-only-manifest.json').read_text()); assert manifest['installable'] is False and manifest['package_built'] is False and manifest['loader_built'] is False and manifest['sdk_used'] is False
            assert manifest['phase']==('failed' if fault else 'application_built_not_hardware_qualified')
            assert manifest['timing_diagnostic'] is diagnostic and manifest['timing_report_schema']==('D8T2' if diagnostic else None)
            assert manifest['version']==('v0.1.0-timing' if diagnostic else 'v0.1.0') and manifest['stack_qualified'] is False
            assert ('-DDABBL8_TIMING_DIAGNOSTIC=1' in manifest['compiler_flags']) is diagnostic
            if not fault:
                assert manifest['source_unchanged']
                assert manifest['stack_windows']['user']['bytes']==0x5f00 and manifest['stack_windows']['supervisor']['bytes']==0x1f00
            if fault == 'source_mutation': assert manifest['source_unchanged'] is False
            if fault == 'generation':
                assert manifest['failure']=='forced generator failure'; checks += 3; continue
            assert manifest['static_budget']['passed'] is False and manifest['qualification_passed'] is False
            assert not list(b.OUT.rglob('*.fwsc')) and not b.LDR.exists()
            for name,value in manifest['artifacts'].items(): assert hashlib.sha256((b.OUT/name).read_bytes()).hexdigest()==value['sha256']
            checks += 5
        assert sentinel.read_bytes()==b'preserve'; checks += 1
    print(f'app-only CLI/orchestration {checks} checks PASS; mocked positive tool execution, actual early CLI refusals')
    return 0
if __name__ == '__main__': sys.exit(main())
