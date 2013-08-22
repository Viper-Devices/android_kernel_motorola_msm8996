/*
 * Copyright (C) 2008-2012 ST-Ericsson
 *
 * Author: Srinidhi KASAGAR <srinidhi.kasagar@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/platform_data/i2c-nomadik.h>
#include <linux/platform_data/db8500_thermal.h>
#include <linux/gpio.h>
#include <linux/amba/bus.h>
#include <linux/amba/pl022.h>
#include <linux/amba/serial.h>
#include <linux/spi/spi.h>
#include <linux/mfd/abx500/ab8500.h>
#include <linux/regulator/ab8500.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/driver.h>
#include <linux/mfd/tc3589x.h>
#include <linux/mfd/tps6105x.h>
#include <linux/mfd/abx500/ab8500-gpio.h>
#include <linux/platform_data/leds-lp55xx.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_data/pinctrl-nomadik.h>
#include <linux/platform_data/dma-ste-dma40.h>

#include <asm/mach-types.h>

#include "setup.h"
#include "devices.h"
#include "irqs.h"

#include "ste-dma40-db8500.h"
#include "db8500-regs.h"
#include "devices-db8500.h"
#include "board-mop500.h"
#include "board-mop500-regulators.h"

static struct abx500_gpio_platform_data ab8500_gpio_pdata = {
	.gpio_base		= MOP500_AB8500_PIN_GPIO(1),
};

struct ab8500_platform_data ab8500_platdata = {
	.irq_base	= MOP500_AB8500_IRQ_BASE,
	.regulator	= &ab8500_regulator_plat_data,
	.gpio		= &ab8500_gpio_pdata,
};

/*
 * TC35892
 */

static void mop500_tc35892_init(struct tc3589x *tc3589x, unsigned int base)
{
	struct device *parent = NULL;
#if 0
	/* FIXME: Is the sdi actually part of tc3589x? */
	parent = tc3589x->dev;
#endif
	mop500_sdi_tc35892_init(parent);
}

static struct tc3589x_gpio_platform_data mop500_tc35892_gpio_data = {
	.gpio_base	= MOP500_EGPIO(0),
	.setup		= mop500_tc35892_init,
};

static struct tc3589x_platform_data mop500_tc35892_data = {
	.block		= TC3589x_BLOCK_GPIO,
	.gpio		= &mop500_tc35892_gpio_data,
	.irq_base	= MOP500_EGPIO_IRQ_BASE,
};

/* I2C0 devices only available on the first HREF/MOP500 */
static struct i2c_board_info __initdata mop500_i2c0_devices[] = {
	{
		I2C_BOARD_INFO("tc3589x", 0x42),
		.irq		= NOMADIK_GPIO_TO_IRQ(217),
		.platform_data  = &mop500_tc35892_data,
	},
};

static struct i2c_board_info __initdata mop500_i2c2_devices[] = {
	{
		/* Light sensor Rohm BH1780GLI */
		I2C_BOARD_INFO("bh1780", 0x29),
	},
};

static int __init mop500_i2c_board_init(void)
{
	if (machine_is_u8500())
		mop500_uib_i2c_add(0, mop500_i2c0_devices,
				   ARRAY_SIZE(mop500_i2c0_devices));
	mop500_uib_i2c_add(2, mop500_i2c2_devices,
			   ARRAY_SIZE(mop500_i2c2_devices));
	return 0;
}
device_initcall(mop500_i2c_board_init);

static void __init mop500_i2c_init(struct device *parent)
{
	db8500_add_i2c0(parent, NULL);
	db8500_add_i2c1(parent, NULL);
	db8500_add_i2c2(parent, NULL);
	db8500_add_i2c3(parent, NULL);
}

static struct gpio_keys_button mop500_gpio_keys[] = {
	{
		.desc			= "SFH7741 Proximity Sensor",
		.type			= EV_SW,
		.code			= SW_FRONT_PROXIMITY,
		.active_low		= 0,
		.can_disable		= 1,
	}
};

static struct regulator *prox_regulator;
static int mop500_prox_activate(struct device *dev);
static void mop500_prox_deactivate(struct device *dev);

