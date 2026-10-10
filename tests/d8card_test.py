#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import importlib.util
import json
import subprocess
import sys
import tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('card',root/'tools/dabbl8_card.py');card=importlib.util.module_from_spec(spec);spec.loader.exec_module(card)
checks=0

def check(value):
    global checks
    checks+=1
    assert value


def refused(profile):
    try:card.resolve(profile)
    except ValueError:return True
    return False


engines=card.registry();names=list(engines)
check({k:v[0] for k,v in engines.items()}==dict(ANALOG=0,PHASE=2,LOFI=3,SAMPLE=4,VOICE=5,TRIO=6,WHEEL=7,GRAIN=8,PHYS=9,DRUM=10,NOISE=11,FM6=12,SLICE=13))
for bits in range(1,8192):
    selected=[n for i,n in enumerate(names) if bits&(1<<i)]
    result=card.resolve({'version':1,'name':'test','engines':selected})
    check(result['engine_mask']==sum(1<<engines[x][0] for x in selected))
    check(not result['firmware_generated'] and not result['hardware_qualified'] and not result['runtime_enforced'])
base={'version':1,'name':'core','engines':['ANALOG','FM6','LOFI','DRUM']}
for bad in [None,[],{},dict(base,version=True),dict(base,version=2),dict(base,name='../core'),dict(base,name='a\n#define X 1'),dict(base,name='A'),dict(base,name='a'*33),dict(base,engines=[]),dict(base,engines=['DIGITAL']),dict(base,engines=['FM6','FM6']),dict(base,engines=[12]),dict(base,engines='FM6'),dict(base,command='make')]:
    check(refused(bad))
with tempfile.TemporaryDirectory() as tmp:
    tmp=Path(tmp);a=tmp/'a';b=tmp/'b'
    card.emit(base,a);card.emit(dict(base,engines=list(reversed(base['engines']))),b)
    for name in ['card.h','card.json']:check((a/name).read_bytes()==(b/name).read_bytes())
    before=(a/'card.json').read_bytes()
    try:card.emit(base,a)
    except FileExistsError:pass
    else:raise AssertionError('existing destination replaced')
    check((a/'card.json').read_bytes()==before)
    for text in ['{"version":1,"version":1}', 'x'*4097, '{"version":NaN}', '\ufffd']:
        path=tmp/'bad.json';path.write_text(text)
        result=subprocess.run([sys.executable,str(root/'tools/dabbl8_card.py'),str(path),str(tmp/'refused')],capture_output=True)
        check(result.returncode!=0 and not (tmp/'refused').exists())
    source=tmp/'core.json';source.write_text(json.dumps(base));original=source.read_bytes()
    result=subprocess.run([sys.executable,str(root/'tools/dabbl8_card.py'),str(source),str(tmp/'cli')],capture_output=True)
    check(result.returncode==0 and source.read_bytes()==original)
    report=json.loads(result.stdout);check(report['engine_mask']==0x1409 and report['required_components']==6)
print(f'd8card profile CLI: {checks} checks passed')
