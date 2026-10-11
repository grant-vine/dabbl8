from pathlib import Path
import contextlib,hashlib,importlib.util,json,os,shutil,subprocess,sys,tempfile
root=Path('/private/tmp/d8-timing-combined-target');src=root/'target-source'
src.mkdir(exist_ok=False)
original=Path('/Users/grantv/Code/dabbl8/.local-baseline/timing-diagnostic')
for name in ['firmware','tools','tests']:shutil.copytree(original/name,src/name)
shutil.copytree(original/'build/gen',src/'build/gen')
deps=Path('/Users/grantv/Code/dabbl8/.local-baseline/deps')
sys.path.insert(0,'/Users/grantv/Code/dabbl8/.local-baseline/dependency-integrity/ci')
from dependency_integrity import verify_archive_tree,verify_sdk,SDK_BUILD_INPUTS
image='debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587'
tc=deps/'jieli-linux-toolchains-20250324.1'
env={'toolchain':verify_archive_tree(deps/'linux-toolchain.tar.xz',tc,tc.name,'f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958'),'sdk':verify_sdk(deps/'ac79-sdk','d179b4484759423312073f5fbb232501aa491047',SDK_BUILD_INPUTS),'image':image,'python':sys.version}
env['container']=json.loads(subprocess.check_output(['docker','image','inspect',image],text=True))[0]
assert env['container']['Architecture']=='amd64' and env['container']['Os']=='linux'
env['compiler_version']=subprocess.check_output(['docker','run','--rm','--platform','linux/amd64','-v',str(tc)+':/opt/jieli:ro',image,'/opt/jieli/pi32v2/bin/clang','--version'],text=True)
(root/'target-environment.json').write_text(json.dumps(env,indent=2)+'\n')
for key in list(os.environ):
 if key.startswith('FELUCCA_') or key in ['BUDGET_UPDATE','GOLDEN_UPDATE']:os.environ.pop(key)
os.environ.update(JIELI_TOOLCHAIN=str(tc),AC79_SDK=str(deps/'ac79-sdk'),JIELI_DOCKER_IMAGE=image)
control=root/'main-control'
control.mkdir()
subprocess.run(['git','archive','fbadbca894419c9626306eb773f38ae22baed52d'],cwd='/Users/grantv/Code/dabbl8',stdout=(root/'main-source.tar').open('wb'),check=True)
import tarfile
with tarfile.open(root/'main-source.tar') as archive:archive.extractall(control,filter='data')
shutil.copytree(original/'build/gen',control/'build/gen')
candidate=src
rows=[]
for mode,diagnostic in [('main-control',False),('candidate-off',False),('candidate-diagnostic',True)]:
 src=control if mode=='main-control' else candidate
 name='8-'+mode
 spec=importlib.util.spec_from_file_location('target_build_'+name,src/'tools/build.py')
 b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
 b.CFLAGS=list(b.CFLAGS)+['-DNPART=8']
 if diagnostic:
  parent=src/'build/timing-diagnostics';parent.mkdir()
  b.OUT=Path(tempfile.mkdtemp(prefix='run-',dir=parent));b.GEN=b.OUT/'gen';b.LDR=b.OUT/'loader'
  shutil.copytree(src/'build/gen',b.GEN)
  b.TIMING_DIAGNOSTIC=True;b.DABBL8_VERSION=b.VERSION='v0.1.0-timing';b.PRODUCT='FM-1_8010'
 before={p.relative_to(src/'build').as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in (src/'build').iterdir() if p.is_file()}
 with (root/(name+'.log')).open('w') as log,contextlib.redirect_stdout(log),contextlib.redirect_stderr(log):
  print('App only; generated inputs copied byte-for-byte from retained generation. No loader/package/install.');print('OUT',b.OUT,'GEN',b.GEN,'CFLAGS',b.CFLAGS,'VERSION',b.VERSION)
  img,syms,dis,rt=b.build_app();errors,notes=b.check(img,syms,dis,rt);print('CHECK',errors,notes)
 (b.OUT/'symbols.txt').write_text(syms);(b.OUT/'ramtext-disassembly.txt').write_text(rt)
 q=subprocess.run([sys.executable,str(src/'tests/target_budget.py'),str(b.OUT/'felucca.dis'),str(src/'tests/target_budget.txt')],text=True,capture_output=True)
 (root/(name+'-budget.log')).write_text(q.stdout+q.stderr+'\nexit='+str(q.returncode)+'\n')
 row={'mode':name,'output':str(b.OUT),'app_bytes':len(img),'app_sha256':hashlib.sha256(img).hexdigest(),'check_errors':errors,'memory_notes':notes,'target_budget_exit':q.returncode,'scope':'App-only actual vendor compile/link; copied retained generated headers, no regeneration/loader/package/install; no hardware timing or stack qualification.'}
 if diagnostic:
  for relative,sha in before.items():assert hashlib.sha256((src/'build'/relative).read_bytes()).hexdigest()==sha
  row['ordinary_top_level_products_preserved']=True
 elif mode=='candidate-off':assert row['app_sha256']==rows[0]['app_sha256'];assert (b.OUT/'felucca.elf').read_bytes()==(control/'build/felucca.elf').read_bytes()
 rows.append(row);(root/'target-results.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(row),flush=True)
print('Actual combined-source default-off identity and isolated diagnostic target app checks complete; unchanged target budget failures retained.',flush=True)
