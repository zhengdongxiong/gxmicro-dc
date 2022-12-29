// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * GXMicro I2C
 *
 * Copyright (C) 2022 GXMicro (ShangHai) Corp.
 *
 * Author:
 * 	Zheng DongXiong <zhengdongxiong@gxmicro.cn>
 */
#include "gxmicro_dc.h"

/* DisplayController 使用 gpio 模拟 i2c 控制 encoder 读取 edid 信息 */
#define GPIOA_94		30		/* gpio94 号引脚 */
#define GPIOA_95		31		/* gpio95 号引脚 */

#define GPIOA_SCL		GPIOA_94	/* PROT C GPIO 94 */
#define GPIOA_SDA		GPIOA_95	/* PROT C GPIO 95 */

static void gxmicro_gpio_set(struct gxmicro_dc_dev *gdev, bool state, uint8_t pin)
{
	uint32_t gpio;
	uint32_t val;

	gpio = gxmicro_read(gdev, GPIOA_PORTC_DDR);
	gpio |= BIT(pin);
	gxmicro_write(gdev, GPIOA_PORTC_DDR, gpio);

	val = gxmicro_read(gdev, GPIOA_PORTC_DR);

	if (state)
		val |= BIT(pin);
	else
		val &= ~BIT(pin);

	gxmicro_write(gdev, GPIOA_PORTC_DR, val);
}

static bool gxmicro_gpio_get(struct gxmicro_dc_dev *gdev, uint8_t pin)
{
	uint32_t gpio;

	gpio = gxmicro_read(gdev, GPIOA_PORTC_DDR);
	gpio &= ~BIT(pin);
	gxmicro_write(gdev, GPIOA_PORTC_DDR, gpio);

	return gxmicro_read(gdev, GPIOA_EXT_PORTC) & BIT(pin);
}

static void gxmicro_setsda(void *priv, int state)
{
	gxmicro_gpio_set(priv, state, GPIOA_SDA);
}

static void gxmicro_setscl(void *priv, int state)
{
	gxmicro_gpio_set(priv, state, GPIOA_SCL);
}

static int gxmicro_getsda(void *priv)
{
	return gxmicro_gpio_get(priv, GPIOA_SDA);
}

static int gxmicro_getscl(void *priv)
{
	return gxmicro_gpio_get(priv, GPIOA_SCL);
}

int gxmicro_i2c_init(struct gxmicro_dc_dev *gdev)
{
	struct i2c_adapter *adap = &gdev->adap;
	struct i2c_algo_bit_data *algo = &gdev->algo;
	int ret = 0;

	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_DDC;
	adap->dev.parent = &gdev->pdev->dev;
	adap->algo_data = algo;
	i2c_set_adapdata(&gdev->adap, gdev);
	snprintf(gdev->adap.name, sizeof(gdev->adap.name), "Qogir i2c bit bus");

	algo->data = gdev;
	algo->setsda = gxmicro_setsda;
	algo->setscl = gxmicro_setscl;
	algo->getsda = gxmicro_getsda;
	algo->getscl = gxmicro_getscl;
	algo->udelay = 20;
	algo->timeout = usecs_to_jiffies(2200);

	ret = i2c_bit_add_bus(&gdev->adap);
	if (ret)
		pci_err(gdev->pdev, "Failed to register bit I2C\n");

	return ret;
}

void gxmicro_i2c_fini(struct gxmicro_dc_dev *gdev)
{
	struct i2c_adapter *adap = &gdev->adap;

	i2c_del_adapter(adap);
}
