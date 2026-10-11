from pathlib import Path
import subprocess
r=Path.cwd();d=Path('/private/tmp/d8-engine-offline-profile-workflow-evidence')
for san in (False,True):
 init=d/('runner-initializer-san' if san else 'initializer')
 flags=['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=undefined','-w','-Ibuild/gen','-Ifirmware/src'] if san else ['-O1','-Wall','-Wno-unused-function','-O2','-w','-Ibuild/gen','-Ifirmware/src']
 if san:subprocess.run(['clang',*flags,'-o',str(init),'firmware/src/d8p1.c','firmware/src/d8pool.c','tools/dabbl8_pool_initialize.c'],check=True)
 for fm in (0,1):
  for sl in (0,1):
   label=f'runner-workflow-fm{fm}-slice{sl}'+('-san' if san else '');exe=d/label
   cmd=['clang',*flags,f'-DFELUCCA_FM4={fm}',f'-DFELUCCA_SLICE={sl}','-o',str(exe),'firmware/src/d8p1.c','tools/dabbl8_project_convert.c','-lm']
   with (d/(label+'.log')).open('w') as log:
    log.write('Exact added normal CC or inherited SCC runner flags; pinned parent receipt verification only, no generation\n');log.flush()
    subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
    subprocess.run(['python3','tests/engine_offline_profile_workflow_test.py',str(exe),'-',str(init),str(fm),str(sl)],stdout=log,stderr=subprocess.STDOUT,check=True)
   print(label+' PASS',flush=True)
