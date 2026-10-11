#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""CLI qualification gate only; never import native MIDI or construct ports."""
import sys,io,contextlib,os
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import dabbl8_readonly_midi as t
import builtins
original=builtins.__import__;attempts=[]
def forbid(name,*args,**kwargs):
 if name in ('mido','rtmidi'):
  attempts.append(name);raise AssertionError('native import reached before qualification refusal')
 return original(name,*args,**kwargs)
argv=['adapter','--bridge','--input-port','selected','--output-port','selected']
for env in ({},{'MIDO_BACKEND':'rtmidi/MACOSX_CORE','DABBL8_ALLOW_UNQUALIFIED_NATIVE':'1','DABBL8_NATIVE_BACKEND':'qualified'}):
 with patch.dict(os.environ,env,clear=True):
  with patch.object(sys,'argv',argv),patch.object(t.platform,'system',return_value='Darwin'),patch.object(builtins,'__import__',side_effect=forbid),patch.object(t,'StrictCoreMidiBackend',side_effect=AssertionError('native wrapper constructed')),patch.object(t,'ReadOnlyMidi',side_effect=AssertionError('native link constructed')):
   output=io.StringIO()
   with contextlib.redirect_stderr(output):
    try:t.main()
    except SystemExit as e:assert e.code==2
    else:raise AssertionError('native CLI not refused')
   assert 'unqualified and disabled' in output.getvalue()
  assert not attempts
for override in ('--allow-unqualified-native','--qualified-backend=1.5.8'):
 with patch.object(sys,'argv',argv+[override]),patch.object(builtins,'__import__',side_effect=forbid),contextlib.redirect_stderr(io.StringIO()):
  try:t.main()
  except SystemExit as e:assert e.code==2
  else:raise AssertionError('unknown qualification override accepted')
assert not attempts
with patch.object(sys,'argv',['adapter','--help']),patch.object(builtins,'__import__',side_effect=forbid),contextlib.redirect_stdout(io.StringIO()):
 try:t.main()
 except SystemExit as e:assert e.code==0
 else:raise AssertionError('help not exited')
assert not attempts
print('native MIDI qualification gate: refusal before import/construct, help lazy PASS')
