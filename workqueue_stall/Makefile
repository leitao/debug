ifeq ($(KERNELRELEASE),)
    # Use the current kernel build directory
    KERNELDIR ?= ${HOME}/Devel/upstream
    # Current directory
    PWD := $(shell pwd)

default:
	$(MAKE) LLVM=1 -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

else
    # Kernel build system portion
    obj-m := workqueue_stall.o
endif

