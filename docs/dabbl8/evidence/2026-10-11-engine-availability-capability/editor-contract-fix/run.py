from pathlib import Path
import subprocess,json,hashlib
r=Path('/Users/grantv/Code/dabbl8/.local-baseline/engine-availability-capability');p=Path('/private/tmp/d8-capability-editor-contract-fix');rows=[]
w=p/'schema-only.c';w.write_text('#define EDITOR_TEST_NO_MAIN 1\n#include "'+str(r/'tests/editor_test.c')+'"\n#undef main\nint main(void) { return d8_capabilities(); }\n')
for tracks in (4,8):
 for fm in (0,1):
  for sl in (0,1):
   for san in (False,True):
    name=f'schema-{tracks}-{fm}-{sl}'+('-san' if san else '-normal')
    flags=['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=undefined'] if san else ['-O2']
    cmd=['clang','-w',*flags,f'-DNPART={tracks}',f'-DFELUCCA_FM4={fm}',f'-DFELUCCA_SLICE={sl}','-I'+str(Path('/Users/grantv/Code/dabbl8/.local-baseline/uac-array-bounds/build/gen')),'-I'+str(r/'firmware/src'),str(w),'-lm','-o',str(p/name)]
    b=subprocess.run(cmd,cwd=r,capture_output=True,text=True);(p/(name+'-build.log')).write_text(b.stdout+b.stderr)
    a=subprocess.run([str(p/name)],cwd=r,capture_output=True,text=True) if b.returncode==0 else None
    if a:(p/(name+'-run.log')).write_text(a.stdout+a.stderr)
    rows.append({'name':name,'command':cmd,'build_exit':b.returncode,'run_exit':a.returncode if a else None});assert a and a.returncode==0,(name,b.stderr,a.stdout+a.stderr if a else '')
    (p/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print(name+' PASS',flush=True)
for san in (False,True):
 name='full-editor'+('-san' if san else '-normal')
 flags=['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=undefined'] if san else ['-O2']
 cmd=['clang','-w',*flags,'-I/Users/grantv/Code/dabbl8/.local-baseline/uac-array-bounds/build/gen','-Ifirmware/src','tests/editor_test.c','-lm','-o',str(p/name)]
 b=subprocess.run(cmd,cwd=r,capture_output=True,text=True);(p/(name+'-build.log')).write_text(b.stdout+b.stderr);assert b.returncode==0
 a=subprocess.run([str(p/name)],cwd=r,capture_output=True,text=True);(p/(name+'-run.log')).write_text(a.stdout+a.stderr);assert a.returncode==0,(name,a.stdout+a.stderr)
 rows.append({'name':name,'command':cmd,'build_exit':b.returncode,'run_exit':a.returncode});(p/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print(name+' PASS',flush=True)
