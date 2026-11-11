/* udprecv PORT IDLE_SECONDS: count datagrams until IDLE_SECONDS of silence. */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
int main(int c, char **v) {
	int s = socket(AF_INET, SOCK_DGRAM, 0), rb = 8 << 20;
	setsockopt(s, SOL_SOCKET, SO_RCVBUF, &rb, sizeof rb);
	struct sockaddr_in a = { .sin_family = AF_INET, .sin_port = htons(atoi(v[1])) };
	bind(s, (void *)&a, sizeof a);
	struct timeval tv = { .tv_sec = atoi(v[2]) }; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
	char buf[2048]; long n = 0, bytes = 0; double t0 = 0, t1 = 0; int k;
	while ((k = recv(s, buf, sizeof buf, 0)) > 0) { if (!n) t0 = now(); n++; bytes += k; t1 = now(); }
	printf("received %ld pkts in %.2f s = %.0f pps, %.1f Mbit/s payload\n", n, t1 - t0, n / (t1 - t0), bytes * 8 / (t1 - t0) / 1e6);
	return 0;
}
