# Read-only clock comparison (D8C1)

This developer diagnostic closes a measurement-method gap: D8T2 reports nominal
24 MHz units, but does not measure its TIMER4 reference against the host. It does
not establish complete audio interrupt timing or stack safety.

`timing clock` exists only when `DABBL8_TIMING_DIAGNOSTIC=1`. In the main loop it
reads TIMER4 configuration, period, one raw counter value, boot count, USB reset
count and UAC profile. It checks the configuration/epoch again before formatting.
It does not mask interrupts, change timer registers, reset diagnostics, touch
project state or add work to an audio/TIMER5 interrupt. There is no new persistent
state, pool allocation or background collection.

The exact response is one line followed by the ordinary console prompt:

```text
d8c1 TICKS BOOTS USB_RESETS TIMER_CON TIMER_PRD UAC CRC32
```

All seven fields are eight uppercase hex digits. CRC32 covers little-endian words
`0x44384331` followed by the six values. The host requires one exact command echo,
complete frame and prompt; extra, stale, malformed or truncated input refuses.
TIMER_CON's low byte must be `0x09` (running, OSC, /1), period `0xFFFFFFFF`.
Pending/status bits outside that byte are not interpreted as clock settings.

## Host bounds

`tools/dabbl8_clock_capture.py` defaults to a no-access plan. It checks the local
package hash and requested version. Actual capture additionally requires that
the diagnostic manifest pins the reviewed clock header and enables timing mode;
an older package refuses before opening a port. The original frozen v023 package
does **not** contain D8C1. Do not install it merely to try this measurement.

After separate authorization, the fixed command profile permits only `status`
and `timing clock`. A fresh matching greeting/status, three samples five seconds
apart and final stable status form one bounded run. There are no retries. Raw
traffic and primary/close errors are retained; unsuccessful close makes the
receipt unsuccessful. A successful rate comparison still records
`hardware_qualified:false` and `device_binary_identity_attested:false`.
Both status observations require healthy flash/trace and zero late-audio events.

For two request/reply brackets `[send1, receive1]` and `[send2, receive2]`, the
actual sample separation lies between `send2-receive1` and `receive2-send1`.
With unsigned counter difference `D`, the rate interval is:

```text
lower = (D - 1) / (receive2 - send1)
upper = (D + 1) / (send2 - receive1)
```

Times are in seconds. One tick accounts for counter quantization. All pairwise
intervals must intersect; output integer Hz rounds outward. The method does not
assume symmetric USB latency or an exact midpoint timestamp. It refuses short or
overlapping brackets, replies wider than one second, changed boot/USB/profile,
unsupported timer setup, contradictory observations and an ambiguous full wrap.
One boundary crossing of the 32-bit counter is supported.

The explicit admission assumptions are a stable TIMER4 between 1 and 100 MHz,
an uninterrupted host monotonic clock, no host sleep, and a fresh request/reply
association. This broad rate band bounds wrap ambiguity; it is not measured
hardware data. A comparison is relative to the Mac's clock. Host oscillator
accuracy, temperature drift and traceable SI calibration remain separate evidence.
Do not rename a narrow interval into full clock/deadline qualification.

## Local checks and current component

Inspect `ci/local_checks.py --check clock-capture` before adding `--run`. It runs
actual C parser/formatter/CDC slices in optimized and fatal ASan/UBSan modes,
enabled/disabled; pure deterministic tests cover timing bounds, asymmetric delays,
wrap, bad frames/configuration, epoch changes, no-access plans and cleanup errors.
The actual C frame is decoded by the host parser. All clocks/transports are fake.

The focused target application at `build/app-only/full/run-ki6fjgj1/` passes the
unchanged budgets: image 527,248 bytes, RAM 94,812/98,304, pool 335,696/344,064.
The new main-loop snapshot uses no extra static RAM/pool. The linked conditional
model gives startup/main 7,516 bytes, audio 2,196 and TIMER5 200. Those remain
software-frame contributions, subject to the ABI/implicit-frame limitations in
the RC status. The prior frozen package is untouched; this is a separate,
non-installable component, not a replacement installer or release.

Reuse the 84 engine/effects scenarios and prior physical persistence evidence.
Before another consolidated installer, resolve omitted interrupt entry/recorder/
exit and arrival-latency measurement, stack ABI and independent recovery. Use
one combined owner session when those methods and the new package are reviewed.
