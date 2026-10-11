from pathlib import Path
import subprocess,json,hashlib
r=Path.cwd();d=Path('/private/tmp/d8-engine-offline-profile-workflow-evidence');results=[]
for san in (False,True):
 init=d/('initializer-san' if san else 'initializer')
 if san:subprocess.run(['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all','-Ifirmware/src','firmware/src/d8p1.c','firmware/src/d8pool.c','tools/dabbl8_pool_initialize.c','-o',str(init)],check=True)
 for fm in (0,1):
  for sl in (0,1):
   label=f'workflow-fm{fm}-slice{sl}'+('-san' if san else '');exe=d/label
   cmd=['clang','-O2','-w',f'-DFELUCCA_FM4={fm}',f'-DFELUCCA_SLICE={sl}','-Ibuild/gen','-Ifirmware/src','firmware/src/d8p1.c','tools/dabbl8_project_convert.c','-lm','-o',str(exe)]
   if san:cmd[1:1]=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
   with (d/(label+'.log')).open('w') as log:
    subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
    subprocess.run(['python3','tests/engine_offline_profile_workflow_test.py',str(exe),str(d/f'parent-fm{fm}-slice{sl}'),str(init),str(fm),str(sl)],stdout=log,stderr=subprocess.STDOUT,check=True)
   results.append({'label':label,'compiler':cmd});print(label+' PASS',flush=True)
(d/'matrix.json').write_text(json.dumps({'base':'370ac2d108be3de6cd258ded66cda179218198d8','runs':results,'source_hashes':{p:hashlib.sha256((r/p).read_bytes()).hexdigest() for p in ['tools/dabbl8_project_convert.c','tools/dabbl8_project_convert.py','tools/dabbl8_migration_bundle.py','tests/engine_offline_profile_workflow_test.py','tests/fixtures/engine-profile-ordinary-cli.json']}},indent=2)+'\n')
