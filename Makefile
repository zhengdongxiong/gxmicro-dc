
DEBUG = y
DRVNAME = gxmicro_dc

ifeq ($(DEBUG),y)
ccflags-y += -DDEBUG
endif

# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)

$(DRVNAME)-objs += gxmicro_dc_drv.o gxmicro_i2c.o gxmicro_drm.o gxmicro_ttm.o gxmicro_kms.o
obj-m += $(DRVNAME).o

# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
#KERNELDIR ?= /home/gx10014/hub/linux/
PWD := $(shell pwd)

default :
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean :
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

install :
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

endif
