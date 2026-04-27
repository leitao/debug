obj-m += wq_va_test.o

KDIR ?= /home/leit/Devel/upstream

all:
	$(MAKE) LLVM=1 -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
