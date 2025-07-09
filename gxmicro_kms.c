// SPDX-License-Identifier: GPL-2.0
/*
 * GXMicro DRM KMS
 *
 * Copyright (C) 2023 GXMicro (ShangHai) Corp.
 *
 * Author:
 * 	Zheng DongXiong <zhengdongxiong@gxmicro.cn>
 */
#include <drm/drm_vram_mm_helper.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_mode.h>
#include <drm/drm_modeset_helper_vtables.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_edid.h>

#include "gxmicro_dc.h"

/* ****************************** DRM Priv ****************************** */

static inline void *drm_get_priv(struct drm_device *dev)
{
	return dev->dev_private;
}

/* ****************************** DRM Mode Config ****************************** */

static const struct drm_mode_config_funcs gxmicro_mode_congfig_funcs = {
	.fb_create = drm_gem_fb_create,
};

static inline void gxmicro_setup_mode_config(struct gxmicro_dc_dev *gdev)
{
	struct drm_device *dev = gdev->dev;

	drm_mode_config_init(dev);

	dev->mode_config.min_width = CURSOR_WIDTH;
	dev->mode_config.min_height = CURSOR_HEIGHT;
	dev->mode_config.max_width = DISPLAY_WIDTH;
	dev->mode_config.max_height = DISPLAY_HEIGHT;
	dev->mode_config.cursor_width = CURSOR_WIDTH;
	dev->mode_config.cursor_height = CURSOR_HEIGHT;
	dev->mode_config.funcs = &gxmicro_mode_congfig_funcs;
}

/* ****************************** Primary Plane ****************************** */

static const uint32_t gxmicro_primary_plane_formats[] = {
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_RGB565,
};

#if 0
static const struct drm_plane_funcs gxmicro_primary_funcs = {
};

static const struct drm_plane_helper_funcs gxmicro_primary_helper_funcs = {
};
#endif

static int gxmicro_primary_plane_init(struct gxmicro_dc_dev *gdev)
{
	struct drm_device *dev = gdev->dev;
	struct drm_plane *primary;
	int ret = 0;

	primary = kzalloc(sizeof(struct drm_plane), GFP_KERNEL);	/* kfree() in drm_plane_funcs.destory */
	if (!primary) {
		pci_err(dev->pdev, "Failed to allocate Primary Plane\n");
		return -ENOMEM;
	}

	ret = drm_universal_plane_init(dev, primary, 0, &drm_primary_helper_funcs,
				gxmicro_primary_plane_formats, ARRAY_SIZE(gxmicro_primary_plane_formats),
				NULL, DRM_PLANE_TYPE_PRIMARY, NULL);
	if (ret < 0) {
		pci_err(dev->pdev, "Failed to init Primary Plane\n");
		goto err_plane_init;
	}
#if 0
	drm_plane_helper_add(primary, &gxmicro_primary_helper_funcs);
#endif
	gdev->primary = primary;

	return 0;

err_plane_init:
	kfree(primary);
	return ret;
}

/* ****************************** Cursor Plane ****************************** */

static int gxmicro_cursor_update(struct gxmicro_dc_dev *gdev, struct drm_framebuffer *fb, struct drm_framebuffer *ofb)
{
	struct drm_device *dev = gdev->dev;
	struct drm_gem_vram_object *gbo;
	int64_t cur_addr;
	int ret;

	if (ofb) {
		gbo = drm_gem_vram_of_gem(ofb->obj[0]);
		drm_gem_vram_unpin(gbo);
	}

	gbo = drm_gem_vram_of_gem(fb->obj[0]);

	ret = drm_gem_vram_pin(gbo, DRM_GEM_VRAM_PL_FLAG_VRAM);
	if (ret) {
		pci_err(dev->pdev, "Failed to pin Cursor Plane\n");
		return ret;
	}

	cur_addr = drm_gem_vram_offset(gbo);
	if (cur_addr < 0) {
		ret = (int)cur_addr;
		pci_err(dev->pdev, "Failed to get Cursor address\n");
		goto err_cursor_offset;
	}

	gxmicro_write(gdev, DC_CURSOR_ADDR, cur_addr);

	pci_dbg(dev->pdev, "Cursor width * height: 0x%08x * 0x%08x, stride: 0x%08x, Cursor fb addr: 0x%08llx\n",
				fb->width, fb->height, fb->pitches[0], FB_CUR_OFFSET(cur_addr));

	return 0;

err_cursor_offset:
	drm_gem_vram_unpin(gbo);
	return ret;
}

