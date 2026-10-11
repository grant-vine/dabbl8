from pathlib import Path
import os,sys,json,hashlib,subprocess,shutil,time
base=Path('/Users/grantv/Code/dabbl8/.local-baseline');r=base/'native-arrangement-final';e=base/'native-arrangement-final-check';e.mkdir(exist_ok=True)
expected='226440712db0808001abb3ad3dba3cfb5a786f1b';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
head=lambda:subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip()
assert head()==expected;assert not subprocess.check_output(['git','status','--porcelain'],cwd=r,text=True).strip()
known=json.loads((base/'native-integration-check/unchanged-artifacts.json').read_text());gen=json.loads((base/'native-autosave-combined-check/generated-inputs.json').read_text())
assert all(sha(r/p)==h for p,h in known.items());assert all(sha(r/p)==h for p,h in gen.items())
dis=r/'build/native-arrangement-4/linked-disassembly.txt';assert sha(dis)==sha(r/'build/felucca.dis')
target=json.loads((base/'native-arrangement-final-target-check/target-results.json').read_text());assert target['4']['sha256']=='3ced057d74785ecd0cc2ccc8f274a1c9e94caa4a6b1203c846014a890d4720fc'
tracked={p:sha(r/p) for p in ['tests/golden.txt','tests/target_budget.txt','tests/cpu_baseline.txt']}
env=os.environ.copy();env.update(PATH=str(base/'venv/bin')+':'+env['PATH'],JIELI_TOOLCHAIN=str(base/'deps/jieli-linux-toolchains-20250324.1'),AC79_SDK=str(base/'deps/ac79-sdk'),JIELI_DOCKER_IMAGE='debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587',ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1',OUT='build/arrangement-tests')
for k in list(env):
 if k.startswith(('FELUCCA_','GOLDEN_','BUDGET_')):env.pop(k)
(e/'before-pins.json').write_text(json.dumps({'source':expected,'six_baseline':known,'generated29':gen,'golden_cost_refs':tracked,'actual_default4_dis':sha(dis),'target8_static_failures':None},indent=2)+'\n')
(e/'environment.json').write_text(json.dumps({k:env[k] for k in ['JIELI_TOOLCHAIN','AC79_SDK','JIELI_DOCKER_IMAGE','ASAN_OPTIONS','UBSAN_OPTIONS','OUT']},indent=2)+'\n')
shutil.copy2(__file__,e/'verify.py')
for f in Path('/private/tmp').glob('arrangement-auto-*.log'):shutil.copy2(f,e/f.name)
start=time.time()
with (e/'full-suite.log').open('w') as f:p=subprocess.run(['sh','tests/run_tests.sh'],cwd=r,env=env,stdout=f,stderr=subprocess.STDOUT)
assert head()==expected;assert not subprocess.check_output(['git','status','--porcelain'],cwd=r,text=True).strip()
assert all(sha(r/q)==h for q,h in known.items());assert all(sha(r/q)==h for q,h in gen.items());assert all(sha(r/q)==h for q,h in tracked.items());assert sha(dis)==sha(r/'build/felucca.dis')
(e/'terminal.json').write_text(json.dumps({'source':expected,'exit':p.returncode,'elapsed_seconds':time.time()-start,'six_baseline_unchanged':True,'generated29_unchanged':True,'golden_cost_refs_unchanged':True,'actual_default4_dis_sha256':sha(dis),'native8_static_failures':None,'hardware':'NOT_RUN','release_qualified':False},indent=2)+'\n')
print('full suite terminal',p.returncode,flush=True);sys.exit(p.returncode)
