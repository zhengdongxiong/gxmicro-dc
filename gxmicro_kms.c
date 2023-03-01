// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * GXMicro DRM KMS
 *
 * Copyright (C) 2022 GXMicro (ShangHai) Corp.
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

#if 0
#define CURRENT_WIDTH		DISPLAY_WIDTH
#define CURRENT_HEIGHT		DISPLAY_HEIGHT
#endif

#if 1
#define CURRENT_WIDTH		640
#define CURRENT_HEIGHT		480
#endif

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

	dev->mode_config.min_width		= 0;
	dev->mode_config.min_height		= 0;
	dev->mode_config.max_width		= CURRENT_WIDTH;
	dev->mode_config.max_height		= CURRENT_HEIGHT;
#if 0
	dev->mode_config.max_width		= DISPLAY_WIDTH;
	dev->mode_config.max_height		= DISPLAY_HEIGHT;
#endif
	dev->mode_config.cursor_width		= CURSOR_WIDTH;
	dev->mode_config.cursor_height		= CURSOR_HEIGHT;
	dev->mode_config.funcs			= &gxmicro_mode_congfig_funcs;
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
		pci_err(gdev->pdev, "Failed to allocate Primary Plane\n");
		return -ENOMEM;
	}

	ret = drm_universal_plane_init(dev, primary, 0, &drm_primary_helper_funcs,
				gxmicro_primary_plane_formats, ARRAY_SIZE(gxmicro_primary_plane_formats),
				NULL, DRM_PLANE_TYPE_PRIMARY, NULL);
	if (ret < 0) {
		kfree(primary);
		pci_err(gdev->pdev, "Failed to init Primary Plane\n");
		return ret;
	}
#if 0
	drm_plane_helper_add(primary, &gxmicro_primary_helper_funcs);
#endif
	gdev->primary = primary;

	return 0;
}

/* ****************************** Cursor Plane ****************************** */

static inline int gxmicro_cursor_prepare_fb(struct drm_framebuffer *fb)
{
	struct drm_gem_vram_object *gbo;

	gbo = drm_gem_vram_of_gem(fb->obj[0]);

	return drm_gem_vram_pin(gbo, DRM_GEM_VRAM_PL_FLAG_VRAM);;
}

static inline void gxmicro_cursor_cleanup_fb(struct drm_framebuffer *fb)
{
	struct drm_gem_vram_object *gbo;

	gbo = drm_gem_vram_of_gem(fb->obj[0]);

	drm_gem_vram_unpin(gbo);
}

#if 0
#ifdef CONFIG_SW64
static inline void gxmicro_cursor_copy(uint32_t *dst, uint32_t *src)
{
	int i;
	for (i = 0; i < CURSOR_WIDTH * CURSOR_HEIGHT; i++)	/* step: 4Bytes */
		writel(src[i], dst + i);
}
#endif
#endif

static int gxmicro_cursor_update(struct gxmicro_dc_dev *gdev, struct drm_framebuffer *fb)
{
	struct drm_gem_vram_object *gbo;
	void *src, *dst;
	int64_t cur_addr;
	int ret = 0;

	gbo = drm_gem_vram_of_gem(fb->obj[0]);

	src = drm_gem_vram_kmap(gbo, true, NULL);
	if (IS_ERR(src)) {
		ret = PTR_ERR(src);
		pci_err(gdev->pdev, "Failed to kmap user Cursor Framebuffer updates\n");
		goto out_user_cursor_kmap;
	}

	dst = drm_gem_vram_kmap(gdev->gbo, false, NULL);	/* kmap in gxmicro_cursor_init() */
	if (IS_ERR(dst)) {
		ret = PTR_ERR(dst);
		pci_err(gdev->pdev, "Failed to kmap Cursor Framebuffer updates\n");
		goto out_gxmicro_cursor_kmap;
	}

	cur_addr = drm_gem_vram_offset(gdev->gbo);
	if (cur_addr < 0) {
		ret = (int)cur_addr;
		pci_err(gdev->pdev, "Failed to get Cursor address\n");
		goto out_gxmicro_cursor_kmap;
	}
#if 0
#ifdef CONFIG_SW64
	memcpy_io();
	gxmicro_cursor_copy(dst, src);	/* sw64 memcpy 报错?? */
#else
	memcpy(dst, src, CURSOR_SIZE);
#endif
#endif
	memcpy(dst, src, CURSOR_SIZE);

	gxmicro_write(gdev, DC_CURSOR_ADDR, cur_addr);

	pci_dbg(gdev->pdev, "Cursor width * height: 0x%08x * 0x%08x, stride: 0x%08x, Cursor fb addr: 0x%08llx\n",
				fb->width, fb->height, fb->pitches[0], FB_CUR_OFFSET(cur_addr));

out_gxmicro_cursor_kmap:
	drm_gem_vram_kunmap(gbo);
out_user_cursor_kmap:
	return ret;
}

