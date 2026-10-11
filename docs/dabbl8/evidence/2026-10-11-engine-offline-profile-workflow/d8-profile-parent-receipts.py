from pathlib import Path
import hashlib,json,subprocess,tempfile
r=Path.cwd();base=Path('/Users/grantv/Code/dabbl8/.local-baseline/engine-offline-staging');d=Path('/private/tmp/d8-engine-offline-profile-workflow-evidence')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
receipt={'format':'dabbl8-ordinary-cli-parent-receipts','version':1,'parent_commit':'370ac2d108be3de6cd258ded66cda179218198d8','compiler':subprocess.check_output(['clang','--version'],text=True),'source_sha256':{'tools/dabbl8_project_convert.c':sha(base/'tools/dabbl8_project_convert.c'),'tests/hostsim.c':sha(base/'tests/hostsim.c'),'firmware/src/d8p1_project.h':sha(base/'firmware/src/d8p1_project.h')},'dependency_sha256':{str(p.relative_to(base)):sha(p) for p in sorted((base/'build/gen').glob('*.h'))},'configs':{}}
response=lambda p:{'status':p.returncode,'stdout_sha256':hashlib.sha256(p.stdout).hexdigest(),'stderr_sha256':hashlib.sha256(p.stderr).hexdigest()}
for fm in (0,1):
 for sl in (0,1):
  name=f'fm{fm}-slice{sl}';be=d/('parent-'+name);ne=d/('candidate-'+name)
  common=['clang','-O2','-w',f'-DFELUCCA_FM4={fm}',f'-DFELUCCA_SLICE={sl}','-I'+str(base/'build/gen'),'-I'+str(base/'firmware/src'),str(base/'firmware/src/d8p1.c'),str(base/'tools/dabbl8_project_convert.c'),'-lm','-o',str(be)]
  log=d/('receipt-'+name+'.log')
  with log.open('w') as out:
   subprocess.run(common,stdout=out,stderr=subprocess.STDOUT,check=True)
   new=common.copy();new[new.index(str(base/'tools/dabbl8_project_convert.c'))]=str(r/'tools/dabbl8_project_convert.c');new[-1]=str(ne)
   subprocess.run(new,stdout=out,stderr=subprocess.STDOUT,check=True)
   subprocess.run(['python3','tests/engine_offline_profile_workflow_test.py',str(ne),str(be),str(d/'initializer'),str(fm),str(sl)],stdout=out,stderr=subprocess.STDOUT,check=True)
  vectors={}
  with tempfile.TemporaryDirectory(prefix='d8-profile-parent-') as tmp:
   for original in sorted((r/'tests/fixtures/projects').glob('*.bin')):
    if '.expected-fun9' in original.name:continue
    p=subprocess.run([str(be),'legacy',str(original),'15'],capture_output=True,check=True);native=Path(tmp)/'native.d8p';native.write_bytes(p.stdout)
    rows={'legacy':response(p)}
    for mode in ('check','canonical'):rows[mode]=response(subprocess.run([str(be),mode,str(native),'15'],capture_output=True,check=True))
    vectors[original.name]=rows
  receipt['configs'][name]={'compile_command':common,'vectors':vectors};print(name+' direct parent comparison PASS',flush=True)
(d/'ordinary-parent-receipts.json').write_text(json.dumps(receipt,indent=2)+'\n')
