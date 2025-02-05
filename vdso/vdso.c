#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/auxv.h>
#include <sys/time.h>


/* From linux/time.h */
#define CLOCK_REALTIME                  0
#define CLOCK_MONOTONIC                 1
#define CLOCK_PROCESS_CPUTIME_ID        2
#define CLOCK_THREAD_CPUTIME_ID         3
#define CLOCK_MONOTONIC_RAW             4
#define CLOCK_REALTIME_COARSE           5
#define CLOCK_MONOTONIC_COARSE          6
#define CLOCK_BOOTTIME                  7
#define CLOCK_REALTIME_ALARM            8
#define CLOCK_BOOTTIME_ALARM            9


void get_time(clockid_t clock) {
	struct timespec ts;
	int result;

	result = clock_gettime(clock, &ts);
	if (result != 0) {
		printf("Error getting time through VDSO\n");
		return;
	}
	printf("%d %lld.%.9ld\n", clock, (long long)ts.tv_sec, ts.tv_nsec);
}

int main() {
	int i;

	for (i = 0; i < 10; i++) {
		get_time(i);
	}

	return 0;
}
