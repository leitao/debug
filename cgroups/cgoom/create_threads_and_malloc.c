#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_CHILDREN 1000
#define ALLOC_SIZE 1024 * 1024 // 1MB
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
	sleep(10);
    }
}
int main() {
    pid_t pid;
    int i;
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
    // Parent process: wait for all children to finish
    while (wait(NULL) > 0);
    return EXIT_SUCCESS;
}
