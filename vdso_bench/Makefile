CC      = clang
CFLAGS  = -Wall -O2 -g -W
ALL_CFLAGS = $(CFLAGS) -D_GNU_SOURCE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

ifeq ($(ARCH), arm64)
    ARCH_CFLAGS = -DCONFIG_ARM -march=armv9-a
    PACA_FLAGS =-mbranch-protection=pac-ret+bti  -march=armv8.7-a
endif

# Compilation targets
PROGS = vdso_bench vdso_bench_paca
ALL = $(PROGS)

all: $(ALL)

%.o: %.c
	$(CC) -o $*.o -c $(ALL_CFLAGS) $<

vdso_bench: vdso_bench.o
	$(CC) $(ALL_CFLAGS) -o $@ $(filter %.o,$^) -lpthread

vdso_bench_paca: vdso_bench.c
	$(CC) $(PACA_FLAGS) $(ALL_CFLAGS) -o $@ $(filter %.c,$^) -lpthread

clean:
	-rm -f *.o $(PROGS) .depend 

ifneq ($(wildcard .depend),)
include .depend
endif
