// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * GXMicro Display Controller Pci Driver
 *
 * Copyright (C) 2022 GXMicro (ShangHai) Corp.
 *
 * Author:
 * 	Zheng DongXiong <zhengdongxiong@gxmicro.cn>
 */
#include "gxmicro_dc.h"

#define DRVNAME			"gxmicro-dc"
#define GXMICRO_VENDOR_ID    	0x10ee
#define GXMICRO_DEVICE_ID    	0x9022

#define GXMICRO_CTRL_BAR	1

#define cpu_reset(gdev) ({ \
	gxmicro_write(gdev, PMU_RCU_CPU_RSTR, gxmicro_read(gdev, PMU_RCU_AHB_RSTR) & ~CPU0_SW);	\
	gxmicro_write(gdev, PMU_RCU_CPU_RSTR, gxmicro_read(gdev, PMU_RCU_AHB_ENR) | CPU0_SW);	\
})

#define ddr_reset(gdev) ({ \
	gxmicro_write(gdev, PMU_RCU_AHB_RSTR, gxmicro_read(gdev, PMU_RCU_AHB_RSTR) & ~RCU_DDR);	\
	gxmicro_write(gdev, PMU_RCU_AHB_ENR, gxmicro_read(gdev, PMU_RCU_AHB_ENR) & ~RCU_DDR);	\
	gxmicro_write(gdev, PMU_RCU_AHB_RSTR, gxmicro_read(gdev, PMU_RCU_AHB_RSTR) | RCU_DDR);	\
	gxmicro_write(gdev, PMU_RCU_AHB_ENR, gxmicro_read(gdev, PMU_RCU_AHB_ENR) | RCU_DDR);	\
})

#define gmac_reset(gdev) ({ \
	gxmicro_write(gdev, PMU_RCU_AHB_RSTR, gxmicro_read(gdev, PMU_RCU_AHB_RSTR) & ~RCU_GMAC);\
	gxmicro_write(gdev, PMU_RCU_AHB_ENR, gxmicro_read(gdev, PMU_RCU_AHB_ENR) & ~RCU_GMAC);	\
	gxmicro_write(gdev, PMU_RCU_AHB_RSTR, gxmicro_read(gdev, PMU_RCU_AHB_RSTR) | RCU_GMAC);	\
	gxmicro_write(gdev, PMU_RCU_AHB_ENR, gxmicro_read(gdev, PMU_RCU_AHB_ENR) | RCU_GMAC);	\
})

#define gxmicro_reset(gdev) ({ cpu_reset(gdev); ddr_reset(gdev); gmac_reset(gdev); })

static int gxmicro_dc_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct gxmicro_dc_dev *gdev;
	int ret;

	gdev = devm_kzalloc(&pdev->dev, sizeof(struct gxmicro_dc_dev), GFP_KERNEL);
	if (!gdev)
		return -ENOMEM;

	gdev->pdev = pdev;
	pci_set_drvdata(pdev, gdev);

	ret = pci_enable_device(pdev);
	if (ret)
		return ret;

	ret = pci_request_regions(pdev, DRVNAME);
	if (ret)
		goto err_request_regions;

	gdev->mmio = pci_iomap(pdev, GXMICRO_CTRL_BAR, 0);
	if (!gdev->mmio) {
		pci_err(pdev, "Failed to map Qogir Bar\n");
		ret = -ENOMEM;
		goto err_pci_iomap;
	}

	gxmicro_reset(gdev);

	ret = gxmicro_i2c_init(gdev);
	if (ret)
		goto err_i2c_init;

	ret = gxmicro_drm_init(gdev);
	if (ret)
		goto err_drm_init;

	return 0;

err_drm_init:
	gxmicro_i2c_fini(gdev);
err_i2c_init:
	pci_iounmap(pdev, gdev->mmio);
err_pci_iomap:
	pci_release_regions(pdev);
err_request_regions:
	pci_disable_device(pdev);
	return ret;
}

static void gxmicro_dc_remove(struct pci_dev *pdev)
{
	struct gxmicro_dc_dev *gdev = pci_get_drvdata(pdev);

	gxmicro_drm_fini(gdev);

	gxmicro_i2c_fini(gdev);

	pci_iounmap(pdev, gdev->mmio);

	pci_release_regions(pdev);

	pci_disable_device(pdev);
}

static const struct pci_device_id gxmicro_dc_table[] = {
	{ PCI_DEVICE(GXMICRO_VENDOR_ID, GXMICRO_DEVICE_ID) },
	{ /* END OF LIST */ }
};
MODULE_DEVICE_TABLE(pci, gxmicro_dc_table);

static struct pci_driver gxmicro_dc_drv = {
	.name = DRVNAME,
	.id_table = gxmicro_dc_table,
	.probe = gxmicro_dc_probe,
	.remove = gxmicro_dc_remove,
};
module_pci_driver(gxmicro_dc_drv);

MODULE_DESCRIPTION("GXMicro DC Driver");
MODULE_AUTHOR("Zheng DongXiong <zhengdongxiong@gxmicro.cn>");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL v2");
