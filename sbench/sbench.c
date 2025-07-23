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

struct thread_data {
	bool print;
	bool raw;
};

typedef unsigned long *(*thread_func)(void *);

volatile bool stopping = false;

#define USE_LIBC_SYSCALL 1

unsigned long *get_pid() {
	unsigned long count = 0;
	unsigned long *ret;

	while (!stopping) {
		pid_t pid = getpid();
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

float create_threads(int thread_count, int msecs, thread_func func, struct thread_data *td)
{
	unsigned long **counts;
	float syscalls_executed = 0;
        pthread_t *threads;
        int ret;

	threads = malloc(thread_count * sizeof(pthread_t));
	counts = malloc(thread_count * sizeof(unsigned long *));

        stopping = false;
	for (int i = 0 ; i < thread_count; i++) {
		ret = pthread_create(&threads[i], NULL, (void *)func, td);
		if (ret) {
			fprintf(stderr, "pthread_create failed: %d\n", ret);
			return -1;
		}
	}
        usleep(1000 * msecs);
        stopping = true;
	for (int i = 0 ; i < thread_count; i++) {
		unsigned long count_per_thread = 0;

		pthread_join(threads[i], (void **)&counts[i]);
		if (!counts[i]) {
			fprintf(stderr, "something wrong with thread %d\n", i);
			continue;
		}
		count_per_thread = *counts[i];
		/* return in Millions */
		syscalls_executed += count_per_thread / (1000.0 * 1000.0);
	}

	free(threads);
	free(counts);
	return syscalls_executed;
}


#define DATAPOINTS 100

// Comparator function for qsort
int compare_doubles(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;

    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

static void sort(double *arr) {
	qsort(arr, DATAPOINTS, sizeof(double), compare_doubles);
}


static void print_data(double *throughput_array, double *latency_array, bool raw)
{

	sort(throughput_array);
	sort(latency_array);



	if (!raw) {
		printf("Throughput (Number of syscalls /s)");
		printf("\t\tLatency (Per syscall in nanoseconds):\n");
	}

	printf(" min=%.2f\tp50=%.2f\tp95=%.2f", 
			throughput_array[0],
		       	throughput_array[DATAPOINTS*50/100],
		       	throughput_array[DATAPOINTS*95/100]);
	printf("\t | \t");
	printf("min=%.2f\tp50=%.2f\tp95=%.2f\n",
		       	latency_array[0],
		       	latency_array[DATAPOINTS*50/100],
		       	latency_array[DATAPOINTS*95/100]);
}
void run_for_secs(int thread_count, int secs, thread_func func, struct thread_data *td)
{
	double latency_array[DATAPOINTS];
	double throughput_array[DATAPOINTS];

	double throughput_s, calls, latency;
	double timeslice_ms;
	int i;

	timeslice_ms = 1000*secs / DATAPOINTS;

	for(i = 0; i < DATAPOINTS ; i++) {
		calls = create_threads(thread_count, timeslice_ms, func, td);
		throughput_s = 1000 * (calls / (thread_count * timeslice_ms));

		latency = 1000/(throughput_s); // in ns
					       //
		if (td->print)
			printf("Number of calls: %.2f M/s per thread. Avg Latency: %.2f ns\n", throughput_s, latency);

		latency_array[i] = latency;
		throughput_array[i] = throughput_s;
	}

	print_data(throughput_array, latency_array, td->raw);
}



void print_help(const char *name)
{
	fprintf(stderr, " Benchmark clock_gettime(3) function:\n\n");
	fprintf(stderr, "%s <arguments>:\n", name);
	fprintf(stderr, "	-h                 : This help\n");
	fprintf(stderr, "	-t <seconds>       : Time running the test\n");
	fprintf(stderr, "	-p <threads_count> : Number of threads\n");
	fprintf(stderr, "	-v		   : verbose\n");
	fprintf(stderr, "	-r		   : raw output\n");

	fprintf(stderr, "\n");
}


int main(int argc, char **argv)
{
	/* Default single thread */
	int threads_count = 1;
	/* default timeout, overwritten by -t */
	int timeout = 1;
	/* print all */
	int arg;

	struct thread_data td = {};

	while ((arg = getopt (argc, argv, "ht:p:vr")) != -1) {
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
			case 'v':
				td.print = true;
				break;
			case 'r':
				td.raw = true;
				break;
		}
	}


	printf("running %d threads for %d seconds\n", threads_count, timeout);

	run_for_secs(threads_count, timeout, get_pid, &td);

	return 0;
}
