/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* FM-1 standalone app entry. The SPL maps app.bin
 * at XIP 0x02000120 and jumps to its first byte with r0 = boot-parameter block
 * (0x01C7FE08, inside the vector table we are about to own). The SPL expects
 * `goto +4; rts` as the first instruction. Before anything else the core state
 * the SPL left is saved for the boot probe; then interrupts off, our stacks. */
#include "hal/fm1_stack_watermark.h"
	.section .text.crt0,"ax",@progbits
	.globl _start
_start:
	goto 1f
	rts
1:	r1 = icfg
	r2 = psr
	cli
	r3 = 0x1C07F00		/* STATUS (debug area) */
	[r3 + 0x20] = r1	/* icfg as handed over */
	[r3 + 0x24] = r2	/* psr */
	[r3 + 0x28] = r0	/* boot-parameter pointer */
	r1 = sp
	[r3 + 0x2C] = r1
	r1 = ssp
	[r3 + 0x30] = r1
	sp = _ustack_top
	ssp = _sstack_top
#if D8_STACK_WATERMARK_DIAGNOSTIC
	.globl d8_stack_watermark_startup_enabled
	.set d8_stack_watermark_startup_enabled, 1
	/* Interrupts off, no app frame yet: color only the two stack windows.
	 * r0 remains the SPL boot argument; no call, push or later repaint. */
	r1 = _ustack_lo
	r2 = _ustack_top
	r3 = FM1_STACK_WATERMARK_PATTERN
2:	[r1] = r3
	r1 += 4
	if (r1 < r2) goto 2b
	r1 = _sstack_lo
	r2 = _sstack_top
3:	[r1] = r3
	r1 += 4
	if (r1 < r2) goto 3b
#endif
	call fm1_cstart
	goto .
