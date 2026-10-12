<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Dabbl firmware brand direction

Owner-supplied four-page `dabbl-brand-direction.pdf`, received12 October2026,
is preserved unchanged in the local brand evidence directory. The guide is design
reference, not permission for device operations or website deployment.

| Brand colour | Source hex | Firmware role |
| --- | --- | --- |
| Golden yellow | #F4C542 | Primary highlight in dark theme; restrained emphasis |
| Warm cream | #F6F0E4 | Dark-mode text and light-mode background |
| Deep teal | #176B73 | Contrast and graph detail; accessible darker/lightened tones as needed |
| Soft charcoal | #252724 | Dark-mode background and light-mode text |
| Coral | #E77B64 | Active detail, supporting track identity |
| Berry | #A4406A | Supporting identity and light-mode interaction accent |

Keep the original Dabbl wordmark and separate Dabbl8 derivative. Avoid rainbow
lettering; keep numerals, bank labels, mute/arm/record labels and selection shape
alongside colour. Golden yellow must not carry cream small text. Dark and light
colours must be checked after RGB565 quantisation and actual renderer blends.
The first light-mode contrast check failed for cream ink on the blended teal
selection (3.79<4.5). Darken that teal tone instead of relaxing the threshold;
final generator contrast checks pass. This is a draft preview, not device proof.

DejaVu is the owner's website font family (https://dejavu-fonts.github.io/).
The firmware may retain its current Inter Tight raster fonts. No DejaVu font
binary was downloaded or adopted for firmware; no website was edited. Evaluate
any later font change separately for240px readability, glyph coverage, licensing
and flash cost. Preserve font/dependency notices if adopted.

The preview appends DABBL DARK/LIGHT palette records, preserving all prior indices.
It does not yet change the shipped default, stored owner preferences, firmware
packages or boot art. The currently added track colours are derived brand tones,
not eight additional brand colours. All existing themes remain selectable.

Local preview:576 actual-renderer frames across12 palettes, two sizes, eight
selected tracks and three scenes;1344 assertionsPASS/zero simulatedDMA errors.
Fatal ASan/UBSan with inherited DSP overflow/shift exclusions. Full layout lint,
actual device contrast/redraw timing and final target qualification remain pending.
Prior palette/UI CI mapping is absent; this focused check was run explicitly.
No hosted checks, flash, release, website edit or golden/budget changes.
