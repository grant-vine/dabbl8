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
final generator contrast checks pass. The approved design passes the generator contrast gates; it is not device proof.

DejaVu is the owner's website font family (https://dejavu-fonts.github.io/).
The firmware may retain its current Inter Tight raster fonts. No DejaVu font
binary was downloaded or adopted for firmware; no website was edited. Evaluate
any later font change separately for240px readability, glyph coverage, licensing
and flash cost. Preserve font/dependency notices if adopted.

The preview appends DABBL DARK/LIGHT palette records, preserving all prior indices.
Dabbl dark is the default written by a new first-use installation. Existing
Dabbl8 settings retain their theme selection; firmware packages and boot art
were not rebuilt by the preview itself. The currently added track colours are derived brand tones,
not eight additional brand colours. All existing themes remain selectable.

Local preview:576 actual-renderer frames across12 palettes, two sizes, eight
selected tracks and three scenes;1344 assertionsPASS/zero simulatedDMA errors.
Fatal ASan/UBSan with inherited DSP overflow/shift exclusions. The final native renderer passed56064 frames with0 layout, ink, neutral, alignment
or lost-declaration errors and1536 bank-label checks. New-install foundation
passed1053687 assertions including write cuts/reboots and the dark saved default.
Actual device contrast/redraw timing remain pending.
Prior palette/UI CI mapping is absent; this focused check was run explicitly.
No hosted checks, flash, release, website edit or golden/budget changes.

The compact menu labels are `DABBL D` (dark) and `DABBL L` (light).
Long labels collided with COLOR in the large-font menu; shorten only the labels,
retaining the approved colours and saved palette IDs.

Final target producer `b8eb7af2d205430d4be7693fc4017c6ce35cffbf`:518084-byte app,
SHA256 `aa6f9b47044ad990807d9b0fadc44ff0702806acd3c948e6a53d930cf3a737e0`.
RAM96084/98304 (2220 free), pool335668/344064 (8396 free), unchanged. Known static
SLICE517/453, ALNK55966/37502 and UAC562/504 still fail; no thresholds rewritten.
The inherited banked-test final phrase "shipping remains four tracks" is stale;
its asserted fixture is eight tracks/eight voices and no device claim is made.
The final follow-up changes documentation only, leaving target inputs identical.
