# Rejected USB48k optimization evidence

Read [the scoped research conclusion](../../native-usb-occupancy-cost.md).
`history-scan.patch` is rejected research, not an accepted production change.
All tests and comparisons concern exact parent
`47db25dbed1057154b4917d13784999c747e2921` and that patch only.

Raw host `parent-bench.log` / `history-scan-bench.log` contain nine rounds,
20,000 blocks per row. `full=0` measures actual 32-frame taps; `full=1` also
includes actual render-start and simulated packet service. `overruns` is the
host USB ring overrun counter, not a hardware deadline result. `summary.json`
contains medians and whole direct-call native closure comparisons, without
claiming indirect call coverage or actual target execution/deadline timing.

The Mac-only optional producer probe was compiled twice, selecting each
source checkout's actual queue test header and linking that checkout's codec
and pool implementations. For reproduction in an isolated copy of the exact
parent (with its pinned generated headers available):

```sh
cc -O2 -w -Ibuild/gen -Ifirmware/src \
  '-DQUEUE_TEST_HEADER="../../../../tests/native_output_queue_test.c"' \
  firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c \
  docs/dabbl8/evidence/2026-10-10-usb-occupancy-cost/producer_probe.c \
  -lm -o /tmp/dabbl8-usb-probe
/tmp/dabbl8-usb-probe > /tmp/dabbl8-usb-parent-bench.log
/tmp/dabbl8-usb-probe /tmp/dabbl8-usb-parent-wave.bin
```

Apply the preserved patch in a **second isolated copy**, recompile to another
output path, then compare raw rounds and the generated wave files. The
production USB source in this research commit remains identical to the parent.
No golden file is generated or replaced.

The preserved source probe includes actual queue oracle checks and actual
DAC/audio IRQ runs in wave mode. The native target builds use the repository
`tools/build.py` application builder, its unchanged flags plus `-DNPART=8`,
JieLi clang4.0.1 and the pinned Docker image. Each candidate has separate
`build/usb-<candidate>-8` objects and images retained privately. The full budget
reports expose USB and inherited reverb overruns rather than suppress them.

`manifest.json` binds exact source hashes, rejected candidate hash, environment,
wave comparison and scoped evidence. Private evidence additionally retains
trial source snapshots/compiled IR, binaries/disassemblies, native logs, original
measurement helper failures and the exact parent checkout. No new CI/full
suite, browser comparison or physical qualification is claimed here.
