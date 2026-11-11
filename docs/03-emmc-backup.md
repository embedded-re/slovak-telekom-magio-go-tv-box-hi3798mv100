# 3. A full copy of the storage

Before changing anything I copied the whole eMMC (3.9 GB) to my PC. Not from Android, but from the
bootloader, so nothing was running on the storage while it was read.

## How

The bootloader can read the eMMC into RAM (`mmc read`) and upload RAM to a PC over the network
(`tftp`). [`bootloader/emmc_dump.sh`](../bootloader/emmc_dump.sh) does that in 256 MB chunks over the
serial console, and [`bootloader/tftp_recv.py`](../bootloader/tftp_recv.py) is a small TFTP server that
accepts uploads. For every chunk, the box computes a checksum (`crc32`) and the script compares it with
the received file. [`bootloader/emmc_assemble.sh`](../bootloader/emmc_assemble.sh) joins the chunks into
one image and cuts out each partition. Details: [`bootloader/emmc-dump-README.md`](../bootloader/emmc-dump-README.md).

Restoring any part is the same in reverse: load the file into RAM over TFTP, `mmc write` it back.

## The stock layout

| # | Name | Size | | # | Name | Size |
|---|---|---|---|---|---|---|
| 1 | fastboot | 1 MB | | 11 | kernel | 40 MB |
| 2 | bootargs | 1 MB | | 12 | misc | 20 MB |
| 3 | recovery | 10 MB | | 13 | userapi | 8 MB |
| 4 | deviceinfo | 2 MB | | 14 | hibdrv | 8 MB |
| 5 | baseparam | 8 MB | | 15 | qbflag | 8 MB |
| 6 | pqparam | 8 MB | | 16 | qbdata | 300 MB |
| 7 | logo | 20 MB | | 17 | system | 500 MB |
| 8 | logobak | 20 MB | | 18 | userdata | 1024 MB |
| 9 | fastplay | 40 MB | | 19 | cache | 400 MB |
| 10 | fastplaybak | 40 MB | | 20 | sdcard | rest |

With the copy safe, I later replaced all of this with a much simpler layout ([step 4](04-mainline-linux.md)).

The copy is not in this repo: it contains the vendor's software and data specific to my box.
