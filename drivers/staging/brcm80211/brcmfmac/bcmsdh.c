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
/* ****************** SDIOCARD Interface Functions ***************************/

#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/pci_ids.h>
#include <linux/sched.h>

#include <defs.h>
#include <brcm_hw_ids.h>
#include <brcmu_utils.h>
#include <brcmu_wifi.h>
#include <soc.h>
#include "bcmsdbus.h"		/* common SDIO/controller interface */
#include "sbsdio.h"		/* BRCM sdio device core */
#include "dngl_stats.h"
#include "dhd.h"

#define SDIOH_API_ACCESS_RETRY_LIMIT	2
const uint brcmf_sdio_msglevel = BRCMF_SD_ERROR_VAL;

struct brcmf_sdio {
	bool init_success;	/* underlying driver successfully attached */
	void *sdioh;		/* handler for sdioh */
	u32 vendevid;	/* Target Vendor and Device ID on SD bus */
	bool regfail;		/* Save status of last
				 reg_read/reg_write call */
	u32 sbwad;		/* Save backplane window address */
};
/* local copy of bcm sd handler */
static struct brcmf_sdio *l_card;

struct brcmf_sdio *brcmf_sdcard_attach(void *cfghdl, void **regsva, uint irq)
{
	struct brcmf_sdio *card;

	card = kzalloc(sizeof(struct brcmf_sdio), GFP_ATOMIC);
	if (card == NULL) {
		BRCMF_SD_ERROR(("sdcard_attach: out of memory"));
		return NULL;
	}

	/* save the handler locally */
	l_card = card;

	card->sdioh = brcmf_sdioh_attach(cfghdl, irq);
	if (!card->sdioh) {
		brcmf_sdcard_detach(card);
		return NULL;
	}

	card->init_success = true;

	*regsva = (u32 *) SI_ENUM_BASE;

	/* Report the BAR, to fix if needed */
	card->sbwad = SI_ENUM_BASE;
	return card;
}

int brcmf_sdcard_detach(void *sdh)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;

	if (card != NULL) {
		if (card->sdioh) {
			brcmf_sdioh_detach(card->sdioh);
			card->sdioh = NULL;
		}
		kfree(card);
	}

	l_card = NULL;
	return 0;
}

int
brcmf_sdcard_iovar_op(void *sdh, const char *name,
		void *params, int plen, void *arg, int len, bool set)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	return brcmf_sdioh_iovar_op(card->sdioh, name, params, plen, arg,
				    len, set);
}

bool brcmf_sdcard_intr_query(void *sdh)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	int status;
	bool on;

	ASSERT(card);
	status = brcmf_sdioh_interrupt_query(card->sdioh, &on);
	if (status == 0)
		return false;
	else
		return on;
}

int brcmf_sdcard_intr_enable(void *sdh)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	ASSERT(card);

	return brcmf_sdioh_interrupt_set(card->sdioh, true);
}

int brcmf_sdcard_intr_disable(void *sdh)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	ASSERT(card);

	return brcmf_sdioh_interrupt_set(card->sdioh, false);
}

int brcmf_sdcard_intr_reg(void *sdh, brcmf_sdiocard_cb_fn_t fn, void *argh)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	ASSERT(card);

	return brcmf_sdioh_interrupt_register(card->sdioh, fn, argh);
}

int brcmf_sdcard_intr_dereg(void *sdh)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	ASSERT(card);

	return brcmf_sdioh_interrupt_deregister(card->sdioh);
}

#if defined(BCMDBG)
bool brcmf_sdcard_intr_pending(void *sdh)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;

	ASSERT(sdh);
	return brcmf_sdioh_interrupt_pending(card->sdioh);
}
#endif

int brcmf_sdcard_devremove_reg(void *sdh, brcmf_sdiocard_cb_fn_t fn, void *argh)
{
	ASSERT(sdh);

	/* don't support yet */
	return -ENOTSUPP;
}

u8 brcmf_sdcard_cfg_read(void *sdh, uint fnc_num, u32 addr, int *err)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	int status;
#ifdef SDIOH_API_ACCESS_RETRY_LIMIT
	s32 retry = 0;
#endif
	u8 data = 0;

	if (!card)
		card = l_card;

	ASSERT(card->init_success);

