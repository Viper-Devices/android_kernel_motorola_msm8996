/*
 * SuperH HSPI bus driver
 *
 * Copyright (C) 2011  Kuninori Morimoto
 *
 * Based on spi-sh.c:
 * Based on pxa2xx_spi.c:
 * Copyright (C) 2011 Renesas Solutions Corp.
 * Copyright (C) 2005 Stephen Street / StreetFire Sound Labs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/io.h>
#include <linux/spi/spi.h>
#include <linux/spi/sh_hspi.h>

#define SPCR	0x00
#define SPSR	0x04
#define SPSCR	0x08
#define SPTBR	0x0C
#define SPRBR	0x10
#define SPCR2	0x14

/* SPSR */
#define RXFL	(1 << 2)

#define hspi2info(h)	(h->dev->platform_data)

struct hspi_priv {
	void __iomem *addr;
	struct spi_master *master;
	struct device *dev;
};

/*
 *		basic function
 */
static void hspi_write(struct hspi_priv *hspi, int reg, u32 val)
{
	iowrite32(val, hspi->addr + reg);
}

static u32 hspi_read(struct hspi_priv *hspi, int reg)
{
	return ioread32(hspi->addr + reg);
}

/*
 *		transfer function
 */
static int hspi_status_check_timeout(struct hspi_priv *hspi, u32 mask, u32 val)
{
	int t = 256;

	while (t--) {
		if ((mask & hspi_read(hspi, SPSR)) == val)
			return 0;

		msleep(20);
	}

	dev_err(hspi->dev, "timeout\n");
	return -ETIMEDOUT;
}

static int hspi_push(struct hspi_priv *hspi, struct spi_message *msg,
		     struct spi_transfer *t)
{
	int i, ret;
	u8 *data = (u8 *)t->tx_buf;

	/*
	 * FIXME
	 * very simple, but polling transfer
	 */
	for (i = 0; i < t->len; i++) {
		/* wait remains */
		ret = hspi_status_check_timeout(hspi, 0x1, 0x0);
		if (ret < 0)
			return ret;

		hspi_write(hspi, SPTBR, (u32)data[i]);

		/* wait recive */
		ret = hspi_status_check_timeout(hspi, 0x4, 0x4);
		if (ret < 0)
			return ret;

		/* dummy read */
		hspi_read(hspi, SPRBR);
	}

	return 0;
}

static int hspi_pop(struct hspi_priv *hspi, struct spi_message *msg,
		    struct spi_transfer *t)
{
	int i, ret;
	u8 *data = (u8 *)t->rx_buf;

	/*
	 * FIXME
	 * very simple, but polling receive
	 */
	for (i = 0; i < t->len; i++) {
		/* wait remains */
		ret = hspi_status_check_timeout(hspi, 0x1, 0);
		if (ret < 0)
			return ret;

		/* dummy write */
		hspi_write(hspi, SPTBR, 0x0);

		/* wait recive */
		ret = hspi_status_check_timeout(hspi, 0x4, 0x4);
		if (ret < 0)
			return ret;

		data[i] = (u8)hspi_read(hspi, SPRBR);
	}

	return 0;
}

/*
 *		spi master function
 */
static int hspi_prepare_transfer(struct spi_master *master)
{
	struct hspi_priv *hspi = spi_master_get_devdata(master);

	pm_runtime_get_sync(hspi->dev);
	return 0;
}

static int hspi_unprepare_transfer(struct spi_master *master)
{
	struct hspi_priv *hspi = spi_master_get_devdata(master);

	pm_runtime_put_sync(hspi->dev);
	return 0;
}

static int hspi_transfer_one_message(struct spi_master *master,
				     struct spi_message *msg)
{
	struct hspi_priv *hspi = spi_master_get_devdata(master);
	struct spi_transfer *t;
	int ret;

	dev_dbg(hspi->dev, "%s\n", __func__);

