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

static inline uint64_t cpuid() {
       uint64_t val;
       asm volatile("mrs x0, mpidr_el1" : "=r"(val));

       printf("level 0 = %d\n", (int)(val & 0x0f));
       printf("level 1 = %d\n", (int)((val & 0x0f0) >> 4));
       printf("level 2 = %d\n", (int)((val & 0x0f00) >> 8));

       printf("mpidr_el1= %lx\n", val);
       return val & 0x0F;
}

int main()
{
	cpuid();
}
