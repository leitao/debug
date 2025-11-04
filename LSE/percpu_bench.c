/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Benchmark for ARM64 per-CPU atomic operations
 * Compares LL/SC vs LSE implementations
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

#define ITERATIONS 100000000
#define WARMUP_ITERATIONS ITERATIONS/1000
#define PERCENTILE_ITERATIONS 100
#define SUB_ITERATIONS (ITERATIONS/PERCENTILE_ITERATIONS)

typedef uint64_t u64;
typedef uint32_t u32;

/* To be used inside the asm inline code */
uint64_t loop, tmp;

/* LL/SC implementation */
static inline void __percpu_add_case_64_llsc(void *ptr, unsigned long val)
{
	asm volatile(
		/* LL/SC */
		"1:  ldxr    %[tmp], %[ptr]\n"
		"    add     %[tmp], %[tmp], %[val]\n"
		"    stxr    %w[loop], %[tmp], %[ptr]\n"
		"    cbnz    %w[loop], 1b"
		: [loop] "=&r"(loop), [tmp] "=&r"(tmp), [ptr] "+Q"(*(u64 *)ptr)
		: [val] "r"((u64)(val))
		: "memory");
}

/* LSE implementation */
static inline void __percpu_add_case_64_lse(void *ptr, unsigned long val)
{
	asm volatile(
		/* LSE atomics */
		"    stadd    %[val], %[ptr]\n"
		: [ptr] "+Q"(*(u64 *)ptr)
		: [val] "r"((u64)(val))
		: "memory");
}

static inline uint64_t get_time_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static int compare_double(const void *a, const void *b)
{
	double val_a = *(const double *)a;
	double val_b = *(const double *)b;
	if (val_a < val_b)
		return -1;
	if (val_a > val_b)
		return 1;
	return 0;
}

static double calculate_percentile(double *sorted_array, uint64_t count,
				      double percentile)
{
	uint64_t index = (uint64_t)((percentile / 100.0) * (count - 1));
	return sorted_array[index];
}

static int get_num_cpus(void)
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

static int set_cpu_affinity(int cpu)
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

static void run_benchmark_on_cpu(int cpu)
{
	u64 counter_llsc = 0;
	u64 counter_lse = 0;
	uint64_t start, end;
	uint64_t i; /* count number of iterations */
	uint64_t z;

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

	/* Warmup - LSE */
	for (i = 0; i < WARMUP_ITERATIONS; i++) {
		__percpu_add_case_64_lse(&counter_lse, 1);
	}
	counter_lse = 0;

	/* Percentile measurements */
	double *latencies_llsc =
		malloc(PERCENTILE_ITERATIONS * sizeof(double));
	double *latencies_lse =
		malloc(PERCENTILE_ITERATIONS * sizeof(double));

	if (!latencies_llsc || !latencies_lse) {
		fprintf(stderr, "Failed to allocate memory for latencies\n");
		free(latencies_llsc);
		free(latencies_lse);
		return;
	}

	/* Measure LL/SC latencies */
	for (i = 0; i < PERCENTILE_ITERATIONS; i++) {
		start = get_time_ns();
		for (z = 0; z < SUB_ITERATIONS; z++)
			__percpu_add_case_64_llsc(&counter_llsc, 1);
		end = get_time_ns();
		latencies_llsc[i] = (double)(end - start) / SUB_ITERATIONS;
	}

	/* Measure LSE latencies */
	for (i = 0; i < PERCENTILE_ITERATIONS; i++) {
		start = get_time_ns();
		for (z = 0; z < SUB_ITERATIONS; z++)
			__percpu_add_case_64_lse(&counter_lse, 1);
		end = get_time_ns();
		latencies_lse[i] = (double)(end - start) / SUB_ITERATIONS;
	}

	/* Sort the latencies */
	qsort(latencies_llsc, PERCENTILE_ITERATIONS, sizeof(double),
	      compare_double);
	qsort(latencies_lse, PERCENTILE_ITERATIONS, sizeof(double),
	      compare_double);

	/* Calculate percentiles */
	printf("\n CPU: %d - Latency Percentiles:\n", cpu);
	printf("====================\n");
	printf("LL/SC: ");
	printf("  p50: %.2f ns\t",
	       calculate_percentile(latencies_llsc, PERCENTILE_ITERATIONS, 50));
	printf("  p95: %.2f ns\t",
	       calculate_percentile(latencies_llsc, PERCENTILE_ITERATIONS, 95));
	printf("  p99: %.2f ns\n",
	       calculate_percentile(latencies_llsc, PERCENTILE_ITERATIONS, 99));

	printf("LSE  : ");
	printf("  p50: %.2f ns\t",
	       calculate_percentile(latencies_lse, PERCENTILE_ITERATIONS, 50));
	printf("  p95: %.2f ns\t",
	       calculate_percentile(latencies_lse, PERCENTILE_ITERATIONS, 95));
	printf("  p99: %.2f ns\n",
	       calculate_percentile(latencies_lse, PERCENTILE_ITERATIONS, 99));

	free(latencies_llsc);
	free(latencies_lse);
}

int main(void)
{
	int num_cpus;
	int i;

	printf("ARM64 Per-CPU Atomic Add Benchmark\n");
	printf("===================================\n");

	printf("Running percentile measurements (%d iterations)...\n",
	       PERCENTILE_ITERATIONS);

	num_cpus = get_num_cpus();
	if (num_cpus <= 0) {
		fprintf(stderr, "Failed to get number of CPUs\n");
		return 1;
	}

	printf("Detected %d CPUs\n", num_cpus);

	/* Run benchmark on each CPU */
	for (i = 0; i < num_cpus; i++) {
		run_benchmark_on_cpu(i);
	}

	printf("\n=== Benchmark Complete ===\n");
	return 0;
}
