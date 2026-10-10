from pathlib import Path
import os,sys,subprocess,json,hashlib,shutil,time,importlib.util
base=Path('/Users/grantv/Code/dabbl8/.local-baseline');repo=base/'native-autosave-combined';e=base/'native-autosave-combined-check';e.mkdir(exist_ok=True)
expected=sys.argv[1];sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
inputs=json.loads((e/'generated-inputs.json').read_text());assert all(sha(repo/p)==h for p,h in inputs.items())
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()==expected
assert not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip()
shutil.copy2(__file__,e/'verify.py');(e/'source-head.txt').write_text(expected+'\n')
known=json.loads((base/'native-integration-check/unchanged-artifacts.json').read_text())
for p in ['build/felucca.bin','build/felucca.fwsc','build/loader/ota.bin']:
 src=base/'main-workspace'/p;assert sha(src)==known[p],p
 dst=repo/p;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
assert all(sha(repo/p)==h for p,h in known.items())
(e/'before-artifacts.json').write_text(json.dumps(known,indent=2)+'\n')
env=os.environ.copy();env.update(JIELI_TOOLCHAIN=str(base/'deps/jieli-linux-toolchains-20250324.1'),AC79_SDK=str(base/'deps/ac79-sdk'),JIELI_DOCKER_IMAGE='debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587',PATH=str(base/'venv/bin')+':'+env['PATH'],ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
for k in list(env):
 if k.startswith(('FELUCCA_','GOLDEN_','BUDGET_')):env.pop(k)
(e/'environment.json').write_text(json.dumps({k:env[k] for k in ['JIELI_TOOLCHAIN','AC79_SDK','JIELI_DOCKER_IMAGE','ASAN_OPTIONS','UBSAN_OPTIONS']},indent=2)+'\n')
checks=[]
def run(name,args,allowed=(0,)):
 with (e/(name+'.log')).open('w') as f:p=subprocess.run(args,cwd=repo,env=env,stdout=f,stderr=subprocess.STDOUT)
 checks.append({'name':name,'exit':p.returncode,'allowed_exits':list(allowed)});(e/'checks.json').write_text(json.dumps(checks,indent=2)+'\n');print(name,p.returncode,flush=True)
 if p.returncode not in allowed:raise SystemExit(p.returncode)
 return p.returncode
# Target builds have separate OUT directories; pinned default artifacts are never replaced.
os.chdir(repo);os.environ.update(env);sys.path.insert(0,str(repo/'tools'))
spec=importlib.util.spec_from_file_location('combined_builder',repo/'tools/build.py');b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
flags=list(b.CFLAGS);targets={}
with (e/'target-build.log').open('w') as log:
 oldout=sys.stdout;sys.stdout=log
 try:
  for n in (4,8):
   b.OUT=repo/f'build/native-combined-{n}';b.OUT.mkdir(parents=True,exist_ok=True);b.CFLAGS=flags+(['-DNPART=8'] if n==8 else [])
   image,symbols,dis,rt=b.build_app();errors,notes=b.check(image,symbols,dis,rt);errors+=b.mmio_check()
   record={'tracks':n,'bytes':len(image),'sha256':hashlib.sha256(image).hexdigest(),'errors':errors,'notes':notes}
   (b.OUT/'linked-symbols.txt').write_text(symbols);(b.OUT/'linked-disassembly.txt').write_text(dis)
   targets[n]=record;(e/'target-results.json').write_text(json.dumps(targets,indent=2)+'\n')
   assert not errors,record
   if n==4:assert record['sha256']==known['build/felucca.bin']
 finally:sys.stdout=oldout
print('target builds and pinned default identity PASS',flush=True)
shutil.copy2(repo/'build/native-combined-4/linked-disassembly.txt',repo/'build/felucca.dis')
for n in (4,8):
 run('target-budget-'+str(n),[str(base/'venv/bin/python'),'tests/target_budget.py',str(repo/f'build/native-combined-{n}/linked-disassembly.txt'),'tests/target_budget.txt'],(0,) if n==4 else (0,1))
# Every native budget failure is preserved; allowed exit1 is recorded evidence, not qualification.
for mode,flags in [('opt',['-O2']),('san',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=undefined'])]:
 for test in ['native_autosave_session_test','native_autosave_integration_test','native_fx_counter_boundary_test','native_signature_equivalence_test','native_autosave_combined_test']:
  sources=['firmware/src/d8p1.c']+([] if test=='native_signature_equivalence_test' else ['firmware/src/d8pool.c','firmware/src/d8pool_mapped.c'])+['tests/'+test+'.c']
  exe=e/(test+'-'+mode)
  run(test+'-'+mode+'-compile',['cc',*flags,'-w','-Ibuild/gen','-Ifirmware/src',*sources,'-lm','-o',str(exe)])
  run(test+'-'+mode,[str(exe)])
run('full-suite',['sh','tests/run_tests.sh'])
after={p:sha(repo/p) for p in known};assert after==known
assert all(sha(repo/p)==h for p,h in inputs.items())
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()==expected
assert not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip()
(e/'unchanged-artifacts.json').write_text(json.dumps(after,indent=2)+'\n')
(e/'terminal.json').write_text(json.dumps({'source':expected,'full_suite':'PASS','six_baseline_hashes_unchanged':True,'native_budget_is_qualification':False,'hardware':'SKIP','skips':[l for l in (e/'full-suite.log').read_text().splitlines() if 'skip' in l.lower()]},indent=2)+'\n')
print('TERMINAL combined full suite PASS; exact head and six baseline hashes unchanged; device qualification open',flush=True)
