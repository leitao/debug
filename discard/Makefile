CC=gcc
CFLAGS=-D_GNU_SOURCE -std=c99

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

discard: discard.o
	$(CC) -o discard discard.o

