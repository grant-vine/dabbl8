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

## Local qualification of the dry-send optimisation

Firmware producer `9be05723195d877077aa2bc809ff9c1dbdae5c2d`; firmware inputs
remain identical after the coverage/documentation follow-up. Frozen comparator
parent `8105e2aa45f40136c7a0bc64f57b509219e1a865` has the same source tree as
merged main `c95d771d0e4009dbdbd57ed39d96793177643c93`.

- Actual C mixer parent/candidate comparison: 832 cases, 40240 assertions,
  identical trace `635ecc5ecc2a8120`. Includes all 13 selectable engines, every
  track/send mask, stored-dry modulation becoming wet, all three stored-wet sends
  modulating to zero, distortion/slicer/muting and shared effect tails. A further
  96-block simultaneous-eight-voice scenario is outside the 832 case count.
- Fatal ASan/UBSan checks pass with inherited DSP signed-overflow/shift exclusions;
  this is bounded host coverage, not every preset or parameter combination.
- Existing effect-tail quiet checks and local selector/shell checks pass.
- Four alternating parent/candidate benchmark pairs, nine batches per process:
  dry track mixing 246.85 to 206.04 ns per call (-16.53%); host instructions
  -18.91%. Wet path +0.25% time/+0.09% instructions. This probe excludes shared
  effect-bus rendering and is neither total interrupt cost nor pi32v2 timing.
- Offline pinned linux/amd64/pi32v2 build: 514936-byte app (+96 bytes), SHA-256
  `ceeb1cd77b52641dc90ffde537c43bcc9feb4fcb527e76614d8a0490de0a34e1`.
  Main RAM 96084/98304 (2220 free); pool 335668/344064 (8396 free), unchanged.
- Static budget check explicitly FAILS: SLICE 517/453, audio interrupt
  55966/37502, UAC 562/504. The branch-span estimator counts both dry/wet code
  paths and the interrupt estimate rises from 55591; it does not measure CPU
  cycles or establish a regression/improvement on the device. No threshold changed.
- No device timing, stack high-water, USB stress or physical recovery test run.
  No flash performed. Existing installation packages remain the pre-optimisation
  candidates and must not be represented as containing this change.

Local raw evidence is preserved under the owner's `.local-baseline/` directory,
including source-bound build inputs, compiler/container identity, disassembly,
explicit failing budget log, comparison receipts and research snapshots.

## Full discoverable repository inventory

The following pins retain inaccessible entries as unknown, not working firmware.