static void gxmicro_cursor_move(struct gxmicro_dc_dev *gdev,
			int32_t hotx, int32_t hoty, int32_t x, int32_t y)
{
	uint32_t cur_ctrl;
	uint32_t cur_loc;

	cur_ctrl = CURSOR_HOTSPOT(hotx, hoty);
	cur_loc = CURSOR_LOCATOIN(x, y);	/* Reserved: 可能需要修改为 (x + hotx, y + hoty), 实际 location (x - hotx, y - hoty), hotspot (x, y) 位置 */

	gxmicro_write(gdev, DC_CURSOR_LOCATION, cur_loc);

	gxmicro_write(gdev, DC_CURSOR_CTRL, cur_ctrl);

	pci_dbg(gdev->pdev, "Cursor ctrl: 0x%08x, loction: 0x%08x, "
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

	ret = gxmicro_cursor_prepare_fb(fb);
	if (ret) {
		pci_err(gdev->pdev, "Failed to prepare Cursor Framebuffer\n");
		goto out_cursor_prepare_fb;
	}

	if (fb != cursor->old_fb) {
		ret = gxmicro_cursor_update(gdev, fb);
		if (ret)
			goto out_cursor_update;
	}

	gxmicro_cursor_move(gdev, fb->hot_x, fb->hot_y, crtc_x, crtc_y);

out_cursor_update:
	gxmicro_cursor_prepare_fb(fb);
out_cursor_prepare_fb:
	return ret;
}

static int gxmicro_cursor_disable_plane(struct drm_plane *cursor, struct drm_modeset_acquire_ctx *ctx)
{
	struct gxmicro_dc_dev *gdev = drm_get_priv(cursor->dev);

	gxmicro_write(gdev, DC_CURSOR_CTRL, CUR_DISABLE);

	return 0;
}

static int gxmicro_cursor_late_register(struct drm_plane *cursor)
{
	struct drm_device *dev = cursor->dev;
	struct gxmicro_dc_dev *gdev = drm_get_priv(dev);
	struct drm_gem_vram_object *gbo;
	size_t size;
	void *cur_addr;
	int ret;

	size = roundup(CURSOR_SIZE, PAGE_SIZE);

	gbo = drm_gem_vram_create(dev, &dev->vram_mm->bdev, size, 0, false);
	if (IS_ERR(gbo)) {
		pci_err(gdev->pdev, "Failed to allocate GEM VRAM object\n");
		return PTR_ERR(gbo);
	}

	ret = drm_gem_vram_pin(gbo, DRM_GEM_VRAM_PL_FLAG_VRAM);
	if (ret) {
		pci_err(gdev->pdev, "Failed to pin Cursor\n");
		goto err_cursor_vram_pin;
	}

	cur_addr = drm_gem_vram_kmap(gbo, true, NULL);
	if (IS_ERR(cur_addr)) {
		ret = PTR_ERR(cur_addr);
		pci_err(gdev->pdev, "Failed to kmap Cursor Framebuffer\n");
		goto err_cursor_gem_vram;
	}

	gdev->gbo = gbo;

	return 0;

err_cursor_gem_vram:
	drm_gem_vram_unpin(gbo);
err_cursor_vram_pin:
	drm_gem_object_put_unlocked(&gbo->bo.base);
	return ret;
}

static void gxmicro_cursor_early_unregister(struct drm_plane *cursor)
{
	struct gxmicro_dc_dev *gdev = drm_get_priv(cursor->dev);
	struct drm_gem_vram_object *gbo = gdev->gbo;

	drm_gem_vram_kunmap(gbo);
	drm_gem_vram_unpin(gbo);
	drm_gem_object_put_unlocked(&gbo->bo.base);
}

static const uint32_t gxmicro_cursor_plane_formats[] = {
	DRM_FORMAT_ARGB8888,
};

static const struct drm_plane_funcs gxmicro_cursor_funcs = {
	.destroy = drm_plane_cleanup,
	.update_plane = gxmicro_cursor_update_plane,
	.disable_plane = gxmicro_cursor_disable_plane,
	.late_register = gxmicro_cursor_late_register,
	.early_unregister = gxmicro_cursor_early_unregister,
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
		pci_err(gdev->pdev, "Failed to init Cursor Plane\n");
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
	struct gxmicro_dc_dev *gdev = drm_get_priv(crtc->dev);

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

	pci_dbg(gdev->pdev, "%s Display Controller, dc ctrl: 0x%08x\n",
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

static int gxmicro_crtc_mode_set_base(struct drm_crtc *crtc, int x, int y, struct drm_framebuffer *old_fb)
{
	struct gxmicro_dc_dev *gdev = drm_get_priv(crtc->dev);
	struct drm_framebuffer *fb = crtc->primary->fb;
	struct drm_gem_vram_object *gbo;
	int ret;
	int64_t fb_addr;

	if (old_fb) {
		gbo = drm_gem_vram_of_gem(old_fb->obj[0]);
		drm_gem_vram_unpin(gbo);
	}

	gbo = drm_gem_vram_of_gem(fb->obj[0]);

	ret = drm_gem_vram_pin(gbo, DRM_GEM_VRAM_PL_FLAG_VRAM);
	if (ret) {
		pci_err(gdev->pdev, "Failed to pin Primary Plane\n");
		goto err_crtc_vram_pin;
	}

	fb_addr = drm_gem_vram_offset(gbo);
	if (fb_addr < 0) {
		ret = (int)fb_addr;
		pci_err(gdev->pdev, "Failed to get Framebuffer address\n");
		goto err_crtc_vram_offset;
	}

	gxmicro_write(gdev, DC_ADDR0, fb_addr);

	pci_dbg(gdev->pdev, "Framebuffer addr: 0x%08llx\n", FB_CUR_OFFSET(fb_addr));

	return 0;

err_crtc_vram_offset:
	drm_gem_vram_unpin(gbo);
err_crtc_vram_pin:
	return ret;
}

static int gxmicro_crtc_mode_set(struct drm_crtc *crtc, struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode, int x, int y, struct drm_framebuffer *old_fb)
{
	struct gxmicro_dc_dev *gdev = drm_get_priv(crtc->dev);
	const struct drm_framebuffer *fb = crtc->primary->fb;
	const uint32_t format = fb->format->format;
	uint32_t hdisplay = 0;
	uint32_t hsync = 0;
	uint32_t vdisplay = 0;
	uint32_t vsync = 0;

	gdev->dctrl &= ~FRAME_BUFFER_FORMAT;

	switch (format) {
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
		gdev->dctrl |= DC_RGB888;
		break;
	case DRM_FORMAT_RGB565:
		gdev->dctrl |= DC_RGB565;
		break;
	default:
		pci_err(gdev->pdev, "Unhandled pixel format 0x%08x\n", format);
		return -EINVAL;
	}

	/* Reserved: 根据 hvsync polarity 配置, 0: positive, 1: negtive */
	/* HDisplay & HSync */
	hdisplay = HVDISPLAY(mode->hdisplay, mode->htotal);
	hsync = HVSYNC(mode->hsync_start, mode->hsync_end);

	/* VDisplay & VSync */
	vdisplay = HVDISPLAY(mode->vdisplay, mode->vtotal);
	vsync = HVSYNC(mode->vsync_start, mode->vsync_end);

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

	gxmicro_crtc_mode_set_base(crtc, x, y, old_fb);

	pci_dbg(gdev->pdev, "Framebuffer format: 0x%08x, mode: \"%s\". "
		"Display Controller Reg: \"dc ctrl: 0x%08x, stride: 0x%08x, panel: 0x%08lx, "
		"hdisplay: 0x%08x, hsync: 0x%08x, vdisplay: 0x%08x, vsync: 0x%08x\"\n",
		format, mode->name, gdev->dctrl, fb->pitches[0], PANEL_CONF, hdisplay, hsync, vdisplay, vsync);

	return 0;
}

static void gxmicro_crtc_disable(struct drm_crtc *crtc)
{
	struct drm_framebuffer *fb;
	struct drm_gem_vram_object *gbo;

	gxmicro_crtc_dpms(crtc, DRM_MODE_DPMS_OFF);

	if (crtc->primary->fb) {
		fb = crtc->primary->fb;
		gbo = drm_gem_vram_of_gem(fb->obj[0]);

		drm_gem_vram_unpin(gbo);
	}

	crtc->primary->fb = NULL;
}

static void gxmicro_crtc_reset(struct drm_crtc *crtc)
{

}

#if 0	/* 非必须, 未测试, 当前无法读 Gamma 相关寄存器 */
static int gxmicro_crtc_gamma_set(struct drm_crtc *crtc,
				uint16_t *r, uint16_t *g, uint16_t *b,
				uint32_t size, struct drm_modeset_acquire_ctx *ctx)
{
	struct gxmicro_dc_dev *gdev = drm_get_priv(crtc->dev);
	uint32_t gamma = 0;
	int i = 0;

	gxmicro_write(gdev, DC_GAMMA_INDEX, 0);

	for (i = 0; i < GAMMA_SIZE; i++) {
		gamma = GAMMA_DATA(r[i], g[i], b[i]);

		gxmicro_write(gdev, DC_GAMMA_DATA, gamma);

		pci_dbg(gdev->pdev, "r/g/b = 0x%04x / 0x%04x / 0x%04x, "
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
		pci_err(gdev->pdev, "Failed to init Crtc\n");
		return ret;
	}

	drm_crtc_helper_add(crtc, &gxmicro_crtc_helper_funcs);
#if 0	/* 非必须, 未测试, 当前无法读 Gamma 相关寄存器 */
	drm_mode_crtc_set_gamma_size(crtc, GAMMA_SIZE);
#endif
	return 0;
}

/* ****************************** Encoder ****************************** */

#define I2C_SIL9134_ADDR		0x39	/* Sil9134 addr地址, 0x39 << 1 = 0x72  */

/* Registers offset for SIL9134 */
#define SIL9134_CTRL			0x08	/* Sil9134 控制寄存器 */

/* Registers offset for SIL9134 DDC */
#define SIL9134_DDC_MANUAL		0xec
#define SIL9134_DDC_ADDR		0xed
#define SIL9134_DDC_SEGM		0xee
#define SIL9134_DDC_OFFSET		0xef
#define SIL9134_DDC_COUNT		0xf0
#if 0
#define SIL9134_DDC_COUNT`		0xf0
#define SIL9134_DDC_COUNT2		0xf1
#endif
#define SIL9134_DDC_STATUS		0xf2
#define SIL9134_DDC_CMD			0xf3
#define SIL9134_DDC_DATA		0xf4
#define SIL9134_DDC_FIFOCNT		0xf5

/* System Control Register */
#define SIL9134_CTRL_CONF		0x35

/* DDC I2C Status Register */
#define SIL9134_DDC_TGT_SLAVE(addr)	(addr << 1)

/* DDC I2C Status Register */
#define SIL9134_IN_PROG			BIT(4)

/* DDC I2C Command Register */
#define SIL9134_DDC_FLT_EN		BIT(5)
#define SIL9134_SDA_DEL_EN		BIT(4)
#define SIL9134_CMD_ABORT_TRANS		0x0f
#define SIL9134_CMD_CLEAR_FIFO		0x09
#define SIL9134_CMD_CLOCK_SCL		0x0a
#define SIL9134_CMD_CUR_RD		0x00
#define SIL9134_CMD_SEQ_RD		0x02
#define SIL9134_CMD_EDDC_RD		0x04
#define SIL9134_CMD_SEQ_WR_NACK		0x06
#define SIL9134_CMD_SEQ_WR_ACK		0x07

#if 0
/* DDC I2C Data Count Register */
#define SIL9134_DATA_COUNT1(d)		((d) & 0xff)
#define SIL9134_DATA_COUNT2(d)		((d >> 8) & 0x03)
#endif

#define sil9134_err(gdev, fmt, ...)	pci_err(gdev->pdev, "Sil9134: " fmt, ##__VA_ARGS__)

static struct i2c_board_info gxmicro_board_info = {
	I2C_BOARD_INFO("sil9134", I2C_SIL9134_ADDR),
};

static inline int sil9134_write(struct gxmicro_dc_dev *gdev, uint8_t reg, uint8_t val)
{
	return i2c_smbus_write_byte_data(gdev->sil9134, reg, val);
}

static inline int sil9134_read(struct gxmicro_dc_dev *gdev, uint8_t reg)
{
	return i2c_smbus_read_byte_data(gdev->sil9134, reg);
}

static int gxmicro_encoder_late_register(struct drm_encoder *encoder)
{
	struct gxmicro_dc_dev *gdev = drm_get_priv(encoder->dev);
	int ret;

	gdev->sil9134 = i2c_new_client_device(&gdev->adap, &gxmicro_board_info);
	if (IS_ERR(gdev->sil9134)) {
		pci_err(gdev->pdev, "Failed to add I2C device\n");
		return PTR_ERR(gdev->sil9134);
	}

	ret = sil9134_write(gdev, SIL9134_CTRL, SIL9134_CTRL_CONF);
	if (ret) {
		sil9134_err(gdev, "Unable to enable Sil9134\n");
		return ret;
	}

	return 0;
}

static void gxmicro_encoder_early_unregister(struct drm_encoder *encoder)
{
	struct gxmicro_dc_dev *gdev = drm_get_priv(encoder->dev);

	i2c_unregister_device(gdev->sil9134);
}

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
	.late_register = gxmicro_encoder_late_register,
	.early_unregister = gxmicro_encoder_early_unregister,
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
		pci_err(gdev->pdev, "Failed to init Encoder\n");
		return ret;
	}

	encoder->possible_crtcs = drm_crtc_mask(crtc);

	drm_encoder_helper_add(encoder, &gxmicro_encoder_helper_funcs);

	return 0;
}

/* ****************************** Connector ****************************** */

#if 1
/* drm_edid_load.c */
#define CURRENT_EDID	MODE_800X600

enum edid_mode {
	MODE_800X600,
	MODE_1280X1024,
	MODE_LAST,
};

static const uint8_t generic_edid[MODE_LAST][EDID_LENGTH] = {
	[MODE_800X600] = {
		0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
		0x31, 0xd8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x05, 0x16, 0x01, 0x03, 0x6d, 0x1b, 0x14, 0x78,
		0xea, 0x5e, 0xc0, 0xa4, 0x59, 0x4a, 0x98, 0x25,
		0x20, 0x50, 0x54, 0x01, 0x00, 0x00, 0x45, 0x40,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xa0, 0x0f,
		0x20, 0x00, 0x31, 0x58, 0x1c, 0x20, 0x28, 0x80,
		0x14, 0x00, 0x15, 0xd0, 0x10, 0x00, 0x00, 0x1e,
		0x00, 0x00, 0x00, 0xff, 0x00, 0x4c, 0x69, 0x6e,
		0x75, 0x78, 0x20, 0x23, 0x30, 0x0a, 0x20, 0x20,
		0x20, 0x20, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x3b,
		0x3d, 0x24, 0x26, 0x05, 0x00, 0x0a, 0x20, 0x20,
		0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,
		0x00, 0x4c, 0x69, 0x6e, 0x75, 0x78, 0x20, 0x53,
		0x56, 0x47, 0x41, 0x0a, 0x20, 0x20, 0x00, 0xc2
	},
	[MODE_1280X1024] = {
		0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
		0x31, 0xd8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x05, 0x16, 0x01, 0x03, 0x6d, 0x2c, 0x23, 0x78,
		0xea, 0x5e, 0xc0, 0xa4, 0x59, 0x4a, 0x98, 0x25,
		0x20, 0x50, 0x54, 0x00, 0x00, 0x00, 0x81, 0x80,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x30, 0x2a,
		0x00, 0x98, 0x51, 0x00, 0x2a, 0x40, 0x30, 0x70,
		0x13, 0x00, 0xbc, 0x63, 0x11, 0x00, 0x00, 0x1e,
		0x00, 0x00, 0x00, 0xff, 0x00, 0x4c, 0x69, 0x6e,
		0x75, 0x78, 0x20, 0x23, 0x30, 0x0a, 0x20, 0x20,
		0x20, 0x20, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x3b,
		0x3d, 0x3e, 0x40, 0x0b, 0x00, 0x0a, 0x20, 0x20,
		0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,
		0x00, 0x4c, 0x69, 0x6e, 0x75, 0x78, 0x20, 0x53,
		0x58, 0x47, 0x41, 0x0a, 0x20, 0x20, 0x00, 0xa0,
	},
};

/* 实际edid信息 */
static const uint8_t vga_edid[EDID_LENGTH] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x61, 0xa9, 0x01, 0xe0, 0x01, 0x00, 0x00, 0x00,
	0x1e, 0x1f, 0x01, 0x03, 0x08, 0x3c, 0x21, 0x78, 0xea, 0x1f, 0xa4, 0xa9, 0x51, 0x4a, 0xaa, 0x27,
	0x0a, 0x50, 0x54, 0xbd, 0xcf, 0x00, 0x71, 0x4f, 0x81, 0x00, 0x81, 0x80, 0x81, 0xc0, 0x95, 0x00,
	0xa9, 0xc0, 0xb3, 0x00, 0x01, 0x01, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c,
	0x45, 0x00, 0x56, 0x50, 0x21, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xff, 0x00, 0x33, 0x33, 0x32,
	0x33, 0x31, 0x30, 0x30, 0x30, 0x30, 0x32, 0x35, 0x37, 0x36, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x30,
	0x3c, 0x38, 0x55, 0x13, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,
	0x00, 0x52, 0x65, 0x64, 0x6d, 0x69, 0x20, 0x32, 0x37, 0x20, 0x4e, 0x51, 0x0a, 0x20, 0x00, 0x71,
};

static const uint8_t hdmi_edid[EDID_LENGTH + EDID_LENGTH] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x41, 0x0c, 0x55, 0x55, 0x01, 0x01, 0x01, 0x01,
	0x0a, 0x19, 0x01, 0x03, 0x80, 0x79, 0x44, 0x78, 0x2a, 0x16, 0x4d, 0x9f, 0x5a, 0x52, 0x9f, 0x26,
	0x0e, 0x47, 0x4a, 0xa1, 0x08, 0x00, 0x95, 0x00, 0xb3, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c,
	0x45, 0x00, 0xb9, 0xa8, 0x42, 0x00, 0x00, 0x1e, 0x66, 0x21, 0x50, 0xb0, 0x51, 0x00, 0x1b, 0x30,
	0x40, 0x70, 0x36, 0x00, 0xb9, 0xa8, 0x42, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x14,
	0x4c, 0x1e, 0x53, 0x0f, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,
	0x00, 0x50, 0x68, 0x69, 0x6c, 0x69, 0x70, 0x73, 0x20, 0x54, 0x56, 0x0a, 0x20, 0x20, 0x01, 0x2d,
	0x02, 0x03, 0x28, 0xf1, 0x4d, 0x1f, 0x90, 0x14, 0x05, 0x13, 0x04, 0x02, 0x03, 0x20, 0x01, 0x12,
	0x06, 0x07, 0x29, 0x09, 0x07, 0x07, 0x15, 0x07, 0x50, 0x57, 0x06, 0x00, 0x83, 0x01, 0x00, 0x00,
	0x67, 0x03, 0x0c, 0x00, 0x10, 0x00, 0xb0, 0x2d, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40,
	0x58, 0x2c, 0x25, 0x00, 0xb9, 0xa8, 0x42, 0x00, 0x00, 0x1e, 0x8c, 0x0a, 0xa0, 0x14, 0x51, 0xf0,
	0x16, 0x00, 0x26, 0x7c, 0x43, 0x00, 0xb9, 0xa8, 0x42, 0x00, 0x00, 0x98, 0x01, 0x1d, 0x80, 0x18,
	0x71, 0x1c, 0x16, 0x20, 0x58, 0x2c, 0x25, 0x00, 0xb9, 0xa8, 0x42, 0x00, 0x00, 0x9e, 0x8c, 0x0a,
	0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e, 0x96, 0x00, 0xb9, 0xa8, 0x42, 0x00, 0x00, 0x18,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d
};

