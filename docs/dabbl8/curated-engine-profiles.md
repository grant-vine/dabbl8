# Experimental curated engine profiles

These profiles select compiled renderers and shared engine-state members. They
retain eight sequencer tracks and eight shared sounding voices. They are
development configurations, without an installable release or physical timing,
recovery or persistence qualification.

| Profile | Stable engine IDs | Engines |
| --- | --- | --- |
| Core | 0, 3, 10, 12 (`0x1409`) | ANALOG, LOFI, DRUM, FM6 |
| Breaks | 0, 4, 10, 12, 13 (`0x3411`) | ANALOG, SAMPLE, DRUM, FM6, SLICE |

FULL remains the default and retains inherited FM4/SLICE build switches.
Reduced profiles require eight tracks, FM4 disabled and the stable SLICE slot
enabled. Arbitrary generated card headers and externally overridden masks are
rejected. Stable IDs, historical four-track records and protocol sentinels are
preserved. Shared sample services and historical FM4-to-FM6 conversion remain
available where needed, even when a visible renderer is excluded.

Unavailable sounds refuse adoption rather than being replaced with another
engine. Browsing hides unavailable sounds while preserving their stored records
and favourite bits. Startup and newly constructed tracks may use an available
default; that rule does not authorize replacing an existing project's sounds.

## Source-bound qualification before integration

The corrected implementation was qualified against base
`4c99a34841aae62098fc996cae81e25c81259077`. Its complete source receipt SHA-256 is
`a2b9e03650caecc595ac938ae673e502bd23b0a9f394ba06ca940e943d09c4fb`.
The staged production source on base
`795eb0189bb02b813a27c131146af2e705c202ef` has identical bytes for all 203
firmware/tool files compared with that receipt's source. Subsequent runner and
build-interface changes require their own evidence.

Corrected vendor app-only builds measured:

| Configuration | App bytes | General RAM bytes | Pool bytes |
| --- | ---: | ---: | ---: |
| FULL, eight tracks, FM4 off, SLICE on | 482000 | 96068 | 335240 |
| Core | 446404 | 90324 | 293672 |
| Breaks | 456348 | 95300 | 293672 |

Both curated profiles save 41568 pool bytes. The shared union decreases from
7020 to 1824 bytes per slot, across eight slots. Eight FULL combinations covering
four/eight tracks and FM4/SLICE switches retained the preceding implementation's
exact app, ELF, disassembly and memory outputs. This is source-specific vendor
build evidence, not a measurement of physical audio deadlines.

Fresh corrected-source host checks passed four import configurations (744 Core
and 662 Breaks assertions per normal/sanitizer run), four arrangement
configurations (83400 Core and 75060 Breaks assertions per run), and 53 retained
preset/chord audio comparisons against freshly compiled FULL8. Golden hashes
were not changed. FULL4/Core8/Breaks8 actual C bridges passed helper and mounted
companion checks. Eleven invalid configurations refused compilation; four
independent removed-guard controls produced the expected assertion failures.

The correction also has six metadata checks, four actual EDIT-knob browsing
configurations and ten negative controls (six metadata, four actual UI).
Sanitizer firmware checks retain the upstream exclusions for signed integer
overflow, shifts, bounds, object size and pointer overflow; they do not establish
strict whole-firmware sanitizer coverage.

Local immutable evidence manifests under `.local-baseline/`:

| Receipt | Entries | Manifest SHA-256 |
| --- | ---: | --- |
| `engine-exclusion-implementation-qualification/corrected-metadata-browsing-4c99` | 393 | `263295965d721c92a51ab04101cfc9c66225775693e94b58c067687435346716` |
| `engine-exclusion-implementation-qualification/corrected-source-runtime-supplement` | 327 | `349a59c9fc9138b99175812a1f7edaae98808599c39aa761180d73071abffc65` |

These are local development receipts, not release artifacts. Earlier failed
fixture setup attempts remain preserved separately and have not been relabelled
as successful executions.

## Open release gates

Static IRQ scores remain over budget: Core 59270 and Breaks 58359 against baseline
37502, with the inherited +10% tolerance. UAC scores remain 562 against baseline
504 with the same tolerance. Missing excluded-renderer diagnostics are
also retained separately. Memory savings do not prove lower worst-case CPU cost.

Remaining work includes a reproducible public app-only interface and runner,
performance fixes and on-device deadline/stack measurements, verified recovery,
per-profile package/source manifests and common persistence qualification.
No loader, flash boundary, hardware write authority or timing budget is changed
by these profiles. Track issue [#53](https://github.com/grant-vine/dabbl8/issues/53)
alongside memory task #12 and performance task #15.