| Repository | Observed source pin | Evidence |
| --- | --- | --- |
| [hugelton/Felucca](https://github.com/hugelton/Felucca) | `ef1f561b6b105f20350f6b1eac0dc0789a357f00` | Source audit; no local run of donor. |
| [isod89/sloop-fm1](https://github.com/isod89/sloop-fm1) | `8fad1a15ff59d5c3ed0bfe2d7a9ae90962cf4d5e` | README beta claims, source inspected; no independent hardware measurements. |
| [charlesvestal/fm1-x0x](https://github.com/charlesvestal/fm1-x0x) | `47f8e4c4ad602591b81c21bc5480db7a94f6d6af` | Source snapshot; independent target qualification unavailable. |
| [charlesvestal/fm1-fomni](https://github.com/charlesvestal/fm1-fomni) | `405ef41aef75f7e70afd6decc01505df07c708e9` | Source snapshot; independent target qualification unavailable. |
| [zvenson/dxsloop](https://github.com/zvenson/dxsloop) | `7dc1ca1fd29d1b5531c29b03b8dd766627201325` | Author explicitly installed/played real FM-1; full14voice worst-case CPU remains open. |
| [jasonpersinger/ghoulbox-fm1-dungeon-synth](https://github.com/jasonpersinger/ghoulbox-fm1-dungeon-synth) | `874cdf29e223349cdb34c1a74b44d42765ddd1e7` | Author tested one physical unit; no general stress qualification. |
| [Quixotic7/ChoralRootFM1](https://github.com/Quixotic7/ChoralRootFM1) | `f4026089b038d4264f9c0fc724b9a2980e9402cf` | Source snapshot; independent target qualification unavailable. |
| [DeadActive/hortator](https://github.com/DeadActive/hortator) | `7e3415971c5800b4a3ad0146a7d95615883d7d96` | README says untested on most hardware; no independent worst-case timing evidence. |
| [jadamsowers/fm1-fimba](https://github.com/jadamsowers/fm1-fimba) | `b2b647b91bb3e9f0014529a468eeb6ac65f90ad9` | Source snapshot; independent target qualification unavailable. |
| [zvenson/zp12](https://github.com/zvenson/zp12) | `ca8f0c0c65ea34164a6074b4710da87b13d2d23b` | Source snapshot; independent target qualification unavailable. |
| [dspaudio/orbit](https://github.com/dspaudio/orbit) | `4c119edd6081e1abe9d20a59d731455f8092c67e` | Source snapshot; independent target qualification unavailable. |
| [charlesvestal/fm1-amb](https://github.com/charlesvestal/fm1-amb) | `60dfbec4602497616d6a196be78e9288dc7c615a` | Source snapshot; independent target qualification unavailable. |
| [charlesvestal/fm1-ctl](https://github.com/charlesvestal/fm1-ctl) | `298b637e961bc90fcacdd768e38fd8a9b12eb55c` | Source snapshot; independent target qualification unavailable. |
| [cartesive/fumi-1](https://github.com/cartesive/fumi-1) | `c83d2c4ab1d8a2c8e0b6d5aabe8d9bf2dd81896d` | Source snapshot; independent target qualification unavailable. |
| [math0ne/fm1-chord](https://github.com/math0ne/fm1-chord) | `377235d9f39a362e673f8fd60a26f0b1748218e7` | Source snapshot; independent target qualification unavailable. |
| [ELI7VH/Felucca](https://github.com/ELI7VH/Felucca) | `66f1d2702f73e4de2319b9b0c7143519f794004e` | Source snapshot; independent target qualification unavailable. |
| [joshbartlettband-web/felucca-rainbow](https://github.com/joshbartlettband-web/felucca-rainbow) | `0d5476cc398f3f56dd7854078686cf8536c2b7be` | Source snapshot; independent target qualification unavailable. |
| [Quixotic7/PurpleMonkeyFM1](https://github.com/Quixotic7/PurpleMonkeyFM1) | `b574e75dd1dcf3283c3eb5d286fd7c6d21d95517` | Source snapshot; independent target qualification unavailable. |
| [hericdk/fm1-quest](https://github.com/hericdk/fm1-quest) | `8b4a2c20287d2a5ac132606d20195d026a1fa827` | Source snapshot; independent target qualification unavailable. |
| [zednaked/jangada](https://github.com/zednaked/jangada) | `07cce380aae0c6d61793fa1b80231dff49d3c7ee` | Source snapshot; independent target qualification unavailable. |
| [keremimo/melodee](https://github.com/keremimo/melodee) | `16daf99ea797f2e4f6b5e40065942df3648d08b9` | Author reports real second-core misread/crash cases, validated job handoff and disable/fallback; Standard/Single-core editions. Hardware variation is a reason to defer dualcore adoption. |
| [shaw-core/Sloop_ALG02](https://github.com/shaw-core/Sloop_ALG02) | `cbfcee3e79f759abffc2613b931f495d29e4c1e5` | Source snapshot; independent target qualification unavailable. |
| [friendsmakenoise-prog/fm1-pocket-sampler](https://github.com/friendsmakenoise-prog/fm1-pocket-sampler) | `unavailable` | HTTP Error 404: Not Found |
| [ip2k/lunar-modulator](https://github.com/ip2k/lunar-modulator) | `bfa042446f36c8647efddc6d76b06753d2a5b612` | Explicit firmware never installed on FM-1; playable simulator only. |
| [NOVALENTI/fm1-polyseq](https://github.com/NOVALENTI/fm1-polyseq) | `b028d3f8ebb76df2505c258ce7bb2e0f63901056` | Source snapshot; independent target qualification unavailable. |
| [w0ts/optimist](https://github.com/w0ts/optimist) | `4c2d447ed5e17447f80f6aa0da5a130212723fc4` | One physical unit since2026-10-09 boots/plays/BLE; most features host-only and recovery paths untested. |
| [catacombius/NoteSorcery](https://github.com/catacombius/NoteSorcery) | `c13ac1b63f0d87cbbeb74dda91510946724c47eb` | Explicit host-only, never tested on device. |
| [Nicodesy06/FM1-MOVE](https://github.com/Nicodesy06/FM1-MOVE) | `e6706e8e71dad8ef8e61e490248db59cbf4dffa9` | Explicit never run on physical FM-1. Host/WASM/target-compiler claims only. |
| [juanjiant-bit/JIANT](https://github.com/juanjiant-bit/JIANT) | `e09ff368a7d7e5b048cb0291647b0290e2335f5e` | Tests and device behavior claimed in changelog, not independently measured. Explicit past guard regressions cut audible tails/clicked; do not copy thresholds without hardware measurement. |
| [zvenson/windsynth](https://github.com/zvenson/windsynth) | `unavailable` | HTTP Error 404: Not Found |
| [okumurari2/maku](https://github.com/okumurari2/maku) | `1c2312a57ac9a3849bc7a5f3baaf8b3be96d2a4e` | Author hardware-verified, with remaining code-only claims marked in SPEC. |
