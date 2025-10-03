CC      = clang
CFLAGS  = -Wall -O2 -g -W
ALL_CFLAGS = $(CFLAGS) -D_GNU_SOURCE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

# Compilation targets
PROGS = cpuid fpcr afp pmccntr cpu_clock_frequency
ALL = $(PROGS)

all: $(ALL)

%.o: %.c
	$(CC) -o $*.o -c $(ALL_CFLAGS) $<

cpuid: cpuid.o
	$(CC) $(ALL_CFLAGS) -o $@ $(filter %.o,$^) -lpthread

clean:
	-rm -f *.o $(PROGS) .depend

ifneq ($(wildcard .depend),)
include .depend
endif
