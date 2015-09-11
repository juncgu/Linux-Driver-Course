obj-m := deadlock_2.o
KERNELDIR ?= /usr/src/linux-headers-$(shell uname -r)
modules:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
.PHONY:modules modules_install clean