#ifdef SDIOH_API_ACCESS_RETRY_LIMIT
	do {
		if (retry)	/* wait for 1 ms till bus get settled down */
			udelay(1000);
#endif
		status =
		    brcmf_sdioh_cfg_read(card->sdioh, fnc_num, addr,
				   (u8 *) &data);
#ifdef SDIOH_API_ACCESS_RETRY_LIMIT
	} while (status != 0
		 && (retry++ < SDIOH_API_ACCESS_RETRY_LIMIT));
#endif
	if (err)
		*err = status;

	BRCMF_SD_INFO(("%s:fun = %d, addr = 0x%x, u8data = 0x%x\n",
		     __func__, fnc_num, addr, data));

	return data;
}

void
brcmf_sdcard_cfg_write(void *sdh, uint fnc_num, u32 addr, u8 data, int *err)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	int status;
#ifdef SDIOH_API_ACCESS_RETRY_LIMIT
	s32 retry = 0;
#endif

	if (!card)
		card = l_card;

	ASSERT(card->init_success);

#ifdef SDIOH_API_ACCESS_RETRY_LIMIT
	do {
		if (retry)	/* wait for 1 ms till bus get settled down */
			udelay(1000);
#endif
		status =
		    brcmf_sdioh_cfg_write(card->sdioh, fnc_num, addr,
				    (u8 *) &data);
#ifdef SDIOH_API_ACCESS_RETRY_LIMIT
	} while (status != 0
		 && (retry++ < SDIOH_API_ACCESS_RETRY_LIMIT));
#endif
	if (err)
		*err = status;

	BRCMF_SD_INFO(("%s:fun = %d, addr = 0x%x, u8data = 0x%x\n",
		     __func__, fnc_num, addr, data));
}

u32 brcmf_sdcard_cfg_read_word(void *sdh, uint fnc_num, u32 addr, int *err)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	int status;
	u32 data = 0;

	if (!card)
		card = l_card;

	ASSERT(card->init_success);

	status = brcmf_sdioh_request_word(card->sdioh, SDIOH_CMD_TYPE_NORMAL,
		SDIOH_READ, fnc_num, addr, &data, 4);

	if (err)
		*err = status;

	BRCMF_SD_INFO(("%s:fun = %d, addr = 0x%x, u32data = 0x%x\n",
		     __func__, fnc_num, addr, data));

	return data;
}

void
brcmf_sdcard_cfg_write_word(void *sdh, uint fnc_num, u32 addr, u32 data,
		      int *err)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	int status;

	if (!card)
		card = l_card;

	ASSERT(card->init_success);

	status =
	    brcmf_sdioh_request_word(card->sdioh, SDIOH_CMD_TYPE_NORMAL,
			       SDIOH_WRITE, fnc_num, addr, &data, 4);

	if (err)
		*err = status;

	BRCMF_SD_INFO(("%s:fun = %d, addr = 0x%x, u32data = 0x%x\n",
		     __func__, fnc_num, addr, data));
}

int brcmf_sdcard_cis_read(void *sdh, uint func, u8 * cis, uint length)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	int status;

	u8 *tmp_buf, *tmp_ptr;
	u8 *ptr;
	bool ascii = func & ~0xf;
	func &= 0x7;

	if (!card)
		card = l_card;

	ASSERT(card->init_success);
	ASSERT(cis);
	ASSERT(length <= SBSDIO_CIS_SIZE_LIMIT);

	status = brcmf_sdioh_cis_read(card->sdioh, func, cis, length);

	if (ascii) {
		/* Move binary bits to tmp and format them
			 into the provided buffer. */
		tmp_buf = kmalloc(length, GFP_ATOMIC);
		if (tmp_buf == NULL) {
			BRCMF_SD_ERROR(("%s: out of memory\n", __func__));
			return -ENOMEM;
		}
		memcpy(tmp_buf, cis, length);
		for (tmp_ptr = tmp_buf, ptr = cis; ptr < (cis + length - 4);
		     tmp_ptr++) {
			ptr += sprintf((char *)ptr, "%.2x ", *tmp_ptr & 0xff);
			if ((((tmp_ptr - tmp_buf) + 1) & 0xf) == 0)
				ptr += sprintf((char *)ptr, "\n");
		}
		kfree(tmp_buf);
	}

	return status;
}

