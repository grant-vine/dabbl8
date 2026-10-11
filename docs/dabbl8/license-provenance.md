# Dabbl8 source and dependency provenance

Dabbl8 is a GPL-3.0-only derivative of Felucca. `LICENSE` and `LICENSING.md` remain the authoritative in-tree license inventory. This ledger records source provenance; it does not establish permission for a new donor, approve a release, or replace file-level notices.

## Audited foundation and integration

The unchanged measurement baseline is Felucca v1.1.5 commit `276f72a4e6ea8a12499a7a6819aadf3165126755`. The initial integrated eight-track source was `6f73dba1576ad12bdc3fa30eac1509cd674dc0a5`, incorporated by PR #81. It deliberately retains the subsequent upstream wake/screen-off fixes; the baseline measurements were not silently upgraded. Existing source history and Leo Kuroshita / Hügelton Instruments notices remain intact.

The accompanying [source inventory](evidence/2026-10-11-license-provenance/inventory.json) audits immutable `main` commit `b2466bf9bc1c675ed8b8f667d37af8f5e4e4b377`. It records tracked C, headers, Python, shell, JavaScript, HTML and CSS in firmware, tools, tests and web, each file's SHA-256, literal SPDX declarations, and whether the file is unchanged, modified or new relative to the baseline. A missing file-level declaration is explicitly listed; the scanner does not invent one or infer donor rights. Later commits need a fresh inventory before distribution.

## Retained components

| Component / paths | In-tree declaration and origin | Notice to retain |
| --- | --- | --- |
| Firmware, build/tools, editor/emulator and host tests | Felucca and Dabbl8 GPL-3.0-only; retain existing copyright headers | `LICENSE`, `LICENSING.md`, file headers |
| `firmware/src/phys_dsp.c` | DaisySP fixed-point port, MIT; Electrosmith and Emilie Gillet, Felucca port credit | `LICENSES/MIT-DaisySP.txt` and full source header |
| `firmware/src/phys_symp.c` | Rings fixed-point port, MIT; Emilie Gillet, Felucca port credit | `LICENSES/MIT-Rings.txt` and full source header |
| `firmware/src/fm6_core.c` | msfa via Dexed, Apache-2.0; Google, Pascal Gauthier, Felucca port credit | `LICENSES/Apache-2.0-msfa.txt` and full source header |
| `firmware/src/eng_phase.c` | Felucca C port of Hügelton CrispyZebra waveforms | Existing source credit and GPL notice |
| `firmware/src/drum_voice.c`, `firmware/src/eng_drum.c`, `tools/gen_waves.py` | Felucca drum voices and generated Hügelton Sample Pack, GPL-3.0-only | Existing GPL/author notices; these drums are not CC0 |
| `assets/samples-cc0/` | VSCO-2 Community Edition / VCSL, Versilian Studios; in-tree CC0 attribution lists original files | `assets/samples-cc0/ATTRIBUTION.txt` |
| `assets/fonts/InterTight[wght].ttf` | Inter Project Authors, SIL OFL 1.1 | `LICENSES/OFL-InterTight.txt`, `assets/fonts/OFL.txt` |
| `web/fukiai.ttf` | Hügelton Fukiai icon font, MIT | `LICENSES/MIT-Fukiai.txt`, `web/FUKIAI-LICENSE.txt` |
| `web/emu/fonts/DotGothic16-subset.woff` | DotGothic16 Project Authors, SIL OFL 1.1 | `LICENSES/OFL-DotGothic16.txt`, `web/emu/fonts/OFL.txt` |
| `web/emu/` | Upstream files explicitly credit charlesvestal/fm1-x0x browser design, GPL | Existing file headers and Felucca GPL inventory |
| VOICE engine reference | Upstream credits klattsch design and Klatt/Hillenbrand formant data; upstream states no klattsch code copied | Preserve upstream credits; do not treat a reference as an imported library |

All paths above are inherited material, not new Dabbl8 engine development. The inventory pins the license/attribution texts and asset hashes against the selected source snapshot. File-level exceptions remain exceptions; the complete firmware remains GPL-3.0-only as described by upstream.

## SDK packaging inputs

The SDK is external to Git. Build evidence pins JieLi AC79 SDK commit `d179b4484759423312073f5fbb232501aa491047`. Upstream packaging reads three Apache-2.0 files; their expected SHA-256 values are enforced by `tools/build.py`:

| SDK path under `cpu/wl82/tools/` | SHA-256 |
| --- | --- |
| `uboot.boot` | `4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4` |
| `cfg_tool.bin` | `276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b` |
| `cfg/eq_cfg_hw.bin` | `41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d` |

Retain `LICENSES/Apache-2.0.txt` with every package. The upstream release builder copies `LICENSE`, `LICENSING.md` and the `LICENSES/` texts. The in-tree inventory is source evidence; an eventual release audit must inspect the actual emitted notice bundle, SDK inputs and matching corresponding-source snapshot. No loader changes are introduced by this ledger.

## New donor and release gates

Research into GrooveOS, BaudGirl workflows, Rust, the local wahwah projects, and optional engine/effect cards does not by itself authorize copying code, assets or presets. Before a new import, record exact donor URL and commit, copied paths, author and license notices, transitive inputs, transformations, and the PR that validates them. Retain originals' notices and corresponding source. Independently audit the actual material; names or similarities are not provenance evidence.

Do not put M-VAVE vendor firmware, private project/sample backups, credentials or private archive receipts into this ledger, Git or releases. The locally retained official restore file remains outside Git. Public size/hash verification metadata is distinct from permission to redistribute its contents.

Issue #19 remains open: coordinated package/USB/updater identity and hardware-compatible recovery still need their acceptance evidence. This ledger changes no identity, VID/PID, protocol, loader or flash boundary. Issue #20 additionally requires verified migration, hardware timing, install/import/rollback and a second tester before release qualification. A successful source scan cannot satisfy those gates.
