obj-m	+= lkm_sample.o
PWD		:= $(shell pwd)
all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(PWD) modules
clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(PWD) clean