static void gxmicro_cursor_move(struct gxmicro_dc_dev *gdev,
			int32_t hotx, int32_t hoty, int32_t x, int32_t y)
{
	struct drm_device *dev = gdev->dev;
	uint32_t cur_ctrl;
	uint32_t cur_loc;

	cur_ctrl = CURSOR_HOTSPOT(hotx, hoty);
	cur_loc = CURSOR_LOCATOIN(x, hotx, y, hoty);

	gxmicro_write(gdev, DC_CURSOR_LOCATION, cur_loc);

	gxmicro_write(gdev, DC_CURSOR_CTRL, cur_ctrl);

	pci_dbg(dev->pdev, "Cursor ctrl: 0x%08x, loction: 0x%08x, "
			"hotx: 0x%08x, hoty: 0x%08x, x: 0x%08x, y: 0x%08x\n",
			cur_ctrl, cur_loc, hotx, hoty, x, y);
}

static int gxmicro_cursor_update_plane(struct drm_plane *cursor, struct drm_crtc *crtc, struct drm_framebuffer *fb,
				int crtc_x, int crtc_y, unsigned int crtc_w, unsigned int crtc_h,
				uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h,
				struct drm_modeset_acquire_ctx *ctx)
{
	struct gxmicro_dc_dev *gdev = drm_get_priv(cursor->dev);
	int ret = 0;

	if (fb != cursor->fb) {
		ret = gxmicro_cursor_update(gdev, fb, cursor->fb);
		if (ret)
			return ret;
	}

	gxmicro_cursor_move(gdev, fb->hot_x, fb->hot_y, crtc_x, crtc_y);

	return 0;
}

static int gxmicro_cursor_disable_plane(struct drm_plane *cursor, struct drm_modeset_acquire_ctx *ctx)
{
	struct drm_device *dev = cursor->dev;
	struct gxmicro_dc_dev *gdev = drm_get_priv(dev);
	struct drm_gem_vram_object *gbo;

	gxmicro_write(gdev, DC_CURSOR_CTRL, CUR_DISABLE);

	if (cursor->fb) {
		gbo = drm_gem_vram_of_gem(cursor->fb->obj[0]);
		drm_gem_vram_unpin(gbo);
	}

	pci_dbg(dev->pdev, "Disable Cursor\n");

	return 0;
}

static const uint32_t gxmicro_cursor_plane_formats[] = {
	DRM_FORMAT_ARGB8888,
};

static const struct drm_plane_funcs gxmicro_cursor_funcs = {
	.destroy = drm_plane_cleanup,
	.update_plane = gxmicro_cursor_update_plane,
	.disable_plane = gxmicro_cursor_disable_plane,
};

#if 0
static const struct drm_plane_helper_funcs gxmicro_cursor_helper_funcs = {
};
#endif

static int gxmicro_cursor_plane_init(struct gxmicro_dc_dev *gdev)
{
	struct drm_device *dev = gdev->dev;
	struct drm_plane *cursor = &gdev->cursor;
	int ret = 0;

	ret = drm_universal_plane_init(dev, cursor, 0, &gxmicro_cursor_funcs,
				gxmicro_cursor_plane_formats, ARRAY_SIZE(gxmicro_cursor_plane_formats),
				NULL, DRM_PLANE_TYPE_CURSOR, NULL);
	if (ret < 0) {
		pci_err(dev->pdev, "Failed to init Cursor Plane\n");
		return ret;
	}
#if 0
	drm_plane_helper_add(cursor, &gxmicro_cursor_helper_funcs);
#endif
	return 0;
}

/* ****************************** Crtc ****************************** */

static void gxmicro_crtc_dpms(struct drm_crtc *crtc, int mode)
{
	struct drm_device *dev = crtc->dev;
	struct gxmicro_dc_dev *gdev = drm_get_priv(dev);

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		/* fallthrough */
	case DRM_MODE_DPMS_STANDBY:
		/* fallthrough */
	case DRM_MODE_DPMS_SUSPEND:
		gdev->dctrl |= DC_ENABLE;
		break;
	case DRM_MODE_DPMS_OFF:
		gdev->dctrl &= ~DC_ENABLE;
		break;
	}

	gxmicro_write(gdev, DC_CTRL, gdev->dctrl);

	pci_dbg(dev->pdev, "%s Display Controller, dc ctrl: 0x%08x\n",
			mode == 3 ? "Disabled" : "Enabled", gdev->dctrl);
}

