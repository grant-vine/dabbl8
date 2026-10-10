from pathlib import Path
import os,subprocess,json,hashlib,shutil,time
base=Path('/Users/grantv/Code/dabbl8/.local-baseline');repo=base/'instrument-write-quarantine';e=base/'instrument-quarantine-full-suite';e.mkdir(exist_ok=True)
head='179e41a91440469242f72af7f266d54bebbd3761'
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()==head
assert not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip()
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
known=json.loads((base/'native-integration-check/unchanged-artifacts.json').read_text())
inputs=json.loads((base/'native-autosave-combined-check/generated-inputs.json').read_text())
assert all(h(repo/p)==v for p,v in known.items())
assert all(h(repo/p)==v for p,v in inputs.items())
assert h(repo/'build/felucca.dis')==h(repo/'build/native-capture-4/linked-disassembly.txt')
assert json.loads((base/'instrument-quarantine-target-check/target-results.json').read_text())['4']['actual_6f73_four_track_hash_matches']
assert json.loads((base/'instrument-quarantine-target-check/terminal.json').read_text())['source']==head
(e/'source-head.txt').write_text(head+'\n');(e/'before-artifacts.json').write_text(json.dumps(known,indent=2)+'\n');(e/'generated-inputs.json').write_text(json.dumps(inputs,indent=2)+'\n');shutil.copy2(__file__,e/'verify.py')
env=os.environ.copy();env.update(JIELI_TOOLCHAIN=str(base/'deps/jieli-linux-toolchains-20250324.1'),AC79_SDK=str(base/'deps/ac79-sdk'),JIELI_DOCKER_IMAGE='debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587',PATH=str(base/'venv/bin')+':'+env['PATH'],ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
for k in list(env):
 if k.startswith(('FELUCCA_','GOLDEN_','BUDGET_')):env.pop(k)
(e/'environment.json').write_text(json.dumps({k:env[k] for k in ['JIELI_TOOLCHAIN','AC79_SDK','JIELI_DOCKER_IMAGE','PATH','ASAN_OPTIONS','UBSAN_OPTIONS']},indent=2)+'\n')
started=time.time();print('START frozen',head,'at',started,flush=True)
with (e/'full-suite.log').open('w') as f:proc=subprocess.run(['sh','tests/run_tests.sh'],cwd=repo,env=env,stdout=f,stderr=subprocess.STDOUT)
after={p:h(repo/p) for p in known};stable=after==known and subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()==head and not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip();genstable=all(h(repo/p)==v for p,v in inputs.items())
(e/'unchanged-artifacts.json').write_text(json.dumps(after,indent=2)+'\n');(e/'result.json').write_text(json.dumps({'head':head,'exit':proc.returncode,'elapsed_seconds':time.time()-started,'head_and_six_baselines_unchanged':stable,'generated_29_inputs_unchanged':genstable,'lines_containing_skip_not_all_skips':[l for l in (e/'full-suite.log').read_text().splitlines() if 'skip' in l.lower()]},indent=2)+'\n');print('TERMINAL',proc.returncode,'stable',stable,'generated',genstable,flush=True)
assert stable and genstable
raise SystemExit(proc.returncode)
