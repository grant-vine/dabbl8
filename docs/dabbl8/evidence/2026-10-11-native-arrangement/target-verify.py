from pathlib import Path
import os,sys,subprocess,json,hashlib,shutil,time,importlib.util
base=Path('/Users/grantv/Code/dabbl8/.local-baseline');repo=base/'native-arrangement-final';e=base/'native-arrangement-final-target-check';e.mkdir(exist_ok=True)
expected=sys.argv[1];sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
inputs=json.loads((base/'native-autosave-combined-check/generated-inputs.json').read_text())
for p,h in inputs.items():
 src=base/'native-autosave-combined'/p; assert sha(src)==h,p
 dst=repo/p; dst.parent.mkdir(parents=True,exist_ok=True)
 if dst.exists(): assert sha(dst)==h,p
 else: shutil.copy2(src,dst)
(e/'generated-inputs.json').write_text(json.dumps(inputs,indent=2)+'\n')
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
# Focused actual runtime checks precede target build; source remains frozen.
run('arrangement-opt-build',['clang','-O2','-w','-Ibuild/gen','-Ifirmware/src','-o','build/arrangement-focused-opt','firmware/src/d8p1.c','firmware/src/d8pool.c','tests/native_arrangement_test.c','-lm'])
run('arrangement-opt',['build/arrangement-focused-opt'])
run('arrangement-san-build',['clang','-O1','-g','-w','-Ibuild/gen','-Ifirmware/src','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=all','-o','build/arrangement-focused-san','firmware/src/d8p1.c','firmware/src/d8pool.c','tests/native_arrangement_test.c','-lm'])
run('arrangement-san',['build/arrangement-focused-san'])
# Target builds have separate OUT directories; pinned default artifacts are never replaced.
os.chdir(repo);os.environ.update(env);sys.path.insert(0,str(repo/'tools'))
spec=importlib.util.spec_from_file_location('capture_builder',repo/'tools/build.py');b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
flags=list(b.CFLAGS);targets={}
with (e/'target-build.log').open('w') as log:
 oldout=sys.stdout;sys.stdout=log
 try:
  for n in (4,8):
   b.OUT=repo/f'build/native-arrangement-{n}';b.OUT.mkdir(parents=True,exist_ok=True);b.CFLAGS=flags+(['-DNPART=8'] if n==8 else [])
   image,symbols,dis,rt=b.build_app();errors,notes=b.check(image,symbols,dis,rt);errors+=b.mmio_check()
   record={'tracks':n,'bytes':len(image),'sha256':hashlib.sha256(image).hexdigest(),'errors':errors,'notes':notes}
   (b.OUT/'linked-symbols.txt').write_text(symbols);(b.OUT/'linked-disassembly.txt').write_text(dis)
   targets[n]=record;(e/'target-results.json').write_text(json.dumps(targets,indent=2)+'\n')
   assert not errors,record
   if n==4:
    record['audited_v115_hash_matches']=(record['sha256']==known['build/felucca.bin'])
    assert record['sha256']=='3ced057d74785ecd0cc2ccc8f274a1c9e94caa4a6b1203c846014a890d4720fc',record
 finally:sys.stdout=oldout
print('target builds PASS; pristine default artifacts preserved; record candidate identity separately',flush=True)
shutil.copy2(repo/'build/native-arrangement-4/linked-disassembly.txt',repo/'build/felucca.dis')
for n in (4,8):
 run('target-budget-'+str(n),[str(base/'venv/bin/python'),'tests/target_budget.py',str(repo/f'build/native-arrangement-{n}/linked-disassembly.txt'),'tests/target_budget.txt'],(0,) if n==4 else (0,1))

assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()==expected
assert not subprocess.check_output(['git','status','--porcelain'],cwd=repo,text=True).strip()
assert all(sha(repo/p)==h for p,h in known.items())
assert all(sha(repo/p)==h for p,h in inputs.items())
(e/'terminal.json').write_text(json.dumps({'source':expected,'target_builds':'PASS','default_baseline_identity':False,'baseline_difference':'Deliberately retained upstream v1.1.5.1 wake fix; selected pristine v1.1.5 measurement baseline unchanged','six_baseline_hashes_unchanged':True,'generated_inputs_unchanged':True,'native_budget_qualification':False,'hardware':'NOT_RUN'},indent=2)+'\n')
print('frozen target builds complete; inspect native budget failures independently',flush=True)
