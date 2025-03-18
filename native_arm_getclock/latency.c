#define _POSIX_C_SOURCE 199309L
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h> // Ensure this is included

static inline uint64_t get_cntvct() {
  uint64_t val;
  asm volatile("mrs %0, cntvct_el0" : "=r"(val));
  return val;
}

uint64_t get_clock_gettime(clockid_t clockid) {
  struct timespec ts;
  clock_gettime(clockid, &ts);
  return ts.tv_sec * 1e9 + ts.tv_nsec;
}

int main() {
  uint64_t start, end, cycles;
  /* used to avoid compiler optiomization */
  uint64_t tmp = 0;

  // Benchmark CNTVCT_EL0
  start = get_cntvct();
  for (int i = 0; i < 1000000; i++) {
    tmp += get_cntvct();
  }
  end = get_cntvct();

  cycles = end - start;
  printf("CNTVCT_EL0 Average Cycles: %" PRIu64 "\n", cycles / 1000000);

  // Benchmark clock_gettime()
  start = get_cntvct();
  for (int i = 0; i < 1000000; i++) {
    tmp += get_clock_gettime(CLOCK_MONOTONIC_COARSE);
  }
  end = get_cntvct();
  cycles = end - start;
  printf("clock_gettime(CLOCK_MONOTONIC_COARSE) Average Cycles: %" PRIu64 "\n",
         cycles / 1000000);

  // Benchmark clock_gettime(REALTIME)
  start = get_cntvct();
  for (int i = 0; i < 1000000; i++) {
    tmp += get_clock_gettime(CLOCK_REALTIME);
  }
  end = get_cntvct();
  cycles = end - start;

  printf("clock_gettime(CLOCK_REALTIME) Average Cycles: %" PRIu64 "\n",
         cycles / 1000000);

  if (tmp < 1)
    printf("%" PRIu64 "\n", tmp);

  return 0;
}
