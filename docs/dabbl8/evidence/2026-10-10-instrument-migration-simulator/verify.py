from pathlib import Path
import os,subprocess,json,hashlib,shutil,time
base=Path('/Users/grantv/Code/dabbl8/.local-baseline');repo=base/'instrument-migration-executor';old=base/'instrument-readonly-capture';e=base/'instrument-migration-full-suite';e.mkdir(exist_ok=True)
head='381537968d16da8f911f4d1ee0ae579f42ca1c3c';parent='6f73dba1576ad12bdc3fa30eac1509cd674dc0a5'
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()==head
assert not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip()
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
known=json.loads((base/'native-integration-check/unchanged-artifacts.json').read_text());inputs=json.loads((base/'native-autosave-combined-check/generated-inputs.json').read_text())
# Every tracked parent file except the explicitly changed host runner is byte
# identical. Includes all firmware/headers/link scripts/assets/build generators.
proof={}
for line in subprocess.check_output(['git','ls-tree','-r',parent],cwd=repo,text=True).splitlines():
 meta,path=line.split('\t',1)
 if path=='tests/run_tests.sh':continue
 data=subprocess.check_output(['git','show',parent+':'+path],cwd=repo)
 assert (repo/path).read_bytes()==data,path
 if path.startswith(('firmware/','tools/','assets/','LICENSES/')) or path in ('build.sh','BUILDING.md','LICENSING.md'):
  proof[path]=h(repo/path)
(e/'unchanged-parent-build-inputs.json').write_text(json.dumps({'parent':parent,'head':head,'scope':'all parent tracked files except host runner compared bytewise; listed firmware/tools/assets/license/build inputs','sha256':proof,'new_executor_not_linked':True,'fresh_target_build':False},indent=2)+'\n')
for p,v in known.items():
 if p.startswith('build/'):
  assert h(old/p)==v;(repo/p).parent.mkdir(parents=True,exist_ok=True);shutil.copy2(old/p,repo/p)
 assert h(repo/p)==v
for p,v in inputs.items():
 assert h(old/p)==v;(repo/p).parent.mkdir(parents=True,exist_ok=True);shutil.copy2(old/p,repo/p)
shutil.copy2(old/'build/felucca.dis',repo/'build/felucca.dis')
(e/'source-head.txt').write_text(head+'\n');(e/'before-artifacts.json').write_text(json.dumps(known,indent=2)+'\n');(e/'generated-inputs.json').write_text(json.dumps(inputs,indent=2)+'\n');shutil.copy2(__file__,e/'verify.py')
env=os.environ.copy();env.update(JIELI_TOOLCHAIN=str(base/'deps/jieli-linux-toolchains-20250324.1'),AC79_SDK=str(base/'deps/ac79-sdk'),JIELI_DOCKER_IMAGE='debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587',PATH=str(base/'venv/bin')+':'+env['PATH'],ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
for k in list(env):
 if k.startswith(('FELUCCA_','GOLDEN_','BUDGET_')):env.pop(k)
(e/'environment.json').write_text(json.dumps({k:env[k] for k in ['JIELI_TOOLCHAIN','AC79_SDK','JIELI_DOCKER_IMAGE','PATH','ASAN_OPTIONS','UBSAN_OPTIONS']},indent=2)+'\n')
started=time.time();print('START frozen',head,'at',started,flush=True)
with (e/'full-suite.log').open('w') as f:proc=subprocess.run(['sh','tests/run_tests.sh'],cwd=repo,env=env,stdout=f,stderr=subprocess.STDOUT)
after={p:h(repo/p) for p in known};stable=after==known and subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()==head and not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip();genstable=all(h(repo/p)==v for p,v in inputs.items())
(e/'unchanged-artifacts.json').write_text(json.dumps(after,indent=2)+'\n');(e/'result.json').write_text(json.dumps({'head':head,'exit':proc.returncode,'elapsed_seconds':time.time()-started,'head_and_six_baselines_unchanged':stable,'generated_29_inputs_unchanged':genstable,'fresh_target_build':False,'target_build_inputs_identical_to':parent,'lines_containing_skip_not_all_skips':[l for l in (e/'full-suite.log').read_text().splitlines() if 'skip' in l.lower()]},indent=2)+'\n');print('TERMINAL',proc.returncode,'stable',stable,'generated',genstable,flush=True)
assert stable and genstable
raise SystemExit(proc.returncode)
