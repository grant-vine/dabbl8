# SPDX-License-Identifier: GPL-3.0-only
import hashlib,json,os,subprocess,tempfile,shlex
from pathlib import Path
s=Path(__file__).resolve().parents[1];parent=s/'build/host/timing-console';parent.mkdir(parents=True,exist_ok=True)
out=Path(os.environ['D8_TIMING_CONSOLE_OUT']) if 'D8_TIMING_CONSOLE_OUT' in os.environ else Path(tempfile.mkdtemp(prefix='run-',dir=parent));out.mkdir(exist_ok=True)
console=(s/'firmware/src/console.c').read_text()
prefix=console[:console.index('static void con_memr')]
suffix=console[console.index('static void con_exec'):]
preamble=r'''
/* Generated from actual candidate console.c. Omitted unrelated diagnostic bodies
 * are read-only stubs. Actual formatter/parser/dispatch/CDC loop remain. Simulated
 * watchdog drains output; no USB hardware or timing claim. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define FELUCCA_FLASH 1
#ifndef DABBL8_TIMING_DIAGNOSTIC
#define DABBL8_TIMING_DIAGNOSTIC 1
#endif
#define NTRK 8
#define HALF_FRAMES 128
#define FS 44100
#define FM1_TICKS_PER_US 24
#define APP_NAME "DABBL8"
#define FELUCCA_VERSION "v0.1.0-test"
#define CI_N 256u
#define CO_N 2048u
#define RING_PUBLISH() do {} while(0)
#include "audio_timing.h"
static struct { uint32_t boots; } felucca_dbg={17};
static struct { uint8_t dtr; } cdc={1};
static struct { uint8_t uboot_req; } usb;
static uint8_t cdc_in[CI_N],cdc_out[CO_N];
static uint32_t ci_r,ci_w,co_r,co_w,fm1_ms;
static char captured[65536];
static uint32_t captured_n,read_enabled=1,drains;
static void drain(void) {
    while(co_r!=co_w) {
        assert(captured_n+1<sizeof captured);
        captured[captured_n++]=(char)cdc_out[co_r++%CO_N];
    }
    captured[captured_n]=0;
}
static void fm1_wdt_feed(void) { fm1_ms++; if(read_enabled){drain();drains++;} }
'''
stubs=r'''
static void con_status(void){con_puts("status stub\r\n");}
static void con_dbg(void){}
static void con_crash(void){}
static void con_inp(void){}
static void con_params(void){}
static void con_memr(const char *p){(void)p;}
static void con_flr(const char *p){(void)p;}
'''
test=r'''
static void command(const char *s) {
    while(*s) {
        assert(ci_w-ci_r<CI_N);
        cdc_in[ci_w++%CI_N]=(uint8_t)*s++;
    }
    cdc_task();drain();
}
int main(void) {
    command("timing report\r\n");
#if DABBL8_TIMING_DIAGNOSTIC
    assert(strstr(captured,"requires a stopped run"));
    command("timing start junk\r");assert(!audio_timing.enabled);
    command("timingstart\r");assert(!audio_timing.enabled);
    command("timing start\r");assert(audio_timing.enabled && audio_timing.generation==1);
    command("timing start\r");assert(audio_timing.generation==1);
    command("timing report\r");assert(!strstr(captured,"d8t1 begin"));
    for(uint32_t i=0;i<1024;i++)audio_timing_record(i*100,i*120,UINT32_MAX-20+i);
    command("timing stop junk\r");assert(audio_timing.enabled);
    command("timing stop\r");assert(!audio_timing.enabled);
    unsigned char state[sizeof audio_timing];
    memcpy(state,(const void*)&audio_timing,sizeof state);
    command("timing report\r");
    assert(!memcmp(state,(const void*)&audio_timing,sizeof state));
    assert(strstr(captured,"d8t1 end") && drains && !con.stalled && !usb.uboot_req);
    const char *begin=strstr(captured,"d8t1 begin");
    const char *end=strstr(begin,"d8t1 end");end=strstr(end,"\r\n")+2;
    assert(begin && end);
    fwrite(begin,1,(size_t)(end-begin),stdout);
    command("timing stop\r");assert(!memcmp(state,(const void*)&audio_timing,sizeof state));
    command("status\r");assert(strstr(captured,"status stub"));
    /* A disconnected DTR reader produces an incomplete frame, not success. */
    captured_n=0;captured[0]=0;co_r=co_w;read_enabled=0;cdc.dtr=0;
    command("timing report\r");assert(con.stalled);
    assert(strstr(captured,"d8t1 begin") && !strstr(captured,"d8t1 end"));
#else
    assert(!audio_timing.enabled && !audio_timing.generation);
    command("timing start\r");assert(!audio_timing.enabled);
    command("help\r");assert(!strstr(captured,"timing start|"));
    assert(!strstr(captured,"d8t1 begin") && !usb.uboot_req);
    command("status\r");assert(strstr(captured,"status stub"));
#endif
    fprintf(stderr,"actual console slice: exact word parsing, argument refusal, dispatch, CDC framing, paused state, read-only status and DTR truncation PASS; simulated only\n");
}
'''
f=out/'console-fixture.c';f.write_text(preamble+prefix+stubs+suffix+test)
r={'scope':'Actual candidate console prefix parser/hex/print and con_exec/cdc_task, new timing formatter; unrelated diagnostic bodies stubbed. Simulated ring and watchdog/host drain, no physical transport or target cost proof.','console_source_sha256':hashlib.sha256(console.encode()).hexdigest(),'fixture_sha256':hashlib.sha256(f.read_bytes()).hexdigest(),'checks':[]}
modes=[('normal',['-O2'])]
if os.environ.get('SANITIZE','1')!='0':modes.append(('strict-san',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=undefined']))
for mode,flags in modes:
 for enabled in (0,1):
  nm=f'{mode}-flag{enabled}';binary=out/nm;cmd=[*shlex.split(os.environ.get('CC','cc')),*flags,'-w',f'-DDABBL8_TIMING_DIAGNOSTIC={enabled}','-Ifirmware/src',str(f),'-o',str(binary)]
  proc=subprocess.run(cmd,cwd=s,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(out/(nm+'-build.log')).write_bytes(proc.stdout);assert proc.returncode==0,(cmd,proc.stdout.decode())
  proc=subprocess.run([str(binary)],cwd=s,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1'),stdout=subprocess.PIPE,stderr=subprocess.PIPE)
  (out/(nm+'.frame')).write_bytes(proc.stdout);(out/(nm+'-run.log')).write_bytes(proc.stderr)
  assert proc.returncode==0,(nm,proc.stderr.decode())
  r['checks'].append({'name':nm,'build':cmd,'exit':proc.returncode,'frame_sha256':hashlib.sha256(proc.stdout).hexdigest()})
  print(nm,'PASS',len(proc.stdout),'frame bytes',flush=True)
if len(modes)>1:assert (out/'normal-flag1.frame').read_bytes()==(out/'strict-san-flag1.frame').read_bytes()
(out/'report.json').write_text(json.dumps(r,indent=2)+'\n')
