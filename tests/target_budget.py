#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
"""Target-side cost estimate of the hot DSP code, from the pi32v2 objdump listing tools/build.py writes
(build/felucca.dis). For each engine's render function and the audio ISR: the instructions
inside natural control-flow loops (including rotated loops), and the hardware divides and calls in them.
Backward jumps to shared exit code are not loops; unresolved flow retains conservative spans. cost = the loop instructions, each weighted 4 per
level of nesting (an inner loop runs several times per sample) and 1 + 8 for a divide (many cycles). A static count, not a cycle count: it changes only when the compiled code changes,
so it is exact run to run, and it catches a render loop that grew (more work per sample, a new divide,
something not inlined any more). Compared with tests/target_budget.txt at +10 %. The host
CPU check (regress.c) is the primary one; this one sees the target compiler's code.
Historical budgets are retained. Compare both binaries with the same checker when its model changes;
a smaller score against an old-model budget is not measured target headroom.
  tests/target_budget.py [DIS [BUDGET]]      BUDGET_UPDATE=1 rewrites BUDGET"""
import os
import re
import sys

FUNCS = ["analog_render", "digital_render", "digital_render_legacy", "digital_render_custom", "phase_render", "lofi_render", "sample_render", "formant_render",
         "trio_render", "trio_pass", "wheel_render", "wheel_block",
         "grain_render", "grain_block", "phys_render", "drum_render", "noise_render", "fm6_render", "fm6_op_run", "fm6_op_fb", "px_modal_block", "px_modal_run", "px_memb_block",
         "px_string_excite", "px_string_run", "px_symp_run",
         "dv_metal_run", "dv_kick_run", "dv_snare_run", "dv_clap_run", "dv_hat_run", "dv_tom_run",   # drum_voice.c
         "dv_rim_run", "dv_bell_run", "dv_cym_run", "dv_out", "dv_metal_mix",
         "slicer_track",
         "slice_render", "slc_rev",                          # SLICE (eng_slice.c): the render, the reverse windows
         "fm1_alnk0_irq", "fm1_timer5_irq",               # the audio ISR; TIMER5: the key / LED scan (hal/fm1_input.h)
         "uac_tap48",                                       # the USB audio input at 48 kHz: the 44.1 -> 48 resampler (usb.c)
         "click_render",                                    # the metronome's click (click.c), while it sounds
         "mod_begin", "mod_voice", "mod_end",                 # the modulation matrix (mod.c), called when active
         "perf_begin", "perf_mute", "perf_pre", "perf_block", "perf_master",   # the FX layer (perform.c), when busy
         "rev_room", "rev_spring"]                # the reverb bus (fx.c): REVERB TYPE ROOM / SPRING
# built only with FELUCCA_FM4=1 (DIGITAL, src/eng_digital.c; not in the default build, so not in BUDGET): absent,
# they are skipped; present, checked against these (their budget lines until the engine was retired in 1.0)
OPTIONAL = {"digital_render": 12, "digital_render_legacy": 333, "digital_render_custom": 558,
            "uac_tap48": 504}           # (absent from a FELUCCA_UAC_48K=0 or FELUCCA_UAC=0 build)
TOL = 0.10                      # exact (no noise): small edits pass, a grown render loop does not
DIV_W = 8                       # a divide weighs 1 + 8 instructions
NEST = 4                        # an instruction in a loop inside a loop weighs 4, two deep 16, ...
MAXD = 4

dis = sys.argv[1] if len(sys.argv) > 1 else "build/felucca.dis"
budget = sys.argv[2] if len(sys.argv) > 2 else "tests/target_budget.txt"
LABEL = re.compile(r"^([A-Za-z_][A-Za-z_0-9.]*):$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(.*)$")
TARGET = re.compile(r"goto -?\d+ <[^>]*: ([0-9a-f]+) >")


def functions(path):
    """{name: [(addr, text)]}: a function runs to the next label not starting with '.' (the compiler's
    jump-table labels are inside functions); jump-table data supplies tbb targets but is not scored"""
    out, cur = {}, None
    table_base = None
    with open(path) as f:
        for line in f:
            m = LABEL.match(line.strip())
            if m:
                name = m.group(1)
                if not name.startswith("."):
                    cur = out.setdefault(name, []) if name in FUNCS else None
                continue
            m = INSN.match(line)
            if cur is not None and m:
                text = m.group(3).strip()
                if text.startswith("<"):
                    if cur and cur[-1][1].startswith("tbb "):
                        if table_base is None:
                            table_base = int(m.group(1), 16)
                        value = re.match(r"< (\d+) :", text)
                        if value:
                            address, branch = cur[-1]
                            cur[-1] = (address, branch + " target=" + hex(table_base + 2 * int(value[1])))
                        else:
                            address, branch = cur[-1]
                            cur[-1] = (address, branch + " invalid-table")
                else:
                    table_base = None
                    cur.append((int(m.group(1), 16), text))
    return out


