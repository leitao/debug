#include <stdio.h>
#include <stdint.h>

int main() {
    uint64_t mmfr1;
    asm volatile("mrs %0, ID_AA64MMFR1_EL1" : "=r"(mmfr1));
    printf("ID_AA64MMFR1_EL1: 0x%016lx\n", mmfr1);
    int afp = (mmfr1 >> 44) & 0xf;
    if (afp == 1)
        printf("FEAT_AFP is implemented.\n");
    else
        printf("FEAT_AFP is not implemented.\n");
    return 0;
}

