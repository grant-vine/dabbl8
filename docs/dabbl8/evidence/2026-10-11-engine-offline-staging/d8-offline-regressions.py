from pathlib import Path
import subprocess
r=Path.cwd();d=Path('/private/tmp/d8-engine-offline-staging-evidence')
for san in (False,True):
 suffix='-san' if san else ''
 for name,src,driver in [('ordinary','tests/d8p1_project_test.c','tests/d8p1_project_test.py'),('bundle','tools/dabbl8_project_convert.c','tests/project_conversion_test.py')]:
  exe=d/(name+suffix);cmd=['clang','-w','-Ibuild/gen','-Ifirmware/src','firmware/src/d8p1.c',src,'-lm','-o',str(exe)]
  if san:cmd[1:1]=['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
  with (d/(name+suffix+'.log')).open('w') as log:
   subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True);subprocess.run(['python3',driver,str(exe)],stdout=log,stderr=subprocess.STDOUT,check=True)
  print(name+suffix+' PASS',flush=True)
