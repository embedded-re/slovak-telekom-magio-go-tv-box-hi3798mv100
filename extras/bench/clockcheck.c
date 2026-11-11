/* Measure the real core clock: a chain of dependent ADDs runs at 1 add/cycle on Cortex-A7. */
#include <stdio.h>
#include <time.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
int main(void) {
	const long IT = 1000000;      /* x 1000 adds = 1e9 dependent adds */
	unsigned x = 0;
	double t = now();
	for (long i = 0; i < IT; i++)
		__asm__ volatile(
		".rept 1000\n add %0, %0, #1\n .endr\n" : "+r"(x));
	t = now() - t;
	printf("dependent adds: %u in %.3f s -> %.0f MHz (if 1 add/cycle)\n", x, t, x / t / 1e6);
	return 0;
}
