# Desktop first build

Do not flash in this session. Confirm owner, folder, macOS, Docker and device revision first.

The fork exists at grant-vine/dabbl8. Fetch the existing dabbl8/develop branch rather than creating it again.

```sh
git clone https://github.com/grant-vine/dabbl8.git Dabbl8
cd Dabbl8
git remote add upstream https://github.com/hugelton/Felucca.git
git fetch upstream --tags
git switch --track origin/dabbl8/develop
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install Pillow fonttools
```

Review upstream BUILDING.md and tools/get_toolchain.sh. Obtain the compiler archive deliberately, retain its hash and record the extracted compiler version. Use SDK tag AC79NN_SDK_V1.2.1_2023-12-13; record its resolved commit. Pin dependency versions after proving this baseline.

With reviewed toolchain and SDK available, configure JIELI_TOOLCHAIN and AC79_SDK to their actual task-specific paths, then:

```sh
./build.sh
tests/run_tests.sh
```

Retain logs, generated linker/map reports, app and loader/package hashes. Do not run an installer, alter golden hashes or change release identity to make the first baseline pass. Optional emulator: web/emu/build.sh, followed by python3 -m http.server -d build/emu 8790. Emulator does not qualify hardware USB or recovery.