static void gxmicro_crtc_prepare(struct drm_crtc *crtc)
{
	gxmicro_crtc_dpms(crtc, DRM_MODE_DPMS_OFF);
}

static void gxmicro_crtc_commit(struct drm_crtc *crtc)
{
	gxmicro_crtc_dpms(crtc, DRM_MODE_DPMS_ON);
}

static int gxmicro_crtc_mode_set_base(struct drm_crtc *crtc, int x, int y, struct drm_framebuffer *ofb)
{
	struct drm_device *dev = crtc->dev;
	struct gxmicro_dc_dev *gdev = drm_get_priv(dev);
	struct drm_framebuffer *fb = crtc->primary->fb;
	struct drm_gem_vram_object *gbo;
	int ret;
	int64_t fb_addr;

	if (ofb) {
		gbo = drm_gem_vram_of_gem(ofb->obj[0]);
		drm_gem_vram_unpin(gbo);
	}

	gbo = drm_gem_vram_of_gem(fb->obj[0]);

	ret = drm_gem_vram_pin(gbo, DRM_GEM_VRAM_PL_FLAG_VRAM);
	if (ret) {
		pci_err(dev->pdev, "Failed to pin Primary Plane\n");
		return ret;
	}

	fb_addr = drm_gem_vram_offset(gbo);
	if (fb_addr < 0) {
		ret = (int)fb_addr;
		pci_err(dev->pdev, "Failed to get Framebuffer address\n");
		goto err_crtc_vram_offset;
	}

	gxmicro_write(gdev, DC_ADDR0, fb_addr);

	pci_dbg(dev->pdev, "Framebuffer addr: 0x%08llx\n", FB_CUR_OFFSET(fb_addr));

	return 0;

err_crtc_vram_offset:
	drm_gem_vram_unpin(gbo);
	return ret;
}

static int gxmicro_crtc_mode_set(struct drm_crtc *crtc, struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode, int x, int y, struct drm_framebuffer *ofb)
{
	struct drm_device *dev = crtc->dev;
	struct gxmicro_dc_dev *gdev = drm_get_priv(dev);
	const struct drm_framebuffer *fb = crtc->primary->fb;
	const uint32_t format = fb->format->format;
	uint32_t hdisplay = 0;
	uint32_t hsync = 0;
	uint32_t vdisplay = 0;
	uint32_t vsync = 0;

	gdev->dctrl &= ~DC_FB_FORMAT;

	switch (format) {
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
		gdev->dctrl |= DC_RGB888;
		break;
	case DRM_FORMAT_RGB565:
		gdev->dctrl |= DC_RGB565;
		break;
	default:
		pci_err(dev->pdev, "Unhandled pixel format 0x%08x\n", format);
		return -EINVAL;
	}

	hdisplay = HVDISPLAY(mode->hdisplay, mode->htotal);
	vdisplay = HVDISPLAY(mode->vdisplay, mode->vtotal);
	hsync = HVSYNC(mode->hsync_start, mode->hsync_end);
	vsync = HVSYNC(mode->vsync_start, mode->vsync_end);
	if (mode->flags & DRM_MODE_FLAG_NHSYNC)
		hsync |= HVSYNC_NEGTIVE;
	if (mode->flags & DRM_MODE_FLAG_NVSYNC)
		vsync |= HVSYNC_NEGTIVE;

	gxmicro_write(gdev, DC_CTRL, gdev->dctrl);

	gxmicro_write(gdev, DC_STRIDE, fb->pitches[0]);		/* Reserved: 切换分辨率可能 改变, fb->pitches[0] 无改变, hdisplay(mode->hdisplay) * bpc(fb->format->cpp[0]) */
	gxmicro_write(gdev, DC_ORIGIN, 0);

	gxmicro_write(gdev, DC_PANEL_CONF, PANEL_CONF);

	/* HDisplay & HSync */
	gxmicro_write(gdev, DC_HDISPLAY, hdisplay);
	gxmicro_write(gdev, DC_HSYNC, hsync);

	/* VDisplay & VSync */
	gxmicro_write(gdev, DC_VDISPLAY, vdisplay);
	gxmicro_write(gdev, DC_VSYNC, vsync);

	gxmicro_crtc_mode_set_base(crtc, x, y, ofb);

	pci_dbg(dev->pdev, "Framebuffer format: 0x%08x, mode: \"%s\". "
		"Display Controller Reg: \"dc ctrl: 0x%08x, stride: 0x%08x, panel: 0x%08lx, "
		"hdisplay: 0x%08x, hsync: 0x%08x, vdisplay: 0x%08x, vsync: 0x%08x\"\n",
		format, mode->name, gdev->dctrl, fb->pitches[0], PANEL_CONF, hdisplay, hsync, vdisplay, vsync);

	return 0;
}

