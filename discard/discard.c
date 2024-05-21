#include <stdio.h>
#include <pthread.h>
#include <stdlib.h> //exit
#include <fcntl.h> // open
#include <unistd.h> // write
#include <stdbool.h> // bool
#include <string.h> // strdup


#define THREADS 100
#define MAXPATH 1024

char *dstdir = "/tmp";

void *do_it(void *ptr)
{
	char filename[MAXPATH];
	const int buffer[] = {1,2,3,4,5,6,7,8,9};
	int fd;

	snprintf(filename, MAXPATH, "%s/tmp_XXXXXX", dstdir);

	fd = mkstemp(filename);

	if (fd < 0){
		perror("Failed to open tmp file");
		exit(1);
	}

	/* printf("filename=%s sizeof=%d\n", filename, sizeof(buffer)); */
	write(fd, &buffer, sizeof(buffer));
	close(fd);
	unlink(filename);

	return NULL;
}


int main (int argc, char **argv)
{
	pthread_t threads[THREADS];
	int ret[THREADS];
	bool loop;
	int i, c;

	while ((c = getopt(argc, argv, "l-d:")) != -1) {
		switch (c) {
			case 'l':
				printf("Running in infinite loop\n");
				loop = true;
				break;
			case 'd':
				dstdir = strdup(optarg);
		}

	}

	do {
		for (i = 0 ; i < THREADS; i++) {
			ret[i] = pthread_create(&threads[i], NULL, do_it, NULL);
		}

		for (i = 0 ; i < THREADS; i++) {
			pthread_join(threads[i], NULL);
		}
	} while (loop);


	return 0;
}
