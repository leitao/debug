CC = clang
CFLAGS = -O2 -march=armv8-a+lse

percpu_bench: percpu_bench.c
	$(CC) $(CFLAGS) percpu_bench.c -o percpu_bench

percpu_bench_debug: percpu_bench.c
	$(CC) -g -march=armv8-a+lse percpu_bench.c -o percpu_bench_debug

asm: percpu_bench.c
	$(CC) $(CFLAGS) -S percpu_bench.c -o percpu_bench.s

clean:
	rm -f percpu_bench percpu_bench.s

all: percpu_bench percpu_bench_debug asm
