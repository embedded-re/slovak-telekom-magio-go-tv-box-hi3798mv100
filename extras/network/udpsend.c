/* udpsend HOST PORT SECONDS PAYLOAD: blast UDP datagrams, report packets/s sent. */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
int main(int c, char **v) {
	int s = socket(AF_INET, SOCK_DGRAM, 0), len = atoi(v[4]); double secs = atof(v[3]);
	struct sockaddr_in a = { .sin_family = AF_INET, .sin_port = htons(atoi(v[2])) };
	inet_pton(AF_INET, v[1], &a.sin_addr); connect(s, (void *)&a, sizeof a);
	char buf[2048]; memset(buf, 0, sizeof buf);
	long ok = 0, fail = 0; double t0 = now(), t;
	do { for (int i = 0; i < 1000; i++) { if (send(s, buf, len, 0) == len) ok++; else fail++; } t = now() - t0; } while (t < secs);
	printf("sent %ld pkts (%ld send errors) in %.2f s = %.0f pps, %.1f Mbit/s payload\n", ok, fail, t, ok / t, ok * len * 8 / t / 1e6);
	return 0;
}
