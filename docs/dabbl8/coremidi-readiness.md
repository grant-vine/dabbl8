# Optional Mac MIDI environment readiness

On 2026-10-11 a separate ignored `.local-baseline/venv-readonly-midi` environment
was prepared with the existing Mac/CLT Python 3.9.6 on arm64 macOS 26.7.1. It
contains only `mido==1.3.3`, `python-rtmidi==1.5.8` and the required
`packaging==26.3`. Exact official wheel hashes and bundled dependency licences are
retained in the [compact report](evidence/2026-10-11-coremidi-readiness/report.json)
and [raw receipt binding](evidence/2026-10-11-coremidi-readiness/receipt.json).
No source-distribution build hooks, global packages or unrelated runtime packages
were installed. The first pip driver refused Python 3.9; its failure is retained.
The successful retry used the existing OS-bundled pip21.2.4 wheel temporarily to
install the reviewed hashlocked wheels offline into the isolated site-packages.
Neither pip nor setuptools was installed into that runtime environment.

The exact lazy backend selector in draft [PR #143](https://github.com/grant-vine/dabbl8/pull/143)
source `c7c44de67f84349ba6b3d8ccb622a1f09848021a` was extracted and exercised:
`mido.backends.rtmidi/MACOSX_CORE`. It was unloaded before `get_input_names()` /
`get_output_names()` and loaded afterwards, with compiled API `MACOSX_CORE`.
Those calls reported one input and one output named `FM-1`. The inspected native
module links to macOS CoreAudio/CoreMIDI/CoreFoundation and system runtime libraries.
Enumeration created and deleted temporary CoreMIDI clients; it opened no MIDI
ports, sent no messages and did not instantiate the adapter's `ReadOnlyMidi` class.

These are **Mido-reported name counts**. Mido1.3.3's inspected `get_devices` merges
identical endpoint names. A coalesced list cannot prove one unique physical endpoint,
input/output pairing or identity; the adapter's injected duplicate-name tests are
not native collision proof. Before actual transport use, raw inventory ambiguity
needs separately reviewed refusal handling. Do not infer functional MIDI/audio,
protocol replies, capture completeness, restore, installation or timing results
from this name listing. No capture/session/device command or firmware install ran.

The original offline adapter/capture receipts remain unchanged. This separate
native import/enumeration observation does not turn them into physical capture
qualification, and production flash-write authority remains closed. Wheels,
installed libraries, complete licences, installer logs, actual endpoint output and
private backups remain outside Git. The compact metadata carries no native library,
SDK/vendor firmware, package, app or sample payload.

Mido is MIT-licensed; python-rtmidi is MIT-licensed and includes RtMidi's modified
MIT notice. Packaging is available under Apache-2.0 or BSD-2-Clause. These optional
host dependencies are not embedded in the firmware or relicensed by Dabbl8.
[Primary Mido backend documentation](https://mido.github.io/mido/backends/rtmidi.html)
and [python-rtmidi API documentation](https://spotlightkid.github.io/python-rtmidi/rtmidi.html)
describe backend API selection and enumeration; the receipt additionally binds the
actual installed wheel/source hashes rather than relying solely on current docs.
