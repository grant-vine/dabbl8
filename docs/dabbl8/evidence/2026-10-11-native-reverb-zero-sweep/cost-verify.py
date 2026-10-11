from pathlib import Path
import shutil,subprocess,csv,json,statistics
b=Path('/Users/grantv/Code/dabbl8/.local-baseline');r=b/'native-reverb-latched-sweep';e=b/'native-reverb-latched-sweep-check'
head='448a4a625731fe2a2aca80445948cf22d9902057'
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip()==head
probe=e/'cost-probe.c';shutil.copy2(b/'native-reverb-indexed-sweep-check/cost-probe.c',probe);shutil.copy2(__file__,e/'cost-verify.py')
with (e/'candidate-cost-compile.log').open('w') as f:
 p=subprocess.run(['clang','-O2','-w','-Ibuild/gen','-Ifirmware/src','-DTREE_UI="'+str(r/'tests/ui_test.c')+'"','-o',str(e/'candidate-cost'),'firmware/src/d8p1.c','firmware/src/d8pool.c','firmware/src/d8pool_mapped.c',str(probe),'-lm'],cwd=r,stdout=f,stderr=subprocess.STDOUT)
assert p.returncode==0
for batch in range(3):
 for who in (('parent','candidate') if batch%2==0 else ('candidate','parent')):
  binary=b/'native-reverb-zero-sweep-check/parent-cost' if who=='parent' else e/'candidate-cost'
  with (e/(who+'-cost-'+str(batch)+'.csv')).open('w') as f:p=subprocess.run([str(binary)],stdout=f,stderr=subprocess.STDOUT)
  assert p.returncode==0
rows={who:[row for batch in range(3) for row in csv.DictReader((e/(who+'-cost-'+str(batch)+'.csv')).open())] for who in ('parent','candidate')}
result=[]
for model in ('0','1'):
 for state in ('0','1'):
  d={'model':'ROOM' if model=='0' else 'SPRING','input':'zero' if state=='0' else 'dense','rounds_per_source':27}
  for key in ('ns','instructions'):
   med={who:statistics.median(int(row[key]) for row in rows[who] if row['model']==model and row['state']==state) for who in rows}
   d[key]={'parent_median':med['parent'],'candidate_median':med['candidate'],'change_percent':100*(med['candidate']/med['parent']-1)}
  result.append(d)
(e/'timing-results.json').write_text(json.dumps({'source':head,'parent':'226440712db0808001abb3ad3dba3cfb5a786f1b','alternating_batches':3,'mac_measurements_not_target_deadline_proof':True,'results':result},indent=2)+'\n')
print(json.dumps(result,indent=2))
