# Project brief

Dabbl8 is a proposed GPL-3.0-only Felucca fork for the M-VAVE FM-1. Its first differentiator is eight independently sequenced internal tracks, with a predictable shared audio budget and a fast performance interface. “One firmware that rules them all” is a direction, not a promise that every feature or every engine combination can run simultaneously.

The recommended first release keeps Felucca’s synthesis identity and adds arrangement power. It does not attempt Groove OS’s advertised voice count or six-effect chain before measuring the hardware. Eight logical tracks and eight sounding voices are different capacities. A drum kit can consume several voices by itself; a four-note chord leaves fewer for the rest of the arrangement.

Current work is research and planning only. Public repositories were cloned and inspected; no GitHub fork was created, no code was modified in those repositories, no target firmware was built, and no device was flashed. Source inspection demonstrates structural problems, not measured runtime performance. Desktop setup and a pristine upstream build come next.

Proposed first-release contract: eight track identities; one engine and independent sequence per track; 64-step grid; inherited probability, ratchets, chords and automation; eight shared sounding voices initially; explicit voice stealing; mixer and mute performance view; MIDI routing for tracks 1–8; a new project format; import of supported four-track Felucca projects; matching editor and backup tools. Availability of every existing engine in eight-track mode remains a memory and deadline gate.

Deferred features: expanded polyphony, multichannel USB audio, richer scene storage, acid synthesis, long event recording, extensive undo, additional effects and Bluetooth. Neither arbitrary third-party patch banks nor vendor firmware will be redistributed.

Success means an eight-track reference song plays reliably while navigating, receiving MIDI, automating parameters and streaming stereo USB audio. It also means power-loss-safe saves and a tested recovery procedure. Sound quality and usability matter more than accumulating a long engine list.
