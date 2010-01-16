/*
 *  linux/arch/arm/mach-integrator/clock.c
 *
 *  Copyright (C) 2004 ARM Limited.
 *  Written by Deep Blue Solutions Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>

#include <asm/hardware/icst525.h>
#include <asm/clkdev.h>
#include <mach/clkdev.h>

int clk_enable(struct clk *clk)
{
	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	return clk->rate;
}
EXPORT_SYMBOL(clk_get_rate);

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	struct icst_vco vco;
	vco = icst525_hz_to_vco(clk->params, rate);
	return icst525_hz(clk->params, vco);
}
EXPORT_SYMBOL(clk_round_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	int ret = -EIO;

	if (clk->setvco) {
		struct icst_vco vco;

		vco = icst525_hz_to_vco(clk->params, rate);
		clk->rate = icst525_hz(clk->params, vco);
		clk->setvco(clk, vco);
		ret = 0;
	}
	return ret;
}
EXPORT_SYMBOL(clk_set_rate);
