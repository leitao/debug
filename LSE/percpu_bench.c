/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Benchmark for ARM64 per-CPU atomic operations
 * Compares LL/SC vs LSE implementations
 */

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "percpu_bench_lib.h"

/* To be used inside the asm inline code */
uint64_t loop, tmp;

/* LL/SC implementation */
void __percpu_add_case_64_llsc(void *ptr, unsigned long val)
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

/* LSE implementation using stadd */
void __percpu_add_case_64_lse(void *ptr, unsigned long val)
{
	asm volatile(
		/* LSE atomics */
		"    stadd    %[val], %[ptr]\n"
		: [ptr] "+Q"(*(u64 *)ptr)
		: [val] "r"((u64)(val))
		: "memory");
}

/* LSE implementation using ldadd */
void __percpu_add_case_64_ldadd(void *ptr, unsigned long val)
{
	asm volatile(
		/* LSE atomics */
		"    ldadd    %[val], %[tmp], %[ptr]\n"
		: [tmp] "=&r"(tmp), [ptr] "+Q"(*(u64 *)ptr)
		: [val] "r"((u64)(val))
		: "memory");
}

/* LSE implementation using PRFM + stadd */
void __percpu_add_case_64_prfm_stadd(void *ptr, unsigned long val)
{
	asm volatile(
		/* Prefetch + LSE atomics */
		"    prfm    pstl1keep, %[ptr]\n"
		"    stadd   %[val], %[ptr]\n"
		: [ptr] "+Q"(*(u64 *)ptr)
		: [val] "r"((u64)(val))
		: "memory");
}

/* LSE implementation using PRFM STRM + stadd */
void __percpu_add_case_64_prfm_strm_stadd(void *ptr, unsigned long val)
{
	asm volatile(
		/* Prefetch streaming + LSE atomics */
		"    prfm    pstl1strm, %[ptr]\n"
		"    stadd   %[val], %[ptr]\n"
		: [ptr] "+Q"(*(u64 *)ptr)
		: [val] "r"((u64)(val))
		: "memory");
}

/* Core benchmark measurement function */
void run_core_benchmark(u64 *counter_llsc, u64 *counter_lse, u64 *counter_ldadd,
			u64 *counter_prfm_stadd, u64 *counter_prfm_strm_stadd,
			double *latencies_llsc, double *latencies_lse,
			double *latencies_ldadd, double *latencies_prfm_stadd,
			double *latencies_prfm_strm_stadd)
{
	uint64_t start, end;
	uint64_t i, z;

	/* Measure LL/SC latencies */
	for (i = 0; i < PERCENTILE_ITERATIONS; i++) {
		start = get_time_ns();
		for (z = 0; z < SUB_ITERATIONS; z++)
			__percpu_add_case_64_llsc(counter_llsc, 1);
		end = get_time_ns();
		latencies_llsc[i] = (double)(end - start) / SUB_ITERATIONS;
	}

	/* Measure LSE (stadd) latencies */
	for (i = 0; i < PERCENTILE_ITERATIONS; i++) {
		start = get_time_ns();
		for (z = 0; z < SUB_ITERATIONS; z++)
			__percpu_add_case_64_lse(counter_lse, 1);
		end = get_time_ns();
		latencies_lse[i] = (double)(end - start) / SUB_ITERATIONS;
	}

	/* Measure LDADD latencies */
	for (i = 0; i < PERCENTILE_ITERATIONS; i++) {
		start = get_time_ns();
		for (z = 0; z < SUB_ITERATIONS; z++)
			__percpu_add_case_64_ldadd(counter_ldadd, 1);
		end = get_time_ns();
		latencies_ldadd[i] = (double)(end - start) / SUB_ITERATIONS;
	}

	/* Measure PRFM+STADD latencies */
	for (i = 0; i < PERCENTILE_ITERATIONS; i++) {
		start = get_time_ns();
		for (z = 0; z < SUB_ITERATIONS; z++)
			__percpu_add_case_64_prfm_stadd(counter_prfm_stadd, 1);
		end = get_time_ns();
		latencies_prfm_stadd[i] = (double)(end - start) / SUB_ITERATIONS;
	}

	/* Measure PRFM_STRM+STADD latencies */
	for (i = 0; i < PERCENTILE_ITERATIONS; i++) {
		start = get_time_ns();
		for (z = 0; z < SUB_ITERATIONS; z++)
			__percpu_add_case_64_prfm_strm_stadd(counter_prfm_strm_stadd, 1);
		end = get_time_ns();
		latencies_prfm_strm_stadd[i] = (double)(end - start) / SUB_ITERATIONS;
	}
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
