from pathlib import Path
import subprocess,json,os,hashlib,itertools
r=Path(__file__).parent; rows=[]
san=['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=undefined']
env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
for k in ['GOLDEN_UPDATE','BUDGET_UPDATE']:env.pop(k,None)
def run(cmd,cwd,name):
 q=subprocess.run(cmd,cwd=cwd,env=env,text=True,capture_output=True)
 (r/(name+'.log')).write_text('COMMAND '+repr(cmd)+'\n'+q.stdout+q.stderr+'\nSTATUS '+str(q.returncode)+'\n')
 rows.append({'name':name,'status':q.returncode,'stdout_sha256':hashlib.sha256(q.stdout.encode()).hexdigest(),'stderr_sha256':hashlib.sha256(q.stderr.encode()).hexdigest()});(r/'host-results.json').write_text(json.dumps(rows,indent=2)+'\n');print(name,q.returncode,flush=True)
 return q
for tracks,fm,sl in itertools.product((4,8),(0,1),(0,1)):
 defs=[f'-DNPART={tracks}',f'-DFELUCCA_FM4={fm}',f'-DFELUCCA_SLICE={sl}']; name=f'{tracks}-fm{fm}-sl{sl}'
 outs=[]
 for mode in ('parent','candidate'):
  src=r/mode
  q=run(['clang','-E','-P','-w',*defs,'-Ibuild/gen','-Ifirmware/src','tests/legacy_fixture_test.c'],src,f'preprocess-{mode}-{name}')
  assert not q.returncode
  outs.append('\n'.join(line for line in q.stdout.replace(str(src),'FROZEN_SOURCE').splitlines() if line.strip()))
 assert outs[0]==outs[1],name+' preprocessing drift'
 for sanitized in (False,True):
  flags=(san if sanitized else ['-O2'])+['-w',*defs,'-Ibuild/gen','-Ifirmware/src']; tag=name+('-san' if sanitized else '-normal')
  for fixture in (['heavy_pool_test'] if tracks==8 else [])+['legacy_fixture_test']:
   for mode in ('parent','candidate'):
    src=r/mode; exe=r/(mode+'-'+fixture+'-'+tag)
    libs=['firmware/src/d8p1.c','firmware/src/d8pool.c'] if tracks==8 and fixture=='legacy_fixture_test' else []
    c=run(['clang',*flags,*libs,'tests/'+fixture+'.c','-lm','-o',str(exe)],src,'compile-'+mode+'-'+fixture+'-'+tag)
    if c.returncode:raise RuntimeError('compile failure retained')
    q=run([str(exe)]+(['tests/fixtures/projects'] if fixture=='legacy_fixture_test' else []),src,mode+'-'+fixture+'-'+tag)
    # Retain inherited/configuration-specific failures; compare parent/candidate below.
  for fixture in (['d8p1_project_test'] if tracks==8 else []):
   for mode in ('parent','candidate'):
    src=r/mode; exe=r/(mode+'-'+fixture+'-'+tag)
    q=run(['clang',*flags,'firmware/src/d8p1.c','firmware/src/d8pool.c','tests/'+fixture+'.c','-lm','-o',str(exe)],src,'compile-'+mode+'-'+fixture+'-'+tag)
    if q.returncode:raise RuntimeError('compile failure retained')
    q=run(['python3','tests/d8p1_project_test.py',str(exe)],src,mode+'-'+fixture+'-'+tag)
    # Retain inherited/configuration-specific failures; compare parent/candidate below.
for mode in ('parent','candidate'):
 src=r/mode;exe=r/(mode+'-regress')
 q=run(['clang','-O2','-w','-Ibuild/gen','-Ifirmware/src','tests/regress.c','-lm','-o',str(exe)],src,'compile-'+mode+'-regress')
 assert not q.returncode
 q=run([str(exe),'tests/golden.txt','tests/cpu_baseline.txt'],src,mode+'-regress')
 # Preserve CPU outcomes; validate output against frozen parent and all audio references independently.
pairs=[]
for row in rows:
 if row['name'].startswith('candidate-'):
  name='parent-'+row['name'][10:];parent=next(x for x in rows if x['name']==name)
  same=all(row[k]==parent[k] for k in ['status','stdout_sha256','stderr_sha256']);pairs.append({'candidate':row['name'],'parent':name,'exact_outcome_identity':same,'status':row['status']});assert same,pairs[-1]
(r/'host-pairs.json').write_text(json.dumps(pairs,indent=2)+'\n')
print('Host matrix completed; inherited failures explicitly retained',flush=True)
