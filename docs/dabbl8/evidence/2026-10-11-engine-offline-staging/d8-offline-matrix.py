from pathlib import Path
import subprocess,hashlib,json
root=Path.cwd();dst=Path('/private/tmp/d8-engine-offline-staging-evidence');fs=sorted(p for p in (root/'tests/fixtures/projects').glob('*.bin') if '.expected-fun9' not in p.name)
results=[]
for fm in (0,1):
 for sl in (0,1):
  for san in (False,True):
   label=f'profile-fm{fm}-slice{sl}'+('-san-clean' if san else '-final')
   cmd=['clang','-w','-DNPART=8',f'-DFELUCCA_FM4={fm}',f'-DFELUCCA_SLICE={sl}','-Ibuild/gen','-Ifirmware/src','firmware/src/d8p1.c','tests/engine_offline_staging_test.c','-lm','-o',str(dst/label)]
   if san:cmd[1:1]=['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-sanitize=shift','-fno-sanitize-recover=all']
   with (dst/(label+'.log')).open('w') as log:
    subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
    subprocess.run([str(dst/label),'tests/fixtures/projects/fun9.bin']+list(map(str,fs)),stdout=log,stderr=subprocess.STDOUT,check=True)
   results.append(label);print(label+' PASS',flush=True)
(dst/'matrix-final.json').write_text(json.dumps({'base':'f0e8043d4e611f0c5df9dbd46888ec2e380bad43','scopes':results,'source_hashes':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in ['firmware/src/d8p1_project.h','tests/engine_offline_staging_test.c']}},indent=2)+'\n')
