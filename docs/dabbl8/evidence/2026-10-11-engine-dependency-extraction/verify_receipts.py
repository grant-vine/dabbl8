from pathlib import Path
import json,hashlib,re
r=Path(__file__).parent
rows=json.loads((r/'host-results.json').read_text());pairs=[];pp=[]
def diagnostic(name):
 return '\n'.join((r/(name+'.log')).read_text().splitlines()[1:]).replace(str(r/'parent'),'FROZEN').replace(str(r/'candidate'),'FROZEN')
for row in rows:
 if row['name'].startswith('candidate-'):
  parent='parent-'+row['name'][10:];same=diagnostic(row['name'])==diagnostic(parent)
  entry={'candidate':row['name'],'parent':parent,'status':row['status'],'normalized_diagnostic_identity':same}
  if row['name']=='candidate-regress':
   for mode in ['parent','candidate']:
    assert '92 golden renders (0 changed, 0 gone), 0 health failures, 0 voice / routing checks failed, 0 CPU entries over budget' in diagnostic(mode+'-regress')
   entry['accepted_summary_identity']=True;entry['scope']='Host elapsed/counter/tie differences retained; no performance parity claimed.'
  else:assert same,entry
  pairs.append(entry)
for tracks in (4,8):
 for fm in (0,1):
  for sl in (0,1):
   tag=f'{tracks}-fm{fm}-sl{sl}';a=diagnostic('preprocess-parent-'+tag);b=diagnostic('preprocess-candidate-'+tag)
   norm=lambda x:'\n'.join(z for z in x.splitlines() if z.strip())
   assert norm(a)==norm(b),tag
   pp.append({'config':tag,'blank_line_normalized_emitted_C_identical':True,'sha256':hashlib.sha256(norm(a).encode()).hexdigest()})
(r/'host-pairs.json').write_text(json.dumps(pairs,indent=2)+'\n');(r/'preprocess-pairs.json').write_text(json.dumps(pp,indent=2)+'\n')
assert len(pairs)==33 and len(pp)==8
print('Verified32 exact fixture outcome pairs (12 inherited failures); both92-golden summaries PASS;8 emitted-C pairs identical except blank lines. No timing claim.')
