CC      = clang
CFLAGS  = -g -Wall -O2 -W -march=armv8.9-a -mcpu=neoverse-n1

ALL_CFLAGS = $(CFLAGS) -D_GNU_SOURCE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

# Compilation targets
PROGS = likely
ALL = $(PROGS)

all: $(ALL)

%.o: %.c
	$(CC) -o $*.o -c $(ALL_CFLAGS) $<

clean:
	-rm -f *.o $(PROGS) .depend 

ifneq ($(wildcard .depend),)
include .depend
endif
