from pathlib import Path
import tempfile,shutil,subprocess
r=Path.cwd();d=Path('/private/tmp/d8-engine-offline-profile-workflow-evidence')
mutations={
'c-profile-bypass':('tools/dabbl8_project_convert.c','profile_status=(unsigned)d8p1_legacy_convert_profile_v1(&state,&workspace,&report,input.bytes,n,(unsigned)mask,profile_mask);','profile_status=d8p1_legacy_convert(&state,&workspace,&report,input.bytes,n,(unsigned)mask)?0:1;'),
'project-profile-ignored':('tools/dabbl8_project_convert.py',"mode = 'legacy-profile-v1' if engine_mask is not None else 'legacy'","mode = 'legacy'"),
'migration-profile-ignored':('tools/dabbl8_migration_bundle.py',"mode = 'legacy-profile-v1' if engine_mask is not None else 'legacy'","mode = 'legacy'"),
'wrong-reply-mask-trusted':('tools/dabbl8_project_convert.py',"type(meta.get('engine_mask')) is not int or meta.get('engine_mask') != engine_mask or","type(meta.get('engine_mask')) is not int or"),
'boolean-requested-id-trusted':('tools/dabbl8_project_convert.py',"type(item.get('requested_engine')) is not int or item.get('requested_engine') != 0 or","item.get('requested_engine') != 0 or"),
'late-cleanup-disabled':('tools/dabbl8_project_convert.py',"for path in destination.glob('*.d8p'):","for path in []:")}
for name,(path,a,b) in mutations.items():
 with tempfile.TemporaryDirectory(prefix='d8-profile-mutant-') as temp:
  t=Path(temp)
  for part in ('tools','web','firmware','tests'):
   shutil.copytree(r/part,t/part,ignore=shutil.ignore_patterns('fixtures','__pycache__') if part=='tests' else None)
  (t/'tests/fixtures').symlink_to(r/'tests/fixtures');p=t/path;s=p.read_text();assert s.count(a)==1;p.write_text(s.replace(a,b))
  exe=d/'workflow-fm0-slice1'
  with (d/(name+'.log')).open('w') as log:
   if path.endswith('.c'):
    exe=t/'mutant';subprocess.run(['clang','-O2','-w','-I'+str(r/'build/gen'),'-I'+str(t/'firmware/src'),str(t/'firmware/src/d8p1.c'),str(t/'tools/dabbl8_project_convert.c'),'-lm','-o',str(exe)],stdout=log,stderr=subprocess.STDOUT,check=True)
   p=subprocess.run(['python3',str(t/'tests/engine_offline_profile_workflow_test.py'),str(exe),'-',str(d/'initializer'),'0','1'],stdout=log,stderr=subprocess.STDOUT)
  assert p.returncode!=0,name;print(name+' DETECTED',flush=True)
