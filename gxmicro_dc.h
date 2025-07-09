/* SPDX-License-Identifier: GPL-2.0 */
/*
 * GXMicro Display Controller
 *
 * Copyright (C) 2023 GXMicro (ShangHai) Corp.
 *
 * Author:
 * 	Zheng DongXiong <zhengdongxiong@gxmicro.cn>
 */
#ifndef __GXMICRO_DC_H__
#define __GXMICRO_DC_H__

#include <linux/pci.h>
#include <linux/i2c-algo-bit.h>
#include <drm/drm_device.h>
#include <drm/drm_plane.h>
#include <drm/drm_crtc.h>
#include <drm/drm_encoder.h>
#include <drm/drm_connector.h>
#include <drm/drm_gem_vram_helper.h>
#include <linux/bits.h>
#include <linux/sizes.h>

/* ****************************** DDR ****************************** */

/* DDR 起始地址, FrameBuffer 和 Cursor 使用, 用于设置 JPEG 寄存器 */
#define FB_CUR_BASE				0x80000000
#define FB_CUR_OFFSET(offset)			(FB_CUR_BASE + offset)

/* ****************************** PMU Controller ****************************** */

#define PMU_BASE				0x006b0000

#define PMU_OFFSET(offset)			(PMU_BASE + offset)

/* Registers offset for PMU */
#define PMU_RCU_CPU_RSTR			PMU_OFFSET(0x34)
#define PMU_RCU_AHB_RSTR			PMU_OFFSET(0x38)
#define PMU_RCU_AHB_ENR				PMU_OFFSET(0x3c)

/* RCU AHB */
#define CPU0_SW					BIT(0)

/* RCU AHB */
#define RCU_GMAC				BIT(4)
#define RCU_DC					BIT(2)
#define RCU_DDR					BIT(0)

/* ****************************** GPIO A Controller ****************************** */

#define GPIOA_BASE				0x00640000

#define GPIOA_OFFSET(offset)			(GPIOA_BASE + offset)

/* Registers offset for GPIOA */
#define GPIOA_PORTA_DR				GPIOA_OFFSET(0x00)
#define GPIOA_PORTA_DDR				GPIOA_OFFSET(0x04)
#define GPIOA_PORTA_CTRL			GPIOA_OFFSET(0x08)
#define GPIOA_PORTB_DR				GPIOA_OFFSET(0x0c)
#define GPIOA_PORTB_DDR				GPIOA_OFFSET(0x10)
#define GPIOA_PORTB_CTRL			GPIOA_OFFSET(0x14)
#define GPIOA_PORTC_DR				GPIOA_OFFSET(0x18)
#define GPIOA_PORTC_DDR				GPIOA_OFFSET(0x1c)
#define GPIOA_PORTC_CTRL			GPIOA_OFFSET(0x20)
#define GPIOA_PORTD_DR				GPIOA_OFFSET(0x24)
#define GPIOA_PORTD_DDR				GPIOA_OFFSET(0x28)
#define GPIOA_PORTD_CTRL			GPIOA_OFFSET(0x2c)
#define GPIOA_EXT_PORTA				GPIOA_OFFSET(0x50)
#define GPIOA_EXT_PORTB				GPIOA_OFFSET(0x54)
#define GPIOA_EXT_PORTC				GPIOA_OFFSET(0x58)
#define GPIOA_EXT_PORTD				GPIOA_OFFSET(0x5c)

/* ****************************** Display Controller ****************************** */

/*
 * Display Controller
 * 	1. FPGA提供说明: 寄存器只有一帧结束后才可正常读取, 否则读取不准确现象
 * 	2. 尽量减少 "读取 --> 配置 "操作流程
 *	3. 使用驱动维护需要读的寄存器
 * 官方提供 demo 中, 设置后将 Reset 置位, 再使能 Display Controller
 */
#define DC_BASE					0x01d40000	/* vga */

#define DC_OFFSET(offset)			(DC_BASE + offset)

/* Display Controller & Cursor, Support weidth and height  */
#define DISPLAY_WIDTH				1920
#define DISPLAY_HEIGHT				1080
#define CURSOR_WIDTH				32
#define CURSOR_HEIGHT				32
#define CURSOR_SIZE				SZ_4K	/* CURSOR_WIDTH * CURSOR_HEIGHT * 4 = SZ_4K */

