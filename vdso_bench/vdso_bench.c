#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/auxv.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>

/* Create a single thread that run clock_gettime() with different clockids,
 * and report the number of operations per second
 */

/* Comment this one to compile on x86 */
/* #define CONFIG_ARM */

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
#define CLOCK_HACK			10

char *clock_names[] = {
	"CLOCK_REALTIME",
	"CLOCK_MONOTONIC",
	"CLOCK_PROCESS_CPUTIME_ID",
	"CLOCK_THREAD_CPUTIME_ID",
	"CLOCK_MONOTONIC_RAW",
	"CLOCK_REALTIME_COARSE",
	"CLOCK_MONOTONIC_COARSE",
	"CLOCK_BOOTTIME",
	"CLOCK_REALTIME_ALARM",
	"CLOCK_BOOTTIME_ALARM",
	"rmikey code (aarch64 specific code)",
};


enum test_type {
	SYSCALL = 1,
	GETTIME
};
struct thread_data {
	enum test_type type;
        clockid_t clockid;
};

typedef unsigned long *(*thread_func)(void *);

volatile bool stopping = false;
uint32_t ticks_per_interval = 0;

#if defined(CONFIG_ARM)
static inline uint32_t read_cntfrq_el0() {
	uint64_t val;
	__asm__ __volatile__("mrs %0, cntfrq_el0" : "=r" (val));
	return val & 0xffffffff;
}

static inline uint32_t read_cntvct_el0() {
	uint64_t val;
	__asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (val));
	return val;
}


/* asm function from rmikey */
static uint64_t gettime_asm(uint32_t per_ms) {
	uint64_t cval;

	if (ticks_per_interval == 0)
		ticks_per_interval = (read_cntfrq_el0() / 1000) * per_ms;

	cval = read_cntvct_el0();

	return ticks_per_interval & cval;
}
#else
static uint64_t gettime_asm(__attribute__((unused)) uint32_t per_ms) {
	return 0;
}
#endif


/*
 * Test without libc to make sure libc is not introducing
 * any PACA instruction
 */
pid_t getpid_raw(void) {
	uint64_t pid;

	/* On ARM64:
	* - System call number for getpid is 172 (goes in x8)
	* - Return value comes in x0
	* - svc #0 is the instruction to trigger the system call
	*/
	asm volatile(
		"mov x8, #172\n"  /* System call number for getpid */
		"svc #0\n"        /* Trigger system call */
		"mov %0, x0"      /* Store return value to pid */
		: "=r" (pid)      /* Output: pid gets assigned the value from x0 */
		:                 /* No input registers */
		: "x8", "x0"      /* Clobbered registers */
	);

	return pid;
}

// #define USE_LIBC_SYSCALL 1
#define USE_RAW_SYSCALL 1

unsigned long *get_pid() {
	unsigned long count = 0;
	unsigned long *ret;

	while (!stopping) {
#ifdef USING_LIBC
		pid_t pid = getpid();
#elif USE_RAW_SYSCALL
		pid_t pid = getpid_raw();
#else
#error "No clear how to call syscall"
#endif
		// Hack to avoid compiler optimization
		if (pid)
			count += 1;
	}

	ret = malloc(sizeof(unsigned long));;
	if (!ret) {
		fprintf(stderr, "Failed to allocate memory (get_pid)\n");
		return NULL;
	}
	*ret = count;
	return ret;
}

unsigned long *get_time(void *_td) {
	struct thread_data *td = (struct thread_data *)_td;
	clockid_t clock = td->clockid;
	unsigned long count = 0;
	unsigned long *ret;
	struct timespec ts;
	int result;

	while (!stopping) {
		if (clock == 10) {
			gettime_asm(1);
		} else {
			result = clock_gettime(clock, &ts);
		}
		if (result != 0) {
			fprintf(stderr, "Error getting time through clock_gettime (clockid_t = %s). clock_gettime(2) returned = %d\n", clock_names[clock], result);
			stopping = true;
			return NULL;
		}
		count += 1;
	}

	ret = malloc(sizeof(unsigned long));;
	if (!ret) {
		fprintf(stderr, "Failed to allocate memory\n");
		return NULL;
	}
	*ret = count;
	return ret;
}

float create_threads(int thread_count, int secs, thread_func func, struct thread_data *td)
{
	unsigned long **counts;
	float total = 0;
        pthread_t *threads;
        int ret;

	threads = malloc(thread_count * sizeof(pthread_t));
	counts = malloc(thread_count * sizeof(unsigned long *));

        stopping = false;
	for (int i = 0 ; i < thread_count; i++) {
		ret = pthread_create(&threads[i], NULL, (void *)func, td);
		if (ret) {
			perror("");
			fprintf(stderr, "pthread_create failed: %d\n", ret);
			return -1;
		}
	}
        sleep(secs);
        stopping = true;
	for (int i = 0 ; i < thread_count; i++) {
		int count_per_thread = 0;

		pthread_join(threads[i], (void **)&counts[i]);
		if (!counts[i]) {
			continue;
		}
		count_per_thread = *counts[i];
		/* return in Millions */
		total += (count_per_thread / (1000.0 * 1000.0)) / secs;
	}

	free(threads);
	free(counts);
	return total;
}

void run_for_secs(int thread_count, int secs, thread_func func, struct thread_data *td)
{
	float calls_per_s;

	calls_per_s = create_threads(thread_count, secs, func, td);

	switch(td->type) {
		case (GETTIME):
			printf("Number of calls to %s : %.2f M/s per thread\n", clock_names[td->clockid], calls_per_s / thread_count);
			break;
		case (SYSCALL):
			printf("Number of calls to getpid(2) : %.2f M/s per thread\n", calls_per_s / thread_count);
			break;
	}
}



void print_help(const char *name)
{
	fprintf(stderr, " Benchmark clock_gettime(3) function:\n\n");
	fprintf(stderr, "%s <arguments>:\n", name);
	fprintf(stderr, "	-h                 : This help\n");
	fprintf(stderr, "	-s       	   : Only test the syscall benchmark\n");
	fprintf(stderr, "	-t <seconds>       : Time running the clock_gettime() in a thread in a loop\n");
	fprintf(stderr, "	-p <threads_count> : Number of threads running clock_gettime() in a loop\n");
	fprintf(stderr, "	-c <clockid>       : clock id argument\n");

	fprintf(stderr, "\t   Supported clock ids\n");
	for (int i = 0; i <= 10; i++) {
		fprintf(stderr, "\t\t%d : %s\n", i, clock_names[i]);
	}
	fprintf(stderr, "\n");
}


int main(int argc, char **argv)
{
	/* Default single thread */
	int threads_count = 1;
	/* default timeout, overwritten by -t */
	int timeout = 1;
	/* print all */
	clockid_t clockid = -1;
	int arg;
	bool syscall_only = false;

	struct thread_data td;

	while ((arg = getopt (argc, argv, "ht:p:c:s")) != -1) {
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
			case 's':
				syscall_only = true;
				break;

		}
	}


	printf("running %d threads for %d seconds\n", threads_count, timeout);

	td.type = SYSCALL;
	run_for_secs(threads_count, timeout, get_pid, &td);

	if (syscall_only)
		return 0;

	td.type = GETTIME;
	if (clockid == -1) {
		/* by default, do not run all types*/
		for (int i = 0; i <= 8; i++) {
			td.clockid = i;
			run_for_secs(threads_count, timeout, get_time, &td);
		}
	} else {
		td.clockid = clockid;
		run_for_secs(threads_count, timeout, get_time, &td);
	}

	return 0;
}