static int gxmicro_connector_get_modes(struct drm_connector *connector)
{
	struct edid *edid = (struct edid *)vga_edid;
	int ret = 0;

	drm_connector_update_edid_property(connector, edid);
	ret = drm_add_edid_modes(connector, edid);

	return ret;
}
#endif

#if 0
static inline int sil9134_ddc_set_cmd(struct gxmicro_dc_dev *gdev, uint32_t cmd)
{
	return sil9134_write(gdev, SIL9134_DDC_CMD, cmd);
}

static int sil9134_ddc_init(struct gxmicro_dc_dev *gdev, uint8_t offset, uint8_t segment)
{
	int ret;

	ret = sil9134_write(gdev, SIL9134_DDC_ADDR, SIL9134_DDC_TGT_SLAVE(DDC_ADDR));
	if (ret) {
		sil9134_err(gdev, "Unable to set Slave Addr Reg\n");
		goto err_ddc_init;
	}

	ret = sil9134_write(gdev, SIL9134_DDC_OFFSET, offset);
	if (ret) {
		sil9134_err(gdev, "Unable to set Slave Offset Reg\n");
		goto err_ddc_init;
	}

	ret = sil9134_write(gdev, SIL9134_DDC_SEGM, segment);
	if (ret) {
		sil9134_err(gdev, "Unable to set Segment Reg\n");
		goto err_ddc_init;
	}

	ret = sil9134_write(gdev, SIL9134_DDC_COUNT, EDID_LENGTH);	/* drm_do_get_edid() 一次获取 EDID_LENGTH */
	if (ret) {
		sil9134_err(gdev, "Unable to set Data Count Reg\n");
		goto err_ddc_init;
	}

	return 0;

err_ddc_init:
	return -1;
}

