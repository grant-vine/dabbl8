# SPDX-License-Identifier: GPL-3.0-only
import contextlib,hashlib,importlib.util,io,json,os,sys,tempfile,unittest
from pathlib import Path
from unittest import mock
SOURCE=Path(os.environ.get('D8_DIAG_SOURCE',str(Path(__file__).resolve().parents[1])))/'tools/build.py'

class BuildIsolationTests(unittest.TestCase):
    def fixture(self,root):
        spec=importlib.util.spec_from_file_location('isolated_build',SOURCE)
        build=importlib.util.module_from_spec(spec);spec.loader.exec_module(build)
        build.SRC=root;build.FW=root/'firmware';build.OUT=root/'build';build.GEN=build.OUT/'gen';build.LDR=build.OUT/'loader'
        return build
    def run_build(self,root,args,check_errors=()):
        b=self.fixture(root)
        observed={}
        def generate():
            b.GEN.mkdir(parents=True,exist_ok=True)
            (b.GEN/'fixture.h').write_bytes(b'generated fixture')
        def loader():
            b.LDR.mkdir(parents=True,exist_ok=True)
            (b.LDR/'ota.bin').write_bytes(b'loader fixture')
            return b'loader fixture'
        def app():
            observed.update(output=b.OUT,gen=b.GEN,loader=b.LDR,version=b.VERSION,dabbl8_version=b.DABBL8_VERSION,product=b.PRODUCT)
            (b.OUT/'felucca.bin').write_bytes(b'app fixture')
            return b'app fixture','','',0
        with mock.patch.object(sys,'argv',['build.py',*args]), mock.patch.dict(os.environ,{'FELUCCA_CDC':'1'}), \
             mock.patch.object(b,'generate',side_effect=generate), mock.patch.object(b,'build_loader',side_effect=loader), \
             mock.patch.object(b,'build_app',side_effect=app), mock.patch.object(b,'check',return_value=(list(check_errors),[])), \
             mock.patch.object(b,'mmio_check',return_value=[]), mock.patch.object(b.fm1pkg_make,'sdk_file',return_value=b'sdk fixture'), \
             mock.patch.object(b.fm1pkg_make,'flash_image',return_value=b'flash fixture'), \
             mock.patch.object(b.fm1pkg_make,'ufw',return_value=b'package fixture'), contextlib.redirect_stdout(io.StringIO()):
            b.main()
        return b,observed
    def test_diagnostics_preserve_normal_products_and_each_previous_run(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory).resolve();ordinary=root/'build'
            for name in ['felucca.bin','felucca.fwsc','dabbl8-0.1.0-dev.fwsc','loader/ota.bin','gen/fixture.h']:
                p=ordinary/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(('existing '+name).encode())
            before={p.relative_to(ordinary).as_posix():p.read_bytes() for p in ordinary.rglob('*') if p.is_file()}
            previous=None
            for i in range(2):
                b,o=self.run_build(root,['--timing-diagnostic','--dabbl8','0.1-dev'])
                self.assertEqual(o['version'],'v0.1.0-timing');self.assertEqual(o['dabbl8_version'],'v0.1.0-timing')
                self.assertEqual(o['gen'],o['output']/'gen');self.assertEqual(o['loader'],o['output']/'loader')
                self.assertEqual(o['output'].parent,ordinary/'timing-diagnostics')
                m=json.loads((o['output']/'diagnostic-manifest.json').read_text())
                self.assertTrue(m['diagnostic']);self.assertFalse(m['hardware_qualified']);self.assertFalse(m['flashing_authorized'])
                self.assertEqual(m['ordinary_package_name'],'dabbl8-0.1.0-dev.fwsc')
                self.assertEqual(m['package_name'],'dabbl8-0.1.0-timing-diagnostic.fwsc')
                self.assertEqual(m['phase'],'build_checks_passed_not_hardware_qualified')
                for name,receipt in m['artifacts'].items():
                    p=o['output']/name;self.assertEqual(receipt['sha256'],hashlib.sha256(p.read_bytes()).hexdigest())
                for name,data in before.items():self.assertEqual((ordinary/name).read_bytes(),data)
                if previous:
                    self.assertNotEqual(previous[0],o['output'])
                    self.assertEqual({p.relative_to(previous[0]).as_posix():p.read_bytes() for p in previous[0].rglob('*') if p.is_file()},previous[1])
                previous=(o['output'],{p.relative_to(o['output']).as_posix():p.read_bytes() for p in o['output'].rglob('*') if p.is_file()})
    def test_four_track_diagnostic_and_default_build_paths(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory).resolve()
            b,o=self.run_build(root,['--timing-diagnostic'])
            self.assertEqual(o['version'],'vTIMING');self.assertIsNone(o['dabbl8_version'])
            self.assertTrue((o['output']/'felucca-timing-diagnostic.fwsc').is_file())
            b,o=self.run_build(root,[])
            self.assertEqual(o['output'],root/'build');self.assertIsNone(o['version'])
            self.assertTrue((root/'build/felucca.fwsc').is_file())
            self.assertFalse((root/'build/diagnostic-manifest.json').exists())
    def test_failed_build_keeps_configured_receipt_without_package(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory).resolve()
            with self.assertRaisesRegex(SystemExit,'checks failed'):
                self.run_build(root,['--timing-diagnostic'],['fixture static-budget refusal'])
            outputs=list((root/'build/timing-diagnostics').iterdir());self.assertEqual(len(outputs),1)
            self.assertEqual(json.loads((outputs[0]/'diagnostic-manifest.json').read_text())['phase'],'configured')
            self.assertFalse(list(outputs[0].glob('*.fwsc')))
    def test_actual_app_flag_selection_uses_isolated_generated_headers(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory).resolve();b=self.fixture(root)
            b.OUT=root/'build/timing-diagnostics/run-fixture';b.GEN=b.OUT/'gen';b.TIMING_DIAGNOSTIC=True
            seen=[]
            def stop(*commands):
                for command in commands:seen.extend(command)
                raise RuntimeError('fixture compiler stop')
            with mock.patch.object(b,'tc_all',side_effect=stop),mock.patch.dict(os.environ,{'FELUCCA_CDC':'1'}):
                with self.assertRaisesRegex(RuntimeError,'fixture compiler stop'):b.build_app()
            self.assertIn('-Ibuild/timing-diagnostics/run-fixture/gen',seen)
            self.assertIn('-DDABBL8_TIMING_DIAGNOSTIC=1',seen)
            self.assertNotIn('-Ibuild/gen',seen)

    def test_actual_sample_generator_auxiliary_paths_preserve_normal_cache(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory).resolve();ordinary=root/'build';ordinary.mkdir()
            (ordinary/'genwav').mkdir();(ordinary/'genwav/existing.wav').write_bytes(b'existing wave')
            (ordinary/'gen_samples.cache').write_bytes(b'existing cache')
            spec=importlib.util.spec_from_file_location('isolated_samples',SOURCE.with_name('gen_samples.py'))
            gen=importlib.util.module_from_spec(spec);spec.loader.exec_module(gen)
            gen.SRC=root;gen.CC0=root/'missing-cc0';gen.GENDIR=ordinary/'genwav';gen.CACHE=ordinary/'gen_samples.cache'
            workspace=ordinary/'timing-diagnostics/run-fixture';workspace.mkdir(parents=True)
            header=workspace/'header.h'
            def waves(args,**kwargs):
                destination=Path(args[-1]);self.assertEqual(destination,workspace/'genwav')
                destination.mkdir();(destination/'fixture.wav').write_bytes(b'generated fixture wave')
                return None
            builder=mock.Mock();builder.header.return_value='fixture header';builder.summary.return_value='fixture summary'
            with mock.patch.object(gen.subprocess,'run',side_effect=waves),mock.patch.object(gen,'Builder',return_value=builder),mock.patch.dict(os.environ,{'FELUCCA_SLICE':'0'}),contextlib.redirect_stdout(io.StringIO()):
                gen.main(header,workspace)
            self.assertEqual((ordinary/'genwav/existing.wav').read_bytes(),b'existing wave')
            self.assertEqual(list((ordinary/'genwav').iterdir()),[ordinary/'genwav/existing.wav'])
            self.assertEqual((ordinary/'gen_samples.cache').read_bytes(),b'existing cache')
            self.assertTrue((workspace/'gen_samples.cache').is_file());self.assertEqual(header.read_text(),'fixture header')
            with self.assertRaises(ValueError):gen.main(header,root.parent/'outside-checkout')

    def test_refusal_before_output_creation(self):
        for args,cdc in [(['--timing-diagnostic','--release','0.1'],'1'),(['--timing-diagnostic'],'0')]:
            with tempfile.TemporaryDirectory() as directory:
                root=Path(directory).resolve();b=self.fixture(root)
                with mock.patch.object(sys,'argv',['build.py',*args]),mock.patch.dict(os.environ,{'FELUCCA_CDC':cdc}),contextlib.redirect_stderr(io.StringIO()):
                    with self.assertRaises(SystemExit) as result:b.main()
                self.assertEqual(result.exception.code,2);self.assertFalse((root/'build').exists())

if __name__=='__main__':unittest.main()
