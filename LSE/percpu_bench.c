/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Benchmark for per-CPU atomic operations
 * ARM64: Compares LL/SC vs LSE implementations
 * x86: Compares CMPXCHG vs LOCK XADD implementations
 */

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "percpu_bench_lib.h"

#ifdef __aarch64__

/* ARM64 LL/SC implementation */
void __percpu_add_case_64_llsc(void *ptr, unsigned long val)
{
	long loop, tmp;

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

/* ARM64 LSE implementation using stadd */
void __percpu_add_case_64_lse(void *ptr, unsigned long val)
{
	asm volatile(
		/* LSE atomics */
		"    stadd    %[val], %[ptr]\n"
		: [ptr] "+Q"(*(u64 *)ptr)
		: [val] "r"((u64)(val))
		: "memory");
}

/* ARM64 LSE implementation using ldadd */
void __percpu_add_case_64_ldadd(void *ptr, unsigned long val)
{
	long tmp;

	asm volatile(
		/* LSE atomics */
		"    ldadd    %[val], %[tmp], %[ptr]\n"
		: [tmp] "=&r"(tmp), [ptr] "+Q"(*(u64 *)ptr)
		: [val] "r"((u64)(val))
		: "memory");
}

/* ARM64 LSE implementation using PRFM + stadd */
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

/* ARM64 LSE implementation using PRFM STRM + stadd */
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

#elif defined(__x86_64__)

/* x86 CMPXCHG loop implementation (equivalent to ARM64 LL/SC) */
void __percpu_add_case_64_llsc(void *ptr, unsigned long val)
{
	long old, new;

	asm volatile(
		/* Compare-and-swap loop */
		"1:  movq    %[ptr], %[old]\n"
		"    leaq    (%[old],%[val]), %[new]\n"
		"    lock cmpxchgq %[new], %[ptr]\n"
		"    jnz     1b"
		: [old] "=&a"(old), [new] "=&r"(new), [ptr] "+m"(*(u64 *)ptr)
		: [val] "r"((u64)(val))
		: "memory", "cc");
}

/* x86 LOCK ADD implementation (equivalent to ARM64 LSE stadd) */
void __percpu_add_case_64_lse(void *ptr, unsigned long val)
{
	asm volatile(
		/* Atomic add without return */
		"    lock addq %[val], %[ptr]\n"
		: [ptr] "+m"(*(u64 *)ptr)
		: [val] "r"((u64)(val))
		: "memory", "cc");
}

/* x86 LOCK XADD implementation (equivalent to ARM64 LSE ldadd) */
void __percpu_add_case_64_ldadd(void *ptr, unsigned long val)
{
	long tmp = val;

	asm volatile(
		/* Atomic add with return of old value */
		"    lock xaddq %[tmp], %[ptr]\n"
		: [tmp] "+r"(tmp), [ptr] "+m"(*(u64 *)ptr)
		:
		: "memory", "cc");
}

/* x86 PREFETCHT0 + LOCK ADD implementation */
void __percpu_add_case_64_prfm_stadd(void *ptr, unsigned long val)
{
	asm volatile(
		/* Prefetch to L1 cache + atomic add */
		"    prefetcht0 %[ptr]\n"
		"    lock addq %[val], %[ptr]\n"
		: [ptr] "+m"(*(u64 *)ptr)
		: [val] "r"((u64)(val))
		: "memory", "cc");
}

/* x86 PREFETCHNTA + LOCK ADD implementation (non-temporal/streaming) */
void __percpu_add_case_64_prfm_strm_stadd(void *ptr, unsigned long val)
{
	asm volatile(
		/* Non-temporal prefetch + atomic add */
		"    prefetchnta %[ptr]\n"
		"    lock addq %[val], %[ptr]\n"
		: [ptr] "+m"(*(u64 *)ptr)
		: [val] "r"((u64)(val))
		: "memory", "cc");
}

#else
#error "Unsupported architecture. Only ARM64 and x86_64 are supported."
#endif

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

#ifdef __aarch64__
	printf("ARM64 Per-CPU Atomic Add Benchmark\n");
	printf("===================================\n");
#elif defined(__x86_64__)
	printf("x86_64 Per-CPU Atomic Add Benchmark\n");
	printf("====================================\n");
#endif

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
