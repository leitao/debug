#include <stdio.h>
#include <time.h>
int main() {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        return 1;
    }
    printf("Monotonic time: %lld seconds, %lld nanoseconds\n",
           (long long)ts.tv_sec, (long long)ts.tv_nsec);
    return 0;
}
