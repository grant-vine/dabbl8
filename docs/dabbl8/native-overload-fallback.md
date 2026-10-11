# Native overload fallback

Eight held lead/bass notes can protect the entire shared eight-voice budget. Main before this fix can request shedding repeatedly while retiring none. The native-only fallback reuses admission’s oldest-protected choice after the existing released-tail and extra-held-note choices fail, and only after two consecutive completed halves exceed the existing 85% threshold. Continuing overload can retire further notes; recovery restores protection. Auxiliary IRQs and isolated slow halves do not activate the fallback.

Native retirement is immediate, as for admission; it does not add an overlapping ninth fade voice. Smooth audible transitions remain a release gate. Four-track firmware retains its original protection and fade policy.

Focused actual-ISR tests use a simulated clock/DMA. Four-track tests pass 24 assertions and native tests pass 66 in normal and inherited ASan/UBSan builds, covering sustained overload, mixed MONO/LEGATO/POLY leads, UNISON, a POLY bass at a nonzero slot, release/extra priority, recovery, auxiliary IRQs and PHYS note-off/retrigger. The regression fails against unchanged native main at the first expected retirement. The initial priority fixture released an unstarted envelope before rendering; it failed and was corrected to release a live envelope after the two overload halves. Its raw failure remains in evidence.

Four fresh pinned target builds show four-track app/object/ELF byte identity. Native app grows by 80 bytes (480,220 to 480,300), with RAM 96,068/98,304 and pool 335,240/344,064 unchanged. The upstream static IRQ score changes from 55,722 to 133,953; UAC remains 555. Both native builds fail the unchanged limits. Static backward-branch spans are not measured elapsed cycles; no budget was raised and no target timing gain is claimed. Whole-device timing, listening, memory high-water and recovery remain unverified.

Raw focused/negative/target logs, exact pins, source hashes and independent source review are in [the evidence directory](evidence/2026-10-11-native-overload-fallback/). Full hosted validation is pending; this fix does not qualify a firmware release or installation.
