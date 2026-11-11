# 4. Mainline Linux

Mainline Linux 6.6 has almost nothing for this chip, only a few drivers shared with its bigger sibling,
the Hi3798CV200. So I wrote the device tree (the description of the hardware for Linux) myself, from
three sources: the vendor's open-source SDK (its kernel, bootloader and device tree), the stock kernel's
`/proc` dump from [step 1](01-stock-box.md), and where both were silent, the stock kernel binary itself.

Every test kernel was loaded over the network into RAM and started from there (`tftp`, `bootm`); only
kernels that worked got written to the eMMC.

## First boot: nothing ever interrupted

The first kernels hung shortly after starting. The real reason took a while to find: **the interrupt
controller address was wrong**. The address I had copied pointed into normal RAM, so no interrupt ever
arrived: no timer tick, no disk, no console input. The actual controller sits at `0xf8a01000`. On top of
that, every interrupt number from the vendor's lists had to be reduced by 32 (they count from a different
starting point than Linux device trees do).

With interrupts working, the next surprises:

- **The CPU's built-in timer is unusable** (its counter returns garbage), so Linux keeps time with the
  chip's other timer blocks instead (later a driver of my own, [step 5](05-drivers.md)).
- **Linux 6.6 couldn't read the partition list.** A one-character bug in the code that parses
  `blkdevparts=` cut off the names. Fixed in a patch (newer kernels have the fix).

## Alpine from the eMMC

Then Alpine ran on the internal storage, and I repartitioned the eMMC:

| # | Name | Size | Contents |
|---|---|---|---|
| 1 | fastboot | 1 MB | the vendor bootloader, never touched |
| 2 | bootargs | 1 MB | bootloader settings |
| 3 | kernel | 32 MB | my kernel |
| 4 | rootfs | 3182 MB | Alpine |
| 5 | baseparam | 8 MB | display settings for the boot logo ([step 5](05-drivers.md#hdmi)) |
| 6 | logo | 20 MB | the boot logo |

```
bootcmd=mmc read 0 0x4000000 0x1000 0x10000; bootm 0x4000000
bootargs=console=ttyAMA0,115200 root=/dev/mmcblk0p4 rootwait=20 panic=5 mem=1G clk_ignore_unused
         blkdevparts=mmcblk0:1M(fastboot),1M(bootargs),32M(kernel),3182M(rootfs),8M(baseparam),20M(logo)
```

Current settings: [`bootloader/uboot-env.txt`](../bootloader/uboot-env.txt).
[`bootloader/ubootenv.py`](../bootloader/ubootenv.py) reads and writes the settings block on the PC.

## A way back

The bootloader can't be stopped at power-on, so a broken kernel on the eMMC would mean a box that never
reaches a prompt again. Two escape hatches:

- **A rescue shell inside every kernel.** A tiny initramfs ([`port/kernel/initramfs-init`](../port/kernel/initramfs-init),
  built by [`build_initramfs.sh`](../port/kernel/build_initramfs.sh)) waits 3 seconds for a key on the serial
  console. A key gives a busybox shell with the tools to repair things; otherwise it starts Alpine.
- **`uboot-prompt -r`** ([`port/board/uboot-prompt`](../port/board/uboot-prompt)) clears `bootcmd` from Linux
  and reboots. The bootloader then stops at its prompt, ready for a new kernel over the network.

## Building it

- Kernel: Linux 6.6 + [`port/kernel/linux-6.6-hi3798mv100.patch`](../port/kernel/linux-6.6-hi3798mv100.patch)
  (device tree and all my drivers) + [`linux-6.6-rtlwifi-v6.12-backport.patch`](../port/kernel/linux-6.6-rtlwifi-v6.12-backport.patch)
  (Wi-Fi), config [`hi3798mv100.config`](../port/kernel/hi3798mv100.config). The device tree is also in
  [`port/kernel/dts/`](../port/kernel/dts/). [`build_kernel.sh`](../port/kernel/build_kernel.sh) builds and
  packs it for the bootloader (load address `0x02000000`).
- Alpine: [`port/rootfs/build_rootfs.sh`](../port/rootfs/build_rootfs.sh) builds a ready image on the PC
  (network, SSH, time sync, watchdog, swap, GPU and Wi-Fi tools).

From here on it was one part of the chip at a time → [step 5](05-drivers.md).
