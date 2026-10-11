from pathlib import Path
import json,os,sys,importlib.util,subprocess,struct,contextlib,itertools
r=Path(__file__).parent;deps=Path('/Users/grantv/Code/dabbl8/.local-baseline/deps')
for k in list(os.environ):
 if k.startswith('FELUCCA_') or k in ['BUDGET_UPDATE','GOLDEN_UPDATE']:os.environ.pop(k)
os.environ.update(JIELI_TOOLCHAIN=str(deps/'jieli-linux-toolchains-20250324.1'),AC79_SDK=str(deps/'ac79-sdk'),JIELI_DOCKER_IMAGE='debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587')
types=['gr_grain_t','gr_part_t','drw_vc_t','phys_slot_t','drum_lane_t','fm6_note_t','voice_t','track_t']
rows=[]
for tracks,fm,sl in itertools.product((4,8),(0,1),(0,1)):
 names=types+(['heavy_state_t'] if tracks==8 else [])
 for mode in ('parent','candidate'):
  src=r/mode;name=f'{tracks}-fm{fm}-sl{sl}-{mode}';probe=src/'firmware/src/d8_type_probe.c'
  probe.write_text('/* GPL-3.0-only diagnostic; never linked/shipped. */\n#include "felucca.c"\n__attribute__((used,section(".d8_types"))) const uint32_t d8_types[] = {\n'+','.join('sizeof('+t+'),__alignof__('+t+')' for t in names)+'};\n')
  spec=importlib.util.spec_from_file_location('types_'+name,src/'tools/build.py');b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
  flags=[*b.CFLAGS,f'-DNPART={tracks}',f'-DFELUCCA_FM4={fm}',f'-DFELUCCA_SLICE={sl}','-Ifirmware/hal','-Ifirmware/src','-Ibuild/gen']
  obj=b.OUT/'d8_type_probe.o';blob=b.OUT/'d8_type_probe.bin'
  with (r/(name+'-types.log')).open('w') as log,contextlib.redirect_stdout(log),contextlib.redirect_stderr(log):
   print('Actual unchanged firmware translation unit included; diagnostic object only, no link/device.');print(flags)
   b.tc('cc',*flags,'-c',probe,'-o',obj);b.tc('common/bin/objcopy','-O','binary','-j','.d8_types',obj,blob)
  values=struct.unpack('<'+'I'*(len(names)*2),blob.read_bytes());row={'name':name,'types':{t:{'sizeof':values[2*i],'alignment':values[2*i+1]} for i,t in enumerate(names)}}
  if mode=='candidate':assert row['types']==rows[-1]['types'];row['parent_type_identity']=True
  rows.append(row);(r/'types-results.json').write_text(json.dumps(rows,indent=2)+'\n');print(name,row['types'],flush=True)
print('All16 target type probes identical.',flush=True)
