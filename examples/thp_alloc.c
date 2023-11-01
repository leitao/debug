#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdbool.h>

#define MAX 100

static void write_to_mem(char *mem, size_t size)
{
	for (int i = 0; i < size; i++) {
		mem[i] = '.';
	}
}

int main() {
    size_t size = 20 * 1024 * 1024;  // 20 MB
    char *memory[MAX];


   for (int i = 0 ; i < MAX; i++) { 
	    memory[i] = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	    if (memory == MAP_FAILED) {
		perror("mmap");
		return 1;
	    }
   }

   while (true) {
	for (int i = 0 ; i < MAX; i++) { 
		if (madvise(memory[i], size, MADV_HUGEPAGE) != 0) {
			perror("madvise");
			return 1;
		}

		write_to_mem(memory[i], size);
	}
   }

   for (int i = 0 ; i < MAX; i++) { 
	    /* printf("Allocated memory with MADV_HUGEPAGE hint\n"); */
	    int ret = munmap(memory[i],  size);
	    if (ret) {
		perror("munmap");
		return 1;
	    }
    }


    return 0;
}
