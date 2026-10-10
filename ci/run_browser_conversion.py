#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Pinned actual C/WASM browser-converter parity; no device or release writes."""
from pathlib import Path
import hashlib,json,os,platform,shutil,subprocess,sys,tarfile,tempfile
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/browser-conversion-evidence';OUT.mkdir(parents=True,exist_ok=True)
LOCK=json.loads((ROOT/'ci/browser-conversion.json').read_text())
report={'result':'FAIL','lock':LOCK,'checks':[],'hardware':'SKIP','device_writes':False,'firmware_package':False}
def command(name,args,env=None):
    with (OUT/(name+'.log')).open('w') as log:
        p=subprocess.run(args,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT)
    report['checks'].append({'name':name,'exit_code':p.returncode,'log':name+'.log'})
    if p.returncode:raise RuntimeError(name+' failed; see preserved log')
try:
    if platform.system()!='Linux' or platform.machine() not in ('x86_64','AMD64'):raise RuntimeError('This CI entry point requires Linux x86_64; Mac uses the isolated recorded SDK directly.')
    if not (ROOT/'build/gen/felucca_tables.h').is_file():raise RuntimeError('Baseline generated tables unavailable; browser checks cannot run.')
    deps=ROOT/'.local-baseline/ci/browser-deps';deps.mkdir(parents=True,exist_ok=True)
    archive=deps/'emscripten-4.0.17.tar.xz'
    if not archive.exists():command('download',['curl','-fL','--retry','3',LOCK['linux_archive_url'],'-o',str(archive)])
    digest=hashlib.sha256(archive.read_bytes()).hexdigest();report['archive']={'bytes':archive.stat().st_size,'sha256':digest}
    if digest!=LOCK['linux_archive_sha256'] or archive.stat().st_size!=LOCK['linux_archive_bytes']:raise RuntimeError('Pinned browser compiler archive mismatch; refusing upgrade.')
    sdk=deps/'sdk'
    if not sdk.exists():
        staging=Path(tempfile.mkdtemp(prefix='sdk-extract-',dir=deps))
        try:
            with tarfile.open(archive) as source:source.extractall(staging,filter='data')
            staging.rename(sdk)
        except Exception:
            shutil.rmtree(staging);raise
    install=sdk/'install'
    # Match the reviewed official emsdk release installer: the immutable
    # build archive contains a -git label; the SDK writes its release label.
    version_file=install/'emscripten/emscripten-version.txt'
    archived_version=version_file.read_text().strip().strip(chr(34))
    if archived_version not in ('4.0.17-git','4.0.17'):raise RuntimeError('Unexpected archive compiler version metadata.')
    version_file.write_text(chr(34)+LOCK['emscripten_version']+chr(34)+'\n')
    report['sdk_release_label']={'archive_label':archived_version,'installed_label':LOCK['emscripten_version'],'method':'Same release metadata normalization as official pinned emsdk; compiler source/binaries unchanged'}
    node=shutil.which('node')
    if not node:raise RuntimeError('Pinned workflow Node is unavailable.')
    config=deps/'.emscripten'
    config.write_text('LLVM_ROOT = '+repr(str(install/'bin'))+'\nBINARYEN_ROOT = '+repr(str(install))+'\nNODE_JS = '+repr(node)+'\n')
    env=os.environ.copy();env.update(EM_CONFIG=str(config),EMCC=str(install/'emscripten/emcc'))
    report['compiler_version']=subprocess.check_output([env['EMCC'],'--version'],cwd=ROOT,env=env,text=True)
    report['node_version']=subprocess.check_output([node,'--version'],text=True).strip()
    command('wasm-build',['sh','web/build-d8converter.sh'],env)
    native=ROOT/'build/host/browser_native_converter';native.parent.mkdir(parents=True,exist_ok=True)
    command('native-build',[os.environ.get('CC','cc'),'-O1','-w','-Ibuild/gen','-Ifirmware/src','firmware/src/d8p1.c','tools/dabbl8_project_convert.c','-lm','-o',str(native)])
    command('browser-parity',[node,'tests/browser_project_conversion_test.mjs','build/d8converter/dabbl8_project_convert.mjs',str(native)],env)
    report['source']=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    report['source_hashes']={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ['firmware/src/d8p1.c','firmware/src/d8p1_project.h','tools/dabbl8_project_convert.c','web/build-d8converter.sh','web/d8project-conversion.js','web/d8project-conversion-page.js','tests/browser_project_conversion_test.mjs']}
    report['generated_artifacts']={p.name:{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in (ROOT/'build/d8converter').iterdir() if p.is_file()}
    report['result']='PASS'
except Exception as error:report['reason']=str(error)
finally:
    report['log_hashes']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in OUT.glob('*.log')}
    (OUT/'results.json').write_text(json.dumps(report,indent=2)+'\n')
print('Browser converter:',report['result'],report.get('reason','actual C/WASM parity and independent ZIP checks'))
sys.exit(0 if report['result']=='PASS' else 1)
