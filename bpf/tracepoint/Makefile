CLANG ?= clang
CFLAGS := -O2 -g # -Wall -Werror

all: tcp_sendmsg_locked_loader

tcp_sendmsg_locked_bpf.o: tcp_sendmsg_locked_bpf.c
	$(CLANG) -target bpf -D__TARGET_ARCH_x86 \
		-I/usr/include/$(shell uname -m)-linux-gnu \
		$(CFLAGS) -c $< -o $@

tcp_sendmsg_locked_loader: tcp_sendmsg_locked_loader.c tcp_sendmsg_locked_bpf.o
	$(CC) $(CFLAGS) -o $@ $< -lbpf

clean:
	rm -f tcp_sendmsg_locked_loader *.o

.PHONY: all clean
