/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Benchmark for ARM64 per-CPU atomic operations - Library implementation
 */

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>

#include "percpu_bench_lib.h"

uint64_t get_time_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int compare_double(const void *a, const void *b)
{
	double val_a = *(const double *)a;
	double val_b = *(const double *)b;
	if (val_a < val_b)
		return -1;
	if (val_a > val_b)
		return 1;
	return 0;
}

double calculate_percentile(double *sorted_array, uint64_t count,
			      double percentile)
{
	uint64_t index = (uint64_t)((percentile / 100.0) * (count - 1));
	return sorted_array[index];
}

int get_num_cpus(void)
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

int set_cpu_affinity(int cpu)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);

	if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
		perror("sched_setaffinity");
		return -1;
	}
	return 0;
}

void run_benchmark_on_cpu(int cpu, struct benchmark *b)
{
	u64 counter = 0;
	uint64_t i; /* count number of iterations */

	/* Set CPU affinity */
	if (set_cpu_affinity(cpu) != 0) {
		fprintf(stderr, "Failed to set affinity to CPU %d\n", cpu);
		return;
	}

	/* Warmup - LL/SC */
	for (i = 0; i < WARMUP_ITERATIONS; i++) {
		/* Incrementing the same memory position (counter_llsc) */
		b->func(&counter, 1);
	}
	counter = 0;

	double *latencies =
		malloc(PERCENTILE_ITERATIONS * sizeof(double));
	if (!latencies) {
		fprintf(stderr, "unable to allocate latency counts\n");
		abort();
	}

	/* Run core benchmark measurements */
	run_core_benchmark(&counter, latencies, b->func);

	/* Sort the latencies */
	qsort(latencies, PERCENTILE_ITERATIONS, sizeof(double),
	      compare_double);

	/* Calculate percentiles */
	printf("%s : ", b->name);
	printf("  p50: %06.2f ns\t",
	       calculate_percentile(latencies, PERCENTILE_ITERATIONS, 50));
	printf("  p95: %06.2f ns\t",
	       calculate_percentile(latencies, PERCENTILE_ITERATIONS, 95));
	printf("  p99: %06.2f ns\n",
	       calculate_percentile(latencies, PERCENTILE_ITERATIONS, 99));

	free(latencies);
}