static struct gpio_keys_platform_data mop500_gpio_keys_data = {
	.buttons	= mop500_gpio_keys,
	.nbuttons	= ARRAY_SIZE(mop500_gpio_keys),
	.enable		= mop500_prox_activate,
	.disable	= mop500_prox_deactivate,
};

static struct platform_device mop500_gpio_keys_device = {
	.name	= "gpio-keys",
	.id	= 0,
	.dev	= {
		.platform_data	= &mop500_gpio_keys_data,
	},
};

static int mop500_prox_activate(struct device *dev)
{
	prox_regulator = regulator_get(&mop500_gpio_keys_device.dev,
						"vcc");
	if (IS_ERR(prox_regulator)) {
		dev_err(&mop500_gpio_keys_device.dev,
			"no regulator\n");
		return PTR_ERR(prox_regulator);
	}

	return regulator_enable(prox_regulator);
}

static void mop500_prox_deactivate(struct device *dev)
{
	regulator_disable(prox_regulator);
	regulator_put(prox_regulator);
}

/* add any platform devices here - TODO */
static struct platform_device *mop500_platform_devs[] __initdata = {
	&mop500_gpio_keys_device,
};

#ifdef CONFIG_STE_DMA40
static struct stedma40_chan_cfg ssp0_dma_cfg_rx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = DMA_DEV_TO_MEM,
	.dev_type = DB8500_DMA_DEV8_SSP0,
};

static struct stedma40_chan_cfg ssp0_dma_cfg_tx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = DMA_MEM_TO_DEV,
	.dev_type = DB8500_DMA_DEV8_SSP0,
};
#endif

struct pl022_ssp_controller ssp0_plat = {
	.bus_id = 0,
#ifdef CONFIG_STE_DMA40
	.enable_dma = 1,
	.dma_filter = stedma40_filter,
	.dma_rx_param = &ssp0_dma_cfg_rx,
	.dma_tx_param = &ssp0_dma_cfg_tx,
#else
	.enable_dma = 0,
#endif
	/* on this platform, gpio 31,142,144,214 &
	 * 224 are connected as chip selects
	 */
	.num_chipselect = 5,
};

static void __init mop500_spi_init(struct device *parent)
{
	db8500_add_ssp0(parent, &ssp0_plat);
}

#ifdef CONFIG_STE_DMA40
static struct stedma40_chan_cfg uart0_dma_cfg_rx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = DMA_DEV_TO_MEM,
	.dev_type = DB8500_DMA_DEV13_UART0,
};

static struct stedma40_chan_cfg uart0_dma_cfg_tx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = DMA_MEM_TO_DEV,
	.dev_type = DB8500_DMA_DEV13_UART0,
};

static struct stedma40_chan_cfg uart1_dma_cfg_rx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = DMA_DEV_TO_MEM,
	.dev_type = DB8500_DMA_DEV12_UART1,
};

static struct stedma40_chan_cfg uart1_dma_cfg_tx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = DMA_MEM_TO_DEV,
	.dev_type = DB8500_DMA_DEV12_UART1,
};

static struct stedma40_chan_cfg uart2_dma_cfg_rx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = DMA_DEV_TO_MEM,
	.dev_type = DB8500_DMA_DEV11_UART2,
};

static struct stedma40_chan_cfg uart2_dma_cfg_tx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = DMA_MEM_TO_DEV,
	.dev_type = DB8500_DMA_DEV11_UART2,
};
#endif

struct amba_pl011_data uart0_plat = {
#ifdef CONFIG_STE_DMA40
	.dma_filter = stedma40_filter,
	.dma_rx_param = &uart0_dma_cfg_rx,
	.dma_tx_param = &uart0_dma_cfg_tx,
#endif
};

struct amba_pl011_data uart1_plat = {
#ifdef CONFIG_STE_DMA40
	.dma_filter = stedma40_filter,
	.dma_rx_param = &uart1_dma_cfg_rx,
	.dma_tx_param = &uart1_dma_cfg_tx,
#endif
};

struct amba_pl011_data uart2_plat = {
#ifdef CONFIG_STE_DMA40
	.dma_filter = stedma40_filter,
	.dma_rx_param = &uart2_dma_cfg_rx,
	.dma_tx_param = &uart2_dma_cfg_tx,
#endif
};

