// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * GXMicro DRM
 *
 * Copyright (C) 2022 GXMicro (ShangHai) Corp.
 *
 * Author:
 * 	Zheng DongXiong <zhengdongxiong@gxmicro.cn>
 */
#include <drm/drm_drv.h>
#include <drm/drm_vram_mm_helper.h>
#include <drm/drm_fb_helper.h>

#include "gxmicro_dc.h"

#define GXMICRO_DRM_NAME	"gxmicro"
#define GXMICRO_DRM_DESC	"GXMICRO DRM"
#define GXMICRO_DRM_DATE	"20221010"
#define GXMICRO_DRM_MAJOR	1
#define GXMICRO_DRM_MINOR	0

static const struct file_operations gxmicro_drm_fops = {
	.owner = THIS_MODULE,
	DRM_VRAM_MM_FILE_OPERATIONS,
};

static struct drm_driver gxmicro_drm_driver = {
	.fops = &gxmicro_drm_fops,
	.name = GXMICRO_DRM_NAME,
	.desc = GXMICRO_DRM_DESC,
	.date = GXMICRO_DRM_DATE,
	.major = GXMICRO_DRM_MAJOR,
	.minor = GXMICRO_DRM_MINOR,
	.driver_features = DRIVER_GEM | DRIVER_MODESET,
	DRM_GEM_VRAM_DRIVER,
};

int gxmicro_drm_init(struct gxmicro_dc_dev *gdev)
{
	struct pci_dev *pdev = gdev->pdev;
	struct drm_device *dev;
	int ret;

	dev = drm_dev_alloc(&gxmicro_drm_driver, &pdev->dev);
	if (IS_ERR(dev))
		return PTR_ERR(dev);

	gdev->dev = dev;
	dev->pdev = pdev;
	dev->dev_private = gdev;

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
	drm_dev_put(dev);
	return ret;
}

void gxmicro_drm_fini(struct gxmicro_dc_dev *gdev)
{
	struct drm_device *dev = gdev->dev;

	drm_dev_unregister(dev);

	gxmicro_kms_fini(gdev);

	gxmicro_ttm_fini(gdev);

	drm_dev_put(dev);
}
