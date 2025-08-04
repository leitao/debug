#include <stdio.h>
#include <stdint.h>

#define M 1000000

/* Example to check the frequency and precision of the clock */

int main() {
    uint64_t cntfrq_el0;
    double precision, freq_in_mhz;

    asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq_el0));

    freq_in_mhz = cntfrq_el0 / M;
    precision = 1/freq_in_mhz;
    printf("cntfrq_el0: %lu\n", cntfrq_el0);
    printf("frequency: %.1f MHz\n", freq_in_mhz);
    printf("precision: %.3f (ns)\n", precision);

    return 0;
}
