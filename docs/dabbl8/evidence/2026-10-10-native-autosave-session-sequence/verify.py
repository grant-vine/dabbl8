from pathlib import Path
import os,subprocess,json,hashlib,shutil,time
base=Path('/Users/grantv/Code/dabbl8/.local-baseline');repo=base/'native-autosave-session-policy';e=base/'native-autosave-session-sequence-full-suite';e.mkdir(exist_ok=True)
head='292ff32beb77381c73d2b8c0e3076c1d100a890c';assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()==head
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();known=json.loads((base/'native-integration-check/unchanged-artifacts.json').read_text())
for path in ['build/felucca.bin','build/felucca.fwsc','build/loader/ota.bin']:
 src=base/'main-workspace'/path;assert h(src)==known[path],path
 dest=repo/path;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dest)
shutil.copy2(repo/'build/native-session-4/linked-disassembly.txt',repo/'build/felucca.dis')
files=list(known);before={p:h(repo/p) for p in files};assert before==known
(e/'source-head.txt').write_text(head+'\n');(e/'before-artifacts.json').write_text(json.dumps(before,indent=2)+'\n');shutil.copy2('/private/tmp/native-session-sequence-full-suite.py',e/'verify.py')
env=os.environ.copy();env.update(JIELI_TOOLCHAIN=str(base/'deps/jieli-linux-toolchains-20250324.1'),AC79_SDK=str(base/'deps/ac79-sdk'),JIELI_DOCKER_IMAGE='debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587',PATH=str(base/'venv/bin')+':'+env['PATH'],ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
for k in list(env):
 if k.startswith(('FELUCCA_','GOLDEN_','BUDGET_')):env.pop(k)
(e/'environment.json').write_text(json.dumps({k:env[k] for k in ['JIELI_TOOLCHAIN','AC79_SDK','JIELI_DOCKER_IMAGE','PATH','ASAN_OPTIONS','UBSAN_OPTIONS']},indent=2)+'\n')
started=time.time();print('START frozen',head,'at',started,flush=True)
with (e/'full-suite.log').open('w') as f:proc=subprocess.run(['sh','tests/run_tests.sh'],cwd=repo,env=env,stdout=f,stderr=subprocess.STDOUT)
after={p:h(repo/p) for p in files};stable=after==before and subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()==head
(e/'unchanged-artifacts.json').write_text(json.dumps(after,indent=2)+'\n');(e/'result.json').write_text(json.dumps({'head':head,'exit':proc.returncode,'elapsed_seconds':time.time()-started,'head_and_six_baselines_unchanged':stable,'skips':[l for l in (e/'full-suite.log').read_text().splitlines() if 'skip' in l.lower()]},indent=2)+'\n');print('TERMINAL',proc.returncode,'stable',stable,flush=True)
assert stable
raise SystemExit(proc.returncode)
