# eMMC full dump — 2026-11-08

Taken from HiSilicon Fastboot 3.3.0 via `mmc read` → `tftp` upload (`emmc_dump.sh`, `tftp_recv.py` in this folder).
Each 256 MiB chunk was CRC32-verified on the board (U-Boot `crc32`) against the host copy.

- Device: eMMC "4FPD3" (MID 0x15), 7,634,944 sectors = 3,909,091,328 bytes (EXT_CSD SEC_COUNT 0x748000)
- Boot partitions: 4 MiB each, not enabled (PARTITION_CONFIG=0) — not dumped
- Image: `dumps/emmc_full.img` (not published), sha256 in `emmc_full.img.sha256`
- Per-partition images: `dumps/parts/pNN_name.img`, sha256 in `parts.SHA256SUMS`

Observed contents: p03 recovery = uImage Linux 3.10.0_s40; p11 kernel = Android bootimg
(kernel 0x3008000, ramdisk 0x4000000, page 16384); p17–p20 = ext4.

Restore a partition (U-Boot): tftp the .img to RAM, then `mmc write 0 <addr> <start_blk> <cnt>`.
