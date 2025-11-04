/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Benchmark for ARM64 parallel atomic operations
 * Compares LL/SC vs LSE implementations under contention
 * Multiple threads access the same shared counter. One atomic operation per
 * CPU, in parallel
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
#include <pthread.h>

#define ITERATIONS 1000000
/* Number of threads touching the same memory region atomically */
#define WARMUP_ITERATIONS ITERATIONS/1000
#define PERCENTILE_ITERATIONS 100
#define SUB_ITERATIONS (ITERATIONS/PERCENTILE_ITERATIONS)

typedef uint64_t u64;
typedef uint32_t u32;

/* Shared counters for parallel access */
static volatile u64 shared_counter_llsc = 0;
static volatile u64 shared_counter_lse = 0;

/* Barrier to synchronize thread start */
static pthread_barrier_t start_barrier;

/* Thread-local data */
struct thread_data {
	int thread_id;
	int cpu;
	double *latencies_llsc;
	double *latencies_lse;
};

/* To be used inside the asm inline code */
__thread uint64_t loop, tmp;

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

static void *thread_benchmark(void *arg)
{
	struct thread_data *data = (struct thread_data *)arg;
	uint64_t start, end;
	uint64_t i, z;

	/* Set CPU affinity */
	if (set_cpu_affinity(data->cpu) != 0) {
		fprintf(stderr, "Thread %d: Failed to set affinity to CPU %d\n",
			data->thread_id, data->cpu);
		return NULL;
	}

	/* Wait for all threads to be ready */
	pthread_barrier_wait(&start_barrier);

	/* Warmup - LL/SC */
	for (i = 0; i < WARMUP_ITERATIONS; i++) {
		__percpu_add_case_64_llsc((void *)&shared_counter_llsc, 1);
	}

	/* Wait for all threads to finish warmup */
	pthread_barrier_wait(&start_barrier);

	/* Warmup - LSE */
	for (i = 0; i < WARMUP_ITERATIONS; i++) {
		__percpu_add_case_64_lse((void *)&shared_counter_lse, 1);
	}

	/* Wait for all threads to finish warmup */
	pthread_barrier_wait(&start_barrier);

	/* Allocate latency arrays */
	data->latencies_llsc = malloc(PERCENTILE_ITERATIONS * sizeof(double));
	data->latencies_lse = malloc(PERCENTILE_ITERATIONS * sizeof(double));

	if (!data->latencies_llsc || !data->latencies_lse) {
		fprintf(stderr, "Thread %d: Failed to allocate memory\n",
			data->thread_id);
		return NULL;
	}

	/* Measure LL/SC latencies under parallel access */
	for (i = 0; i < PERCENTILE_ITERATIONS; i++) {
		start = get_time_ns();
		for (z = 0; z < SUB_ITERATIONS; z++)
			__percpu_add_case_64_llsc((void *)&shared_counter_llsc, 1);
		end = get_time_ns();
		data->latencies_llsc[i] = (double)(end - start) / SUB_ITERATIONS;
	}

	/* Wait for all threads to finish LL/SC measurements */
	pthread_barrier_wait(&start_barrier);

	/* Measure LSE latencies under parallel access */
	for (i = 0; i < PERCENTILE_ITERATIONS; i++) {
		start = get_time_ns();
		for (z = 0; z < SUB_ITERATIONS; z++)
			__percpu_add_case_64_lse((void *)&shared_counter_lse, 1);
		end = get_time_ns();
		data->latencies_lse[i] = (double)(end - start) / SUB_ITERATIONS;
	}

	return NULL;
}

int main(void)
{
	int num_cpus;
	int i;
	pthread_t *threads;
	struct thread_data *thread_data_array;

	printf("ARM64 Parallel Atomic Add Benchmark\n");
	printf("====================================\n");

	printf("Running parallel atomic operations with contention...\n");
	printf("Percentile measurements (%d iterations per thread)...\n",
	       PERCENTILE_ITERATIONS);

	num_cpus = get_num_cpus();
	if (num_cpus <= 0) {
		fprintf(stderr, "Failed to get number of CPUs\n");
		return 1;
	}

	printf("Detected %d CPUs, creating %d threads\n", num_cpus, num_cpus);

	/* Allocate thread structures */
	threads = malloc(num_cpus * sizeof(pthread_t));
	thread_data_array = malloc(num_cpus * sizeof(struct thread_data));

	if (!threads || !thread_data_array) {
		fprintf(stderr, "Failed to allocate memory\n");
		free(threads);
		free(thread_data_array);
		return 1;
	}

	/* Initialize barrier (need 4 sync points: 2 warmups, 2 measurements) */
	pthread_barrier_init(&start_barrier, NULL, num_cpus);

	/* Create threads, one per CPU */
	for (i = 0; i < num_cpus; i++) {
		thread_data_array[i].thread_id = i;
		thread_data_array[i].cpu = i;
		thread_data_array[i].latencies_llsc = NULL;
		thread_data_array[i].latencies_lse = NULL;

		if (pthread_create(&threads[i], NULL, thread_benchmark,
				   &thread_data_array[i]) != 0) {
			fprintf(stderr, "Failed to create thread %d\n", i);
			return 1;
		}
	}

	/* Wait for all threads to complete */
	for (i = 0; i < num_cpus; i++) {
		pthread_join(threads[i], NULL);
	}

	/* Print results for each thread */
	for (i = 0; i < num_cpus; i++) {
		struct thread_data *data = &thread_data_array[i];

		if (!data->latencies_llsc || !data->latencies_lse)
			continue;

		/* Sort the latencies */
		qsort(data->latencies_llsc, PERCENTILE_ITERATIONS,
		      sizeof(double), compare_double);
		qsort(data->latencies_lse, PERCENTILE_ITERATIONS,
		      sizeof(double), compare_double);

		/* Calculate percentiles */
		printf("\n Thread %d (CPU %d) - Latency Percentiles:\n",
		       data->thread_id, data->cpu);
		printf("====================\n");
		printf("LL/SC: ");
		printf("  p50: %07.2f ns\t",
		       calculate_percentile(data->latencies_llsc,
					    PERCENTILE_ITERATIONS, 50));
		printf("  p95: %07.2f ns\t",
		       calculate_percentile(data->latencies_llsc,
					    PERCENTILE_ITERATIONS, 95));
		printf("  p99: %07.2f ns\n",
		       calculate_percentile(data->latencies_llsc,
					    PERCENTILE_ITERATIONS, 99));

		printf("LSE  : ");
		printf("  p50: %07.2f ns\t",
		       calculate_percentile(data->latencies_lse,
					    PERCENTILE_ITERATIONS, 50));
		printf("  p95: %07.2f ns\t",
		       calculate_percentile(data->latencies_lse,
					    PERCENTILE_ITERATIONS, 95));
		printf("  p99: %07.2f ns\n",
		       calculate_percentile(data->latencies_lse,
					    PERCENTILE_ITERATIONS, 99));

		free(data->latencies_llsc);
		free(data->latencies_lse);
	}

	printf("\nShared counters final values:\n");
	printf("LL/SC counter: %lu\n", shared_counter_llsc);
	printf("LSE counter:   %lu\n", shared_counter_lse);
	printf("Expected:      %lu\n",
	       (uint64_t)(WARMUP_ITERATIONS + PERCENTILE_ITERATIONS * SUB_ITERATIONS) * num_cpus);

	/* Cleanup */
	pthread_barrier_destroy(&start_barrier);
	free(threads);
	free(thread_data_array);

	printf("\n=== Benchmark Complete ===\n");
	return 0;
}
