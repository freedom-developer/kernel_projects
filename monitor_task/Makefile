PWD := $(shell pwd)
KVER := $(shell uname -r)
KDIR := /lib/modules/$(KVER)/build

obj-m := monitor.o
monitor-objs := main.o

ccflags-y  := -I$(KDIR)/arch/x86/include

all:
	$(MAKE) -C $(KDIR) M=$(PWD)

clean:
	$(RM) -rf .tmp_versions .*.mk .*.cmd *.ko *.mod.c *.o *.symvers *.order