#define sil9134_mdelay(n) ({ mdelay(n); 1; })

static bool sil9134_ddc_idle(struct gxmicro_dc_dev *gdev)
{
	unsigned long timeout = jiffies + 5 * HZ;
	int stat;

	do {
		stat = sil9134_read(gdev, SIL9134_DDC_STATUS);
		if (stat < 0) {
			sil9134_err(gdev, "Unable to read Status Reg\n");
			return false;
		}

	} while ((stat & SIL9134_IN_PROG) && sil9134_mdelay(10) && time_before(jiffies, timeout));

	return !(stat & SIL9134_IN_PROG);
}

static void sil9134_ddc_abort(struct gxmicro_dc_dev *gdev)
{
	int ret;

	ret = sil9134_ddc_set_cmd(gdev, SIL9134_CMD_ABORT_TRANS);
	if (ret) {
		sil9134_err(gdev, "Unable to abort transaction\n");
		goto err_ddc_abort;
	}

	ret = sil9134_ddc_set_cmd(gdev, SIL9134_CMD_CLEAR_FIFO);
	if (ret) {
		sil9134_err(gdev, "Unable to clear Fifo Reg\n");
		goto err_ddc_abort;
	}

	ret = sil9134_ddc_set_cmd(gdev, SIL9134_CMD_ABORT_TRANS);
	if (ret) {
		sil9134_err(gdev, "Unable to abort transaction\n");
		goto err_ddc_abort;
	}

err_ddc_abort:
	return ;
}

