#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/auxv.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

/* Meassure context switch benchmark */

#if defined (__aarch64__)
/* Usually 1 GHz, so, 1 nano second per cycle */
static inline uint64_t cycles_arm() {
	uint64_t val;
	asm volatile("mrs %0, cntvct_el0"
		     : "=r"(val));
	return val;
}

/* How many cycles passed.*/
double get_cpu_frequency_arm(void) {
	uint64_t freq;
	asm volatile("mrs %0, cntfrq_el0" : "=r" (freq));
	return freq & 0xffffffff;
}

double measure_getpid_arm(double frequency)
{
	double time_passed_ns;
	uint64_t t0, t1;
	pid_t pid;

	t0 = cycles_arm();
	pid = getpid();
	t1 = cycles_arm();

	/* Avoid compiler optimizations */
	if (!pid)
		printf("Pid 0 ?!\n");

	/* miliseconds -> micro -> nano*/
	time_passed_ns = (t1 - t0) * (1e9 / frequency);

	return time_passed_ns;
}

#elif defined(__x86_64__)

uint64_t cycles_x86(uint32_t *cpu_id)
{
	uint32_t lo, hi;
	asm volatile ("rdtscp" : "=a" (lo), "=d" (hi), "=c" (*cpu_id) :: "memory");

	return ((uint64_t)hi << 32) | lo;
}

/* This one should match the CPU frequency:
 * cat /proc/cpuinfo | grep "cpu MHz"
 */
double get_cpu_frequency_x86() {
	uint32_t cpu0, cpu1;
	uint64_t t0, t1;

	do {
		printf("Calibrating TSC...\n");
		// Get start timestamp
		t0 = cycles_x86(&cpu0);

		// Sleep for 1 second
		sleep(1);

		// Get end timestamp
		t1 = cycles_x86(&cpu1);
		/* Try again if it moved CPUs */
	} while (cpu0 != cpu1);

	// Calculate cycles per second (Hz)
	double cycles_per_second = t1 - t0;

	// Convert to GHz
	return cycles_per_second;
}

static double measure_getpid_x86(double frequency)
{
	uint64_t t0, t1;
	uint32_t cpuid;
	pid_t pid;

	t0 = cycles_x86(&cpuid);
	pid = getpid();
	t1 = cycles_x86(&cpuid);

	if (t0 >= t1) {
		fprintf(stderr, "Out-of-order. Exiting...\n");
		exit(-1);
	}


	if (!pid)
		printf("pid 0 !?\n");

	double time_passed_ns = (t1 - t0) * (1e9 / frequency);

	return time_passed_ns;
}
#else
#error "No architecture set"
#endif

#define COUNT 1000000

static double get_frequency()
{
#if defined(__x86_64__)
	return get_cpu_frequency_x86();
#elif defined (__aarch64__)
	return get_cpu_frequency_arm();
#endif
}

static double measure_getpid(double frequency)
{
#if defined(__x86_64__)
	return measure_getpid_x86(frequency);
#elif defined (__aarch64__)
	return measure_getpid_arm(frequency);
#endif
}

static int compare(const void *a, const void *b) {
	double x = *(double*)a;
	double y = *(double*)b;

	if (x < y) return -1;
	if (x > y) return 1;
	return 0;
}

static void print_percentiles(double *array)
{
	double numbers[] = {50, 90, 95, 99, 99.5, 99.8, 99.9};
	int length = sizeof(numbers) / sizeof(double);

	for (int i =0; i < length; i++) {
		int idx = numbers[i] * (COUNT/100);
		printf("\t- p%.1f = %.2f ns", numbers[i], array[idx]);
	}
}

static void print_data(double avg, double *array)
{
	qsort(array, COUNT, sizeof(double), compare);
	printf("Min: %.2f ns", array[0]);
	/* printf("\t- p50: %.2f ns", array[COUNT / 2]); */
	printf("\t- Average = %.2f ns", avg);
	print_percentiles(array);
	printf("\t- Max: %.2f ns", array[COUNT - 1]);
	printf("\n");
}

static void collect_getpid(size_t count)
{
	double *array = malloc(COUNT * sizeof(double));
	double frequency;
	uint64_t acc = 0;
	double avg = 0;

	if (!array) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(-1);
	}

	frequency = get_frequency();
	printf("Frequency: %f\n", frequency);

	for (size_t i = 0; i < count; i++) {
		for (size_t j = 0 ; j < COUNT; j++) {
			array[j] = measure_getpid(frequency);
			acc += array[j];
		}

		/* Use the first iteration as warmup */
		if (i % 2)
			continue;

		avg = acc / COUNT;
		print_data(avg, array);
		acc = 0;
	}

	free(array);
}

void print_help(const char *name)
{
	fprintf(stderr, "usage: %s <options>\n", name);
	fprintf(stderr, "\t -i \t\t Run in non-stop mode\n");
	fprintf(stderr, "\t -c <count> \t Run <count> iterations\n");

	exit(1);
}
int main(int argc, char **argv)
{
	size_t count = 20;
	int arg;

	while ((arg = getopt (argc, argv, "c:ih")) != -1) {
		switch (arg)
                {
			case 'c':
                                count = atoi(optarg);
                                break;
			case 'i':
                                count = SIZE_MAX;
                                break;
			case 'h':
                                print_help(argv[0]);
		}
	}

	collect_getpid(count);
}
