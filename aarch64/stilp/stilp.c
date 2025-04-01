#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


typedef struct {
    int val1;
    int val2;
} vals_t;

vals_t vals;

#define __unused __attribute__((unused))

void *thread1_func(void __unused *arg) {
    asm volatile("stilp x0, x1, [%[addr]]"
                 : : [addr] "r" (&vals)
                 : "x0", "x1");
    return NULL;
}

void *thread2_func(void __unused *arg) {
    // Read both locations atomically using LDIAPP
    /* vals_t local_vals; */

    /* asm volatile("ldiapp %0, %1, [%[addr]]" */
    /*              : "=r" (local_vals.val1), "=r" (local_vals.val2) */
    /*              : [addr] "r" (&vals)); */

    /* printf("Thread 2 read: val1 = %d, val2 = %d\n", local_vals.val1, local_vals.val2); */

    return NULL;
}
int main() {
    pthread_t thread1, thread2;

    vals.val1 = 42;
    vals.val2 = 24;

    pthread_create(&thread1, NULL, thread1_func, NULL);
    pthread_create(&thread2, NULL, thread2_func, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    return 0;
}
