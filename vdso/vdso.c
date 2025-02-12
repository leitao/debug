#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/auxv.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>


/* From linux/time.h */
#define CLOCK_REALTIME                  0
#define CLOCK_MONOTONIC                 1
#define CLOCK_PROCESS_CPUTIME_ID        2
#define CLOCK_THREAD_CPUTIME_ID         3
#define CLOCK_MONOTONIC_RAW             4
#define CLOCK_REALTIME_COARSE           5
#define CLOCK_MONOTONIC_COARSE          6
#define CLOCK_BOOTTIME                  7
#define CLOCK_REALTIME_ALARM            8
#define CLOCK_BOOTTIME_ALARM            9

char *clock_names[] = {
	"CLOCK_REALTIME",
	"CLOCK_MONOTONIC",
	"CLOCK_PROCESS_CPUTIME_ID",
	"CLOCK_THREAD_CPUTIME_ID",
	"CLOCK_MONOTONIC_RAW",
	"CLOCK_REALTIME_COARSE",
	"CLOCK_MONOTONIC_COARSE",
	"CLOCK_BOOTTIME",
	"CLOCK_REALTIME_ALARM",
	"CLOCK_BOOTTIME_ALARM"
};


struct thread_data {
        clockid_t clockid;
};

typedef void *(*thread_func)(void *);

volatile bool stopping = false;

void *get_time(void *_td) {
	struct thread_data *td = (struct thread_data *)_td;
	clockid_t clock = td->clockid;
	unsigned long count = 0;
	unsigned long *ret;
	struct timespec ts;
	int result;

	while (!stopping) {
		result = clock_gettime(clock, &ts);
		if (result != 0) {
			printf("Error getting time through VDSO\n");
			return NULL;
		}
		count += 1;
	}

	ret = malloc(sizeof(unsigned long));;
	if (!count) {
		printf("Failed to allocate memory\n");
		return NULL;
	}
	*ret = count;
	return ret;
}


void run_for_secs(int secs, thread_func func, struct thread_data *td)
{
	unsigned long *count;
	unsigned long calls_per_sec_in_m;
        pthread_t thread;
        int ret;

        stopping = false;
        ret = pthread_create(&thread, NULL, func, td);
        if (ret) {
                fprintf(stderr, "pthread_create failed: %d\n", ret);
                exit(1);
        }
        sleep(secs);
        stopping = true;
        pthread_join(thread, (void **)&count);


	calls_per_sec_in_m = (*count / (1000 * 1000)) / secs;
	printf("Number of calls to %s : %lu M/s\n", clock_names[td->clockid], calls_per_sec_in_m);
}



int main() {
	int runtime = 2;

	struct thread_data td;

	for (int i = 0; i < 9; i++) {
		td.clockid = i;
		run_for_secs(runtime, get_time, &td);
	}

	return 0;
}
