from pathlib import Path
import shutil,subprocess,tempfile
r=Path.cwd();d=Path('/private/tmp/d8-engine-offline-staging-evidence');s=(r/'firmware/src/d8p1_project.h').read_text()
changes={
'original-mask-bypass':('project_t *q=&workspace->project;unsigned target;','project_t *q=&workspace->project;unsigned target;engine_mask=D8ENGINE_EFFECTIVE_MASK;'),
'wrong-added-identity':('unsigned e=eng_vis_mask(0,engine_mask);','unsigned e=0;'),
'wrong-fm6-patch':('fm6_unpack(FM6_FACTORY[slot],patch);','fm6_unpack(FM6_INIT,patch);'),
'full-output-drift':('for(unsigned t=4;t<8;t++)if(!(engine_mask&(1u<<q->t[t].engine)))','for(unsigned t=4;t<8;t++)if(1)'),
'early-report-publication':('project_t *q=&workspace->project;unsigned target;','*report=result;project_t *q=&workspace->project;unsigned target;')}
for label,(a,b) in changes.items():
 assert s.count(a)==1,label
 with tempfile.TemporaryDirectory(prefix='d8-offline-mutant-') as temp:
  t=Path(temp);shutil.copytree(r/'firmware',t/'firmware');shutil.copytree(r/'tests',t/'tests',ignore=shutil.ignore_patterns('fixtures','__pycache__'));(t/'tests/fixtures').symlink_to(r/'tests/fixtures')
  (t/'firmware/src/d8p1_project.h').write_text(s.replace(a,b));exe=t/'test'
  with (d/(label+'.log')).open('w') as log:
   subprocess.run(['clang','-w','-DNPART=8','-I'+str(r/'build/gen'),'-I'+str(t/'firmware/src'),str(t/'firmware/src/d8p1.c'),str(t/'tests/engine_offline_staging_test.c'),'-lm','-o',str(exe)],stdout=log,stderr=subprocess.STDOUT,check=True)
   run=subprocess.run([str(exe),str(r/'tests/fixtures/projects/fun9.bin')],stdout=log,stderr=subprocess.STDOUT)
  assert run.returncode!=0,label;print(label+' DETECTED',flush=True)
