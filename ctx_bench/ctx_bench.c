#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/auxv.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>

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

double get_cpu_frequency_x86() {
	uint32_t cpu0, cpu1;
	uint64_t t0, t1;

	do {
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

#define MEGA 1e6

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

static void collect_getpid(size_t count)
{
	uint64_t acc = 0;
	double avg = 0;
	double frequency;

	frequency = get_frequency();

	for (size_t i = 0; i < count; i++) {
		for (size_t x = 0 ; x < MEGA; x++) {
			acc += measure_getpid(frequency);
		}

		avg = acc / MEGA;
		printf("Average= %.2f ns\n", avg);
		acc = 0;
	}
}

int main(int argc, char **argv)
{
	size_t count = 5;
	int arg;

	while ((arg = getopt (argc, argv, "ht:p:c:s")) != -1) {
		switch (arg)
                {
			case 'c':
                                count = atoi(optarg);
                                break;
		}
	}

	collect_getpid(count);
}
