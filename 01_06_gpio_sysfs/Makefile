
obj-m +=  gpio-sysfs.o

# ARCH=arm
# Auto-detect architecture
ARCH := $(shell uname -m | sed 's/aarch64/arm64/;s/armv.*/arm/')
CROSS_COMPILE := $(shell if [ "$(ARCH)" = "arm64" ]; then echo "aarch64-linux-gnu-"; else echo "arm-linux-gnueabihf-"; fi)

KDIR := /lib/modules/$(shell uname -r)/build
# KDIR := /home/rp1/tmp/linux 
PWD  := $(shell pwd)

gpio-sysfs-objs +=  gpio_sysfs.o  syscalls.o

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) clean

help:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) help


dt:
	dtc -@ -I dts -O dtb -o dd.dtbo dd.dts 


dt-add: dt
	sudo dtoverlay dd.dtbo

dt-remove:
	sudo dtoverlay -r dd