static int brcmf_sdcard_set_sbaddr_window(void *sdh, u32 address)
{
	int err = 0;
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	brcmf_sdcard_cfg_write(card, SDIO_FUNC_1, SBSDIO_FUNC1_SBADDRLOW,
			 (address >> 8) & SBSDIO_SBADDRLOW_MASK, &err);
	if (!err)
		brcmf_sdcard_cfg_write(card, SDIO_FUNC_1,
				       SBSDIO_FUNC1_SBADDRMID,
				       (address >> 16) & SBSDIO_SBADDRMID_MASK,
				       &err);
	if (!err)
		brcmf_sdcard_cfg_write(card, SDIO_FUNC_1,
				       SBSDIO_FUNC1_SBADDRHIGH,
				       (address >> 24) & SBSDIO_SBADDRHIGH_MASK,
				       &err);

	return err;
}

u32 brcmf_sdcard_reg_read(void *sdh, u32 addr, uint size)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	int status;
	u32 word = 0;
	uint bar0 = addr & ~SBSDIO_SB_OFT_ADDR_MASK;

	BRCMF_SD_INFO(("%s:fun = 1, addr = 0x%x, ", __func__, addr));

	if (!card)
		card = l_card;

	ASSERT(card->init_success);

	if (bar0 != card->sbwad) {
		if (brcmf_sdcard_set_sbaddr_window(card, bar0))
			return 0xFFFFFFFF;

		card->sbwad = bar0;
	}

	addr &= SBSDIO_SB_OFT_ADDR_MASK;
	if (size == 4)
		addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;

	status = brcmf_sdioh_request_word(card->sdioh, SDIOH_CMD_TYPE_NORMAL,
				    SDIOH_READ, SDIO_FUNC_1, addr, &word, size);

	card->regfail = (status != 0);

	BRCMF_SD_INFO(("u32data = 0x%x\n", word));

	/* if ok, return appropriately masked word */
	if (status == 0) {
		switch (size) {
		case sizeof(u8):
			return word & 0xff;
		case sizeof(u16):
			return word & 0xffff;
		case sizeof(u32):
			return word;
		default:
			card->regfail = true;

		}
	}

	/* otherwise, bad sdio access or invalid size */
	BRCMF_SD_ERROR(("%s: error reading addr 0x%04x size %d\n", __func__,
		      addr, size));
	return 0xFFFFFFFF;
}

u32 brcmf_sdcard_reg_write(void *sdh, u32 addr, uint size, u32 data)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	int status;
	uint bar0 = addr & ~SBSDIO_SB_OFT_ADDR_MASK;
	int err = 0;

	BRCMF_SD_INFO(("%s:fun = 1, addr = 0x%x, uint%ddata = 0x%x\n",
		     __func__, addr, size * 8, data));

	if (!card)
		card = l_card;

	ASSERT(card->init_success);

	if (bar0 != card->sbwad) {
		err = brcmf_sdcard_set_sbaddr_window(card, bar0);
		if (err)
			return err;

		card->sbwad = bar0;
	}

	addr &= SBSDIO_SB_OFT_ADDR_MASK;
	if (size == 4)
		addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;
	status =
	    brcmf_sdioh_request_word(card->sdioh, SDIOH_CMD_TYPE_NORMAL,
			       SDIOH_WRITE, SDIO_FUNC_1, addr, &data, size);
	card->regfail = (status != 0);

	if (status == 0)
		return 0;

	BRCMF_SD_ERROR(("%s: error writing 0x%08x to addr 0x%04x size %d\n",
		      __func__, data, addr, size));
	return 0xFFFFFFFF;
}

bool brcmf_sdcard_regfail(void *sdh)
{
	return ((struct brcmf_sdio *) sdh)->regfail;
}

int
brcmf_sdcard_recv_buf(struct brcmf_sdio *card, u32 addr, uint fn, uint flags,
		u8 *buf, uint nbytes, struct sk_buff *pkt,
		brcmf_sdio_cmplt_fn_t complete, void *handle)
{
	int status;
	uint incr_fix;
	uint width;
	uint bar0 = addr & ~SBSDIO_SB_OFT_ADDR_MASK;
	int err = 0;

	ASSERT(card);
	ASSERT(card->init_success);

	BRCMF_SD_INFO(("%s:fun = %d, addr = 0x%x, size = %d\n",
		     __func__, fn, addr, nbytes));

	/* Async not implemented yet */
	ASSERT(!(flags & SDIO_REQ_ASYNC));
	if (flags & SDIO_REQ_ASYNC)
		return -ENOTSUPP;

	if (bar0 != card->sbwad) {
		err = brcmf_sdcard_set_sbaddr_window(card, bar0);
		if (err)
			return err;

		card->sbwad = bar0;
	}

	addr &= SBSDIO_SB_OFT_ADDR_MASK;

	incr_fix = (flags & SDIO_REQ_FIXED) ? SDIOH_DATA_FIX : SDIOH_DATA_INC;
	width = (flags & SDIO_REQ_4BYTE) ? 4 : 2;
	if (width == 4)
		addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;

	status = brcmf_sdioh_request_buffer(card->sdioh, SDIOH_DATA_PIO,
		incr_fix, SDIOH_READ, fn, addr, width, nbytes, buf, pkt);

	return status;
}

