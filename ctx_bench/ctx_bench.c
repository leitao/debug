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

static inline uint64_t get_cntvct() {
	uint64_t val;
	asm volatile("mrs %0, cntvct_el0"
		     : "=r"(val));
	return val;
}

static inline uint64_t cpuid() {
	uint64_t val;
	asm volatile("mrs x0, mpidr_el1" : "=r"(val));

	printf("level 0 = %d\n", val & 0x0f);
	printf("level 1 = %d\n", (val & 0x0f0) >> 4);
	printf("level 2 = %d\n", (val & 0x0f00) >> 8);
	printf("mpidr_el1= %lx\n", val);
	return val & 0x0F;
}

int main(int argc, char **argv)
{
	uint64_t t0, t1, cpu;
	pid_t pid;

	cpu = cpuid();
	t0 = get_cntvct();
	pid = getpid();
	t1 = get_cntvct();

	/* Avoid compiler optimizations */
	if (pid == 0)
		printf("Pid 0 ?!\n");

	printf("cycles to call gettpid() = %ld \n", t1 - t0, cpu);

}
