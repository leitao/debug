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

/* Usually 1 GHz, so, 1 nano second per cycle */
static inline uint64_t get_cntvct() {
	uint64_t val;
	asm volatile("mrs %0, cntvct_el0"
		     : "=r"(val));
	return val;
}

/* How many cycles passed.*/
uint64_t get_cpu_frequency(void) {
	uint64_t freq;
	asm volatile("mrs %0, cntfrq_el0" : "=r" (freq));
	return freq & 0xffffffff;
}

uint64_t measure_getpid()
{
	uint64_t t0, t1, frequency, time_passed_ns;
	pid_t pid;

	t0 = get_cntvct();
	pid = getpid();
	t1 = get_cntvct();

	/* Avoid compiler optimizations */
	if (pid == 0)
		printf("Pid 0 ?!\n");

	frequency = get_cpu_frequency();

	/* miliseconds -> micro -> nano*/
	time_passed_ns = (t1 - t0) * (1000*1000*1000 / frequency);

	return time_passed_ns;
}

#define MEGA (1000*1000)

void collect_getpid(size_t count)
{
	uint64_t acc = 0;
	float avg = 0;

	for (size_t i = 0; i < count; i++) {
		for (size_t x = 0 ; x < MEGA; x++) {
			acc += measure_getpid();
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
