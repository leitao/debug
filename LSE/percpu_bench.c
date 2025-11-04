/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Benchmark for ARM64 per-CPU atomic operations
 * Compares LL/SC vs LSE implementations
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ITERATIONS 200000000
#define WARMUP_ITERATIONS ITERATIONS/1000
#define PERCENTILE_ITERATIONS 1000
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

static int compare_uint64(const void *a, const void *b)
{
	uint64_t val_a = *(const uint64_t *)a;
	uint64_t val_b = *(const uint64_t *)b;
	if (val_a < val_b)
		return -1;
	if (val_a > val_b)
		return 1;
	return 0;
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

int main(void)
{
	u64 counter_llsc = 0;
	u64 counter_lse = 0;
	uint64_t start, end;
	/* difference between start and end for each test */
	uint64_t time_llsc, time_lse;
	uint64_t i; /* count number of iterations */
	uint64_t z; 

	printf("ARM64 Per-CPU Atomic Add Benchmark\n");
	printf("===================================\n\n");

	printf("Warming up....\n");
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

	// /* Benchmark LL/SC */
	// printf("Running LL/SC benchmark (%d iterations)...\n", ITERATIONS);
	// start = get_time_ns();
	// for (i = 0; i < ITERATIONS; i++) {
	// 	__percpu_add_case_64_llsc(&counter_llsc, 1);
	// }
	// end = get_time_ns();
	// time_llsc = end - start;

	// printf("LL/SC final value: %lu\n", counter_llsc);
	// printf("LL/SC time: %lu ns (%.2f ns per operation)\n\n", time_llsc,
	//        (double)time_llsc / ITERATIONS);

	// /* Benchmark LSE */
	// printf("Running LSE benchmark (%d iterations)...\n", ITERATIONS);
	// start = get_time_ns();
	// for (i = 0; i < ITERATIONS; i++) {
	// 	__percpu_add_case_64_lse(&counter_lse, 1);
	// }
	// end = get_time_ns();
	// time_lse = end - start;

	// printf("LSE final value: %lu\n", counter_lse);
	// printf("LSE time: %lu ns (%.2f ns per operation)\n\n", time_lse,
	//        (double)time_lse / ITERATIONS);

	/* Percentile measurements */
	printf("Running percentile measurements (%d iterations)...\n",
	       PERCENTILE_ITERATIONS);

	double *latencies_llsc =
		malloc(PERCENTILE_ITERATIONS * sizeof(double));
	double *latencies_lse =
		malloc(PERCENTILE_ITERATIONS * sizeof(double));

	if (!latencies_llsc || !latencies_lse) {
		fprintf(stderr, "Failed to allocate memory for latencies\n");
		return 1;
	}

	/* Measure LL/SC latencies */
	printf("Measuring LL/SC individual operation latencies...\n");
	for (i = 0; i < PERCENTILE_ITERATIONS; i++) {
		start = get_time_ns();
		for (z = 0; z < SUB_ITERATIONS; z++)
			__percpu_add_case_64_llsc(&counter_llsc, 1);
		end = get_time_ns();
		latencies_llsc[i] = (double)(end - start) / SUB_ITERATIONS;
	}

	/* Measure LSE latencies */
	printf("Measuring LSE individual operation latencies...\n");
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
	printf("\nLatency Percentiles:\n");
	printf("====================\n");
	printf("LL/SC:\n");
	printf("  p50: %.2f ns\n",
	       calculate_percentile(latencies_llsc, PERCENTILE_ITERATIONS, 50));
	printf("  p95: %.2f ns\n",
	       calculate_percentile(latencies_llsc, PERCENTILE_ITERATIONS, 95));
	printf("  p99: %.2f ns\n\n",
	       calculate_percentile(latencies_llsc, PERCENTILE_ITERATIONS, 99));

	printf("LSE:\n");
	printf("  p50: %.2f ns\n",
	       calculate_percentile(latencies_lse, PERCENTILE_ITERATIONS, 50));
	printf("  p95: %.2f ns\n",
	       calculate_percentile(latencies_lse, PERCENTILE_ITERATIONS, 95));
	printf("  p99: %.2f ns\n\n",
	       calculate_percentile(latencies_lse, PERCENTILE_ITERATIONS, 99));

	free(latencies_llsc);
	free(latencies_lse);

	return 0;
}
