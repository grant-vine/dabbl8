# Experimental Dabbl8 identity profile

`tools/build.py --dabbl8 0.1-dev` explicitly selects eight sequencer tracks,
eight shared sounding voices and coordinated app identity. The canonical
version is `v0.1.0-dev`, package `dabbl8-0.1.0-dev.fwsc`, and OTA/product
marker `FM-1_8010`. This creates a local experimental package, not a published
release, and does not enable quarantined native storage writes.

The numeric convention is `FM-1_8XYZ`, where each of major/minor/patch is one
digit. Missing patch means zero. Lowercase alphanumeric suffixes of 1–8
characters identify candidates; they do not alter the numeric OTA identity.
Exact version/source/hash manifests must distinguish candidates sharing a
numeric identity. The full editor name plus version fits its 24-character
field. Invalid inputs and simultaneous upstream `--release` are refused
before dependency access. Upstream builds without this option retain their
original names, descriptors and four-track default.

The app USB product is Dabbl8, manufacturer Dabbl; editor, console, splash
and About identify Dabbl8. About retains Felucca authors, foundation,
upstream links and complete notices. Browser editor and sample upload
recognise both old Felucca and Dabbl8 port names; names grant no capabilities.
Actual native CAPS negotiation and validation remain required.

The FM-1 model prefix, package codec/marker, loader `ota-FM-1_900`,
Felucca Update descriptor and `FELUCCA-LOADER-1` signature are preserved.
App-only identity flags are not passed to loader compilation. The existing
updater checks model before installation and exact numeric identity after
restart; simulations are required to cover old/new ports, unchanged-loader
resume, wrong model, wrong post-reboot identity and missing loader marker.

USB VID 0x1209/PID 0x0001 and loader PID 0x0002 remain inherited **test IDs**,
not assigned Dabbl8 IDs. Physical USB enumeration, update/recovery, timing,
stack and second-tester acceptance remain open. This option does not satisfy
those gates, authorise flashing or authorise a public firmware download.
Keep official vendor restore firmware private and outside Git. GPL-3.0-only,
file-level notices and LICENSING.md remain authoritative.
