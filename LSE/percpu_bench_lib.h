/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Benchmark for ARM64 per-CPU atomic operations - Library header
 */

#ifndef PERCPU_BENCH_LIB_H
#define PERCPU_BENCH_LIB_H

#include <stdint.h>
#include <stdatomic.h>

#define ITERATIONS (1000UL * 1000 * 1000)
#define WARMUP_ITERATIONS ITERATIONS/1000
#define PERCENTILE_ITERATIONS 100
#define SUB_ITERATIONS (ITERATIONS/PERCENTILE_ITERATIONS)

typedef uint64_t u64;
typedef uint32_t u32;

struct benchmark {
	void (*func)(void *, unsigned long);
	const char *name;
	long contention;
	long duty;
};

struct contender {
	void (*func)(void *, unsigned long);
	u64 *counter;
	long contention;
	int cpu;
	atomic_bool done;
};

/* Global variables used in inline assembly */
extern uint64_t loop, tmp;

/* Atomic operation function declarations (implemented in percpu_bench.c) */
void run_core_benchmark(u64 *counter, double *latencies, void (*func)(void *, unsigned long), long duty);

/* Helper function declarations */
uint64_t get_time_ns(void);
int compare_double(const void *a, const void *b);
double calculate_percentile(double *sorted_array, uint64_t count, double percentile);
int get_num_cpus(void);
int set_cpu_affinity(int cpu);
void run_benchmark_on_cpu(int cpu, struct benchmark *b);

#endif /* PERCPU_BENCH_LIB_H */
