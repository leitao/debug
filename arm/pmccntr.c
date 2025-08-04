#include <stdio.h>
#include <stdint.h>

int main() {
    uint64_t val;
    asm volatile("mrs %0, pmccntr_el0" : "=r"(val));
    printf("PMCCNTR_EL0: %lu\n", val);
    return 0;
}
