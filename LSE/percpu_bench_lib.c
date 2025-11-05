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

void run_benchmark_on_cpu(int cpu)
{
	u64 counter_llsc = 0;
	u64 counter_lse = 0;
	u64 counter_ldadd = 0;
	u64 counter_prfm_stadd = 0;
	u64 counter_prfm_strm_stadd = 0;
	uint64_t i; /* count number of iterations */

	/* Set CPU affinity */
	if (set_cpu_affinity(cpu) != 0) {
		fprintf(stderr, "Failed to set affinity to CPU %d\n", cpu);
		return;
	}

	/* Warmup - LL/SC */
	for (i = 0; i < WARMUP_ITERATIONS; i++) {
		/* Incrementing the same memory position (counter_llsc) */
		__percpu_add_case_64_llsc(&counter_llsc, 1);
	}
	counter_llsc = 0;

	/* Warmup - LSE (stadd) */
	for (i = 0; i < WARMUP_ITERATIONS; i++) {
		__percpu_add_case_64_lse(&counter_lse, 1);
	}
	counter_lse = 0;

	/* Warmup - LDADD */
	for (i = 0; i < WARMUP_ITERATIONS; i++) {
		__percpu_add_case_64_ldadd(&counter_ldadd, 1);
	}
	counter_ldadd = 0;

	/* Warmup - PRFM+STADD */
	for (i = 0; i < WARMUP_ITERATIONS; i++) {
		__percpu_add_case_64_prfm_stadd(&counter_prfm_stadd, 1);
	}
	counter_prfm_stadd = 0;

	/* Warmup - PRFM_STRM+STADD */
	for (i = 0; i < WARMUP_ITERATIONS; i++) {
		__percpu_add_case_64_prfm_strm_stadd(&counter_prfm_strm_stadd, 1);
	}
	counter_prfm_strm_stadd = 0;

	/* Percentile measurements */
	double *latencies_llsc =
		malloc(PERCENTILE_ITERATIONS * sizeof(double));
	double *latencies_lse =
		malloc(PERCENTILE_ITERATIONS * sizeof(double));
	double *latencies_ldadd =
		malloc(PERCENTILE_ITERATIONS * sizeof(double));
	double *latencies_prfm_stadd =
		malloc(PERCENTILE_ITERATIONS * sizeof(double));
	double *latencies_prfm_strm_stadd =
		malloc(PERCENTILE_ITERATIONS * sizeof(double));

	if (!latencies_llsc || !latencies_lse || !latencies_ldadd ||
	    !latencies_prfm_stadd || !latencies_prfm_strm_stadd) {
		fprintf(stderr, "Failed to allocate memory for latencies\n");
		free(latencies_llsc);
		free(latencies_lse);
		free(latencies_ldadd);
		free(latencies_prfm_stadd);
		free(latencies_prfm_strm_stadd);
		return;
	}

	/* Run core benchmark measurements */
	run_core_benchmark(&counter_llsc, &counter_lse, &counter_ldadd,
			   &counter_prfm_stadd, &counter_prfm_strm_stadd,
			   latencies_llsc, latencies_lse, latencies_ldadd,
			   latencies_prfm_stadd, latencies_prfm_strm_stadd);

	/* Sort the latencies */
	qsort(latencies_llsc, PERCENTILE_ITERATIONS, sizeof(double),
	      compare_double);
	qsort(latencies_lse, PERCENTILE_ITERATIONS, sizeof(double),
	      compare_double);
	qsort(latencies_ldadd, PERCENTILE_ITERATIONS, sizeof(double),
	      compare_double);
	qsort(latencies_prfm_stadd, PERCENTILE_ITERATIONS, sizeof(double),
	      compare_double);
	qsort(latencies_prfm_strm_stadd, PERCENTILE_ITERATIONS, sizeof(double),
	      compare_double);

	/* Calculate percentiles */
	printf("\n CPU: %d - Latency Percentiles:\n", cpu);
	printf("====================\n");
	printf("LL/SC           : ");
	printf("  p50: %06.2f ns\t",
	       calculate_percentile(latencies_llsc, PERCENTILE_ITERATIONS, 50));
	printf("  p95: %06.2f ns\t",
	       calculate_percentile(latencies_llsc, PERCENTILE_ITERATIONS, 95));
	printf("  p99: %06.2f ns\n",
	       calculate_percentile(latencies_llsc, PERCENTILE_ITERATIONS, 99));

	printf("STADD           : ");
	printf("  p50: %06.2f ns\t",
	       calculate_percentile(latencies_lse, PERCENTILE_ITERATIONS, 50));
	printf("  p95: %06.2f ns\t",
	       calculate_percentile(latencies_lse, PERCENTILE_ITERATIONS, 95));
	printf("  p99: %06.2f ns\n",
	       calculate_percentile(latencies_lse, PERCENTILE_ITERATIONS, 99));

	printf("LDADD           : ");
	printf("  p50: %06.2f ns\t",
	       calculate_percentile(latencies_ldadd, PERCENTILE_ITERATIONS, 50));
	printf("  p95: %06.2f ns\t",
	       calculate_percentile(latencies_ldadd, PERCENTILE_ITERATIONS, 95));
	printf("  p99: %06.2f ns\n",
	       calculate_percentile(latencies_ldadd, PERCENTILE_ITERATIONS, 99));

	printf("PRFM_KEEP+STADD : ");
	printf("  p50: %06.2f ns\t",
	       calculate_percentile(latencies_prfm_stadd, PERCENTILE_ITERATIONS, 50));
	printf("  p95: %06.2f ns\t",
	       calculate_percentile(latencies_prfm_stadd, PERCENTILE_ITERATIONS, 95));
	printf("  p99: %06.2f ns\n",
	       calculate_percentile(latencies_prfm_stadd, PERCENTILE_ITERATIONS, 99));

	printf("PRFM_STRM+STADD : ");
	printf("  p50: %06.2f ns\t",
	       calculate_percentile(latencies_prfm_strm_stadd, PERCENTILE_ITERATIONS, 50));
	printf("  p95: %06.2f ns\t",
	       calculate_percentile(latencies_prfm_strm_stadd, PERCENTILE_ITERATIONS, 95));
	printf("  p99: %06.2f ns\n",
	       calculate_percentile(latencies_prfm_strm_stadd, PERCENTILE_ITERATIONS, 99));

	free(latencies_llsc);
	free(latencies_lse);
	free(latencies_ldadd);
	free(latencies_prfm_stadd);
	free(latencies_prfm_strm_stadd);
}
