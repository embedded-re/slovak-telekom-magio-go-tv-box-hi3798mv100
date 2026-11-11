# 5. Drivers, one part at a time

Each part followed the same loop: find the registers in the vendor SDK (or in the stock kernel when the
SDK didn't have them), try them by hand with `devmem` from Linux, then write the device tree entry or a
small driver, and test from RAM before flashing. All addresses are in [`hardware/HARDWARE.md`](../hardware/HARDWARE.md);
all code is in [`port/kernel/linux-6.6-hi3798mv100.patch`](../port/kernel/linux-6.6-hi3798mv100.patch).

## Storage at full speed

The eMMC worked from the start, but only at low speed: faster modes gave read errors. The cause was a
wrong number in my device tree. The controller's input clock is **100 MHz**, not 50, so every speed Linux
asked for came out twice as fast as intended; the "errors at high speed" were errors at double the rated
speed. With the right clock, the eMMC runs in DDR52 mode: **79 MB/s** read instead of 42. I checked every
read and write against the storage copy from [step 3](03-emmc-backup.md).

## Network

The Ethernet block is a variant of one mainline Linux already supports (`hisi-femac`). It needed two small
driver changes and the right addresses. The bootloader sets up the network chip on every boot, so Linux
doesn't have to. Result: **94 Mbit/s** each way, the limit of 100 Mbit Ethernet ([step 6](06-performance.md)).

## USB

The USB controllers are standard, but their PHY (the analog part that drives the wires) isn't: it isn't
even in the memory map, the addresses I first tried froze the whole chip. The vendor driver programs it
through clock registers and an indirect "test bus". I wrote a PHY driver that does the same
(`phy-hi3798mv100-usb2.c`). The box has a single USB port; the second USB2 controller and the USB3
controller (which also works, with a small start-up driver, `xhci-hi3798mv100.c`) have no external port.

## All four cores

The vendor kernel starts the other cores with a few writes to reset registers plus a small jump
instruction at a fixed address. I reproduced that (`platsmp.c`); CPU hotplug works too. Lesson learned:
the vendor also switches the CPU clock to 400 MHz during start-up, and copying that broke the network
until the next reset.

## Timers

Without the CPU's own timer ([step 4](04-mainline-linux.md)), Linux needed one timer per core from the
chip's timer blocks. A small driver (`timer-hi3798mv100.c`) gives each core its own, so the kernel can
sleep idle cores and use high-resolution timers.

## Watchdog and random numbers

- **Watchdog:** a standard ARM block (SP805). If Linux stops responding for 30 seconds, the box resets.
- **Random numbers:** the chip's hardware generator works with an existing mainline driver at the address
  from the SDK's crypto driver. Linux has good random numbers 0.85 s after boot instead of 8.7 s.

## Temperature and CPU speed

The temperature sensor isn't documented anywhere in the SDK. I found it in the stock kernel binary, in the
function the vendor uses to read the temperature, and turned that into a driver
(`hi3798mv100_thermal.c`). A CPU frequency driver (`hi3798mv100-cpufreq.c`) switches between 800 MHz and
1.2 GHz. Together, Linux slows the CPU down at 75 °C and shuts down at 95 °C. How much cooling the box
needs is in [step 6](06-performance.md).

## GPU

The Mali-450 is supported by the mainline `lima` driver; what was missing was turning it on. A small driver
(`hi3798mv100-gpu.c`) switches on its clocks, releases its reset, sets up its power control the way the
vendor driver does, and changes its speed (200–432 MHz). I also brought in fixes from newer kernels that
let the GPU recover after a stuck job.

With Mesa, OpenGL ES 2.0 works. Measured limits: no OpenGL ES 3, no compute shaders, and fragment
shaders compute in 16-bit floating point. Test programs: [`extras/gpu/`](../extras/gpu/).

## Wi-Fi

The internal Wi-Fi is a USB chip, the Realtek RTL8192DU. Linux 6.6 has no driver for it; Linux 6.12 does.
I brought the whole Realtek driver family over from 6.12 (two small changes to fit 6.6). It connects to
5 GHz WPA2 networks at about 4 MB/s.

## IR receiver

The IR receiver uses a mainline driver, but nothing was ever received. The driver had a bug: a missing
pair of brackets meant it configured a pulse width of zero. With the fix, remote control buttons arrive as
key presses.

## HDMI

There's no open driver for the display hardware, but the bootloader has the vendor's full display driver.
It shows a boot logo when it finds two partitions, `baseparam` (display settings) and `logo` (a JPEG),
listed by name in the partition string. I added both and wrote tools for them
([`extras/splash/`](../extras/splash/)): one sets the resolution to 1080p60, the other packs my own
picture as the boot logo.

The bootloader leaves the screen running, so Linux simply keeps drawing into the same memory (a
"simple framebuffer"). That gives an HDMI console with a login prompt and USB keyboard input. Changing
the resolution would need a real display driver.

What's left open is the SD card slot → [appendix](appendix-sd-card.md).
