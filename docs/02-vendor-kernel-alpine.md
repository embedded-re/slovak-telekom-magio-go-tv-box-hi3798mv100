# 2. Alpine on the vendor kernel

The first goal was modest: keep the stock kernel, but run Alpine Linux from a USB stick instead of
Android.

## Why the obvious way failed

Pointing the stock kernel at the USB stick (`root=/dev/sda2`) never worked: the stick was never found.

- **The kernel doesn't switch on USB by itself.** The vendor's USB driver only starts the USB hardware
  after Android's start-up program (`/init`) has run its setup. The bootloader had USB working, but the
  kernel reset it and waited for Android.
- **Android's `/init` ignores the usual escape.** Normally `init=/bin/sh` on the kernel command line
  replaces the start-up program. This one ignores it.
- **The USB kernel had no Android start-up files.** The kernel on the eMMC carries a small built-in file
  system (an **initramfs**) with Android's `/init` appended to it; the kernel copy I loaded from USB did not.

## The detour through Android

So I let Android start, just long enough:

1. Cut the initramfs out of the eMMC kernel partition and load it from USB next to the kernel.
2. Android's `/init` starts and switches USB on. The stick appears.
3. From a root shell, bind-mount `/dev`, `/proc` and `/sys` into Alpine and `switch_root` into it.

```
fatload usb 0:1 0x4000000 hi_kernel.uImage
fatload usb 0:1 0x8000000 uInitrd.img
setenv bootargs mem=1G mmz=ddr,0,0,380M console=ttyAMA0,115200 blkdevparts=mmcblk0:...
bootm 0x4000000 0x8000000
```

Alpine ran, but on a 2017 vendor kernel, through Android, from USB. Good enough to prove the box was
usable; not something to build on. The next step was a kernel of my own, and before writing anything to
the eMMC, a full copy of it → [step 3](03-emmc-backup.md).
