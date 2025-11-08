CC = clang

# Architecture-specific flags
ARCH ?=
ARM64_CFLAGS = -O2 -Wall -march=armv8-a+lse
X86_CFLAGS = -O2 -Wall -march=x86-64

# Select architecture flags based on ARCH variable
ifeq ($(ARCH),arm64)
    CFLAGS = $(ARM64_CFLAGS)
else ifeq ($(ARCH),x86)
    CFLAGS = $(X86_CFLAGS)
else
    CFLAGS =
endif

# Default target - require architecture selection
default:
	@echo "Please specify architecture: make ARCH=x86 or make ARCH=arm64"
	@echo "Or use convenience targets: make x86 or make arm64"
	@exit 1

# Convenience targets
x86:
	$(MAKE) ARCH=x86 all

arm64:
	$(MAKE) ARCH=arm64 all

percpu_bench: percpu_bench.c percpu_bench_lib.c percpu_bench_lib.h
	@if [ -z "$(ARCH)" ]; then echo "Error: ARCH not set. Use make ARCH=x86 or make ARCH=arm64"; exit 1; fi
	$(CC) $(CFLAGS) -pthread percpu_bench.c percpu_bench_lib.c -o percpu_bench

percpu_bench_debug: percpu_bench.c percpu_bench_lib.c percpu_bench_lib.h
	@if [ -z "$(ARCH)" ]; then echo "Error: ARCH not set. Use make ARCH=x86 or make ARCH=arm64"; exit 1; fi
	$(CC) -pthread -g -Wall $(CFLAGS) percpu_bench.c percpu_bench_lib.c -o percpu_bench_debug

parallel_atomic_bench: parallel_atomic_bench.c
	@if [ -z "$(ARCH)" ]; then echo "Error: ARCH not set. Use make ARCH=x86 or make ARCH=arm64"; exit 1; fi
	$(CC) $(CFLAGS) -pthread parallel_atomic_bench.c -o parallel_atomic_bench

parallel_atomic_bench_debug: parallel_atomic_bench.c
	@if [ -z "$(ARCH)" ]; then echo "Error: ARCH not set. Use make ARCH=x86 or make ARCH=arm64"; exit 1; fi
	$(CC) -g -Wall $(CFLAGS) -pthread parallel_atomic_bench.c -o parallel_atomic_bench_debug

asm: percpu_bench.c percpu_bench_lib.c
	@if [ -z "$(ARCH)" ]; then echo "Error: ARCH not set. Use make ARCH=x86 or make ARCH=arm64"; exit 1; fi
	$(CC) $(CFLAGS) -S percpu_bench.c -o percpu_bench.s
	$(CC) $(CFLAGS) -S percpu_bench_lib.c -o percpu_bench_lib.s

parallel_asm: parallel_atomic_bench.c
	@if [ -z "$(ARCH)" ]; then echo "Error: ARCH not set. Use make ARCH=x86 or make ARCH=arm64"; exit 1; fi
	$(CC) $(CFLAGS) -S parallel_atomic_bench.c -o parallel_atomic_bench.s

clean:
	rm -f percpu_bench percpu_bench.s percpu_bench_lib.s percpu_bench_debug parallel_atomic_bench parallel_atomic_bench.s parallel_atomic_bench_debug

all: percpu_bench percpu_bench_debug parallel_atomic_bench parallel_atomic_bench_debug asm parallel_asm

.PHONY: default x86 arm64 clean all
