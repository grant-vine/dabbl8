from pathlib import Path
import subprocess
r=Path.cwd();d=Path('/private/tmp/d8-engine-offline-staging-evidence');s=(r/'tests/run_tests.sh').read_text()
for san in (False,True):
 for fm in (0,1):
  for sl in (0,1):
   label=f'runner-profile-fm{fm}-slice{sl}'+('-san' if san else '')
   flags=['-O1','-Wall','-Wno-unused-function','-O2','-w','-Ibuild/gen','-Ifirmware/src'] if not san else ['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=undefined','-w','-Ibuild/gen','-Ifirmware/src']
   cmd=['clang',*flags,'-DNPART=8',f'-DFELUCCA_FM4={fm}',f'-DFELUCCA_SLICE={sl}','-o',str(d/label),'firmware/src/d8p1.c','tests/engine_offline_staging_test.c','-lm']
   with (d/(label+'.log')).open('w') as log:
    log.write('Exact added tests/run_tests.sh CC/SCC command flags, clang on Mac; ASAN_OPTIONS=detect_leaks=0\n');log.flush()
    subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
    subprocess.run(['python3','tests/engine_offline_staging_test.py',str(d/label)],stdout=log,stderr=subprocess.STDOUT,check=True)
   print(label+' PASS',flush=True)
