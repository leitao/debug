CC = clang
CFLAGS = -O2 -march=armv8-a+lse

percpu_bench: percpu_bench.c
	$(CC) $(CFLAGS) percpu_bench.c -o percpu_bench

asm: percpu_bench.c
	$(CC) $(CFLAGS) -S percpu_bench.c -o percpu_bench.s

clean:
	rm -f percpu_bench percpu_bench.s

