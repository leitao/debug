#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/auxv.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>


/* Create a single thread that run clock_gettime() with different clockids,
 * and report the number of operations per second
 */

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

char *clock_names[] = {
	"CLOCK_REALTIME", /* vdso */
	"CLOCK_MONOTONIC", /* vdso */
	"CLOCK_PROCESS_CPUTIME_ID", /* syscall */
	"CLOCK_THREAD_CPUTIME_ID", /* syscall */
	"CLOCK_MONOTONIC_RAW", /* vdso */
	"CLOCK_REALTIME_COARSE", /* vdso */
	"CLOCK_MONOTONIC_COARSE", /* vdso */
	"CLOCK_BOOTTIME", /* vdso */
	"CLOCK_REALTIME_ALARM", /* invalid  on aarch 64 */
	"CLOCK_BOOTTIME_ALARM" /* invalid on aarch64 */
};


struct thread_data {
        clockid_t clockid;
};

typedef void *(*thread_func)(void *);

volatile bool stopping = false;

void *get_time(void *_td) {
	struct thread_data *td = (struct thread_data *)_td;
	clockid_t clock = td->clockid;
	unsigned long count = 0;
	unsigned long *ret;
	struct timespec ts;
	int result;

	while (!stopping) {
		result = clock_gettime(clock, &ts);
		if (result != 0) {
			printf("Error getting time through VDSO\n");
			stopping = true;
			return NULL;
		}
		count += 1;
	}

	ret = malloc(sizeof(unsigned long));;
	if (!count) {
		printf("Failed to allocate memory\n");
		return NULL;
	}
	*ret = count;
	return ret;
}

unsigned long create_threads(int thread_count, int secs, thread_func func, struct thread_data *td)
{
	unsigned long **counts;
	unsigned long total = 0;
        pthread_t *threads;
        int ret;


	threads = malloc(thread_count * sizeof(pthread_t));
	counts = malloc(thread_count * sizeof(unsigned long *));

        stopping = false;
	for (int i = 0 ; i < thread_count; i++) {
		ret = pthread_create(&threads[i], NULL, func, td);
		if (ret) {
			fprintf(stderr, "pthread_create failed: %d\n", ret);
			return -1;
		}
	}
        sleep(secs);
        stopping = true;
	for (int i = 0 ; i < thread_count; i++) {
		int count_per_thread;

		pthread_join(threads[i], (void **)&counts[i]);
		count_per_thread = *counts[i];
		/* return in Millions */
		total += (count_per_thread / (1000 * 1000)) / secs;
	}



	return total;

}

void run_for_secs(int thread_count, int secs, thread_func func, struct thread_data *td)
{
	unsigned long calls_per_s;

	calls_per_s = create_threads(thread_count, secs, func, td);

	printf("Number of calls to %s : %lu M/s per thread\n", clock_names[td->clockid], calls_per_s / thread_count);
}



void print_help(const char *name)
{
	fprintf(stderr, " Benchmark clock_gettime(3) function:\n\n");
	fprintf(stderr, "%s <arguments>:\n", name);
	fprintf(stderr, "	-h                 : This help\n");
	fprintf(stderr, "	-t <seconds>       : Time running the clock_gettime() in a thread in a loop\n");
	fprintf(stderr, "	-p <threads_count> : Number of threads running clock_gettime() in a loop\n");
	fprintf(stderr, "	-c <clockid>       : clock id argument\n");

	fprintf(stderr, "\t   Supported clock ids\n");
	for (int i = 0; i < 9; i++) {
		fprintf(stderr, "\t\t%d : %s\n", i, clock_names[i]);
	}
	fprintf(stderr, "\n");
}

int main (int argc, char **argv)
{
	/* Default single thread */
	int threads_count = 1;
	/* default timeout, overwritten by -t */
	int timeout = 1;
	/* print all */
	clockid_t clockid = -1;
	int arg;

	struct thread_data td;

	while ((arg = getopt (argc, argv, "ht:p:c:")) != -1) {
		switch (arg)
		{
			case 'h':
				print_help(argv[0]);
				return 0;
			case 't':
				// Timeout
				timeout = atoi(optarg);
				break;
			case 'p':
				threads_count = atoi(optarg);
				break;
			case 'c':
				clockid = atoi(optarg);
				break;

		}
	}


	printf("running %d threads for %d seconds\n", threads_count, timeout);


	if (clockid == -1) {
		/* by default, run all clock types */
		for (int i = 0; i < 9; i++) {
			td.clockid = i;
			run_for_secs(threads_count, timeout, get_time, &td);
		}
	} else {
		td.clockid = clockid;
		run_for_secs(threads_count, timeout, get_time, &td);
	}

	return 0;
}
