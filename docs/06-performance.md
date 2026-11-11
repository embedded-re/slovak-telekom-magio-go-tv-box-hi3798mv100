# 6. Performance and cooling

## Cooling

At full load on all four cores at 1.2 GHz, the stock heatsink isn't enough: the chip passed 100 °C within
15 minutes, and above about 90 °C it simply froze. So Linux slows down to 800 MHz at 75 °C
([step 5](05-drivers.md#temperature-and-cpu-speed)). I then tried several cooling setups with the same
load test, 4 busy cores ([`extras/thermal/`](../extras/thermal/)):

| Setup | Full speed (1.2 GHz) held for | Then |
|---|---|---|
| stock thermal pad + stock heatsink | ~20 s | 800 MHz, 77 °C after 5 min and rising |
| good pad + stock heatsink | ~100 s | switching between 800 MHz and 1.2 GHz (3 min test) |
| good pad + new heatsink | ~200 s | 800 MHz, 78 °C after 10 min, still rising slowly |
| good pad + large piece of metal | never throttled in 10 min | 74 °C at most |
| **good pad + new heatsink + fan** | **never throttled** | **flat at 59 °C** (CPU + GPU: 60 °C) |

The stock thermal pad was the biggest problem: every setup with it heated up much faster. Without
airflow, every setup kept getting warmer. With a small fan, the box runs at full speed indefinitely.

## Speed

Measured with the fan, all at 1.2 GHz ([`extras/bench/`](../extras/bench/)):

| Test | 1 core | 4 cores |
|---|---|---|
| CoreMark (integer) | 3,619 | 14,420 |
| 32-bit floating point (NEON, best case) | 2.3 GFLOPS | 9.2 GFLOPS |
| 64-bit floating point | 0.48 GFLOPS | 1.9 GFLOPS |
| Memory bandwidth (STREAM copy) | 2.6 GB/s | 3.9 GB/s |

All four cores scale perfectly. 64-bit floating point is five times slower than 32-bit. The vector unit
gains nothing from 128-bit operations over 64-bit ones on this core.

**A trap I fell into:** the box has no battery-backed clock, so it boots with a date that is days old. The
time service then didn't jump to the right time, it sped the clock up by about 8 % to catch up slowly,
which made every first measurement about 8 % too low. The fix is one line in the time service's
settings (`makestep 1 -1`, now in [`port/rootfs/build_rootfs.sh`](../port/rootfs/build_rootfs.sh)).

## Network

Plain TCP between the box and a PC ([`extras/network/`](../extras/network/)):

| Test | Result | CPU used |
|---|---|---|
| box → PC | 94.2 Mbit/s | 5 % |
| PC → box | 94.3 Mbit/s | 4 % |
| both directions at once | 92 + 92 Mbit/s | 25 % |
| smallest packets (64-byte frames) | ~150,000 per second | ~50 % |

That is the limit of 100 Mbit Ethernet; the box has CPU to spare even with the smallest packets.

## Everything at once

A 5-minute test loading everything together ([`extras/bench/stress_all.sh`](../extras/bench/stress_all.sh)):
all four cores, 2 × 128 MB memory being written and checked, the GPU rendering and checking every frame,
the eMMC writing and re-reading a 256 MB file, and the network at full speed. Result: 58 °C at most, no
slowdown, **0 errors** in 187 GB of memory checks, every GPU frame and every file correct, and the
watchdog never had to step in.

## What it's good for

A box like this is a good small server: network services, monitoring, small databases, many small
parallel jobs. It's not built for heavy computing (1 GB of RAM, a slow 64-bit floating-point unit, a GPU
without compute support) or video playback (no driver for the video decoder).
