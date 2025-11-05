/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Benchmark for ARM64 per-CPU atomic operations - Library header
 */

#ifndef PERCPU_BENCH_LIB_H
#define PERCPU_BENCH_LIB_H

#include <stdint.h>

#define ITERATIONS 100000000
#define WARMUP_ITERATIONS ITERATIONS/1000
#define PERCENTILE_ITERATIONS 100
#define SUB_ITERATIONS (ITERATIONS/PERCENTILE_ITERATIONS)

typedef uint64_t u64;
typedef uint32_t u32;

/* Global variables used in inline assembly */
extern uint64_t loop, tmp;

/* Atomic operation function declarations (implemented in percpu_bench.c) */
void __percpu_add_case_64_llsc(void *ptr, unsigned long val);
void __percpu_add_case_64_lse(void *ptr, unsigned long val);
void __percpu_add_case_64_ldadd(void *ptr, unsigned long val);
void __percpu_add_case_64_prfm_stadd(void *ptr, unsigned long val);
void __percpu_add_case_64_prfm_strm_stadd(void *ptr, unsigned long val);
void run_core_benchmark(u64 *counter_llsc, u64 *counter_lse, u64 *counter_ldadd,
			u64 *counter_prfm_stadd, u64 *counter_prfm_strm_stadd,
			double *latencies_llsc, double *latencies_lse,
			double *latencies_ldadd, double *latencies_prfm_stadd,
			double *latencies_prfm_strm_stadd);

/* Helper function declarations */
uint64_t get_time_ns(void);
int compare_double(const void *a, const void *b);
double calculate_percentile(double *sorted_array, uint64_t count, double percentile);
int get_num_cpus(void);
int set_cpu_affinity(int cpu);
void run_benchmark_on_cpu(int cpu);

#endif /* PERCPU_BENCH_LIB_H */