def loop_members(insns: list[tuple[int, str]]) -> list[set[int]]:
    """Natural-loop membership, including rotated loops with forward backedges.

    Preserve old conservative spans for unresolved/computed or irreducible flow. This
    is an instruction-cost model, not a bound on target execution time.
    """
    index = {address: i for i, (address, _) in enumerate(insns)}
    heads: dict[int, int] = {}
    edges: list[set[int]] = [set() for _ in insns]
    unresolved = False
    for i, (address, text) in enumerate(insns):
        branch = TARGET.search(text)
        if branch:
            target = int(branch.group(1), 16)
            if insns[0][0] <= target < address:
                heads[target] = max(heads.get(target, address), address)
            if target in index:
                edges[i].add(index[target])
            elif insns[0][0] <= target <= insns[-1][0]:
                unresolved = True
            if not text.startswith("goto") and i + 1 < len(insns):
                edges[i].add(i + 1)
        elif text.startswith("tbb "):
            targets = [int(value, 16) for value in re.findall(r"target=(0x[0-9a-f]+)", text)]
            if not targets or "invalid-table" in text or any(target not in index for target in targets):
                unresolved = True
            else:
                edges[i].update(index[target] for target in targets)
        elif "goto" in text or text.startswith("pc =") or text.startswith("tbh "):
            unresolved = True
        elif not (text.startswith("{pc") or text == "rets" or text == "reti") and i + 1 < len(insns):
            edges[i].add(i + 1)
    if unresolved:
        return [{address for address, _ in insns if head <= address <= end}
                for head, end in heads.items()]
    reachable = {0}
    pending = [0]
    while pending:
        node = pending.pop()
        for successor in edges[node] - reachable:
            reachable.add(successor)
            pending.append(successor)
    predecessors: list[set[int]] = [set() for _ in insns]
    for node in reachable:
        for successor in edges[node]:
            predecessors[successor].add(node)
    all_bits = sum(1 << node for node in reachable)
    dominators = [all_bits for _ in insns]
    dominators[0] = 1
    changed = True
    while changed:
        changed = False
        for node in sorted(reachable - {0}):
            bits = all_bits
            for predecessor in predecessors[node]:
                bits &= dominators[predecessor]
            bits |= 1 << node
            if bits != dominators[node]:
                dominators[node] = bits
                changed = True
    # Removing all dominance backedges must leave a DAG. An irreducible
    # cycle has no single dominating header: retain the conservative model.
    forward = [{successor for successor in edges[node]
                if not dominators[node] & (1 << successor)} for node in range(len(insns))]
    incoming = [0 for _ in insns]
    for node in reachable:
        for successor in forward[node]:
            incoming[successor] += 1
    pending = [node for node in reachable if not incoming[node]]
    visited = 0
    while pending:
        node = pending.pop()
        visited += 1
        for successor in forward[node]:
            incoming[successor] -= 1
            if not incoming[successor]:
                pending.append(successor)
    if visited != len(reachable):
        return [{address for address, _ in insns if head <= address <= end}
                for head, end in heads.items()]
    loops: dict[int, set[int]] = {}
    for node in reachable:
        for header in edges[node]:
            if dominators[node] & (1 << header):
                members = loops.setdefault(header, {header})
                pending = [node] if node != header else []
                while pending:
                    member = pending.pop()
                    if member not in members:
                        members.add(member)
                        pending.extend(predecessors[member] - members)
    return [{insns[node][0] for node in members} for members in loops.values()]


def cost(insns):
    """Instructions in natural loops retain the original nesting/divide weights."""
    loops = loop_members(insns)
    n = divs = calls = 0
    w = 0
    for a, t in insns:
        d = min(sum(a in members for members in loops), MAXD)
        if not d:
            continue
        k = NEST ** (d - 1)
        n += 1
        if re.search(r"= r\d+ / r\d+", t):
            divs += 1
            k *= 1 + DIV_W
        calls += t.startswith("call")
        w += k
    return {"insns": len(insns), "loop": n, "div": divs, "call": calls, "cost": w}


def main():
    if not os.path.exists(dis):
        print(f"target: skip ({dis} missing: run ./build.sh)")
        return 0
    fns = functions(dis)
    res = {n: cost(fns[n]) for n in FUNCS if fns.get(n)}
    missing = [n for n in FUNCS if n not in res and n not in OPTIONAL]
    base = {}
    if os.path.exists(budget):
        for line in open(budget):
            p = line.split()
            if len(p) >= 2 and not line.startswith("#"):
                base[p[0]] = int(p[1])
    if os.environ.get("BUDGET_UPDATE"):
        with open(budget, "w") as f:
            f.write("# FELUCCA target cost budget (tests/target_budget.py): instructions in the loops of each\n"
                    f"# function in build/felucca.dis, x{NEST} per nesting level, divides x{1 + DIV_W}. The check allows "
                    f"+{TOL * 100:.0f} %.\n# Rewritten by BUDGET_UPDATE=1.\n")
            for n, r in res.items():
                if n not in OPTIONAL:
                    f.write(f"{n} {r['cost']}\n")
        print(f"target: budget {budget} rewritten ({len(res)} functions)")
    fail = 0
    for n in missing:
        print(f"target: FAIL {n} not found in {dis} (renamed? inlined? update FUNCS)")
        fail += 1
    for n, r in res.items():
        b = base.get(n, OPTIONAL.get(n))
        state = "no budget (BUDGET_UPDATE=1 adds it)" if b is None else "ok"
        if b is not None and not os.environ.get("BUDGET_UPDATE"):
            if r["cost"] > b * (1 + TOL):
                state = f"OVER BUDGET (+{(r['cost'] / b - 1) * 100:.0f} %, limit +{TOL * 100:.0f} %)"
                fail += 1
            elif r["cost"] < b * (1 - TOL):
                state = f"note: {(r['cost'] / b - 1) * 100:.0f} % (BUDGET_UPDATE=1 to keep it)"
        print(f"target: {n:15s} {r['insns']:5d} instructions, {r['loop']:4d} in loops, {r['div']} divides, "
              f"{r['call']:2d} calls there: cost {r['cost']:5d} (budget {b if b is not None else '-'}) {state}")
    return 1 if fail else 0


if __name__ == "__main__":
    sys.exit(main())