static void gxmicro_crtc_disable(struct drm_crtc *crtc)
{
	struct drm_plane *primary = crtc->primary;
	struct drm_gem_vram_object *gbo;

	gxmicro_crtc_dpms(crtc, DRM_MODE_DPMS_OFF);

	if (primary->fb) {
		gbo = drm_gem_vram_of_gem(primary->fb->obj[0]);
		drm_gem_vram_unpin(gbo);
	}

	primary->fb = NULL;
}

static void gxmicro_crtc_reset(struct drm_crtc *crtc)
{

}

#if 0	/* 非必须, 未测试, 当前无法读 Gamma 相关寄存器 */
static int gxmicro_crtc_gamma_set(struct drm_crtc *crtc,
				uint16_t *r, uint16_t *g, uint16_t *b,
				uint32_t size, struct drm_modeset_acquire_ctx *ctx)
{
	struct drm_device *dev = crtc->dev;
	struct gxmicro_dc_dev *gdev = drm_get_priv(dev);
	uint32_t gamma = 0;
	int i = 0;

	gxmicro_write(gdev, DC_GAMMA_INDEX, 0);

	for (i = 0; i < GAMMA_SIZE; i++) {
		gamma = GAMMA_DATA(r[i], g[i], b[i]);

		gxmicro_write(gdev, DC_GAMMA_DATA, gamma);

		pci_dbg(dev->pdev, "r/g/b = 0x%04x / 0x%04x / 0x%04x, "
				"gamma data 0x%08x\n", r[i], g[i], b[i], gamma);	/* 验证成功后可去除 */
	}

	return 0;
}
#endif

static const struct drm_crtc_funcs gxmicro_crtc_funcs = {
	.destroy = drm_crtc_cleanup,
	.set_config = drm_crtc_helper_set_config,
	.reset = gxmicro_crtc_reset,
#if 0	/* 非必须, 未测试, 当前无法读 Gamma 相关寄存器 */
	.gamma_set = gxmicro_crtc_gamma_set,
#endif
};

static const struct drm_crtc_helper_funcs gxmicro_crtc_helper_funcs = {
	.dpms = gxmicro_crtc_dpms,
	.prepare = gxmicro_crtc_prepare,
	.commit = gxmicro_crtc_commit,
	.mode_set = gxmicro_crtc_mode_set,
	.mode_set_base = gxmicro_crtc_mode_set_base,
	.disable = gxmicro_crtc_disable,
};

static int gxmicro_crtc_init(struct gxmicro_dc_dev *gdev)
{
	struct drm_device *dev = gdev->dev;
	struct drm_plane *primary = gdev->primary;
	struct drm_plane *cursor = &gdev->cursor;
	struct drm_crtc *crtc = &gdev->crtc;
	int ret = 0;

	ret = drm_crtc_init_with_planes(dev, crtc, primary, cursor, &gxmicro_crtc_funcs, NULL);
	if (ret) {
		pci_err(dev->pdev, "Failed to init Crtc\n");
		return ret;
	}

	drm_crtc_helper_add(crtc, &gxmicro_crtc_helper_funcs);
#if 0	/* 非必须, 未测试, 当前无法读 Gamma 相关寄存器 */
	drm_mode_crtc_set_gamma_size(crtc, GAMMA_SIZE);
#endif
	return 0;
}

/* ****************************** Encoder ****************************** */

static void gxmicro_encoder_dpms(struct drm_encoder *encoder, int mode)
{

}

