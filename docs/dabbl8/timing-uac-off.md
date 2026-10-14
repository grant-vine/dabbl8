<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Timing capture without USB audio

The private v0.1.5 diagnostic has USB audio disabled. Its console omits the
`uac_*` status fields. Use `--uac-disabled` explicitly when preparing timing
capture for that package; the default collector still requires enabled-audio
status fields and never infers disabled audio from missing fields.

The collector rejects any UAC status field in disabled mode and requires the
paused, CRC-validated D8T2 report to declare the same UAC profile. It retains
raw reports on profile mismatch and compares boot/USB counters before/after.
No missing counter is replaced by zero. Normal enabled-audio captures retain
their sample-rate/start-counter checks.

First run the existing CLI without `--capture` to verify the package checksum,
version and selected profile. A candidate manifest must identify the original
package source commit and exact package name/size/hash; host-tool edits do not
change the installed firmware identity. The source/version and package checks
are observations, not a cryptographic readback of the device application.

Actual collection requires an explicit physical port, new output directory
and `--capture`. It sends only status and timing start/stop/report commands;
timing start changes diagnostic RAM counters, not firmware or storage. Do not
label a plan-only or simulated run as physical measurement. Keep workload
details, raw frame, journal, host elapsed interval, counter/profile checks and
cleanup result together. Observer overhead, nominal clock and histogram-bin
percentile bounds still limit interpretation. A UAC-off run cannot qualify USB
audio, both sample rates, full engine combinations or release timing.
