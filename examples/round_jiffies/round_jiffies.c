// SPDX-License-Identifier: GPL-2.0
/*
 * Simulate round_jiffies_common() to show per-CPU timer offsets.
 *
 * Usage: round_jiffies <max_cpu> [skew] [HZ] [j_relative]
 *   max_cpu     : highest CPU number to simulate (0..max_cpu)
 *   skew        : jiffies per CPU offset (default: 3)
 *   HZ          : timer frequency (default: 1000)
 *   j_relative  : relative delay in jiffies, e.g. HZ+1 (default: HZ+1)
 */
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned long hz;
static unsigned long jiffies_val;

static bool time_is_after_jiffies(unsigned long j)
{
	return (long)(j - jiffies_val) > 0;
}

static unsigned long round_jiffies_common(unsigned long j, int cpu,
					  int skew, bool force_up)
{
	unsigned long original = j;
	unsigned long rem;

	j += cpu * skew;

	rem = j % hz;

	if (rem < hz / 4 && !force_up)
		j = j - rem;
	else
		j = j - rem + hz;

	j -= cpu * skew;

	return time_is_after_jiffies(j) ? j : original;
}

static unsigned long round_jiffies_relative(unsigned long j, int cpu, int skew)
{
	unsigned long j0 = jiffies_val;

	return round_jiffies_common(j + j0, cpu, skew, false) - j0;
}

int main(int argc, char *argv[])
{
	unsigned long j, result, prev_result, min_result, max_result;
	int n, cpu, skew;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <max_cpu> [skew] [HZ] [j_relative]\n",
			argv[0]);
		fprintf(stderr, "  skew       : jiffies per CPU offset (default: 3)\n");
		fprintf(stderr, "  HZ         : timer frequency (default: 1000)\n");
		fprintf(stderr, "  j_relative : relative delay in jiffies (default: 10000)\n");
		return 1;
	}

	n    = atoi(argv[1]);
	skew = argc > 2 ? atoi(argv[2]) : 3;
	hz   = argc > 3 ? atoi(argv[3]) : 1000;
	j    = argc > 4 ? (unsigned long)atoi(argv[4]) : 10000;

	/* Start mid-second to make rounding more realistic */
	jiffies_val = hz * 5 + hz / 2;

	printf("HZ=%lu  jiffies=%lu  j_relative=%lu  skew=%d\n\n",
	       hz, jiffies_val, j, skew);
	printf("%-6s  %-10s  %-12s  %s\n",
	       "CPU", "result", "ms_offset", "delta_to_prev");
	printf("%-6s  %-10s  %-12s  %s\n",
	       "---", "------", "---------", "-------------");

	prev_result = 0;
	min_result = ULONG_MAX;
	max_result = 0;
	for (cpu = 0; cpu <= n; cpu++) {
		result = round_jiffies_relative(j, cpu, skew);
		if (result < min_result)
			min_result = result;
		if (result > max_result)
			max_result = result;
		if (cpu == 0) {
			printf("%-6d  %-10lu  %-12ld  -\n",
			       cpu, result, (long)result * 1000 / (long)hz);
		} else {
			printf("%-6d  %-10lu  %-12ld  %+ldms\n",
			       cpu, result,
			       (long)result * 1000 / (long)hz,
			       ((long)result - (long)prev_result) * 1000 / (long)hz);
		}
		prev_result = result;
	}

	printf("\nTimer region: min=%lums  max=%lums  spread=%lums\n",
	       min_result * 1000 / hz,
	       max_result * 1000 / hz,
	       (max_result - min_result) * 1000 / hz);

	return 0;
}
