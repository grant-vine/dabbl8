# SPDX-License-Identifier: GPL-3.0-only
"""Execute shipping audio HAL and boot ordering on a simulated DMA clock only."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def function(source: str, name: str, *, optional: bool = False) -> str:
    match = re.search(r'(?m)^(?:FM1_INLINE|static(?: inline)?|void) [^\n]*\b' + re.escape(name) + r'\([^;]*?\)[^{;]*\{', source)
    if match is None:
        if optional:
            return ''
        raise ValueError('Missing shipping function: ' + name)
    depth = 1
    end = match.end()
    while depth:
        if source[end] == '{':
            depth += 1
        elif source[end] == '}':
            depth -= 1
        end += 1
    return source[match.start():end]

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--tracks', type=int, choices=(4, 8), default=8)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    sources = {name: (args.source_root / name).read_text() for name in (
        'firmware/hal/fm1_audio.h', 'firmware/src/audio.c',
        'firmware/src/main.c', 'firmware/src/usb.c')}
    hal = sources['firmware/hal/fm1_audio.h']
    audio = sources['firmware/src/audio.c']
    startup = sources['firmware/src/main.c']
    boot = re.search(r'(?m)^    master_boot\(\);.*?(?=^    for \(ms = 0; ms < 400u;)', startup, re.S)
    if boot is None:
        raise ValueError('Missing shipping boot startup sequence')
    bodies = '\n'.join(function(hal, name, optional=name == 'fm1_audio_start') for name in (
        'fm1__pc_out', 'fm1_audio_init', 'fm1_audio_start'))
    bodies += '\n' + function(startup, 'fm1_timer5_irq')
    bodies += '\n' + function(sources['firmware/src/usb.c'], 'usb_start')
    bodies += '\n' + function(audio, 'audio_init')
    bodies += '\n' + function(audio, 'audio_start', optional=True)
    prefix = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define FM1_INLINE static inline
#define NTRK 8
#define FELUCCA_UART 1
#define FELUCCA_UAC 0
#define FELUCCA_SIE_IRQ_BUDGET 0
#define FM1_TICKS_PER_US 24u
#define HALF_FRAMES 128u
#define HALF_WORDS (HALF_FRAMES * 2u)
#define PERIOD (HALF_FRAMES * 544u)
static uint16_t con0, con1, len;
static uint8_t con2, con3;
static uint32_t adr3, clk2, iomap5, gpio[4];
#define FM1_ALNK_CON0 con0
#define FM1_ALNK_CON1 con1
#define FM1_ALNK_CON2 con2
#define FM1_ALNK_CON3 con3
#define FM1_ALNK_ADR3 adr3
#define FM1_ALNK_LEN len
#define FM1_CLK_CON2 clk2
#define FM1_IOMAP_CON5 iomap5
#define FM1_PC 0
#define FM1_DIE 0
#define FM1_OUT 1
#define FM1_DIR 2
#define FM1_PR(p, r) gpio[r]
#define FM1_IRQ_ALNK0 11u
static uint64_t now, dma_start, next_half, blackout_ticks;
static unsigned running, irqs, attached, late, rendered, phase;
static volatile uint8_t boot_midi_ready;
static unsigned adopted, restore_checks, early_uart, ready_uart;
static uint32_t fm1_ms, t5_nested_ticks;
static struct { unsigned timer_irqs, nested, in_audio; } felucca_dbg;
static struct { unsigned buttons; } fm1_in;
static unsigned boot_clean = 1;
void fm1_timer5_irq(void);
static void scan_ten_ticks(void) { for (unsigned i = 0; i < 10; i++) fm1_timer5_irq(); }
static uint32_t fm1_ticks(void) { return (uint32_t)now; }
static void fm1_timer5_ack(void) {}
static void fm1_input_tick(void) {}
static void usb_poll(void) {}
static void uart_midi_poll(void) { if (!adopted) early_uart++; else ready_uart++; }
static void panel_setup(void) {}
static void settings_save(void) {}
static void autosave_boot(unsigned clean);

static void sync_dma(void) {
    if ((con0 & 0x800u) && !running) {
        running = 1; dma_start = now; next_half = now + PERIOD;
    }
}
static void render(void) {
    uint64_t before = (now - dma_start) / PERIOD;
    now += 680u * 24u;
    if ((now - dma_start) / PERIOD != before) late++;
    rendered++;
    while (next_half <= now) next_half += PERIOD;
}
static void advance(uint32_t ticks) {
    sync_dma();
    if (!running || !irqs) {
        if (running) blackout_ticks += ticks;
        now += ticks; return;
    }
    uint64_t target = now + ticks;
    while (next_half <= target) {
        now = next_half; next_half += PERIOD; render();
    }
    if (now < target) now = target;
}
static void fm1_delay_ms(uint32_t ms) { advance(ms * 24000u); }
static void fm1_irq_attach(unsigned n, void (*isr)(void), unsigned prio) {
    (void)isr; (void)prio; if (n != 11 || irqs || (con0 & 0x800u)) attached = 99; else attached++;
}
static void fm1_irq_enable_all(void) {
    sync_dma(); irqs = 1;
    if (running && now >= next_half) render();
    scan_ten_ticks();
}
static void master_boot(void) {}
static void uart_midi_init(void) {}
static void boot_leds(void) {}
static void timer5_start(void) {}
static void fm1_guard_lock_top(void) {}
static unsigned kb_boot_hold;
static int32_t abuf[2u * HALF_WORDS];
static uint8_t audio_nonzero[2];
static void isr_alnk0(void) {}
static unsigned sie_failed, usb_cdc_on;
static struct { unsigned timeouts, up; } usb;
static uint8_t ep0buf[64];
static void autosave_boot(unsigned clean) {
    scan_ten_ticks();
    restore_checks += clean && !usb.up && !boot_midi_ready && !early_uart;
    adopted = 1;
}

#define S_POWER 0
#define S_INTRUSBE 1
#define S_INTRTX1E 2
#define S_INTRTX2E 3
#define S_INTRRX1E 4
#define S_INTRRX2E 5
static void fm1_usb_reset(void) {}
static void fm1_usb_attach(void *p) { (void)p; advance(phase); }
static void sie_wr(unsigned reg, unsigned value) { (void)reg; (void)value; }
static unsigned fm1_usb_sie_on(void) { return 1; }
static void fm1_usb_ep_enable(unsigned mask) { (void)mask; }
'''
    suffix = r'''
int main(void) {
    unsigned fails = 0, late_total = 0;
    for (unsigned i = 0; i < 32; i++) {
        con0 = con1 = con2 = con3 = 0; memset(gpio, 0, sizeof gpio);
        now = dma_start = next_half = blackout_ticks = 0;
        running = irqs = attached = rendered = 0;
        boot_midi_ready = adopted = restore_checks = early_uart = ready_uart = 0;
        usb.up = 0;
        /* Existing lifetime diagnostic history must not be cleared/ignored. */
        late = 7; phase = (PERIOD * i) / 32;
        shipping_boot();
        scan_ten_ticks();
        unsigned added = late - 7;
        printf("phase=%u blackout_ticks=%llu additional_late=%u rendered=%u attached=%u restore_checks=%u early_uart=%u ready_uart=%u\n",
               i, (unsigned long long)blackout_ticks, added, rendered, attached, restore_checks, early_uart, ready_uart);
        late_total += added;
        if (blackout_ticks || late != 7 || !rendered || attached != 1 || restore_checks != 1 || early_uart || !ready_uart) fails++;
    }
    printf("startup DMA/IRQ: phases=32 failed=%u additional_late=%u; simulated hardware only\n", fails, late_total);
    return fails ? 1 : 0;
}
'''
    prefix = prefix.replace('#define NTRK 8', f'#define NTRK {args.tracks}')
    code = prefix + bodies + '\nstatic void shipping_boot(void) {\n' + boot.group() + '\n}\n' + suffix
    generated = args.out / 'audio-startup-order.c'
    generated.write_text(code)
    binary = args.out / 'audio-startup-order'
    command = shlex.split(os.environ.get('CC', 'cc')) + [
        '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
        '-Wno-unused-variable', '-O1', '-g', '-fsanitize=address,undefined',
        '-fno-sanitize-recover=all', str(generated), '-o', str(binary)]
    build = subprocess.run(command, capture_output=True, text=True)
    (args.out / 'compile.log').write_text(build.stdout + build.stderr)
    result = None if build.returncode else subprocess.run([str(binary)], capture_output=True, text=True)
    (args.out / 'run.log').write_text('' if result is None else result.stdout + result.stderr)
    receipt = {
        'status': 'PASS' if result is not None and result.returncode == 0 else 'FAIL',
        'scope': 'Shipping HAL functions and main startup sequence; simulated registers, DMA and IRQ only. Not physical late-event attribution or qualification.',
        'source_root': str(args.source_root), 'tracks': args.tracks,
        'compile': command, 'compile_exit': build.returncode,
        'run': [str(binary)], 'run_exit': None if result is None else result.returncode,
        'inputs_sha256': {name: hashlib.sha256(text.encode()).hexdigest() for name, text in sources.items()},
        'generated_sha256': hashlib.sha256(code.encode()).hexdigest(),
    }
    (args.out / 'results.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))
    return 0 if receipt['status'] == 'PASS' else 1

if __name__ == '__main__':
    raise SystemExit(main())
