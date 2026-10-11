from pathlib import Path
import shutil,subprocess,json,hashlib
r=Path('/Users/grantv/Code/dabbl8/.local-baseline/engine-capability-capture');out=Path('/private/tmp/d8-engine-capture-matrix');src=out/'late-dispatch-control';src.mkdir();rows=[]
for d in ['firmware','tests']:shutil.copytree(r/d,src/d)
shutil.copytree(r/'build/gen',src/'build/gen')
p=src/'firmware/src/editor.c';s=p.read_text();a=s.index('    /* Additive read-only schema query.');b=s.index('#if NPART >= NVOICE',a);block=s[a:b];s=s[:a]+s[b:];needle='    ed_capture.active=0;\n#endif\n';assert s.count(needle)==1;s=s.replace(needle,needle+block);p.write_text(s)
def run(name,cmd,cwd):
 log=out/(name+'.log')
 with log.open('w') as f:q=subprocess.run(cmd,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
 rows.append({'name':name,'command':cmd,'exit':q.returncode,'log_sha256':hashlib.sha256(log.read_bytes()).hexdigest()});return q.returncode
binary=out/'late-dispatch-control-binary';build=run('late-dispatch-build',['clang','-O2','-w','-DNPART=8','-DFELUCCA_FM4=1','-DFELUCCA_SLICE=1','-Ibuild/gen','-Ifirmware/src','-o',str(binary),'firmware/src/d8p1.c','firmware/src/d8pool.c','firmware/src/d8pool_mapped.c','tests/engine_availability_capture_test.c','-lm'],src);assert not build
assert run('late-dispatch-run',[str(binary)],src)==1
assert '!memcmp(&retained_capture,&ed_capture,sizeof ed_capture)' in (out/'late-dispatch-run.log').read_text()
assert run('wrong-four-track-capture-build',['clang','-O2','-w','-DNPART=4','-Ibuild/gen','-Ifirmware/src','-o',str(out/'wrong-four-track-capture'),'firmware/src/d8p1.c','firmware/src/d8pool.c','firmware/src/d8pool_mapped.c','tests/engine_availability_capture_test.c','-lm'],r)!=0
assert 'native capture fixture requires NPART=8' in (out/'wrong-four-track-capture-build.log').read_text()
(out/'negative-results.json').write_text(json.dumps({'rows':rows,'control_source_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'scope':'Mutated private candidate late-dispatch control, not pristine parent; proves actual capture retention assertions fail. Wrong4 build refuses rather than silently native8.'},indent=2)+'\n');print('Both expected negative controls observed')