int
brcmf_sdcard_send_buf(void *sdh, u32 addr, uint fn, uint flags,
		u8 *buf, uint nbytes, void *pkt,
		brcmf_sdio_cmplt_fn_t complete, void *handle)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	uint incr_fix;
	uint width;
	uint bar0 = addr & ~SBSDIO_SB_OFT_ADDR_MASK;
	int err = 0;

	ASSERT(card);
	ASSERT(card->init_success);

	BRCMF_SD_INFO(("%s:fun = %d, addr = 0x%x, size = %d\n",
		     __func__, fn, addr, nbytes));

	/* Async not implemented yet */
	ASSERT(!(flags & SDIO_REQ_ASYNC));
	if (flags & SDIO_REQ_ASYNC)
		return -ENOTSUPP;

	if (bar0 != card->sbwad) {
		err = brcmf_sdcard_set_sbaddr_window(card, bar0);
		if (err)
			return err;

		card->sbwad = bar0;
	}

	addr &= SBSDIO_SB_OFT_ADDR_MASK;

	incr_fix = (flags & SDIO_REQ_FIXED) ? SDIOH_DATA_FIX : SDIOH_DATA_INC;
	width = (flags & SDIO_REQ_4BYTE) ? 4 : 2;
	if (width == 4)
		addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;

	return brcmf_sdioh_request_buffer(card->sdioh, SDIOH_DATA_PIO,
		incr_fix, SDIOH_WRITE, fn, addr, width, nbytes, buf, pkt);
}

int brcmf_sdcard_rwdata(void *sdh, uint rw, u32 addr, u8 *buf, uint nbytes)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;

	ASSERT(card);
	ASSERT(card->init_success);
	ASSERT((addr & SBSDIO_SBWINDOW_MASK) == 0);

	addr &= SBSDIO_SB_OFT_ADDR_MASK;
	addr |= SBSDIO_SB_ACCESS_2_4B_FLAG;

	return brcmf_sdioh_request_buffer(card->sdioh, SDIOH_DATA_PIO,
		SDIOH_DATA_INC, (rw ? SDIOH_WRITE : SDIOH_READ), SDIO_FUNC_1,
		addr, 4, nbytes, buf, NULL);
}

int brcmf_sdcard_abort(void *sdh, uint fn)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;

	return brcmf_sdioh_abort(card->sdioh, fn);
}

int brcmf_sdcard_start(void *sdh, int stage)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;

	return brcmf_sdioh_start(card->sdioh, stage);
}

int brcmf_sdcard_stop(void *sdh)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;

	return brcmf_sdioh_stop(card->sdioh);
}

int brcmf_sdcard_query_device(void *sdh)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;
	card->vendevid = (PCI_VENDOR_ID_BROADCOM << 16) | 0;
	return card->vendevid;
}

uint brcmf_sdcard_query_iofnum(void *sdh)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;

	if (!card)
		card = l_card;

	return brcmf_sdioh_query_iofnum(card->sdioh);
}

int brcmf_sdcard_reset(struct brcmf_sdio *sdh)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;

	return brcmf_sdioh_reset(card->sdioh);
}

void *brcmf_sdcard_get_sdioh(struct brcmf_sdio *sdh)
{
	ASSERT(sdh);
	return sdh->sdioh;
}

/* Function to pass device-status bits to DHD. */
u32 brcmf_sdcard_get_dstatus(void *sdh)
{
	return 0;
}

u32 brcmf_sdcard_cur_sbwad(void *sdh)
{
	struct brcmf_sdio *card = (struct brcmf_sdio *) sdh;

	if (!card)
		card = l_card;

	return card->sbwad;
}

void brcmf_sdcard_chipinfo(void *sdh, u32 chip, u32 chiprev)
{
	return;
}
