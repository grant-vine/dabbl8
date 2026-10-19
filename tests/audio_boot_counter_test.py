# SPDX-License-Identifier: GPL-3.0-only
"""Actual boot/status counter slices and pure host parsing; no device adapter."""
from pathlib import Path
import hashlib
import json
import os
import re
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from dabbl8_timing_capture import boot_audio_status


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit('usage: audio_boot_counter_test.py NEW_OUTPUT_DIR')
    out = Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=False)
    startup = (ROOT / 'firmware/src/main.c').read_text()
    console = (ROOT / 'firmware/src/console.c').read_text()
    assignment = re.search(r'^    audio_late_entry = .*?;', startup, re.M)
    snapshot = re.search(r'^    uint32_t late = .*?;', console, re.M)
    publication = re.search(r'#if DABBL8_TIMING_DIAGNOSTIC\n(    con_kv\("audio_late".*?)(?=#else)', console, re.S)
    if assignment is None or snapshot is None or publication is None:
        raise SystemExit('Missing actual boot/status counter implementation')
    if assignment.start() > startup.index('    audio_init();'):
        raise SystemExit('Boot counter entry must precede audio initialization')
    code = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static struct { uint32_t late; } felucca_dbg;
static uint32_t audio_late_entry, values[3], rows, inject, failures, checks;
static const char *keys[] = {"audio_late", "audio_late_entry", "audio_late_boot"};
static void check(unsigned ok) { checks++; failures += !ok; }
static void con_kv(const char *key, int32_t value) {
    check(rows < 3 && !strcmp(key, keys[rows]));
    if(rows < 3) values[rows++] = (uint32_t)value;
    if(inject) { inject = 0; felucca_dbg.late++; }
}
static void boot(void) { ASSIGNMENT }
static void report(void) { SNAPSHOT
PUBLICATION
}
int main(void) {
    const uint32_t cases[][3] = {{0,0,0}, {2,2,0}, {0,2,2},
        {UINT32_MAX-1,1,3}, {100,99,UINT32_MAX}};
    for(unsigned i=0;i<5;i++) for(unsigned irq=0;irq<2;irq++) {
        felucca_dbg.late=cases[i][0]; audio_late_entry=77;
        boot(); check(felucca_dbg.late==cases[i][0] && audio_late_entry==cases[i][0]);
        felucca_dbg.late=cases[i][1]; rows=0; inject=irq; report();
        check(rows==3 && values[0]==cases[i][1] && values[1]==cases[i][0] && values[2]==cases[i][2]);
        check(felucca_dbg.late==cases[i][1]+irq); /* no history clearing */
    }
    printf("actual boot/status counters: cases=10 checks=%u failures=%u; simulated IRQ injection only\n",checks,failures);
    return failures ? 1 : 0;
}
'''.replace('ASSIGNMENT', assignment.group()).replace('SNAPSHOT', snapshot.group()).replace('PUBLICATION', publication.group(1))
    source = out / 'counter.c'
    source.write_text(code)
    binary = out / 'counter'
    command = shlex.split(os.environ.get('CC', 'clang')) + ['-std=c11', '-O1', '-g',
        '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
        '-fno-sanitize-recover=all', str(source), '-o', str(binary)]
    receipt = {'commands': [command, [str(binary)]], 'device_access': False,
        'sources_sha256': {name: hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
            for name in ('firmware/src/main.c', 'firmware/src/console.c',
                         'tools/dabbl8_timing_capture.py', 'tests/audio_boot_counter_test.py')}}
    for name, argv in zip(('compile', 'run'), receipt['commands']):
        result = subprocess.run(argv, capture_output=True, timeout=60, check=False)
        (out / (name + '.stdout')).write_bytes(result.stdout)
        (out / (name + '.stderr')).write_bytes(result.stderr)
        receipt[name + '_exit'] = result.returncode
        if result.returncode:
            (out/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
            return result.returncode
    accepted = [(0,0,0), (2,2,0), (2,0,2), (-1,-1,0), (1,-2,3), (99,100,-1)]
    for total, entry, delta in accepted:
        reply = f'audio_late {total}\r\naudio_late_entry {entry}\r\naudio_late_boot {delta}\r\n'
        assert boot_audio_status(reply) == tuple(v & 0xffffffff for v in (total,entry,delta))
    good = 'audio_late 2\naudio_late_entry 2\naudio_late_boot 0\n'
    bad = [good.replace('entry 2','entry 1'), good.replace('boot 0','boot 1'),
           good + 'audio_late 2\n', good.replace('audio_late_entry 2\n',''),
           good.replace('audio_late 2','audio_late 2147483648'),
           good.replace('audio_late 2','audio_late 0x2')]
    for reply in bad:
        try:
            boot_audio_status(reply)
        except ValueError:
            continue
        raise AssertionError('Malformed/inconsistent boot counter tuple accepted')
    receipt.update(accepted=True, producer_cases=10, parser_cases=len(accepted)+len(bad))
    (out/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(receipt, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
