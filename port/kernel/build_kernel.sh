#!/bin/sh
# Usage: port/kernel/build_kernel.sh bXX  — build zImage+DTB, package uImage into bXX/ and tftp/. Fails loudly.
set -e
B=$1; [ -n "$B" ] || { echo "usage: $0 bXX"; exit 1; }
P=/home/arch/projects/RE/HIsilicon; K=/home/arch/projects/kernel/linux-6.6
MK="make -C $K ARCH=arm CROSS_COMPILE=arm-none-eabi-"
$MK -j$(nproc) zImage 2>&1 | grep -v norelro | grep -E "error|Error|warning:|is ready" || true
$MK -j$(nproc) zImage >/dev/null 2>&1   # re-run: propagates failure via set -e
$MK hisilicon/hi3798mv100-bigfish.dtb >/dev/null
mkdir -p $P/$B
cat $K/arch/arm/boot/zImage $K/arch/arm/boot/dts/hisilicon/hi3798mv100-bigfish.dtb > $P/$B/zImage-6.6-dtb
/home/arch/projects/RE/RockChipTablet/u-boot/tools/mkimage -A arm -O linux -T kernel -C none \
	-a 0x02000000 -e 0x02000000 -n "Linux 6.6 $B" -d $P/$B/zImage-6.6-dtb $P/$B/uImage-$B >/dev/null
cp $P/$B/uImage-$B $P/tftp/
strings $K/arch/arm/boot/Image 2>/dev/null | grep -m1 "^Linux version" || true
ls -la $P/$B/uImage-$B
