#include <stdio.h>

#define unlikely(x)    __builtin_expect(!!(x), 0)
#define likely(x)    __builtin_expect(!!(x), 1)
#define noinline                      __attribute__((__noinline__))

static noinline int bar(int i)
{
	if (i > 9999)
		return 1;

	if (likely(i == 10000))
		return 1;

	return 0;
}
int main()
{
	int i, j, t = 0;

	for (j = 0 ; j < 10000; j++)
		for (i=0 ; i <= 10000; i++) 
			if (unlikely(bar(i)))
				t += 1;

	printf("tot = %d\n", t);
	return 0;
}
