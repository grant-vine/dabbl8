from pathlib import Path
import subprocess,json,hashlib,os
r=Path('/Users/grantv/Code/dabbl8/.local-baseline/engine-capability-capture');out=Path('/private/tmp/d8-engine-capture-matrix');sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();rows=[]
source={f:sha(r/f) for f in ['tests/engine_availability_capture_test.c','tests/engine_availability_caps_test.c','tests/run_tests.sh','firmware/src/editor.c']};gen={str(p.relative_to(r/'build/gen')):sha(p) for p in sorted((r/'build/gen').rglob('*')) if p.is_file()};(out/'inputs.json').write_text(json.dumps({'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip(),'source_hashes':source,'gen_hashes':gen,'compiler':subprocess.check_output(['clang','--version'],text=True)},indent=2)+'\n')
def cmd(name,args):
 p=out/(name+'.log')
 with p.open('w') as f:q=subprocess.run(args,cwd=r,stdout=f,stderr=subprocess.STDOUT)
 rows.append({'name':name,'command':args,'exit':q.returncode,'log_sha256':sha(p)});(out/'results.json').write_text(json.dumps(rows,indent=2)+'\n');return q.returncode
for mode in ['normal','inherited-san']:
 flags=['-O2','-w'] if mode=='normal' else ['-O1','-g','-w','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=undefined']
 for tracks in [4,8]:
  for fm4 in [0,1]:
   for slice in [0,1]:
    name=f'{mode}-{tracks}-fm{fm4}-slice{slice}';binary=out/name;test='tests/engine_availability_caps_test.c' if tracks==4 else 'tests/engine_availability_capture_test.c';sources=[test] if tracks==4 else ['firmware/src/d8p1.c','firmware/src/d8pool.c','firmware/src/d8pool_mapped.c',test]
    if cmd(name+'-build',['clang',*flags,f'-DNPART={tracks}',f'-DFELUCCA_FM4={fm4}',f'-DFELUCCA_SLICE={slice}','-Ibuild/gen','-Ifirmware/src','-o',str(binary),*sources,'-lm'])==0:cmd(name+'-run',[str(binary)])
print('commands',len(rows),'failures',[x['name'] for x in rows if x['exit']])
