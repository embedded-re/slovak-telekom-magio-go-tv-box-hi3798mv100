# 1. The stock box

The box is the first **Magio GO TV Box** from Slovak Telekom, an Android set-top box for its streaming TV
service. It identifies itself in its Android settings
(`ro.product.manufacturer=MagioGOTVBox`, `ro.buyer.name=SlovakTelekom`, model `G6O110SVT`, factory name
`SlovakTelekom_SC7250`, Android 4.4.2 built in April 2017). It's not the later Magio GO TV Box (KAON
KSTB6077, Broadcom chip, Android TV 8).

Inside is a HiSilicon **Hi3798MV100**: four ARM Cortex-A7 cores, a Mali-450 GPU, 1 GB of RAM and 4 GB of
internal storage (**eMMC**). The board is HiSilicon's "bigfish" reference design.

## Serial console

The board has a serial port (**UART**, 115200 baud, `ttyAMA0` in Linux). It shows everything: the
bootloader, the kernel log and an Android shell. The stock boot log is in
[`hardware/vendor/stock-android-boot.txt`](../hardware/vendor/stock-android-boot.txt).

## The bootloader

The bootloader is **HiSilicon Fastboot 3.3.0**, a modified U-Boot (the usual bootloader on ARM boards).
It has the normal U-Boot commands: read and write the eMMC, load files over the network (**TFTP**) or
from USB, and save its settings.

Two of its settings decide everything:

- `bootcmd`: what to run at power-on (normally: read the kernel from the eMMC and start it)
- `bootargs`: the command line handed to the kernel

One thing it does not have is a way to stop the automatic start: no countdown, and keys are ignored.
It only stops at its command prompt when `bootcmd` is empty. Later I built a small tool for that
(`uboot-prompt`, [step 4](04-mainline-linux.md)).

## No partition table

The eMMC has no normal partition table. Instead, the kernel command line lists the partitions, as sizes
and names, in a `blkdevparts=` string: 20 partitions for Android, among them `fastboot` (the bootloader),
`bootargs` (its settings), `kernel`, `system` and `userdata`. The full list is in
[step 3](03-emmc-backup.md).

## What Android knew

From a root shell on the stock kernel, I saved what it knew about the hardware: memory map,
interrupts, devices, kernel config ([`hardware/vendor/stock-android-discovery.txt`](../hardware/vendor/stock-android-discovery.txt)).
That list was the starting point for the Linux port.
