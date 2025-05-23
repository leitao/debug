#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_CHILDREN 2
#define ALLOC_SIZE 51 * 1024 // 500K
			       //
void allocate_memory_until_oom() {
    void* ptr = NULL;
    size_t total_allocated = 0;
    while (1) {
        ptr = malloc(ALLOC_SIZE);
        if (ptr == NULL) {
            printf("Out of memory! Total allocated: %zu MB\n", total_allocated / (1024 * 1024));
            break;
        }

        // "Use" the allocated memory to prevent optimization
        *(char*)ptr = 'x';
        total_allocated += ALLOC_SIZE;
	sleep(1);
    }
}

int main() {
    pid_t pid;
    int i;
    // Sleep to wait for the other processes to start
    sleep(5);
    for (i = 0; i < NUM_CHILDREN; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            allocate_memory_until_oom();
            exit(EXIT_SUCCESS);
        }
    }
   
    while (wait(NULL) > 0);
    return EXIT_SUCCESS;
}
