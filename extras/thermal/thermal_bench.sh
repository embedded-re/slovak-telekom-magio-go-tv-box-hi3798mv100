#!/bin/sh
# thermal benchmark: idle 120 s, CPU 4x busy 1800 s, +GPU stress 600 s, cooldown 300 s; log every 10 s
LOG=/root/thermal_bench.log
P=/sys/devices/system/cpu/cpufreq/policy0
Z=/sys/class/thermal
G=/sys/class/devfreq/f9200000.gpu
: > $LOG
t=0
sample() { echo "t=$t phase=$1 T=$(cat $Z/thermal_zone0/temp) f=$(cat $P/cpuinfo_cur_freq) cool0=$(cat $Z/cooling_device0/cur_state) cool1=$(cat $Z/cooling_device1/cur_state) gpu=$(cat $G/cur_freq) load=$(cut -d' ' -f1 /proc/loadavg)" >> $LOG; }
run() { end=$((t + $2)); while [ $t -lt $end ]; do sample $1; sleep 10; t=$((t + 10)); done; }
run idle 30
PIDS=""
for i in 1 2 3 4; do sh -c 'while :; do :; done' & PIDS="$PIDS $!"; done
run cpu 600
/root/gltest/gles2_stress 300 16 1 > /root/thermal_bench_gpu.log 2>&1 &
GP=$!
run cpu+gpu 300
kill $PIDS $GP 2>/dev/null
run cooldown 120
echo done >> $LOG
