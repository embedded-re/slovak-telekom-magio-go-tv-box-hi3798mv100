/* memtest SECONDS MB: fill buffer with a per-pass pattern, verify, count errors. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
int main(int argc, char **argv) {
	int secs = atoi(argv[1]); size_t n = (size_t)atoi(argv[2]) << 20 >> 2;
	uint32_t *b = malloc(n * 4); if (!b) { puts("malloc failed"); return 1; }
	time_t end = time(0) + secs; unsigned long pass = 0, errs = 0; double gb = 0;
	while (time(0) < end) {
		uint32_t s = 0x9e3779b9u * (uint32_t)(pass + 1);
		for (size_t i = 0; i < n; i++) b[i] = s ^ (uint32_t)i * 2654435761u;
		for (size_t i = 0; i < n; i++) if (b[i] != (s ^ (uint32_t)i * 2654435761u)) errs++;
		pass++; gb += n * 8.0 / 1e9;
	}
	printf("memtest: %lu passes, %.1f GB written+verified, %lu errors\n", pass, gb, errs);
	return errs != 0;
}
