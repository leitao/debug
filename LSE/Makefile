CC = gcc
CFLAGS = -O2 -march=armv8-a+lse

percpu_bench: percpu_bench.c
	$(CC) $(CFLAGS) percpu_bench.c -o percpu_bench

clean:
	rm -f percpu_bench