static void __init mop500_uart_init(struct device *parent)
{
	db8500_add_uart0(parent, &uart0_plat);
	db8500_add_uart1(parent, &uart1_plat);
	db8500_add_uart2(parent, &uart2_plat);
}

static void __init mop500_init_machine(void)
{
	struct device *parent = NULL;
	int i;

	platform_device_register(&db8500_prcmu_device);
	mop500_gpio_keys[0].gpio = GPIO_PROX_SENSOR;

	mop500_pinmaps_init();
	parent = u8500_init_devices();

	for (i = 0; i < ARRAY_SIZE(mop500_platform_devs); i++)
		mop500_platform_devs[i]->dev.parent = parent;

	platform_add_devices(mop500_platform_devs,
			ARRAY_SIZE(mop500_platform_devs));

	mop500_i2c_init(parent);
	mop500_sdi_init(parent);
	mop500_spi_init(parent);
	mop500_uart_init(parent);

	/* This board has full regulator constraints */
	regulator_has_full_constraints();
}


static void __init snowball_init_machine(void)
{
	struct device *parent = NULL;

	platform_device_register(&db8500_prcmu_device);

	snowball_pinmaps_init();
	parent = u8500_init_devices();

	mop500_i2c_init(parent);
	snowball_sdi_init(parent);
	mop500_spi_init(parent);
	mop500_uart_init(parent);

	/* This board has full regulator constraints */
	regulator_has_full_constraints();
}

static void __init hrefv60_init_machine(void)
{
	struct device *parent = NULL;
	int i;

	platform_device_register(&db8500_prcmu_device);
	/*
	 * The HREFv60 board removed a GPIO expander and routed
	 * all these GPIO pins to the internal GPIO controller
	 * instead.
	 */
	mop500_gpio_keys[0].gpio = HREFV60_PROX_SENSE_GPIO;

	hrefv60_pinmaps_init();
	parent = u8500_init_devices();

	for (i = 0; i < ARRAY_SIZE(mop500_platform_devs); i++)
		mop500_platform_devs[i]->dev.parent = parent;

	platform_add_devices(mop500_platform_devs,
			ARRAY_SIZE(mop500_platform_devs));

	mop500_i2c_init(parent);
	hrefv60_sdi_init(parent);
	mop500_spi_init(parent);
	mop500_uart_init(parent);

	/* This board has full regulator constraints */
	regulator_has_full_constraints();
}

MACHINE_START(U8500, "ST-Ericsson MOP500 platform")
	/* Maintainer: Srinidhi Kasagar <srinidhi.kasagar@stericsson.com> */
	.atag_offset	= 0x100,
	.smp		= smp_ops(ux500_smp_ops),
	.map_io		= u8500_map_io,
	.init_irq	= ux500_init_irq,
	/* we re-use nomadik timer here */
	.init_time	= ux500_timer_init,
	.init_machine	= mop500_init_machine,
	.init_late	= ux500_init_late,
	.restart        = ux500_restart,
MACHINE_END

MACHINE_START(U8520, "ST-Ericsson U8520 Platform HREFP520")
	.atag_offset	= 0x100,
	.map_io		= u8500_map_io,
	.init_irq	= ux500_init_irq,
	.init_time	= ux500_timer_init,
	.init_machine	= mop500_init_machine,
	.init_late	= ux500_init_late,
	.restart        = ux500_restart,
MACHINE_END

MACHINE_START(HREFV60, "ST-Ericsson U8500 Platform HREFv60+")
	.atag_offset	= 0x100,
	.smp		= smp_ops(ux500_smp_ops),
	.map_io		= u8500_map_io,
	.init_irq	= ux500_init_irq,
	.init_time	= ux500_timer_init,
	.init_machine	= hrefv60_init_machine,
	.init_late	= ux500_init_late,
	.restart        = ux500_restart,
MACHINE_END

MACHINE_START(SNOWBALL, "Calao Systems Snowball platform")
	.atag_offset	= 0x100,
	.smp		= smp_ops(ux500_smp_ops),
	.map_io		= u8500_map_io,
	.init_irq	= ux500_init_irq,
	/* we re-use nomadik timer here */
	.init_time	= ux500_timer_init,
	.init_machine	= snowball_init_machine,
	.init_late	= NULL,
	.restart        = ux500_restart,
MACHINE_END
