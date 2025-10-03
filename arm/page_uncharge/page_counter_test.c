/*
 * Userspace test for page_counter_uncharge() function
 *
 * This program creates cgroup hierarchies and performs memory operations
 * that will indirectly exercise the page_counter_uncharge() function in
 * the kernel when memory is freed.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

/* Test configuration */
#define MAX_HIERARCHY_DEPTH 8
#define MAX_PATH_LEN 256
#define MEMORY_SIZE (1 * 1024 * 1024 * 1024UL) /* 1G per test */
#define ALLOCATION_SIZE 4096UL			 /* 4KB allocations */
#define NUM_ALLOCATIONS (MEMORY_SIZE / ALLOCATION_SIZE)
#define NUM_THREADS 4
#define STRESS_ITERATIONS 1000

/* Cgroup paths */
#define CGROUP_V2_MOUNT "/sys/fs/cgroup"
#define TEST_CGROUP_BASE "page_counter_test"

struct test_config {
	char base_path[MAX_PATH_LEN - 20];
	int hierarchy_depth;
	int num_threads;
};

struct perf_result {
	double single_level_ms;
	double shallow_hierarchy_ms;
	double deep_hierarchy_ms;
	double stress_test_ms;
	double parallel_test_ms;
};

static struct test_config config;
static struct perf_result results;

/*
 * Utility functions
 */
static double timespec_to_ms(struct timespec *ts)
{
	return ts->tv_sec * 1000.0 + ts->tv_nsec / 1000000.0;
}

static int write_to_file(const char *path, const char *content)
{
	FILE *f = fopen(path, "w");
	if (!f) {
		fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
		return -1;
	}

	if (fprintf(f, "%s", content) < 0) {
		fprintf(stderr, "Failed to write to %s: %s\n", path, strerror(errno));
		fclose(f);
		return -1;
	}

	fclose(f);
	return 0;
}


/*
 * Cgroup setup (cgroup v2 only)
 */

static int setup_cgroup_hierarchy(int depth, char paths[][MAX_PATH_LEN])
{
	int i;

	printf("Setting up cgroup hierarchy with depth %d\n", depth);

	for (i = 0; i < depth; i++) {
		if (i == 0) {
			snprintf(paths[i], MAX_PATH_LEN, "%s/%s",
					config.base_path, TEST_CGROUP_BASE);
		} else {
			snprintf(paths[i], MAX_PATH_LEN, "%s/level_%d",
					paths[i-1], i);
		}

		/* Create directory */
		if (mkdir(paths[i], 0755) < 0 && errno != EEXIST) {
			fprintf(stderr, "Failed to create cgroup %s: %s\n",
					paths[i], strerror(errno));
			return -1;
		}

		/* Enable memory controller for cgroup v2 */
		if (i < depth - 1) {
			char subtree_control[MAX_PATH_LEN];
			snprintf(subtree_control, sizeof(subtree_control),
					"%s/cgroup.subtree_control", paths[i]);
			write_to_file(subtree_control, "+memory");
		}

		printf("Created cgroup level %d: %s\n", i, paths[i]);
	}

	return 0;
}

static int move_out_cgroup()
{
	char procs_path[MAX_PATH_LEN];
	char pid_str[32];


	sprintf(procs_path, "/sys/fs/cgroup/cgroup.procs");
	snprintf(pid_str, sizeof(pid_str), "%d", getpid());

	return write_to_file(procs_path, pid_str);
}

static void cleanup_cgroup_hierarchy(int depth, char paths[][MAX_PATH_LEN])
{
	int i;
	/* Need to move pid out so we can delete */
	move_out_cgroup();

	/* Remove cgroups in reverse order */
	for (i = depth - 1; i >= 0; i--) {
		if (rmdir(paths[i]) < 0 && errno != ENOENT) {
			fprintf(stderr, "Failed to remove cgroup %s: %s\n",
					paths[i], strerror(errno));
			exit(-1);
		}
	}
}

static int move_to_cgroup(const char *cgroup_path)
{
	char procs_path[MAX_PATH_LEN * 2];
	char pid_str[32];

	snprintf(procs_path, sizeof(procs_path), "%s/cgroup.procs", cgroup_path);
	snprintf(pid_str, sizeof(pid_str), "%d", getpid());

	return write_to_file(procs_path, pid_str);
}


/*
 * Memory allocation/deallocation functions
 */
