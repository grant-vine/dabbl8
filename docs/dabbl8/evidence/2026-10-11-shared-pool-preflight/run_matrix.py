import concurrent.futures,json,os,subprocess,time
from pathlib import Path
root=Path('/Users/grantv/Code/dabbl8/.local-baseline/shared-pool-scan-optimization');out=Path('/private/tmp/d8-shared-pool-scan-evidence')
tests=['d8p1_pool_runtime_test','native_project_menu_test','d8p1_flash_runtime_test','native_autosave_session_test','native_arrangement_test','competing_workspace_callers_test','native_migration_session_test']
libs=['firmware/src/d8p1.c','firmware/src/d8pool.c','firmware/src/d8pool_mapped.c']
def run(case):
 name,mode=case;exe=out/(name+'-'+mode);flags=['-O2','-w'] if mode=='normal' else ['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=undefined','-w'];cmd=['clang',*flags,'-Ibuild/gen','-Ifirmware/src','-o',str(exe),*libs,'tests/'+name+'.c','-lm'];r={'test':name,'mode':mode,'command':cmd};start=time.time()
 with (out/(name+'-'+mode+'-build.log')).open('wb') as log:r['build_exit']=subprocess.run(cmd,cwd=root,stdout=log,stderr=subprocess.STDOUT).returncode
 if not r['build_exit']:
  with (out/(name+'-'+mode+'.log')).open('wb') as log:r['run_exit']=subprocess.run([str(exe)],cwd=root,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'}).returncode
 r['elapsed_seconds']=round(time.time()-start,3);print(json.dumps(r),flush=True);return r
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:result=list(pool.map(run,[(n,m) for n in tests for m in ('normal','san')]))
(out/'matrix-results.json').write_text(json.dumps(result,indent=2)+'\n')
raise SystemExit(any(x['build_exit'] or x.get('run_exit',1) for x in result))
