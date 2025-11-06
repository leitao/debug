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
void run_core_benchmark(u64 *counter, double *latencies, void (*func)(void *, unsigned long), long duty)
{
	uint64_t start, end;
	uint64_t i, z, d;

	for (i = 0; i < PERCENTILE_ITERATIONS; i++) {
		start = get_time_ns();
		for (z = 0; z < SUB_ITERATIONS; z++) {
			func(counter, 1);
			for (d = 0; d < duty; d++)
				__asm__ volatile ("nop");
		}
		end = get_time_ns();
		latencies[i] = (double)(end - start) / SUB_ITERATIONS;
	}
}

int main(void)
{
	int num_cpus;
	int i, b;

	struct benchmark benchmarks[] = {
		{
			.func = __percpu_add_case_64_lse,
			.name = "LSE (stadd)    ",
			.contention = 0,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_lse,
			.name = "LSE (stadd)    ",
			.contention = 0,
			.duty = 100,
		}, {
			.func = __percpu_add_case_64_lse,
			.name = "LSE (stadd)    ",
			.contention = 0,
			.duty = 200,
		}, {
			.func = __percpu_add_case_64_lse,
			.name = "LSE (stadd)    ",
			.contention = 10,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_lse,
			.name = "LSE (stadd)    ",
			.contention = 10,
			.duty = 300,
		}, {
			.func = __percpu_add_case_64_lse,
			.name = "LSE (stadd)    ",
			.contention = 10,
			.duty = 500,
		}, {
			.func = __percpu_add_case_64_lse,
			.name = "LSE (stadd)    ",
			.contention = 30,
			.duty = 0,
		},
		{
			.func = __percpu_add_case_64_llsc,
			.name = "LL/SC          ",
			.contention = 0,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_llsc,
			.name = "LL/SC          ",
			.contention = 0,
			.duty = 10,
		}, {
			.func = __percpu_add_case_64_llsc,
			.name = "LL/SC          ",
			.contention = 0,
			.duty = 20,
		}, {
			.func = __percpu_add_case_64_llsc,
			.name = "LL/SC          ",
			.contention = 10,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_llsc,
			.name = "LL/SC          ",
			.contention = 10,
			.duty = 10,
		}, {
			.func = __percpu_add_case_64_llsc,
			.name = "LL/SC          ",
			.contention = 10,
			.duty = 20,
		}, {
			.func = __percpu_add_case_64_llsc,
			.name = "LL/SC          ",
			.contention = 1000,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_llsc,
			.name = "LL/SC          ",
			.contention = 1000,
			.duty = 10,
		}, {
			.func = __percpu_add_case_64_llsc,
			.name = "LL/SC          ",
			.contention = 1000000,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_llsc,
			.name = "LL/SC          ",
			.contention = 1000000,
			.duty = 10,
		},
		{
			.func = __percpu_add_case_64_ldadd,
			.name = "LDADD          ",
			.contention = 0,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_ldadd,
			.name = "LDADD          ",
			.contention = 0,
			.duty = 100,
		}, {
			.func = __percpu_add_case_64_ldadd,
			.name = "LDADD          ",
			.contention = 0,
			.duty = 200,
		}, {
			.func = __percpu_add_case_64_ldadd,
			.name = "LDADD          ",
			.contention = 0,
			.duty = 300,
		}, {
			.func = __percpu_add_case_64_ldadd,
			.name = "LDADD          ",
			.contention = 1,
			.duty = 10,
		}, {
			.func = __percpu_add_case_64_ldadd,
			.name = "LDADD          ",
			.contention = 10,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_ldadd,
			.name = "LDADD          ",
			.contention = 10,
			.duty = 10,
		}, {
			.func = __percpu_add_case_64_ldadd,
			.name = "LDADD          ",
			.contention = 100,
			.duty = 0,
		},
		{
			.func = __percpu_add_case_64_prfm_stadd,
			.name = "PFRM_KEEP+STADD",
			.contention = 0,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_prfm_stadd,
			.name = "PFRM_KEEP+STADD",
			.contention = 10,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_prfm_stadd,
			.name = "PFRM_KEEP+STADD",
			.contention = 1000,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_prfm_stadd,
			.name = "PFRM_KEEP+STADD",
			.contention = 1000000,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_prfm_strm_stadd,
			.name = "PFRM_STRM+STADD",
			.contention = 0,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_prfm_strm_stadd,
			.name = "PFRM_STRM+STADD",
			.contention = 10,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_prfm_strm_stadd,
			.name = "PFRM_STRM+STADD",
			.contention = 1000,
			.duty = 0,
		}, {
			.func = __percpu_add_case_64_prfm_strm_stadd,
			.name = "PFRM_STRM+STADD",
			.contention = 1000000,
			.duty = 0,
		},
	};

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
		printf("\n CPU: %d - Latency Percentiles:\n", i);
		printf("====================\n");

		for (b = 0; b < (sizeof(benchmarks) / sizeof(benchmarks[0])); ++b)
			run_benchmark_on_cpu(i, benchmarks + b);
	}

	printf("\n=== Benchmark Complete ===\n");
	return 0;
}
