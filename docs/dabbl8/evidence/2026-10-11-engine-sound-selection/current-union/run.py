from pathlib import Path
import subprocess,json
root=Path('/Users/grantv/Code/dabbl8/.local-baseline/engine-sound-selection');out=Path('/private/tmp/d8-engine-sound-selection-current-union');results=[]
for mode in ('normal','san'):
 for tracks in (4,8):
  for fm in (0,1):
   for sl in (0,1):
    for path in ('editor','ui','adoption'):
     name=f'{mode}_{tracks}_{fm}_{sl}_{path}';exe=out/name
     flags=['-O2'] if mode=='normal' else ['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=undefined']
     args=['clang',*flags,'-w',f'-DNPART={tracks}',f'-DFELUCCA_FM4={fm}',f'-DFELUCCA_SLICE={sl}','-Ibuild/gen','-Ifirmware/src','firmware/src/d8p1.c', 'tests/engine_adoption_test.c' if path=='adoption' else 'tests/engine_sound_selection_test.c','-lm','-o',str(exe)]
     if path=='ui':args.insert(1,'-DD8_SOUND_UI_TEST')
     with (out/(name+'.log')).open('w') as f:
      f.write('COMMAND '+repr(args)+'\n');f.flush();build=subprocess.run(args,cwd=root,stdout=f,stderr=subprocess.STDOUT)
      run=subprocess.run([str(exe)],cwd=root,stdout=f,stderr=subprocess.STDOUT) if not build.returncode else None
     row={'name':name,'compile':build.returncode,'run':run.returncode if run else None};results.append(row);(out/'results.json').write_text(json.dumps(results,indent=2))
     if build.returncode or run.returncode: print(row,flush=True);raise SystemExit(1)
 print(mode+' matrix PASS',flush=True)
