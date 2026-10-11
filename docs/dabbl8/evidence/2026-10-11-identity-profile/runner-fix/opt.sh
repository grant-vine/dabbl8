set -eu
CC="clang -O2"
OUT="/Users/grantv/Code/dabbl8/.local-baseline/native-identity-runner-fix-check/opt"
mkdir -p "$OUT"
run(){ title="$1"; shift; echo "$title"; "$@"; }
for identity in upstream dabbl8 loader; do
    identity_flags=""
    [ "$identity" = upstream ] || identity_flags="-DDABBL8_IDENTITY=1"
    [ "$identity" != loader ] || identity_flags="$identity_flags -DFELUCCA_LOADER=1 -DFELUCCA_USB_PID=2"
    $CC $identity_flags -o "$OUT/d8identity_usb_test" tests/d8identity_usb_test.c
    run "Dabbl8 identity descriptors: $identity" "$OUT/d8identity_usb_test"
done