/* Registers offset for Display 0 */
#define DC_CTRL					DC_OFFSET(0x1240)
#define DC_ADDR0				DC_OFFSET(0x1260)
#define DC_ADDR1				DC_OFFSET(0x1580)
#define DC_STRIDE				DC_OFFSET(0x1280)
#define DC_ORIGIN				DC_OFFSET(0x12a0)
#define DC_DITHER_CONF				DC_OFFSET(0x1360)
#define DC_DITHER_TABLE_LOW			DC_OFFSET(0x1380)
#define DC_DITHER_TABLE_HIGH			DC_OFFSET(0x13a0)
#define DC_PANEL_CONF				DC_OFFSET(0x13c0)
#define DC_PANEL_TIMING				DC_OFFSET(0x13e0)
#define DC_HDISPLAY				DC_OFFSET(0x1400)
#define DC_HSYNC				DC_OFFSET(0x1420)
#define DC_VDISPLAY				DC_OFFSET(0x1480)
#define DC_VSYNC				DC_OFFSET(0x14a0)
#define DC_GAMMA_INDEX				DC_OFFSET(0x14e0)
#define DC_GAMMA_DATA				DC_OFFSET(0x1500)

#if 0
/* Registers offset for Display 1 */
#define DC_FB1_CONF				DC_OFFSET(0x1250)
#define DC_FB1_ADDR0				DC_OFFSET(0x1270)
#define DC_FB1_ADDR1				DC_OFFSET(0x1590)
#define DC_FB1_STRIDE				DC_OFFSET(0x1290)
#define DC_FB1_ORIGIN				DC_OFFSET(0x12b0)
#define DC_FB1_DITHER_CONF			DC_OFFSET(0x1370)
#define DC_FB1_DITHER_TABLE_LOW			DC_OFFSET(0x1390)
#define DC_FB1_DITHER_TABLE_HIGH		DC_OFFSET(0x13b0)
#define DC_FB1_PANEL_CONF			DC_OFFSET(0x13d0)
#define DC_FB1_PANEL_TIMING			DC_OFFSET(0x13f0)
#define DC_FB1_HDISPLAY				DC_OFFSET(0x1410)
#define DC_FB1_HSYNC				DC_OFFSET(0x1430)
#define DC_FB1_VDISPLAY				DC_OFFSET(0x1490)
#define DC_FB1_VSYNC				DC_OFFSET(0x14b0)
#define DC_FB1_GAMMA_INDEX			DC_OFFSET(0x14f0)
#define DC_FB1_GAMMA_DATA			DC_OFFSET(0x1510)
#endif

/* Registers offset for Cursor */
#define DC_CURSOR_CTRL				DC_OFFSET(0x1520)
#define DC_CURSOR_ADDR				DC_OFFSET(0x1530)
#define DC_CURSOR_LOCATION			DC_OFFSET(0x1540)
#define DC_CURSOR_BACKGROUND			DC_OFFSET(0x1550)
#define DC_CURSOR_FOREGROUND			DC_OFFSET(0x1560)

/* Registers offset for Interrupt */
#define DC_INTERRUPT				DC_OFFSET(0x1600)
#define DC_INTERRUPT_ENABLE			DC_OFFSET(0x1610)

#if 0
/* Registers offset for Clock 没有在手册找到, 使用官方设置 */
#define DC_CLOCK_LOW				DC_OFFSET(0x1700)
# define DC_CLOCK_LOW_DATA			0x10
#define DC_CLOCK_HIGH				DC_OFFSET(0x1710)
# define DC_CLOCK_HIGH_DATA			0x00
#endif

/* FrameBuffer Configuration */
#define RESET_DC_CTRL				BIT(20)
#define GAMMA_ENABLE				BIT(12)	/* 非必须, 未测试, 当前无法读 Gamma 相关寄存器 */
#define SWITCH_PANEL				BIT(9)
#define OUTPUT_ENABLE				BIT(8)
#define DC_FB_FORMAT				GENMASK(2, 0)
# define DC_RGB888				BIT(2)
# define DC_RGB565				GENMASK(1, 0)
# define DC_RGB555				BIT(1)
# define DC_RGB444				BIT(0)
#define DC_ENABLE				(RESET_DC_CTRL | OUTPUT_ENABLE)

