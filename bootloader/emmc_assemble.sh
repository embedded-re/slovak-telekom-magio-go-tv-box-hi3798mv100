#!/usr/bin/env bash
# Join verified chunks into emmc_full.img, hash it, and split into named partitions
# according to the HiSilicon blkdevparts layout.
set -euo pipefail
D=/home/arch/projects/RE/HIsilicon/dumps
cd "$D"
[ "$(wc -l < chunks.txt)" = 15 ] || { echo "expected 15 verified chunks"; exit 1; }
cat $(awk '{print $1}' chunks.txt) > emmc_full.img
[ "$(stat -c %s emmc_full.img)" = 3909091328 ] || { echo "bad size"; exit 1; }
sha256sum emmc_full.img > emmc_full.img.sha256

mkdir -p parts
# name:size_MiB  (last = remainder)
LAYOUT="fastboot:1 bootargs:1 recovery:10 deviceinfo:2 baseparam:8 pqparam:8 logo:20 logobak:20
fastplay:40 fastplaybak:40 kernel:40 misc:20 userapi:8 hibdrv:8 qbflag:8 qbdata:300 system:500
userdata:1024 cache:400 sdcard:-"
off=0; n=1; TOTAL_MIB=$((3909091328 / 1048576))
for e in $LAYOUT; do
  name=${e%%:*}; sz=${e##*:}
  if [ "$sz" = "-" ]; then
    dd if=emmc_full.img of=parts/p$(printf %02d $n)_$name.img bs=1M skip=$off status=none
  else
    dd if=emmc_full.img of=parts/p$(printf %02d $n)_$name.img bs=1M skip=$off count=$sz status=none
    off=$((off + sz))
  fi
  n=$((n + 1))
done
(cd parts && sha256sum *.img > SHA256SUMS)
ls -la parts
cat emmc_full.img.sha256
