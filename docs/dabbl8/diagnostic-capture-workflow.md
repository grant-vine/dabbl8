# Physical diagnostic capture preparation

Prepared for issues #14 and #20. No installation or device access is authorised
by this document. Stock vendor V15 cannot provide Dabbl8 timing reports. Verify
recovery equipment and obtain separate permission before a diagnostic install.

The current local bundle is `v01-install-candidate-02c3caf`, source
`02c3caffc15714f265f1e4e8e829297c36e53eb3`. Normal RAM margin is 2,220 bytes;
diagnostic RAM margin is 1,084 bytes; both pool margins are 8,396 bytes. These
linked margins do not measure dynamic stack use. Earlier margins/build references
in timing-diagnostic.md and target-evidence-plan.md are historical.

## Collector boundaries

`tools/dabbl8_timing_capture.py` uses Python's standard library and defaults to
plan-only mode. It verifies the selected local diagnostic package checksum before
opening any port. It requires `--capture`, an explicit `/dev/cu.usbmodem…` name,
and a new output directory for device access. It does not enumerate/select a port,
open MIDI/audio, install firmware, change boot mode or issue a storage command.
Its complete command allowlist is `status`, `timing start`, `timing stop`, and
`timing report`. Collection commands change diagnostic RAM, not flash.

The native Mac adapter opens an exclusive character-device descriptor, refuses
symlink paths, configures raw CDC input and asserts DTR. Baud rate is ignored by
the firmware. Opening/configuring a tty can affect its host-side control lines;
DTR and termios observed after open are restored on close, but this is not proof
of the device's pre-open state. Native tty/modem behavior is **hardware unverified**.
Close other terminal/console applications before capture. The exclusive tty flag
blocks subsequent opens; it does not prove that no pre-existing handle exists.
A fresh diagnostic banner, exact command echoes and diagnostic version status
are required; a refused start does not stop or reset somebody else's existing run.

The collector reads status, starts a 1–60 second measurement, stops collection,
retrieves the D8T2 report and reads status again. It emits no repeated status
queries during the measurement. The user or a separate workload source operates
music and controls. Workloads should be configured before starting capture.
Stopping collection does not stop playback. Stable boot/USB configuration is
required across the outer observation window; changes cause refusal rather than
a successful combined report. CRC/format/invalid/taint/saturation checks use the
existing strict parser. Empty or four-track reports are refused.

Each run retains timestamped received/transmitted bytes and an acceptance/failure
receipt. When retrieval succeeds it also retains the report frame and parsed results
when valid. A disconnect or ambiguous
start can leave collection active; inspect explicitly rather than retrying or
assuming playback stopped. A confirmed collector-owned run gets a best-effort
`timing stop` on an interrupted session. Existing output directories are never
replaced. Captures and private workload data stay outside Git.

A local package hash and a device version string do not cryptographically attest
which binary is running. Receipts say that explicitly. Reports do not calibrate
clock, observer/wrapper overhead, full interrupt duration, peak stack, DMA deadlines,
audio quality, recovery or a release. A valid report can still show late blocks
or stream errors; accepted means valid evidence, not a passing hardware test.

## Plan-only example (safe before installation)

From the firmware checkout:

```sh
python3 tools/dabbl8_timing_capture.py \
  --manifest /Users/grantv/Code/dabbl8/.local-baseline/v01-install-candidate-02c3caf/manifest.json \
  --workload 'Planned: eight tracks, effects off, stable USB configuration'
```

Only after a separately authorised diagnostic installation and port verification,
add `--capture --port <exact-CDC-port> --output <new-private-folder> --seconds 5`.
Do not copy a guessed port or run the capture against stock firmware. The workflow
sheet is `diagnostic-workloads.json`; none of its physical rows are completed yet.

## Local verification

Scripted transport tests cover fragmented replies, strict command/echo boundaries,
identity refusal, existing-run refusal, CRC/truncation, boot/USB changes, timeout,
disconnect, bounded replies and package hash/path checks. These tests do not open
USB/serial/MIDI, establish native driver behavior or satisfy a hardware gate.
The standard local runner includes the collector tests. No firmware source,
loader, flash boundaries, target budgets or golden audio changes are needed.
