#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ALLOC_SIZE 1024*1024*10 // 10 MB

#define SLEEP_TIME 15

int main() {
	char *ptr;

	// Sleep to wait for the other processes to be created
	sleep(SLEEP_TIME);

	ptr = malloc(ALLOC_SIZE);
	for (int i = 0; i < ALLOC_SIZE; i++)
		ptr[i] = 1;

	sleep(1);


	// Just it case the above didn't hit the CGMEM limits
	//  Don't worry about the leaked memory
	while (1) {
		ptr = malloc(ALLOC_SIZE);
		sleep(1);
	};

	return EXIT_SUCCESS;
}