	ret = 0;
	list_for_each_entry(t, &msg->transfers, transfer_list) {
		if (t->tx_buf) {
			ret = hspi_push(hspi, msg, t);
			if (ret < 0)
				goto error;
		}
		if (t->rx_buf) {
			ret = hspi_pop(hspi, msg, t);
			if (ret < 0)
				goto error;
		}
		msg->actual_length += t->len;
	}
error:

	msg->status = ret;
	spi_finalize_current_message(master);

	return ret;
}

static int hspi_setup(struct spi_device *spi)
{
	struct hspi_priv *hspi = spi_master_get_devdata(spi->master);
	struct device *dev = hspi->dev;
	struct sh_hspi_info *info = hspi2info(hspi);
	u32 data;

	if (8 != spi->bits_per_word) {
		dev_err(dev, "bits_per_word should be 8\n");
		return -EIO;
	}

	/* setup first of all in under pm_runtime */
	data = SH_HSPI_CLK_DIVC(info->flags);

	if (info->flags & SH_HSPI_FBS)
		data |= 1 << 7;
	if (info->flags & SH_HSPI_CLKP_HIGH)
		data |= 1 << 6;
	if (info->flags & SH_HSPI_IDIV_DIV128)
		data |= 1 << 5;

	hspi_write(hspi, SPCR, data);
	hspi_write(hspi, SPSR, 0x0);
	hspi_write(hspi, SPSCR, 0x1);	/* master mode */

	dev_dbg(dev, "%s setup\n", spi->modalias);

	return 0;
}

static void hspi_cleanup(struct spi_device *spi)
{
	struct hspi_priv *hspi = spi_master_get_devdata(spi->master);
	struct device *dev = hspi->dev;

	dev_dbg(dev, "%s cleanup\n", spi->modalias);
}

static int __devinit hspi_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct spi_master *master;
	struct hspi_priv *hspi;
	int ret;

	/* get base addr */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "invalid resource\n");
		return -EINVAL;
	}

	master = spi_alloc_master(&pdev->dev, sizeof(*hspi));
	if (!master) {
		dev_err(&pdev->dev, "spi_alloc_master error.\n");
		return -ENOMEM;
	}

	hspi = spi_master_get_devdata(master);
	dev_set_drvdata(&pdev->dev, hspi);

	/* init hspi */
	hspi->master	= master;
	hspi->dev	= &pdev->dev;
	hspi->addr	= devm_ioremap(hspi->dev,
				       res->start, resource_size(res));
	if (!hspi->addr) {
		dev_err(&pdev->dev, "ioremap error.\n");
		ret = -ENOMEM;
		goto error1;
	}

	master->num_chipselect	= 1;
	master->bus_num		= pdev->id;
	master->setup		= hspi_setup;
	master->cleanup		= hspi_cleanup;
	master->mode_bits	= SPI_CPOL | SPI_CPHA;
	master->prepare_transfer_hardware	= hspi_prepare_transfer;
	master->transfer_one_message		= hspi_transfer_one_message;
	master->unprepare_transfer_hardware	= hspi_unprepare_transfer;
	ret = spi_register_master(master);
	if (ret < 0) {
		dev_err(&pdev->dev, "spi_register_master error.\n");
		goto error2;
	}

	pm_runtime_enable(&pdev->dev);

	dev_info(&pdev->dev, "probed\n");

	return 0;

 error2:
	devm_iounmap(hspi->dev, hspi->addr);
 error1:
	spi_master_put(master);

	return ret;
}

static int __devexit hspi_remove(struct platform_device *pdev)
{
	struct hspi_priv *hspi = dev_get_drvdata(&pdev->dev);

	pm_runtime_disable(&pdev->dev);

	spi_unregister_master(hspi->master);
	devm_iounmap(hspi->dev, hspi->addr);

	return 0;
}

static struct platform_driver hspi_driver = {
	.probe = hspi_probe,
	.remove = __devexit_p(hspi_remove),
	.driver = {
		.name = "sh-hspi",
		.owner = THIS_MODULE,
	},
};
module_platform_driver(hspi_driver);

MODULE_DESCRIPTION("SuperH HSPI bus driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>");
MODULE_ALIAS("platform:sh_spi");
