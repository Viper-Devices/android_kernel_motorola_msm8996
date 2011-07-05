/*
 * Copyright (c) 2010 Broadcom Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <linux/types.h>
#include <linux/sched.h>	/* request_irq() */
#include <linux/netdevice.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/errno.h>
#include <net/cfg80211.h>

#include <defs.h>
#include <brcmu_utils.h>
#include <brcmu_wifi.h>
#include "sdio_host.h"
#include "bcmsdbus.h"
#include "sdiovar.h"		/* to get msglevel bit values */
#include "dngl_stats.h"
#include "dhd.h"

#if !defined(SDIO_VENDOR_ID_BROADCOM)
#define SDIO_VENDOR_ID_BROADCOM		0x02d0
#endif				/* !defined(SDIO_VENDOR_ID_BROADCOM) */

#define SDIO_DEVICE_ID_BROADCOM_DEFAULT	0x0000

#if !defined(SDIO_DEVICE_ID_BROADCOM_4325_SDGWB)
#define SDIO_DEVICE_ID_BROADCOM_4325_SDGWB	0x0492	/* BCM94325SDGWB */
#endif		/* !defined(SDIO_DEVICE_ID_BROADCOM_4325_SDGWB) */
#if !defined(SDIO_DEVICE_ID_BROADCOM_4325)
#define SDIO_DEVICE_ID_BROADCOM_4325	0x0493
#endif		/* !defined(SDIO_DEVICE_ID_BROADCOM_4325) */
#if !defined(SDIO_DEVICE_ID_BROADCOM_4329)
#define SDIO_DEVICE_ID_BROADCOM_4329	0x4329
#endif		/* !defined(SDIO_DEVICE_ID_BROADCOM_4329) */
#if !defined(SDIO_DEVICE_ID_BROADCOM_4319)
#define SDIO_DEVICE_ID_BROADCOM_4319	0x4319
#endif		/* !defined(SDIO_DEVICE_ID_BROADCOM_4329) */

#include <bcmsdh_sdmmc.h>

#include "dhd_dbg.h"
#include "wl_cfg80211.h"

/* module param defaults */
static int clockoverride;

module_param(clockoverride, int, 0644);
MODULE_PARM_DESC(clockoverride, "SDIO card clock override");

struct brcmf_sdmmc_instance *gInstance;

struct device sdmmc_dev;

static int brcmf_ops_sdio_probe(struct sdio_func *func,
			      const struct sdio_device_id *id)
{
	int ret = 0;
	static struct sdio_func sdio_func_0;
	sd_trace(("sdio_probe: %s Enter\n", __func__));
	sd_trace(("sdio_probe: func->class=%x\n", func->class));
	sd_trace(("sdio_vendor: 0x%04x\n", func->vendor));
	sd_trace(("sdio_device: 0x%04x\n", func->device));
	sd_trace(("Function#: 0x%04x\n", func->num));

	if (func->num == 1) {
		sdio_func_0.num = 0;
		sdio_func_0.card = func->card;
		gInstance->func[0] = &sdio_func_0;
		if (func->device == 0x4) {	/* 4318 */
			gInstance->func[2] = NULL;
			sd_trace(("NIC found, calling brcmf_sdio_probe...\n"));
			ret = brcmf_sdio_probe(&sdmmc_dev);
		}
	}

	gInstance->func[func->num] = func;

	if (func->num == 2) {
		wl_cfg80211_sdio_func(func);
		sd_trace(("F2 found, calling brcmf_sdio_probe...\n"));
		ret = brcmf_sdio_probe(&sdmmc_dev);
	}

	return ret;
}

static void brcmf_ops_sdio_remove(struct sdio_func *func)
{
	sd_trace(("%s Enter\n", __func__));
	sd_info(("func->class=%x\n", func->class));
	sd_info(("sdio_vendor: 0x%04x\n", func->vendor));
	sd_info(("sdio_device: 0x%04x\n", func->device));
	sd_info(("Function#: 0x%04x\n", func->num));

	if (func->num == 2) {
		sd_trace(("F2 found, calling brcmf_sdio_remove...\n"));
		brcmf_sdio_remove(&sdmmc_dev);
	}
}

