from pathlib import Path
import contextlib,hashlib,importlib.util,json,os,shutil,subprocess,sys,itertools
r=Path(__file__).parent; deps=Path('/Users/grantv/Code/dabbl8/.local-baseline/deps')
sys.path.insert(0,'/Users/grantv/Code/dabbl8/.local-baseline/dependency-integrity/ci')
from dependency_integrity import verify_archive_tree,verify_sdk,SDK_BUILD_INPUTS
image='debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587';tc=deps/'jieli-linux-toolchains-20250324.1'
env={'toolchain':verify_archive_tree(deps/'linux-toolchain.tar.xz',tc,tc.name,'f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958'),'sdk':verify_sdk(deps/'ac79-sdk','d179b4484759423312073f5fbb232501aa491047',SDK_BUILD_INPUTS),'image':image,'python':sys.version}
env['container']=json.loads(subprocess.check_output(['docker','image','inspect',image],text=True))[0];assert env['container']['Architecture']=='amd64' and env['container']['Os']=='linux'
env['compiler_version']=subprocess.check_output(['docker','run','--rm','--platform','linux/amd64','-v',str(tc)+':/opt/jieli:ro',image,'/opt/jieli/pi32v2/bin/clang','--version'],text=True)
(r/'target-environment.json').write_text(json.dumps(env,indent=2)+'\n')
for k in list(os.environ):
 if k.startswith('FELUCCA_') or k in ['BUDGET_UPDATE','GOLDEN_UPDATE']:os.environ.pop(k)
os.environ.update(JIELI_TOOLCHAIN=str(tc),AC79_SDK=str(deps/'ac79-sdk'),JIELI_DOCKER_IMAGE=image)
rows=[]
for tracks,fm,sl in itertools.product((4,8),(0,1),(0,1)):
 for mode in ('parent','candidate'):
  src=r/mode;name=f'{tracks}-fm{fm}-sl{sl}-{mode}';spec=importlib.util.spec_from_file_location('build_'+name,src/'tools/build.py');b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
  b.CFLAGS=list(b.CFLAGS)+[f'-DNPART={tracks}',f'-DFELUCCA_FM4={fm}',f'-DFELUCCA_SLICE={sl}']
  with (r/(name+'-target.log')).open('w') as log,contextlib.redirect_stdout(log),contextlib.redirect_stderr(log):
   print('Frozen35266b6 parent/candidate, identical generated files. App-only, no loader/package/device.');print(b.CFLAGS);img,syms,dis,rt=b.build_app();errors,notes=b.check(img,syms,dis,rt);print('CHECK',errors,notes)
  row={'name':name,'errors':errors,'memory_notes':notes,'app_sha256':hashlib.sha256(img).hexdigest()}
  for art in ['felucca.bin','felucca.elf','felucca.dis']:
   blob=(b.OUT/art).read_bytes();row[art+'_sha256']=hashlib.sha256(blob).hexdigest();shutil.copy2(b.OUT/art,r/(name+'-'+art))
  (r/(name+'-symbols.txt')).write_text(syms);(r/(name+'-ramtext.txt')).write_text(rt)
  q=subprocess.run([sys.executable,str(src/'tests/target_budget.py'),str(b.OUT/'felucca.dis'),str(src/'tests/target_budget.txt')],text=True,capture_output=True);row['budget_status']=q.returncode;(r/(name+'-budget.log')).write_text(q.stdout+q.stderr+'\nSTATUS '+str(q.returncode)+'\n')
  if mode=='candidate':
   prev=rows[-1];row['parent_binary_identity']={art:row[art+'_sha256']==prev[art+'_sha256'] for art in ['felucca.bin','felucca.elf','felucca.dis']};row['parent_memory_identity']=notes==prev['memory_notes'];assert all(row['parent_binary_identity'].values()) and row['parent_memory_identity'] and errors==prev['errors'] and row['budget_status']==prev['budget_status'],row
  rows.append(row);(r/'target-results.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(row),flush=True)
print('All16 frozen vendor builds complete.',flush=True)
