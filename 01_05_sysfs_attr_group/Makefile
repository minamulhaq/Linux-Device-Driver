
obj-m +=  pcd_sysfs.o

ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

pcd_sysfs-objs +=  pcd_platform_driver_dt_sysfs.o  pcd_syscalls.o

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) clean

help:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) help

dt-add:
	dtc -@ -I dts -O dtb -o dd.dtbo dd.dts
	sudo dtoverlay dd.dtbo

dt-remove:
	sudo dtoverlay -r dd

