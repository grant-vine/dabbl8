<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Application stack watermark diagnostic

`D8_STACK_WATERMARK_DIAGNOSTIC` defaults to `0`. Only the literal values `0`
and `1` are the supported inputs for the C and assembler preprocessing paths.
Recorded rejection cases include `0+0`, `1+0`, `0&&1`, `1<<0`, parentheses,
empty values and expressions using unresolved `banana` identifiers. Token
pasting at both ends selects a callable sentinel, so ordinary malformed
expressions fail preprocessing. These configuration checks are not a security
boundary or proof about every possible C preprocessor input. Enabling it
for `FELUCCA_LOADER=1` is rejected. This is a local diagnostic capability;
qualifying the source does not authorize installation or device operations.

An enabled application startup colors both existing linker-defined stack
windows with the same 32-bit pattern. Painting occurs after `cli` and stack
selection, before the first C call. The loop uses only scratch registers
`r1`–`r3`, preserves the boot argument in `r0`, and excludes the guard bands,
NOINIT, mailbox and vector table. It adds no C call or stack push. There is
no active-stack repaint, reset or generation-reset API.

| Window | Low, inclusive | Top, exclusive | Capacity |
| --- | --- | --- | --- |
| User | `0x01C74100` | `0x01C7A000` | 24,320 bytes |
| Supervisor | `0x01C7A100` | `0x01C7C000` | 7,936 bytes |

These names describe linker windows. The current SP/SSP/USP mode aliases are
not established by this diagnostic. The official pinned AC79 SDK describes
32-bit stack entries and downward growth in `include_lib/system/os/os_cpu.h`;
the application already selects both stack tops in `firmware/crt0.S`.

`fm1_stack_watermark_read()` obtains both windows from the linker and returns
separate results. A result reports bounds, capacity, surviving pattern prefix
and observed depth in bytes. Status `0` means unavailable, `1` means observed,
and `2` means invalid bounds. Disabled readout returns unavailable without
referencing linker symbols or reading memory. Before either enabled scan,
both windows must be nonzero, aligned to four bytes, ordered, disjoint and at
most 64 KiB each. That independent cap bounds the readout work; it does not
change the memory map.

The scanner reads volatile words from low to high until the first non-pattern
word, or the exclusive top. It masks no interrupts and allocates no heap.
Call it from a paused diagnostic context and bind its output to the caller's
current boot/session/source provenance. It retains no boot counter or mutable
state; the measurement starts at application entry and lasts until the next
application startup. Repeated readouts do not reset it. Compile the startup
and readout with the same flag. Enabled startup exports an absolute capability
symbol with value `1`; enabled application readout references and checks it.
An enabled readout linked against disabled startup therefore fails to link.
Disabled startup exports no extra symbol and remains byte-identical to the
baseline object. Pure window helpers are host-test seams; product readout uses
the application wrapper with this capability binding.

Observed depth is a lower bound inferred from surviving writes, **not the
maximum stack pointer displacement or proof of stack safety**. A write equal
to the pattern, restored pattern contents, or a reserved frame whose deeper
words were never written can hide use. A mismatch at the low boundary reports
full capacity but cannot establish whether an overflow occurred. Interrupts
can change memory while it is scanned, so this is not an atomic snapshot.
Readout's own C frame and caller output can themselves increase stack use.
No target timing, real workload peak, ABI mode or hardware behavior is inferred
from host fixtures or target object inspection.

Run the focused local checks without dependency downloads:

```sh
sh tests/run_stack_watermark_checks.sh "$PWD/build/stack-watermark-host"
```

Use a new output directory for every invocation; its parent must already exist.
The runner refuses an existing output directory to preserve earlier evidence.

For startup and readout objects only, add the existing cached toolchain and
offline container route:

```sh
D8_STACK_TARGET=1 \
JIELI_TOOLCHAIN=/absolute/path/to/jieli-linux-toolchains-20250324.1 \
sh tests/run_stack_watermark_checks.sh "$PWD/build/stack-watermark-target"
```

The target route first inspects the cached image, requires Linux/amd64,
records its immutable image ID, then invokes that exact ID with `--pull never`
and networking disabled. To inspect this route without compiling or running
tests, append `--target-plan` to the target invocation, using a new output
directory. Its `STACK_WATERMARK_TARGET_PLAN_ONLY` result is not a test pass.

The runner records argument vectors, logs and exits for optimized and strict
ASan/UBSan host tests with the flag unset, off and on; invalid flags and loader
enablement; and optional target compilation. Host fixtures cover independent
windows, pattern collisions, full/unwritten windows, invalid pairs and
read-only pages with inaccessible boundaries. The target checks compare
baseline/unset/off startup objects byte for byte and retain enabled object and
focused linked-startup disassembly, a matching enabled link and a rejected
C-on/startup-off link. The link uses the unchanged application
map and a non-returning stub; it is not a firmware/package build.