static int sil9134_read_edid(struct gxmicro_dc_dev *gdev, uint32_t cmd, uint8_t *buf)
{
	int ret;
	int i = 0;
	int32_t fifo;

	ret = sil9134_ddc_set_cmd(gdev, cmd);
	if (ret) {
		sil9134_err(gdev, "Unable to start DDC read, cmd 0x%04x\n", cmd);
		goto err_read_from_fifo;
	}

	fifo = sil9134_read(gdev, SIL9134_DDC_FIFOCNT);
	if (fifo <= 0) {	/* fifo empty */
		sil9134_err(gdev, "Read Fifo Count Reg %d\n", fifo);
		goto err_read_from_fifo;
	}

	while (i < EDID_LENGTH) {	/* drm_do_get_edid() 一次获取 EDID_LENGTH */

		fifo = sil9134_read(gdev, SIL9134_DDC_DATA);
		if (fifo < 0) {
			sil9134_err(gdev, "Unable to read I2C Data Reg\n");
			goto err_read_from_fifo;
		}

		buf[i++] = fifo;
	}

#if 0
	while (i < EDID_LENGTH) {	/* drm_do_get_edid() 一次获取 EDID_LENGTH */

		fifo_cnt = sil9134_read(gdev, SIL9134_DDC_FIFOCNT);
		if (fifo_cnt <= 0) {	/* fifo empty */
			sil9134_err(gdev, "Read Fifo Count Reg %d\n", fifo);
			goto err_read_from_fifo;
		}

		for (j = 0; j < fifo_cnt; j++) {

			data = sil9134_read(gdev, SIL9134_DDC_DATA);
			if (data < 0) {
				sil9134_err(gdev, "Unable to read I2C Data Reg\n");
				goto err_read_from_fifo;
			}

			buf[i++] = data;
		}
	}
#endif
	return 0;

err_read_from_fifo:
	return -1;
}

