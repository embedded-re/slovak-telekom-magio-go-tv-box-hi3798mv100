#!/usr/bin/env bash
# Full eMMC user-area dump from HiSilicon Fastboot via TFTP upload.
# Requires: board at fastboot# prompt, kstb-bridge running, bootloader/tftp_recv.py on host :69.
# Usage: bootloader/emmc_dump.sh [first_chunk]
set -u
DUMP=/home/arch/projects/RE/HIsilicon/dumps
ADDR=0x10000000
TOTAL=7634944          # sectors (EXT_CSD SEC_COUNT 0x748000)
CHUNK=524288           # 256 MiB in 512-byte sectors
N=$(( (TOTAL + CHUNK - 1) / CHUNK ))
START=${1:-0}

kstb "setenv serverip 192.168.1.38" >/dev/null

for ((i=START; i<N; i++)); do
  blk=$(( i * CHUNK )); cnt=$(( TOTAL - blk < CHUNK ? TOTAL - blk : CHUNK ))
  bytes=$(( cnt * 512 )); f=$(printf "emmc_%02d.bin" "$i")
  for attempt in 1 2; do
    echo "=== chunk $i/$((N-1)) blk=$blk cnt=$cnt ($f) attempt $attempt ==="
    out=$(kstb -t 5 -m 60 "mmc read 0 $ADDR $(printf 0x%x $blk) $(printf 0x%x $cnt)")
    grep -q "blocks read: OK" <<<"$out" || { echo "mmc read FAILED: $out"; continue; }
    out=$(kstb -t 5 -m 60 "crc32 $ADDR $(printf 0x%x $bytes)")
    bcrc=$(grep -o '==> [0-9a-f]*' <<<"$out" | awk '{print $2}')
    rm -f "$DUMP/$f"
    out=$(kstb -t 60 -m 1800 "tftp $ADDR $f $(printf 0x%x $bytes)")
    grep -q "upload ok" <<<"$out" || { echo "tftp FAILED: $(tail -5 <<<"$out")"; kstb -r '\x03' -t 2 >/dev/null; continue; }
    sleep 1
    [ -f "$DUMP/$f" ] || { echo "file missing on host"; continue; }
    hsize=$(stat -c %s "$DUMP/$f")
    hcrc=$(python3 -c "import zlib,sys;c=0
f=open(sys.argv[1],'rb')
while (b:=f.read(1<<24)): c=zlib.crc32(b,c)
print('%08x'%c)" "$DUMP/$f")
    if [ "$hsize" = "$bytes" ] && [ "$hcrc" = "$bcrc" ]; then
      echo "OK chunk $i size=$hsize crc=$hcrc"; echo "$f $blk $cnt $hcrc" >> "$DUMP/chunks.txt"; break
    fi
    echo "MISMATCH chunk $i: board crc=$bcrc host crc=$hcrc size=$hsize/$bytes"
    [ "$attempt" = 2 ] && { echo "ABORT at chunk $i"; exit 1; }
  done
  [ -f "$DUMP/$f" ] || { echo "ABORT at chunk $i"; exit 1; }
done
echo "ALL $N CHUNKS OK"
