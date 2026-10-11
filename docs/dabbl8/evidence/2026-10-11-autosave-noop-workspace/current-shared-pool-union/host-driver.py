from pathlib import Path
import subprocess,json
q=Path('/private/tmp/d8-autosave-union-qualification');r=q/'host-source';results=[]
for t in ['competing_workspace_callers_test','native_autosave_combined_test','native_autosave_session_test']:
 for san in [False,True]:
  name=t+('-inherited-san' if san else '-normal');cmd=['clang','-w','-Ibuild/gen','-Ifirmware/src']
  cmd+=['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=undefined'] if san else ['-O2']
  cmd+=['-o',str(q/name),'firmware/src/d8p1.c','firmware/src/d8pool.c','firmware/src/d8pool_mapped.c','tests/'+t+'.c','-lm']
  b=subprocess.run(cmd,cwd=r,capture_output=True,text=True);(q/(name+'-build.log')).write_text(b.stdout+b.stderr)
  a=subprocess.run([str(q/name)],cwd=r,capture_output=True,text=True) if not b.returncode else None
  if a:(q/(name+'-run.log')).write_text(a.stdout+a.stderr)
  results.append({'name':name,'command':cmd,'build_exit':b.returncode,'run_exit':a.returncode if a else None,'output':a.stdout+a.stderr if a else b.stderr})
  (q/'host-results.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(results[-1]),flush=True)