/* Panel Configuration */
#define HWSEQ					BIT(31)
#define CLOCK_POLARITY				BIT(9)
#define CLOCK					BIT(8)
#define DATA_POLARITY				BIT(5)
#define DE_POLARITY				BIT(1)
#define DE					BIT(0)
#define PANEL_CONF				(HWSEQ | CLOCK_POLARITY | CLOCK | DE)

/* HVDisplay & HVSync  */
#define HVDISPLAY_TOTAL(t)			(((t) & 0xfff) << 16)
#define HVDISPLAY_END(e)			((e) & 0xfff)
#define HVDISPLAY(e, t)				(HVDISPLAY_TOTAL(t) | HVDISPLAY_END(e))
#define HVSYNC_NEGTIVE				BIT(31)	/* 0: positive, 1: negtive */
#define PULSE_ENABLE				BIT(30)
#define HVSYNC_END(e)				(((e) & 0xfff) << 16)
#define HVSYNC_START(s)				((s) & 0xfff)
#define HVSYNC(s, e)				(PULSE_ENABLE | HVSYNC_END(e) | HVSYNC_START(s))

/* Gamma Data */ /* 非必须, 未测试, 当前无法读 Gamma 相关寄存器 */
#define GAMMA_SIZE				SZ_256
#define GAMMA_RED(r)				(((r) & 0xff00) << 8)
#define GAMMA_GREEN(g)				((g) & 0xff00)
#define GAMMA_BULE(b)				(((b) & 0xff00) >> 8)
#define GAMMA_DATA(r, g, b)			(GAMMA_RED(r) | GAMMA_GREEN(g) | GAMMA_BULE(b))

/* Cursor Configuration */
#define CURSOR_HOTSPOT_X(x)			(((x) & 0x1f) << 16)
#define CURSOR_HOTSPOT_Y(y)			(((y) & 0x1f) << 8)
#define CURSOR_DISPLAY				BIT(4)	/* Always in FB 0 */
#define CURSOR_FORMAT				GENMASK(1, 0)
# define CUR_ARGB8888				BIT(1)
/*
 * 当 CURSOR 使能时才会修改 HOTSPOT, 因此直接加入 CUR_ARGB8888 使能,
 * 避免读 Register 不准确, 导致配置寄存器错误
 */
#define CURSOR_HOTSPOT(x, y)			(CURSOR_HOTSPOT_X(x) | CURSOR_HOTSPOT_Y(y) | CUR_ARGB8888)
#define CUR_DISABLE				0

/* Cursor Location */
#define CURSOR_Y(y)				(((y) & 0x7ff) << 16)
#define CURSOR_X(x)				((x) & 0x7ff)
/*
 * 光标位置(用户设置)
 * 	curosr location (x, y)
 * 	hotspot (hotx, hoty)
 * dc 实际显示
 * 	cursor location (x - hotx, y - hoty)
 * 	hotspot (hotx, hoty)
 * 因此需要如下设置, 符合用户设置
 * 	cursor location (x + hotx, y + hoty)
 * 	hotspot (hotx, hoty)
 */
#define CURSOR_LOCATOIN(x, hotx, y, hoty)	(CURSOR_Y((y) + (hoty)) | CURSOR_X((x) + (hotx)))

/* ****************************** JPEG Controller ****************************** */

#define JPEG_BASE				0x00670000

#define JPEG_OFFSET(offset)			(JPEG_BASE + offset)

#define JPEG_RATE				30
#define JPEG_MIN_WIDTH				640
#define JPEG_MIN_HEIGHT				480
#define JPEG_MIN_PCLK				12587500	/* 640 x 480 x 30Hz, pclk (640 x 480 60Hz) / 2 */
#define JPEG_MAX_WIDTH				1920
#define JPEG_MAX_HEIGHT				1080
#define JPEG_MAX_PCLK				74250000	/* 1920 x 1080 x 30Hz, pclk (1920 x 1080 60Hz) / 2 */
#define JPEG_BUFFERS				3		/* default 3, ikvm 默认申请3个buffer */

