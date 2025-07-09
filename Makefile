# SPDX-License-Identifier: GPL-2.0

gxmicro_dc-y := gxmicro_drv.o gxmicro_i2c.o gxmicro_kms.o gxmicro_ttm.o
obj-$(CONFIG_DRM_GXMICRO) += gxmicro_dc.o

ccflags-y += -Werror