static void gxmicro_encoder_prepare(struct drm_encoder *encoder)
{

}

static void gxmicro_encoder_commit(struct drm_encoder *encoder)
{

}

static void gxmicro_encoder_mode_set(struct drm_encoder *encoder,
			struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{

}

static const struct drm_encoder_funcs gxmicro_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static const struct drm_encoder_helper_funcs gxmicro_encoder_helper_funcs = {
	.dpms = gxmicro_encoder_dpms,
	.prepare = gxmicro_encoder_prepare,
	.commit = gxmicro_encoder_commit,
	.mode_set = gxmicro_encoder_mode_set,
};

static int gxmicro_encoder_init(struct gxmicro_dc_dev *gdev)
{
	struct drm_device *dev = gdev->dev;
	struct drm_crtc *crtc = &gdev->crtc;
	struct drm_encoder *encoder = &gdev->encoder;
	int ret = 0;

	ret = drm_encoder_init(dev, encoder, &gxmicro_encoder_funcs, DRM_MODE_ENCODER_DAC, NULL);
	if (ret) {
		pci_err(dev->pdev, "Failed to init Encoder\n");
		return ret;
	}

	encoder->possible_crtcs = drm_crtc_mask(crtc);

	drm_encoder_helper_add(encoder, &gxmicro_encoder_helper_funcs);

	return 0;
}

/* ****************************** Connector ****************************** */

static int gxmicro_connector_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct gxmicro_dc_dev *gdev = drm_get_priv(dev);
	struct edid *edid;
	int count = 0;

	edid = drm_get_edid(connector, &gdev->adap);
	if (!edid) {
		pci_err(dev->pdev, "Failed to get edid\n");
		return -ENODEV;
	}

	drm_connector_update_edid_property(connector, edid);
	count = drm_add_edid_modes(connector, edid);
	kfree(edid);

	return count;
}

static enum drm_mode_status gxmicro_connector_mode_valid(struct drm_connector *connector, struct drm_display_mode *mode)
{
	return MODE_OK;
}

static struct drm_encoder *gxmicro_connector_best_single_encoder(struct drm_connector *connector)
{
	struct gxmicro_dc_dev *gdev = drm_get_priv(connector->dev);

	return &gdev->encoder;
}

static const struct drm_connector_funcs gxmicro_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
};

static const struct drm_connector_helper_funcs gxmicro_connector_helper_funcs = {
	.get_modes = gxmicro_connector_get_modes,
	.mode_valid = gxmicro_connector_mode_valid,
	.best_encoder = gxmicro_connector_best_single_encoder,
};

static int gxmicro_connector_init(struct gxmicro_dc_dev *gdev)
{
	struct drm_device *dev = gdev->dev;
	struct drm_encoder *encoder = &gdev->encoder;
	struct drm_connector *connector = &gdev->connector;
	int ret = 0;

	ret = drm_connector_init(dev, connector, &gxmicro_connector_funcs, DRM_MODE_CONNECTOR_VGA);
	if (ret) {
		pci_err(dev->pdev, "Failed to init Connector\n");
		return ret;
	}

	drm_connector_helper_add(connector, &gxmicro_connector_helper_funcs);

	connector->polled = 0;

	drm_connector_attach_encoder(connector, encoder);

	return 0;
}

/* ****************************** KMS Init & Fini ****************************** */

int gxmicro_kms_init(struct gxmicro_dc_dev *gdev)
{
	struct drm_device *dev = gdev->dev;
	int ret;

	gxmicro_setup_mode_config(gdev);

	ret = gxmicro_primary_plane_init(gdev);
	if (ret)
		goto err_kms_init;

	ret = gxmicro_cursor_plane_init(gdev);
	if (ret)
		goto err_kms_init;

	ret = gxmicro_crtc_init(gdev);
	if (ret)
		goto err_kms_init;

	ret = gxmicro_encoder_init(gdev);
	if (ret)
		goto err_kms_init;

	ret = gxmicro_connector_init(gdev);
	if (ret)
		goto err_kms_init;

	drm_mode_config_reset(dev);

	return 0;

err_kms_init:
	drm_mode_config_cleanup(dev);
	return ret;
}

void gxmicro_kms_fini(struct gxmicro_dc_dev *gdev)
{
	struct drm_device *dev = gdev->dev;

	drm_mode_config_cleanup(dev);
}
