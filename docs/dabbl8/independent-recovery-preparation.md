# Independent recovery preparation — no device operations

The demonstrated Mac stock-v15 restoration remains valid evidence for recovery
when a working application can enter mask ROM. It does not prove entry from an
unresponsive application. No recovery tool, loader, USB interface or RP2040 was
opened or operated during this local preparation.

## Board identified; compatibility remains untested

The owner's [Neotronics product 979](https://neotronics.co.za/index.php?route=product/product&product_id=979)
now identifies a **Waveshare RP2040-Matrix**, product code WAVE-RP2040, with a
5×5 RGB matrix. This identifies the supplied product listing; visually confirm
the actual board/revision before wiring.

The [manufacturer's pinout](https://www.waveshare.com/wiki/RP2040-Matrix#Pinout)
labels GP0 and GP1 on the right edge by the USB connector, and GND on the left.
Its RGB matrix uses GP16. Those are GPIO names; XIAO D6/D7 labels do not apply.
These observations make GP0/GP1 a plausible porting choice. They do not establish
signal integrity, supply isolation, firmware compatibility or a working recovery.

[FM-1 Transporter's pinned instructions](https://github.com/kurogedelic/FM-1-transporter/blob/a632d923203170e3a08565f4056dbb3fd3d65ca5/README.md)
recommend Seeed XIAO RP2040, list other RP2040 boards as untested, and require
D+ on GP0, D− on GP1 and shared ground, with FM-1 battery power and **no VBUS**.
The separately unsupported RP2350-USB-A is a different board. Do not apply its
specific erratum to the owner's RP2040-Matrix.

The pinned bridge's `recovery.c` drives XIAO RGB pins 17/16/25. Matrix GP16
instead feeds addressable LEDs. A Matrix port must deliberately replace that
board-specific indicator code, even though GP0/GP1 match the proposed USB pins.
Its startup also actively emits USB_KEY pulses; merely connecting a programmed
bridge to the FM-1 is not passive discovery.

No wiring is requested or approved by this document. The exposed 5V/3V3 pins and
the Matrix board's USB connector are not substitutes for reviewed D+/D− wiring.

## Qualification sequence for a separate authorized session

1. Confirm the physical board and connector orientation against the manufacturer's
   pinout. Inspect a reviewed cable/breakout with continuity checks while
   disconnected. Confirm VBUS isolation; do not connect two USB hosts together.
2. Pin the transporter source, Pico SDK, PIO-USB dependency and loader inputs.
   Review the selected board configuration and actual firmware GPIO assignments.
   The cached Mac environment has CMake, but no ARM GCC or picotool in PATH.
   No new SDK, compiler or board firmware was installed in this preparation.
3. Build/test the RP2040 firmware offline before programming the board. The
   transporter's instructions use Pico SDK 2.2.0 and an external `wl82loader.bin`.
   Retain MIT/dependency notices. The historical loader hash is known; its
   underlying binary provenance is still unresolved.
4. With separate device authorization, first establish ROM entry independent of
   the running Dabbl8 application, then identify the chip and read/verify the full
   device image. Agree on permitted RAM upload and restart actions beforehand.
   A readback alone does not prove erase/program/verify recovery.
5. A separately authorized recovery write needs exact board/package range guards,
   per-sector verification, a complete readback and stock boot/sound/control
   confirmation. Preserve boot/calibration/non-application bytes. Do not repeat
   the historical working-console restoration merely for reassurance.

The [pinned host CLI](https://github.com/kurogedelic/FM-1-transporter/blob/a632d923203170e3a08565f4056dbb3fd3d65ca5/tools/fm1t.py)
calls `ensure_uboot` for both `info` and `dump`, and also before its write dry-run
check. These are not passive discovery commands: they can request ROM entry.
The [chip-info implementation](https://github.com/kurogedelic/FM-1-transporter/blob/a632d923203170e3a08565f4056dbb3fd3d65ca5/src/jieli_uboot.c)
calls `jl_ensure_loader`, which uploads and executes the external RAM loader.
The bridge's read handler does likewise and retries a failed read up to three
times. A future stop-on-first-failure recovery harness must address that behavior
explicitly. The host's default dry-run means no flash-sector write, not no RAM
upload or device state change. We inspected the source and did not execute it.

## Stack evidence narrowed, not closed

The exact AC79 SDK revision `d179b4484759423312073f5fbb232501aa491047`
was inspected as small read-only source files, without changing the cached SDK.
Its [os_cpu.h](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/blob/d179b4484759423312073f5fbb232501aa491047/include_lib/system/os/os_cpu.h)
defines 32-bit stack entries and downward growth. WL82
[hwi.h](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/blob/d179b4484759423312073f5fbb232501aa491047/include_lib/driver/cpu/wl82/asm/hwi.h)
identifies ALNK=11, TIMER5=63 and higher numeric priority as higher priority.
Its [irq.h](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/blob/d179b4484759423312073f5fbb232501aa491047/include_lib/driver/cpu/wl82/asm/irq.h)
saves/restores ICFG and clears bit8 to mask interrupts. These agree
with the relevant Dabbl8 source choices.

[JieLi's general MCU FAQ, item10](https://doc.zh-jieli.com/GPMCU/zh-cn/master/other/faq/index.html)
describes SSP as the interrupt stack and USP as the main stack. This is evidence
of vendor terminology, not an exact WL82 exception-entry contract. The reviewed
headers do not establish automatic bank switching, implicit frame size, nested
fault behavior or the active modes inherited by our standalone application.
Do not turn those general descriptions into a complete stack-safety claim.

Private retrieved-source hashes and pinout observations are retained under
`build/rc-clock-20261019/research/`. RP2040/failed-app recovery and full stack
qualification remain explicit release gates.
