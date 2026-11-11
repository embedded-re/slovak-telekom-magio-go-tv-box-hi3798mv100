#!/bin/sh
# Full-system stress: CPU NEON, memory verify, GPU, eMMC write/verify; sampler. Usage: stress_all.sh SECONDS
DUR=${1:-300}; cd /root/bench; O=/root/bench/stress; rm -rf $O; mkdir -p $O
END=$(( $(cut -d. -f1 /proc/uptime) + DUR ))
up() { cut -d. -f1 /proc/uptime; }
( while [ $(up) -lt $END ]; do echo "t=$(up) T=$(cat /sys/class/thermal/thermal_zone0/temp) f=$(cat /sys/devices/system/cpu/cpufreq/policy0/cpuinfo_cur_freq) cool=$(cat /sys/class/thermal/cooling_device0/cur_state)/$(cat /sys/class/thermal/cooling_device1/cur_state) gpu=$(cat /sys/class/devfreq/f9200000.gpu/cur_freq) memavail=$(awk '/MemAvailable/{print int($2/1024)}' /proc/meminfo)M load=$(cut -d' ' -f1 /proc/loadavg)"; sleep 5; done ) > $O/sampler.log 2>&1 &
( while [ $(up) -lt $END ]; do ./flops3 | grep "q x12 (128-bit FMA)"; done ) > $O/cpu.log 2>&1 &
./memtest $DUR 128 > $O/mem1.log 2>&1 &
./memtest $DUR 128 > $O/mem2.log 2>&1 &
/root/gltest/gles2_stress $DUR 16 1 > $O/gpu.log 2>&1 &
( n=0; while [ $(up) -lt $END ]; do n=$((n+1)); a=$(up); dd if=/dev/urandom of=/root/stress.bin bs=1M count=256 conv=fsync 2>/dev/null; h1=$(sha256sum /root/stress.bin | cut -c1-16); sync; echo 3 > /proc/sys/vm/drop_caches; h2=$(sha256sum /root/stress.bin | cut -c1-16); b=$(up); [ "$h1" = "$h2" ] && r=OK || r=MISMATCH; echo "loop $n: 256MB write+readback ${r} in $((b-a))s"; done; rm -f /root/stress.bin ) > $O/emmc.log 2>&1 &
wait
dmesg | grep -iE "error|fail|timeout|oops|bug|lima|mmc0" | grep -vE "regulator|Direct firmware" | tail -20 > $O/dmesg.log
echo done > $O/DONE
