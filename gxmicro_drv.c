// SPDX-License-Identifier: GPL-2.0
/*
 * GXMicro Display Controller driver
 *
 * Copyright (C) 2023 GXMicro (ShangHai) Corp.
 *
 * Author:
 * 	Zheng DongXiong <zhengdongxiong@gxmicro.cn>
 */
#include <drm/drm_drv.h>
#include <drm/drm_vram_mm_helper.h>
#include <drm/drm_fb_helper.h>

#include "gxmicro_dc.h"

/* PCI */
#define GXMICRO_VENDOR_ID    	0x10ee
#define GXMICRO_DEVICE_ID    	0x9022

#define GXMICRO_CTRL_BAR	1

/* DRM */
#define GXMICRO_DRM_DESC	"gxmicro drm"
#define GXMICRO_DRM_DATE	"20221010"
#define GXMICRO_DRM_MAJOR	1
#define GXMICRO_DRM_MINOR	0

/* ****************************** PCIe ****************************** */

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
#define dc_reset(gdev) ({ \
	gxmicro_write(gdev, PMU_RCU_AHB_RSTR, gxmicro_read(gdev, PMU_RCU_AHB_RSTR) & ~RCU_DC);\
	gxmicro_write(gdev, PMU_RCU_AHB_RSTR, gxmicro_read(gdev, PMU_RCU_AHB_RSTR) | RCU_DC);	\
})
#if 0
#define dc_reset(gdev) ({ \
	gxmicro_write(gdev, PMU_RCU_AHB_RSTR, gxmicro_read(gdev, PMU_RCU_AHB_RSTR) & ~RCU_DC);\
	gxmicro_write(gdev, PMU_RCU_AHB_ENR, gxmicro_read(gdev, PMU_RCU_AHB_ENR) & ~RCU_DC);	\
	gxmicro_write(gdev, PMU_RCU_AHB_RSTR, gxmicro_read(gdev, PMU_RCU_AHB_RSTR) | RCU_DC);	\
	gxmicro_write(gdev, PMU_RCU_AHB_ENR, gxmicro_read(gdev, PMU_RCU_AHB_ENR) | RCU_DC);	\
})
#endif

static int gxmicro_pcie_init(struct pci_dev *pdev)
{
	struct gxmicro_dc_dev *gdev = pci_get_drvdata(pdev);
	int ret;

	ret = pci_enable_device(pdev);
	if (ret)
		return ret;

	ret = pci_request_regions(pdev, KBUILD_MODNAME);
	if (ret)
		goto err_request_regions;

	gdev->mmio = pci_iomap(pdev, GXMICRO_CTRL_BAR, 0);
	if (!gdev->mmio) {
		pci_err(pdev, "Failed to map Qogir Bar\n");
		ret = -ENOMEM;
		goto err_pci_iomap;
	}

	//cpu_reset(gdev);
	ddr_reset(gdev);
	dc_reset(gdev);

	return 0;

err_pci_iomap:
	pci_release_regions(pdev);
err_request_regions:
	pci_disable_device(pdev);
	return ret;
}

static void gxmicro_pcie_fini(struct pci_dev *pdev)
{
	struct gxmicro_dc_dev *gdev = pci_get_drvdata(pdev);

	pci_iounmap(pdev, gdev->mmio);

	pci_release_regions(pdev);

	pci_disable_device(pdev);
}

/* ****************************** DRM ****************************** */

static const struct file_operations gxmicro_drm_fops = {
	.owner = THIS_MODULE,
	DRM_VRAM_MM_FILE_OPERATIONS,
};

static struct drm_driver gxmicro_drm_drv = {
	.fops = &gxmicro_drm_fops,
	.name = KBUILD_MODNAME,
	.desc = GXMICRO_DRM_DESC,
	.date = GXMICRO_DRM_DATE,
	.major = GXMICRO_DRM_MAJOR,
	.minor = GXMICRO_DRM_MINOR,
	.driver_features = DRIVER_GEM | DRIVER_MODESET,
	DRM_GEM_VRAM_DRIVER,
};

static int gxmicro_drm_init(struct pci_dev *pdev)
{
	struct gxmicro_dc_dev *gdev = pci_get_drvdata(pdev);
	struct drm_device *dev;
	int ret;

	dev = drm_dev_alloc(&gxmicro_drm_drv, &pdev->dev);
	if (IS_ERR(dev)) {
		pci_err(pdev, "Failed to alloc drm device\n");
		return PTR_ERR(dev);
	}

	gdev->dev = dev;
	dev->pdev = pdev;
	dev->dev_private = gdev;

	ret = gxmicro_i2c_init(gdev);
	if (ret)
		goto err_i2c_init;

	ret = gxmicro_ttm_init(gdev);
	if (ret)
		goto err_ttm_init;

	ret = gxmicro_kms_init(gdev);
	if (ret)
		goto err_kms_init;

	ret = drm_dev_register(dev, 0);
	if (ret)
		goto err_drm_register;

	ret = drm_fbdev_generic_setup(dev, 32);
	if (ret)
		goto err_fbdev_setup;

	return 0;

err_fbdev_setup:
	drm_dev_unregister(dev);
err_drm_register:
	gxmicro_kms_fini(gdev);
err_kms_init:
	gxmicro_ttm_fini(gdev);
err_ttm_init:
	gxmicro_i2c_fini(gdev);
err_i2c_init:
	drm_dev_put(dev);
	return ret;
}

static void gxmicro_drm_fini(struct pci_dev *pdev)
{
	struct gxmicro_dc_dev *gdev = pci_get_drvdata(pdev);
	struct drm_device *dev = gdev->dev;

	drm_dev_unregister(dev);

	gxmicro_kms_fini(gdev);

	gxmicro_ttm_fini(gdev);

	gxmicro_i2c_fini(gdev);

	drm_dev_put(dev);
}

/* ****************************** PCI Probe & Remove ****************************** */

static int gxmicro_dc_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct gxmicro_dc_dev *gdev;
	int ret;

	gdev = devm_kzalloc(&pdev->dev, sizeof(struct gxmicro_dc_dev), GFP_KERNEL);
	if (!gdev)
		return -ENOMEM;

	pci_set_drvdata(pdev, gdev);

	ret = gxmicro_pcie_init(pdev);
	if (ret)
		return ret;

	ret = gxmicro_drm_init(pdev);
	if (ret)
		goto err_drm_init;

	return 0;

err_drm_init:
	gxmicro_pcie_fini(pdev);
	return ret;
}

static void gxmicro_dc_remove(struct pci_dev *pdev)
{
	gxmicro_drm_fini(pdev);

	gxmicro_pcie_fini(pdev);
}

static const struct pci_device_id gxmicro_dc_ids[] = {
	{ PCI_DEVICE(GXMICRO_VENDOR_ID, GXMICRO_DEVICE_ID) },
	{ /* END OF LIST */ }
};
MODULE_DEVICE_TABLE(pci, gxmicro_dc_ids);

static struct pci_driver gxmicro_dc_drv = {
	.name = KBUILD_MODNAME,
	.id_table = gxmicro_dc_ids,
	.probe = gxmicro_dc_probe,
	.remove = gxmicro_dc_remove,
};
module_pci_driver(gxmicro_dc_drv);

MODULE_DESCRIPTION("GXMicro Display Controller driver");
MODULE_AUTHOR("Zheng DongXiong <zhengdongxiong@gxmicro.cn>");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL v2");
