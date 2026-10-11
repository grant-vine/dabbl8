import concurrent.futures,json,os,subprocess
from pathlib import Path
out=Path('/private/tmp/d8-shared-pool-scan-evidence');root=Path('/Users/grantv/Code/dabbl8/.local-baseline/shared-pool-scan-optimization');control=Path('/private/tmp/d8-shared-pool-scan-control')
def run(mode):
 cwd=control if mode=='control' else root;flags=['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=undefined','-w'] if mode=='san' else ['-O2','-w'];exe=out/('final-'+mode);cmd=['clang',*flags,'-Ibuild/gen','-Ifirmware/src','-o',str(exe),'firmware/src/d8p1.c','firmware/src/d8pool.c','tests/shared_pool_scan_test.c','-lm'];r={'mode':mode,'command':cmd}
 with (out/('final-'+mode+'-build.log')).open('wb') as f:r['build_exit']=subprocess.run(cmd,cwd=cwd,stdout=f,stderr=subprocess.STDOUT).returncode
 if not r['build_exit']:
  for name,args in ([('cost',['--cost']),('negative',['--negative'])] if mode=='control' else [('safety',[])]):
   with (out/('final-'+mode+'-'+name+'.log')).open('wb') as f:r[name+'_exit']=subprocess.run([str(exe),*args],cwd=cwd,stdout=f,stderr=subprocess.STDOUT,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'}).returncode
 print(json.dumps(r),flush=True);return r
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:r=list(pool.map(run,['normal','san','control']))
(out/'final-results.json').write_text(json.dumps(r,indent=2)+'\n')
raise SystemExit(any(x['build_exit'] or (x.get('cost_exit',1)!=0 or x.get('negative_exit',0)!=1 if x['mode']=='control' else x.get('safety_exit',1)!=0) for x in r))
