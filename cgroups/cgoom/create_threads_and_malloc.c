#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ALLOC_SIZE 1024*100
#define SLEEP_TIME 15

int main() {
	char *ptr;

	// Sleep to wait for the other processes to start
	sleep(SLEEP_TIME);

	ptr = malloc(ALLOC_SIZE);
	*(char *)ptr = 'x';

	return EXIT_SUCCESS;
}
