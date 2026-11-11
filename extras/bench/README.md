# Benchmarks

Built on the box with its own gcc (Alpine), flags `-O2 -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard`
(`-O3` for CoreMark). Results: [`docs/06-performance.md`](../../docs/06-performance.md).

**Check the clock first.** The box has no RTC; if the time service is still catching up, every timing is
off by up to 8 %. `chronyc tracking` must show "System time" near 0 (fix: `chronyc makestep`).
`clockcheck` should report about 1200 MHz.

| File | What |
|---|---|
| `clockcheck.c` | real CPU clock from a chain of dependent adds (1 per cycle) |
| `flops3.c` | peak floating point with inline assembly (the compiler can't merge or drop the work): NEON 128/64-bit FMA, mul+add, scalar FP32/FP64. `-fopenmp`, `OMP_NUM_THREADS=1` for one core |
| `memtest.c` | `memtest SECONDS MB`: write and verify patterns, count errors |
| `stress_all.sh` | 5-minute everything-at-once test: NEON, 2× memtest, GPU stress, eMMC write/verify, temperature sampler |

[CoreMark](https://github.com/eembc/coremark) (not included), from its source folder:

```
F="-O3 -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard"
gcc $F -Iposix -I. -DFLAGS_STR="\"$F\"" -DITERATIONS=0 -DPERFORMANCE_RUN=1 \
    core_list_join.c core_main.c core_matrix.c core_state.c core_util.c posix/core_portme.c -o coremark_1 -lrt
# 4 cores: add -DMULTITHREAD=4 -DUSE_PTHREAD -pthread
```

[STREAM](https://github.com/jeffhammond/STREAM) (not included): `gcc $F -fopenmp -DSTREAM_ARRAY_SIZE=20000000 stream.c`.
