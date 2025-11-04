/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Benchmark for ARM64 per-CPU atomic operations
 * Compares LL/SC vs LSE implementations
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define ITERATIONS 200000000
#define WARMUP_ITERATIONS ITERATIONS/1000

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

int main(void)
{
	u64 counter_llsc = 0;
	u64 counter_lse = 0;
	uint64_t start, end;
	/* difference between start and end for each test */
	uint64_t time_llsc, time_lse;
	uint64_t i; /* count number of iterations */

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

	/* Benchmark LL/SC */
	printf("Running LL/SC benchmark (%d iterations)...\n", ITERATIONS);
	start = get_time_ns();
	for (i = 0; i < ITERATIONS; i++) {
		__percpu_add_case_64_llsc(&counter_llsc, 1);
	}
	end = get_time_ns();
	time_llsc = end - start;

	printf("LL/SC final value: %lu\n", counter_llsc);
	printf("LL/SC time: %lu ns (%.2f ns per operation)\n\n", time_llsc,
	       (double)time_llsc / ITERATIONS);

	/* Benchmark LSE */
	printf("Running LSE benchmark (%d iterations)...\n", ITERATIONS);
	start = get_time_ns();
	for (i = 0; i < ITERATIONS; i++) {
		__percpu_add_case_64_lse(&counter_lse, 1);
	}
	end = get_time_ns();
	time_lse = end - start;

	printf("LSE final value: %lu\n", counter_lse);
	printf("LSE time: %lu ns (%.2f ns per operation)\n\n", time_lse,
	       (double)time_lse / ITERATIONS);

	/* Results */
	printf("Results:\n");
	printf("========\n");
	if (time_lse < time_llsc) {
		double speedup = (double)time_llsc / time_lse;
		printf("LSE is FASTER by %.2fx\n", speedup);
		printf("LSE is %.2f%% faster than LL/SC\n",
		       ((double)(time_llsc - time_lse) / time_llsc) * 100);
	} else {
		double speedup = (double)time_lse / time_llsc;
		printf("LL/SC is FASTER by %.2fx\n", speedup);
		printf("LL/SC is %.2f%% faster than LSE\n",
		       ((double)(time_lse - time_llsc) / time_lse) * 100);
	}

	return 0;
}