static void **allocate_memory_chunks(int num_chunks, size_t chunk_size)
{
	void **chunks = malloc(num_chunks * sizeof(void *));
	int i;

	if (!chunks) {
		return NULL;
	}

	// printf("Allocating %d chunks of %lu\n", num_chunks, chunk_size);
	for (i = 0; i < num_chunks; i++) {
		chunks[i] = malloc(chunk_size);
		if (!chunks[i]) {
			/* Cleanup on failure */
			for (i--; i >= 0; i--) {
				free(chunks[i]);
			}
			free(chunks);
			return NULL;
		}

		/* Touch the memory to ensure it's actually allocated */
		memset(chunks[i], i % 256, chunk_size);
	}

	return chunks;
}

static void free_memory_chunks(void **chunks, int num_chunks)
{
	int i;

	if (!chunks) {
		return;
	}

	for (i = 0; i < num_chunks; i++) {
		if (chunks[i]) {
			free(chunks[i]);
		}
	}

	free(chunks);
}

/*
 * Test Case 1: Single level cgroup
 */
static double test_single_level(void)
{
	char paths[1][MAX_PATH_LEN];
	void **chunks;
	struct timespec start, end;
	double elapsed;

	printf("\n=== Test 1: Single Level Cgroup ===\n");

	if (setup_cgroup_hierarchy(1, paths) < 0) {
		return -1;
	}

	if (move_to_cgroup(paths[0]) < 0) {
		fprintf(stderr, "Failed to move to cgroup\n");
		cleanup_cgroup_hierarchy(1, paths);
		return -1;
	}

	/* Allocate memory */
	chunks = allocate_memory_chunks(NUM_ALLOCATIONS, ALLOCATION_SIZE);
	if (!chunks) {
		fprintf(stderr, "Failed to allocate memory\n");
		cleanup_cgroup_hierarchy(1, paths);
		return -1;
	}

	/* Measure free time (this triggers page_counter_uncharge) */
	clock_gettime(CLOCK_MONOTONIC, &start);
	free_memory_chunks(chunks, NUM_ALLOCATIONS);
	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed = timespec_to_ms(&end) - timespec_to_ms(&start);

	cleanup_cgroup_hierarchy(1, paths);

	printf("Single level free time: %.2f ms\n", elapsed);
	return elapsed;
}

/*
 * Test Case 2: Shallow hierarchy (3 levels)
 */
static double test_shallow_hierarchy(void)
{
	char paths[3][MAX_PATH_LEN];
	void **chunks;
	struct timespec start, end;
	double elapsed;

	printf("\n=== Test 2: Shallow Hierarchy (3 levels) ===\n");

	if (setup_cgroup_hierarchy(3, paths) < 0) {
		return -1;
	}

	/* Move to deepest cgroup */
	if (move_to_cgroup(paths[2]) < 0) {
		fprintf(stderr, "Failed to move to cgroup\n");
		cleanup_cgroup_hierarchy(3, paths);
		return -1;
	}

	chunks = allocate_memory_chunks(NUM_ALLOCATIONS, ALLOCATION_SIZE);
	if (!chunks) {
		fprintf(stderr, "Failed to allocate memory\n");
		cleanup_cgroup_hierarchy(3, paths);
		return -1;
	}

	clock_gettime(CLOCK_MONOTONIC, &start);
	free_memory_chunks(chunks, NUM_ALLOCATIONS);
	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed = timespec_to_ms(&end) - timespec_to_ms(&start);

	cleanup_cgroup_hierarchy(3, paths);

	printf("Shallow hierarchy free time: %.2f ms\n", elapsed);
	return elapsed;
}

/*
 * Test Case 3: Deep hierarchy (8 levels)
 */
static double test_deep_hierarchy(void)
{
	char paths[MAX_HIERARCHY_DEPTH][MAX_PATH_LEN];
	void **chunks;
	struct timespec start, end;
	double elapsed;

	printf("\n=== Test 3: Deep Hierarchy (%d levels) ===\n", MAX_HIERARCHY_DEPTH);

	if (setup_cgroup_hierarchy(MAX_HIERARCHY_DEPTH, paths) < 0) {
		return -1;
	}

	/* Move to deepest cgroup */
	if (move_to_cgroup(paths[MAX_HIERARCHY_DEPTH-1]) < 0) {
		fprintf(stderr, "Failed to move to cgroup\n");
		cleanup_cgroup_hierarchy(MAX_HIERARCHY_DEPTH, paths);
		return -1;
	}

	chunks = allocate_memory_chunks(NUM_ALLOCATIONS, ALLOCATION_SIZE);
	if (!chunks) {
		fprintf(stderr, "Failed to allocate memory\n");
		cleanup_cgroup_hierarchy(MAX_HIERARCHY_DEPTH, paths);
		return -1;
	}

	clock_gettime(CLOCK_MONOTONIC, &start);
	free_memory_chunks(chunks, NUM_ALLOCATIONS);
	clock_gettime(CLOCK_MONOTONIC, &end);

	elapsed = timespec_to_ms(&end) - timespec_to_ms(&start);

	cleanup_cgroup_hierarchy(MAX_HIERARCHY_DEPTH, paths);

	printf("Deep hierarchy free time: %.2f ms\n", elapsed);
	return elapsed;
}

