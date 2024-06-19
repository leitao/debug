#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


#define BUFSIZE 1024
#define THREADS 10

char buffer[BUFSIZE][THREADS];

void *read_file(void *thread) {
	int fd = open("/dev/hwrng", O_RDONLY);
	int t = *(int *)thread;

	if (fd == -1) {
		perror("Failed to open /dev/hwrng\n");
		exit(-1);
	}

	lseek(fd, 0, SEEK_SET);
	int ret = read(fd, &buffer[t], BUFSIZE);
	if (ret == BUFSIZE) {
		printf(".");
	}
}

int main() {
	pthread_t thread[THREADS];

	for (int i =0 ; i < THREADS; i++) {
		pthread_create(&thread[i], NULL, read_file, &i);
	}

	for (int i =0 ; i < THREADS; i++) {
		pthread_join(thread[i], NULL);
	}
}
