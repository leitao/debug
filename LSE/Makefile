CC = clang
CFLAGS = -O2 -Wall -march=armv8-a+lse

percpu_bench: percpu_bench.c
	$(CC) $(CFLAGS) percpu_bench.c -o percpu_bench

percpu_bench_debug: percpu_bench.c
	$(CC) -g -march=armv8-a+lse percpu_bench.c -o percpu_bench_debug

parallel_atomic_bench: parallel_atomic_bench.c
	$(CC) $(CFLAGS) -pthread parallel_atomic_bench.c -o parallel_atomic_bench

parallel_atomic_bench_debug: parallel_atomic_bench.c
	$(CC) -g -Wall -march=armv8-a+lse -pthread parallel_atomic_bench.c -o parallel_atomic_bench_debug

asm: percpu_bench.c
	$(CC) $(CFLAGS) -S percpu_bench.c -o percpu_bench.s

parallel_asm: parallel_atomic_bench.c
	$(CC) $(CFLAGS) -S parallel_atomic_bench.c -o parallel_atomic_bench.s

clean:
	rm -f percpu_bench percpu_bench.s parallel_atomic_bench parallel_atomic_bench.s parallel_atomic_bench_debug

all: percpu_bench percpu_bench_debug parallel_atomic_bench parallel_atomic_bench_debug asm parallel_asm
