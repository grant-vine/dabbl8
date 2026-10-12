<!-- SPDX-License-Identifier: GPL-3.0-only -->
# FM-1 firmware comparison and feature-preserving playback direction

Research observed 12 October 2026. The inventory covers 31 discovered public
repository entries (29 accessible pins, two inaccessible entries), plus six
stock/proprietary classification records with overlap. Discovery used the
[community index](https://github.com/cicloid/awesome-fm-1), primary author pages
and exact source snapshots. It is a bounded inventory of discoverable projects,
not a claim that every private or newly published fork is known. No donor builds,
firmware binaries, device operations or external repository scripts were run.

## What eight tracks establishes

Track count is arrangement capacity, not unrestricted synthesis capacity.
Keep separate the shared sounding-voice limit, heavy-engine arenas, drum retrigger
rules, sample rate, shared versus per-track effects and physical qualification.
A simulator demonstrates workflow and functional behaviour, not target deadlines.

| Family and observed version | Processing approach | Practical evidence and trade-offs |
| --- | --- | --- |
| [GrooveOS 1.3](https://www.groove-os.com/features) | Up to eight tracks; stock FM and VA; shared 12-FM/8-VA pools, shared reverb/delay plus per-track inserts | Published product specifications. Drum pads self-choke; FM kits share a six-voice ceiling. This is not Felucca's broad engine collection; internal CPU/RAM costs are unavailable. |
| [Baud Girl FM-1+VA](https://baudgirl.com/work/FM-1+VA) | Stock-derived FM plus VA/8-bit, navigation and drum behaviour | Stable97/beta98 observed. Eight independent tracks and exact voice limits are not established by the reviewed documentation. |
| [Felucca 1.5.1](https://github.com/hugelton/Felucca/tree/ef1f561b6b105f20350f6b1eac0dc0789a357f00) | Four tracks/eight shared voices; broad engine collection, silent-track return after tails, shared buses | Seven commits beyond the audited baseline, with substantive source changes. New insert/HALL/motion work is not silently inherited by Dabbl8. |
| [SLOOP 2.6](https://github.com/isod89/sloop-fm1) | Three synth tracks plus drums, pooled heavy states, shared effects and track insert | Extensive performance features; different workload from eight general synth tracks. Source reviewed, no independent deadline measurement here. |
| [FM1 MOVE](https://github.com/Nicodesy06/FM1-MOVE) | Eight interchangeable tracks/eight shared voices, three heavy arenas, sparse clip storage | Author explicitly states no hardware testing. Admission handles all-eight-protected voices; its overload helper still needs review for that case. |
| [NoteSorcery 0.1](https://github.com/catacombius/NoteSorcery) | Six synth plus two drum tracks; narrower engines and sample capacity | Explicitly host-only. Does not demonstrate unrestricted eight-track hardware synthesis. |
| [Hortator](https://github.com/DeadActive/hortator) | Eight drum-model tracks, bounded ACID use, choke/duck and shared effects | Useful rhythmic workload, not eight general polyphonic engines; hardware qualification is limited. |
| [JIANT 0.6.55](https://github.com/juanjiant-bit/JIANT/tree/e09ff368a7d7e5b048cb0291647b0290e2335f5e) | Six tracks/eight shared voices, measured-load hysteresis, bounded FM6 and resource modes | Removes user samples for storage. Changelog documents clicks from aggressive tail stealing/spike-triggered shedding; calibrated hysteresis matters. Installer0.6.11 and cached0.6.31 are stale. |
| [Melodee 1.2.1](https://github.com/keremimo/melodee) | Weighted voice units, internal-RAM audio and guarded dual-core processing | Current source removes external distortion/slicer/chorus/delay/reverb/performance and multiple engines. Author reports second-core memory faults on some units. Wholesale adoption conflicts with preserving features. |
| [sloopDX 3.5](https://github.com/zvenson/dxsloop) | One DX7 engine, three synth parts plus dedicated drums | Author device use; full synth/drum simultaneous CPU qualification remains open. |
| [April OS](https://github.com/beashoko/april-os) | Eight sample tracks | Sample-focused workload, source/version availability unresolved; does not establish eight rich synth tracks. |
| [Optimist 0.1](https://github.com/w0ts/optimist) | Four tracks, explicit build-time engine/effect cards | One-unit reports, broad hardware/recovery qualification remains open. |

Other reviewed families include X0X, FoMni, Jangada, GHOULBOX, ChoralRoot, zp12,
ORBIT, AMB-1, FiMba, FuMi, MAKU, chord/controller/child-oriented forks and clean-sheet
experiments. Lunar is a simulator, not established installed firmware. Windsynth's
author reports a browser demonstration before device availability; its guessed
repository was inaccessible. An inaccessible entry is not classified as working.

Complete pinned repository/version/source-hash inventory and author-page evidence
are retained locally, outside tracked source. Public source/licence availability
must be checked before any donor code is adopted. Proprietary feature descriptions
supply ideas, not a licence to copy implementation or distribute binary code.

## Dabbl8 v0.1 direction

1. Retain eight tracks sharing eight sounding voices and the existing FULL engine
   collection, heavy-state pools, all current effect models and shared buses.
2. Optimize provably unused work first. The current Felucca/SLOOP/JIANT mixers
   still prepare send division/clamping when all three sends are zero. Dabbl8's
   candidate all-dry path uses the actual post-modulation sends and skips only
   unused send work; distortion, slicer, performance, modulation restoration,
   LFO clocks and resident shared-effect tails keep running.
3. Keep exact fixed-point audio/state comparisons and fresh stopped-write proofs.
   Removing tail state or bounds checks merely to lower estimates is unacceptable.
4. Retain actual render/late-block diagnostics. Do not copy another firmware's
   percentages as calibrated thresholds or infer target speed from a Mac benchmark.
5. Consider optional drum choke groups as a musical choice; allow kick/snare overlap.
   Do not reserve an entire drum track or restrict arbitrary tracks without evidence.
6. Defer dual-core/whole-audio-RAM changes until tested on available hardware.
   The current normal candidate has2220B main-RAM margin; diagnostic1084B. Linked
   size is not stack high-water, and multi-unit second-core evidence is mixed.
7. Retain three saved projects/shared autosave and three sample slots. Compression
   on save can improve storage but does not improve live synthesis deadlines;
   no new storage redesign or external-firmware migration is required for v0.1.
8. Keep optional curated profiles as fallback preparation. Do not remove engines
   or effects from the default FULL candidate solely because another project did.

This is the selected software direction pending target timing measurements,
not a hardware-qualified performance guarantee. The Felucca audit baseline remains
`276f72a4e6ea8a12499a7a6819aadf3165126755`; development source is advanced through
reviewed Dabbl8 changes, not an unreviewed upstream upgrade. Current known static
SLICE/audio-interrupt/UAC warnings remain explicit until source-specific evidence
resolves them. No golden hashes or budget thresholds are rewritten.