/* devices we support, null terminated */
static const struct sdio_device_id brcmf_sdmmc_ids[] = {
	{SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_DEFAULT)},
	{SDIO_DEVICE
	 (SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_4325_SDGWB)},
	{SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_4325)},
	{SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_4329)},
	{SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_4319)},
	{ /* end: all zeroes */ },
};

MODULE_DEVICE_TABLE(sdio, brcmf_sdmmc_ids);

#ifdef CONFIG_PM
static int brcmf_sdio_suspend(struct device *dev)
{
	mmc_pm_flag_t sdio_flags;
	int ret = 0;

	sd_trace(("%s\n", __func__));

	sdio_flags = sdio_get_host_pm_caps(gInstance->func[1]);
	if (!(sdio_flags & MMC_PM_KEEP_POWER)) {
		sd_err(("Host can't keep power while suspended\n"));
		return -EINVAL;
	}

	ret = sdio_set_host_pm_flags(gInstance->func[1], MMC_PM_KEEP_POWER);
	if (ret) {
		sd_err(("Failed to set pm_flags\n"));
		return ret;
	}

	brcmf_sdio_wdtmr_enable(false);

	return ret;
}

static int brcmf_sdio_resume(struct device *dev)
{
	brcmf_sdio_wdtmr_enable(true);
	return 0;
}

static const struct dev_pm_ops brcmf_sdio_pm_ops = {
	.suspend	= brcmf_sdio_suspend,
	.resume		= brcmf_sdio_resume,
};
#endif		/* CONFIG_PM */

static struct sdio_driver brcmf_sdmmc_driver = {
	.probe = brcmf_ops_sdio_probe,
	.remove = brcmf_ops_sdio_remove,
	.name = "brcmfmac",
	.id_table = brcmf_sdmmc_ids,
#ifdef CONFIG_PM
	.drv = {
		.pm = &brcmf_sdio_pm_ops,
	},
#endif		/* CONFIG_PM */
};

struct sdos_info {
	struct sdioh_info *sd;
	spinlock_t lock;
};

int brcmf_sdioh_osinit(struct sdioh_info *sd)
{
	struct sdos_info *sdos;

	sdos = kmalloc(sizeof(struct sdos_info), GFP_ATOMIC);
	sd->sdos_info = (void *)sdos;
	if (sdos == NULL)
		return -ENOMEM;

	sdos->sd = sd;
	spin_lock_init(&sdos->lock);
	return 0;
}

void brcmf_sdioh_osfree(struct sdioh_info *sd)
{
	struct sdos_info *sdos;
	ASSERT(sd && sd->sdos_info);

	sdos = (struct sdos_info *)sd->sdos_info;
	kfree(sdos);
}

/* Interrupt enable/disable */
int brcmf_sdioh_interrupt_set(struct sdioh_info *sd, bool enable)
{
	unsigned long flags;
	struct sdos_info *sdos;

	sd_trace(("%s: %s\n", __func__, enable ? "Enabling" : "Disabling"));

	sdos = (struct sdos_info *)sd->sdos_info;
	ASSERT(sdos);

	if (enable && !(sd->intr_handler && sd->intr_handler_arg)) {
		sd_err(("%s: no handler registered, will not enable\n",
			__func__));
		return -EINVAL;
	}

	/* Ensure atomicity for enable/disable calls */
	spin_lock_irqsave(&sdos->lock, flags);

	sd->client_intr_enabled = enable;
	if (enable)
		brcmf_sdioh_dev_intr_on(sd);
	else
		brcmf_sdioh_dev_intr_off(sd);

	spin_unlock_irqrestore(&sdos->lock, flags);

	return 0;
}

/*
 * module init
*/
int brcmf_sdio_function_init(void)
{
	int error = 0;
	sd_trace(("brcmf_sdio_function_init: %s Enter\n", __func__));

	gInstance = kzalloc(sizeof(struct brcmf_sdmmc_instance), GFP_KERNEL);
	if (!gInstance)
		return -ENOMEM;

	memset(&sdmmc_dev, 0, sizeof(sdmmc_dev));
	error = sdio_register_driver(&brcmf_sdmmc_driver);

	return error;
}

/*
 * module cleanup
*/
void brcmf_sdio_function_cleanup(void)
{
	sd_trace(("%s Enter\n", __func__));

	sdio_unregister_driver(&brcmf_sdmmc_driver);

	kfree(gInstance);
}