/*
 * Stress test worker
 */
struct thread_data {
	int thread_id;
	char cgroup_path[MAX_PATH_LEN];
	double elapsed_ms;
};

static void *stress_worker(void *arg)
{
	struct thread_data *data = (struct thread_data *)arg;
	void **chunks;
	struct timespec start, end;
	int i;

	/* Move to assigned cgroup */
	if (move_to_cgroup(data->cgroup_path) < 0) {
		fprintf(stderr, "Thread %d: Failed to move to cgroup\n", data->thread_id);
		return NULL;
	}

	clock_gettime(CLOCK_MONOTONIC, &start);

	/* Perform many small allocations and frees */
	for (i = 0; i < STRESS_ITERATIONS; i++) {
		chunks = allocate_memory_chunks(10, ALLOCATION_SIZE);
		if (chunks) {
			free_memory_chunks(chunks, 10);
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	data->elapsed_ms = timespec_to_ms(&end) - timespec_to_ms(&start);

	printf("Thread %d completed in %.2f ms\n", data->thread_id, data->elapsed_ms);
	return NULL;
}

/*
 * Test Case 4: Parallel stress test
 */
static double test_parallel_stress(void)
{
	char paths[MAX_HIERARCHY_DEPTH][MAX_PATH_LEN];
	pthread_t threads[NUM_THREADS];
	struct thread_data thread_data[NUM_THREADS];
	double total_elapsed = 0;
	int i;

	printf("\n=== Test 4: Parallel Stress Test (%d threads) ===\n", NUM_THREADS);

	if (setup_cgroup_hierarchy(MAX_HIERARCHY_DEPTH, paths) < 0) {
		return -1;
	}

	/* Create worker threads */
	for (i = 0; i < NUM_THREADS; i++) {
		thread_data[i].thread_id = i;
		strcpy(thread_data[i].cgroup_path, paths[MAX_HIERARCHY_DEPTH-1]);
		thread_data[i].elapsed_ms = 0;

		if (pthread_create(&threads[i], NULL, stress_worker, &thread_data[i]) != 0) {
			fprintf(stderr, "Failed to create thread %d\n", i);
			break;
		}
	}

	/* Wait for threads to complete */
	for (i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
		total_elapsed += thread_data[i].elapsed_ms;
	}

	cleanup_cgroup_hierarchy(MAX_HIERARCHY_DEPTH, paths);

	printf("Total parallel time: %.2f ms (avg per thread: %.2f ms)\n",
		   total_elapsed, total_elapsed / NUM_THREADS);
	return total_elapsed / NUM_THREADS;
}

/*
 * Print summary results
 */
static void print_summary(void)
{
	printf("\n=== Performance Summary ===\n");
	printf("Single level:	  %.2f ms\n", results.single_level_ms);
	printf("Shallow hierarchy: %.2f ms\n", results.shallow_hierarchy_ms);
	printf("Deep hierarchy:	%.2f ms\n", results.deep_hierarchy_ms);
	printf("Parallel stress:   %.2f ms (avg per thread)\n", results.parallel_test_ms);
}

/*
 * Main function
 */
int main() {
	printf("page_counter_uncharge Performance Test\n");
	printf("=====================================\n");

	/* Setup configuration */
	strcpy(config.base_path, CGROUP_V2_MOUNT);
	config.hierarchy_depth = MAX_HIERARCHY_DEPTH;
	config.num_threads = NUM_THREADS;

	printf("Using cgroup v2 at %s\n", config.base_path);
	printf("Test parameters: %lu MB memory, %lu KB chunks, %lu allocations\n",
		   MEMORY_SIZE / (1024*1024), ALLOCATION_SIZE / 1024, NUM_ALLOCATIONS);

	/* Check if we have permissions */
	if (geteuid() != 0) {
		printf("Warning: Running as non-root. Some operations may fail.\n");
	}

	/* Run tests */
	results.single_level_ms = test_single_level();
	results.shallow_hierarchy_ms = test_shallow_hierarchy();
	results.deep_hierarchy_ms = test_deep_hierarchy();
	results.parallel_test_ms = test_parallel_stress();

	/* Print results */
	print_summary();

	return 0;
}
