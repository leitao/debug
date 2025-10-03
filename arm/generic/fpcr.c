#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>

// Inline assembly helpers for reading/writing FPCR
static inline uint64_t read_fpcr(void) {
	uint64_t val;
	asm volatile("mrs %0, fpcr" : "=r"(val));

	return val;
}

static inline void write_fpcr(uint64_t val) {
	asm volatile("msr fpcr, %0" :: "r"(val));
}

int main(void) {
	uint64_t fpcr;

	// Read current FPCR
	fpcr = read_fpcr();
	printf("Initial FPCR: 0x%016" PRIx64 "\n", fpcr);

	for (int i = 1; i < 31; i++ ) {
		// Set AH bit (bit 1)
		fpcr |= (1ULL << 1);
		write_fpcr(fpcr);
		uint64_t new_fpcr = read_fpcr();
		printf("FPCR after setting AH[1]: 0x%016" PRIx64 "\n", new_fpcr);
	}

	return 0;
}