/* Registers offset for JPEG  */
#define JPEG_CTRL				JPEG_OFFSET(0x00)
#define JPEG_CONF				JPEG_OFFSET(0x04)
#define JPEG_WIDTH				JPEG_OFFSET(0x08)
#define JPEG_HEIGHT				JPEG_OFFSET(0x0c)
#define JPEG_ENC_QP				JPEG_OFFSET(0x10)
#define JPEG_FB_BASE				JPEG_OFFSET(0x14)
#define JPEG_BS_BASE				JPEG_OFFSET(0x18)
#define JPEG_BS_LENGTH				JPEG_OFFSET(0x1c)
#define JPEG_BS_LEN_MAX				JPEG_OFFSET(0x20)
#define JPEG_INTR				JPEG_OFFSET(0x24)
#define JPEG_VERSION				JPEG_OFFSET(0x2c)

/* JEPG Crtl Resgister */
#define JPEG_ENC_START				BIT(0)	/* 启动一帧 jpeg 编码, 若需要再次编码: 重新使能 */
#define JPEG_ENC_STOP				0

/* JEPG Configuration Resgister */
#define JPEG_INTR_ENABLE			BIT(12)
#define JPEG_BS_FORMAT				GENMASK(7, 4)
# define JEPG_BS_YUV444				GENMASK(5, 4)
# define JEPG_BS_YUV420				BIT(4)
# define JPEG_CHROMA_SUBSAMPLING		~(BIT(V4L2_JPEG_CHROMA_SUBSAMPLING_444) | BIT(V4L2_JPEG_CHROMA_SUBSAMPLING_420))
#define JPEG_ENC_FORMAT				GENMASK(3, 0)
# define JPEG_ENC_RBG565			0
# define JPEG_ENC_RBG888			BIT(0)	/* Not Support */
# define JPEG_ENC_YUV422			BIT(1)
# define JPEG_ENC_XRGB8888			GENMASK(1, 0)
#define JPEG_32BPP				32	/* ARGB8888, XRGB8888 */
#define JPEG_24BPP				24	/* RGB888, YUV444 */
#define JPEG_16BPP				16	/* RGB565, YUV422 */
#define JPEG_12BPP				12	/* YUV420 */
#define JPEG_BPL(w, bpp)			((((w) * (bpp)) / 8))
#define JPEG_SZ(h, bpl)				((h) * (bpl))

/* JEPG BS Len Max Resgister */
#define JPEG_MAX_BS				(((JPEG_MAX_WIDTH) * (JPEG_MAX_HEIGHT) * (JPEG_32BPP)) / 8)
#define JPEG_MIN_BS				0

/* JEPG Quality Resgister */
#define JPEG_QP_MAX				2047
#define JPEG_QP_MIN				1
#define JPEG_QP_DEF				128

/* JEPG Intr Resgister */
#define JPEG_BS_OVERFLOW			BIT(8)
#define JPEG_EOF				BIT(0)
#define JPEG_INTR_CLEAN				(JPEG_BS_OVERFLOW | JPEG_EOF)

struct gxmicro_dc_dev {
	struct drm_device *dev;
	struct drm_plane *primary;
	struct drm_plane cursor;
	struct drm_crtc crtc;
	struct drm_encoder encoder;
	struct drm_connector connector;

	struct i2c_adapter adap;
	struct i2c_algo_bit_data algo;
	struct i2c_client *sil9134;

	void __iomem *mmio;

	uint32_t dctrl;
};

static inline uint32_t gxmicro_read(struct gxmicro_dc_dev *gdev, uint32_t reg)
{
	return ioread32(gdev->mmio + reg);
}

static inline void gxmicro_write(struct gxmicro_dc_dev *gdev, uint32_t reg, uint32_t val)
{
	iowrite32(val, gdev->mmio + reg);
}

int gxmicro_i2c_init(struct gxmicro_dc_dev *gdev);
void gxmicro_i2c_fini(struct gxmicro_dc_dev *gdev);

int gxmicro_ttm_init(struct gxmicro_dc_dev *gdev);
void gxmicro_ttm_fini(struct gxmicro_dc_dev *gdev);

int gxmicro_kms_init(struct gxmicro_dc_dev *gdev);
void gxmicro_kms_fini(struct gxmicro_dc_dev *gdev);

#endif /* __GXMICRO_DC_H__ */
