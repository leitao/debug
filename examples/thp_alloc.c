#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <string.h>

#define MAX 100

static void write_to_mem(char *mem, size_t size)
{
	for (int i = 0; i < size; i++) {
		mem[i] = '.';
	}
}

void print_rss() {
    FILE *statusFile = fopen("/proc/self/status", "r");

    if (statusFile == NULL) {
        perror("Failed to open /proc/self/status");
        return;
    }

    char line[256];  // Buffer to read lines from the file

    // Read the file line by line
    while (fgets(line, sizeof(line), statusFile)) {
        // Check if the line contains "RSS"
        if (strstr(line, "RSS")) {
            // Print the line that contains "RSS"
            printf("%s", line);
        }
    }

    fclose(statusFile);
}

int main(int argc, char** argv) {
	char *memory[MAX];
	size_t size;

	if (argc != 2) {
		size = 20;
	} else {
		size = atoi(argv[1]);
	}
	printf("Using a total of %lu MB\n", size);
	fflush(stdout); // Flush the stdout buffer


	size *= 1024 * 1024; 

	while (true) {
		for (int i = 0 ; i < MAX; i++) { 
			memory[i] = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

			if (memory == MAP_FAILED) {
				perror("mmap");
				return 1;
			}
		}

		for (int i = 0 ; i < MAX; i++) { 
			if (madvise(memory[i], size, MADV_HUGEPAGE) != 0) {
				perror("madvise");
				return 1;
			}

			write_to_mem(memory[i], size);
		}

		for (int i = 0 ; i < MAX; i++) { 
			int ret = munmap(memory[i],  size);
			if (ret) {
				perror("munmap");
				return 1;
			}
		}
		print_rss();
	}

	return 0;
}
