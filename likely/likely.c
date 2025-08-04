#include <stdio.h>
#include <unistd.h> // for getopt
#include <stdbool.h>
#include <stdlib.h>

#define unlikely(x)    __builtin_expect(!!(x), 0)
#define likely(x)      __builtin_expect(!!(x), 1)
#define noinline       __attribute__((__noinline__))

static noinline int bar(int i)
{
	if (unlikely(i > 9999))
		return 1;

	return 0;
}

#define MAX 100000

int main(int argc, char **argv)
{
	int i, j = 0;
	float tot = 0.0f;
	bool use_likely = true;
	int opt;
	int max = MAX;

	// Parse command line options
	while ((opt = getopt(argc, argv, "rm:")) != -1) {
		switch (opt) {
		case 'r':
			printf("Using likely instead of unlikely\n");
			use_likely = true;
			break;
		case 'm':
			max = atoi(optarg);
			break;
		}
	}

	for (j = 0 ; j < max; j++)
		for (i=0 ; i <= max; i++) 
			if (use_likely) {
				if (likely(bar(i)))
					tot += 0.1f;
			} else {
				if (unlikely(bar(i)))
					tot += 0.1f;
			}

	printf("tot = %f\n", tot);
	return 0;
}