#define STANDARD_EDID	0	/* segment = 0 */

static int sil9134_ddc_get_edid(void *data, uint8_t *buf, unsigned int block, size_t len)
{
	struct gxmicro_dc_dev *gdev = data;
	uint8_t offset = block * EDID_LENGTH;
	uint8_t segment = block >> 1;	/* 256 Bytes 之后使用eddc, 使用 segment reg */
	int ret;
	int cmd;

	if (!sil9134_ddc_idle(gdev)) {
		sil9134_err(gdev, "Wait for DDC Reg timeout, reset Sil9134\n");
		sil9134_ddc_abort(gdev);
	}

	ret = sil9134_ddc_init(gdev, offset, segment);
	if (ret)
		goto err_ddc_get_edid;

	switch (segment) {
	case STANDARD_EDID:
		cmd = SIL9134_CMD_SEQ_RD;
		break;
	default:
		cmd = SIL9134_CMD_EDDC_RD;
		break;
	}

	ret = sil9134_read_edid(gdev, cmd, buf);
	if (ret)
		goto err_ddc_get_edid;

	return 0;

err_ddc_get_edid:
	sil9134_ddc_abort(gdev);
	return -1;
}

static int gxmicro_connector_get_modes(struct drm_connector *connector)
{
	struct gxmicro_dc_dev *gdev = drm_get_priv(connector->dev);
	struct edid *edid;
	int count = 0;

	edid = drm_do_get_edid(connector, sil9134_ddc_get_edid, gdev);	/* 每次获取 EDID_LENGTH */
	if (edid) {

		drm_connector_update_edid_property(connector, edid);

		count = drm_add_edid_modes(connector, edid);

		kfree(edid);	/* kmalloc in drm_do_get_edid() */
	}

	return count;
}
#endif

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
		pci_err(gdev->pdev, "Failed to init Connector\n");
		return ret;
	}

	drm_connector_helper_add(connector, &gxmicro_connector_helper_funcs);

	connector->polled = 0;

	drm_connector_attach_encoder(connector, encoder);

	return 0;
}

/* ****************************** DRM Init & Fini ****************************** */

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
