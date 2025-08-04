#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define M 1000000

int main() {
    uint64_t cntfrq_el0;
    double freq_in_mhz, precision_ns;

    asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq_el0));

    freq_in_mhz = (double)cntfrq_el0 / M;
    precision_ns = 1000.0 / freq_in_mhz;  // ns per tick

    printf("cntfrq_el0: %" PRIu64 "\n", cntfrq_el0);
    printf("Frequency: %.1f MHz\n", freq_in_mhz);
    printf("Precision: %.3f ns\n", precision_ns);

    return 0;
}
