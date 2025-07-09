// SPDX-License-Identifier: GPL-2.0
/*
 * GXMicro DRM TTM
 *
 * Copyright (C) 2023 GXMicro (ShangHai) Corp.
 *
 * Author:
 * 	Zheng DongXiong <zhengdongxiong@gxmicro.cn>
 */
#include <drm/drm_vram_mm_helper.h>

#include "gxmicro_dc.h"

#define GXMICRO_FB_BAR		0
#define GXMICRO_FB_SIZE		SZ_8M

int gxmicro_ttm_init(struct gxmicro_dc_dev *gdev)
{
	struct drm_device *dev = gdev->dev;
	struct drm_vram_mm *vmm;

	vmm = drm_vram_helper_alloc_mm(dev, pci_resource_start(dev->pdev, GXMICRO_FB_BAR),
				GXMICRO_FB_SIZE, &drm_gem_vram_mm_funcs);
	if (IS_ERR(vmm)) {
		pci_err(dev->pdev, "Failed to init VRAM MM\n");
		return PTR_ERR(vmm);
	}

	return 0;
}

void gxmicro_ttm_fini(struct gxmicro_dc_dev *gdev)
{
	struct drm_device *dev = gdev->dev;

	drm_vram_helper_release_mm(dev);
}
