from pathlib import Path
import subprocess,json
r=Path('/Users/grantv/Code/dabbl8/.local-baseline/engine-ui-availability');o=Path('/private/tmp/d8-engine-ui-availability-evidence');rows=[]
for kind in ('normal','san'):
 flags=['-O2'] if kind=='normal' else ['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=undefined']
 for name,libs,fm in [('editor_test',[],0),('ui_test',[],0),('ui_test',[],1),('native_ui_undo_test',['firmware/src/d8p1.c','firmware/src/d8pool.c'],0)]:
  label=f'{kind}_regression_{name}_fm{fm}';exe=o/label;cmd=['clang',*flags,'-w',f'-DFELUCCA_FM4={fm}','-Ibuild/gen','-Ifirmware/src',*libs,f'tests/{name}.c','-lm','-o',str(exe)]
  with (o/(label+'.log')).open('w') as f:
   f.write(repr(cmd)+'\n');f.flush();rc=subprocess.run(cmd,cwd=r,stdout=f,stderr=subprocess.STDOUT).returncode
   assert rc==0,cmd
   run_rc=subprocess.run([str(exe)],cwd=r,stdout=f,stderr=subprocess.STDOUT).returncode
   rows.append({'name':label,'compile':rc,'run':run_rc});(o/'regression-results.json').write_text(json.dumps(rows,indent=2)+'\n')
   assert run_rc==0,(name,fm,run_rc)
  print(label+' PASS',flush=True)
