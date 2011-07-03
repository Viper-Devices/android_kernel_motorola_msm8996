/******************************************************************************
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2007 - 2011 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110,
 * USA
 *
 * The full GNU General Public License is included in this distribution
 * in the file called LICENSE.GPL.
 *
 * Contact Information:
 *  Intel Linux Wireless <ilw@linux.intel.com>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *
 * BSD LICENSE
 *
 * Copyright(c) 2005 - 2011 Intel Corporation. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
#include "iwl-dev.h"
#include "iwl-trans.h"
#include "iwl-core.h"
#include "iwl-helpers.h"
/*TODO remove uneeded includes when the transport layer tx_free will be here */
#include "iwl-agn.h"
#include "iwl-core.h"

static int iwl_trans_rx_alloc(struct iwl_priv *priv)
{
	struct iwl_rx_queue *rxq = &priv->rxq;
	struct device *dev = priv->bus.dev;

	memset(&priv->rxq, 0, sizeof(priv->rxq));

	spin_lock_init(&rxq->lock);
	INIT_LIST_HEAD(&rxq->rx_free);
	INIT_LIST_HEAD(&rxq->rx_used);

	if (WARN_ON(rxq->bd || rxq->rb_stts))
		return -EINVAL;

	/* Allocate the circular buffer of Read Buffer Descriptors (RBDs) */
	rxq->bd = dma_alloc_coherent(dev, sizeof(__le32) * RX_QUEUE_SIZE,
				     &rxq->bd_dma, GFP_KERNEL);
	if (!rxq->bd)
		goto err_bd;
	memset(rxq->bd, 0, sizeof(__le32) * RX_QUEUE_SIZE);

	/*Allocate the driver's pointer to receive buffer status */
	rxq->rb_stts = dma_alloc_coherent(dev, sizeof(*rxq->rb_stts),
					  &rxq->rb_stts_dma, GFP_KERNEL);
	if (!rxq->rb_stts)
		goto err_rb_stts;
	memset(rxq->rb_stts, 0, sizeof(*rxq->rb_stts));

	return 0;

err_rb_stts:
	dma_free_coherent(dev, sizeof(__le32) * RX_QUEUE_SIZE,
			rxq->bd, rxq->bd_dma);
	memset(&rxq->bd_dma, 0, sizeof(rxq->bd_dma));
	rxq->bd = NULL;
err_bd:
	return -ENOMEM;
}

static void iwl_trans_rxq_free_rx_bufs(struct iwl_priv *priv)
{
	struct iwl_rx_queue *rxq = &priv->rxq;
	int i;

	/* Fill the rx_used queue with _all_ of the Rx buffers */
	for (i = 0; i < RX_FREE_BUFFERS + RX_QUEUE_SIZE; i++) {
		/* In the reset function, these buffers may have been allocated
		 * to an SKB, so we need to unmap and free potential storage */
		if (rxq->pool[i].page != NULL) {
			dma_unmap_page(priv->bus.dev, rxq->pool[i].page_dma,
				PAGE_SIZE << priv->hw_params.rx_page_order,
				DMA_FROM_DEVICE);
			__iwl_free_pages(priv, rxq->pool[i].page);
			rxq->pool[i].page = NULL;
		}
		list_add_tail(&rxq->pool[i].list, &rxq->rx_used);
	}
}

static int iwl_trans_rx_init(struct iwl_priv *priv)
{
	struct iwl_rx_queue *rxq = &priv->rxq;
	int i, err;
	unsigned long flags;

	if (!rxq->bd) {
		err = iwl_trans_rx_alloc(priv);
		if (err)
			return err;
	}

	spin_lock_irqsave(&rxq->lock, flags);
	INIT_LIST_HEAD(&rxq->rx_free);
	INIT_LIST_HEAD(&rxq->rx_used);

	iwl_trans_rxq_free_rx_bufs(priv);

	for (i = 0; i < RX_QUEUE_SIZE; i++)
		rxq->queue[i] = NULL;

	/* Set us so that we have processed and used all buffers, but have
	 * not restocked the Rx queue with fresh buffers */
	rxq->read = rxq->write = 0;
	rxq->write_actual = 0;
	rxq->free_count = 0;
	spin_unlock_irqrestore(&rxq->lock, flags);

	return 0;
}

static void iwl_trans_rx_free(struct iwl_priv *priv)
{
	struct iwl_rx_queue *rxq = &priv->rxq;
	unsigned long flags;

	/*if rxq->bd is NULL, it means that nothing has been allocated,
	 * exit now */
	if (!rxq->bd) {
		IWL_DEBUG_INFO(priv, "Free NULL rx context\n");
		return;
	}

	spin_lock_irqsave(&rxq->lock, flags);
	iwl_trans_rxq_free_rx_bufs(priv);
	spin_unlock_irqrestore(&rxq->lock, flags);

	dma_free_coherent(priv->bus.dev, sizeof(__le32) * RX_QUEUE_SIZE,
			  rxq->bd, rxq->bd_dma);
	memset(&rxq->bd_dma, 0, sizeof(rxq->bd_dma));
	rxq->bd = NULL;

	if (rxq->rb_stts)
		dma_free_coherent(priv->bus.dev,
				  sizeof(struct iwl_rb_status),
				  rxq->rb_stts, rxq->rb_stts_dma);
	else
		IWL_DEBUG_INFO(priv, "Free rxq->rb_stts which is NULL\n");
	memset(&rxq->rb_stts_dma, 0, sizeof(rxq->rb_stts_dma));
	rxq->rb_stts = NULL;
}

static int iwl_trans_rx_stop(struct iwl_priv *priv)
{

	/* stop Rx DMA */
	iwl_write_direct32(priv, FH_MEM_RCSR_CHNL0_CONFIG_REG, 0);
	return iwl_poll_direct_bit(priv, FH_MEM_RSSR_RX_STATUS_REG,
			    FH_RSSR_CHNL0_RX_STATUS_CHNL_IDLE, 1000);
}

static inline int iwlagn_alloc_dma_ptr(struct iwl_priv *priv,
				    struct iwl_dma_ptr *ptr, size_t size)
{
	if (WARN_ON(ptr->addr))
		return -EINVAL;

	ptr->addr = dma_alloc_coherent(priv->bus.dev, size,
				       &ptr->dma, GFP_KERNEL);
	if (!ptr->addr)
		return -ENOMEM;
	ptr->size = size;
	return 0;
}

static inline void iwlagn_free_dma_ptr(struct iwl_priv *priv,
				    struct iwl_dma_ptr *ptr)
{
	if (unlikely(!ptr->addr))
		return;

	dma_free_coherent(priv->bus.dev, ptr->size, ptr->addr, ptr->dma);
	memset(ptr, 0, sizeof(*ptr));
}

static int iwl_trans_txq_alloc(struct iwl_priv *priv, struct iwl_tx_queue *txq,
		      int slots_num, u32 txq_id)
{
	size_t tfd_sz = priv->hw_params.tfd_size * TFD_QUEUE_SIZE_MAX;
	int i;

	if (WARN_ON(txq->meta || txq->cmd || txq->txb || txq->tfds))
		return -EINVAL;

	txq->q.n_window = slots_num;

	txq->meta = kzalloc(sizeof(txq->meta[0]) * slots_num,
			    GFP_KERNEL);
	txq->cmd = kzalloc(sizeof(txq->cmd[0]) * slots_num,
			   GFP_KERNEL);

	if (!txq->meta || !txq->cmd)
		goto error;

	for (i = 0; i < slots_num; i++) {
		txq->cmd[i] = kmalloc(sizeof(struct iwl_device_cmd),
					GFP_KERNEL);
		if (!txq->cmd[i])
			goto error;
	}

	/* Alloc driver data array and TFD circular buffer */
	/* Driver private data, only for Tx (not command) queues,
	 * not shared with device. */
	if (txq_id != priv->cmd_queue) {
		txq->txb = kzalloc(sizeof(txq->txb[0]) *
				   TFD_QUEUE_SIZE_MAX, GFP_KERNEL);
		if (!txq->txb) {
			IWL_ERR(priv, "kmalloc for auxiliary BD "
				  "structures failed\n");
			goto error;
		}
	} else {
		txq->txb = NULL;
	}

	/* Circular buffer of transmit frame descriptors (TFDs),
	 * shared with device */
	txq->tfds = dma_alloc_coherent(priv->bus.dev, tfd_sz, &txq->q.dma_addr,
				       GFP_KERNEL);
	if (!txq->tfds) {
		IWL_ERR(priv, "dma_alloc_coherent(%zd) failed\n", tfd_sz);
		goto error;
	}
	txq->q.id = txq_id;

	return 0;
error:
	kfree(txq->txb);
	txq->txb = NULL;
	/* since txq->cmd has been zeroed,
	 * all non allocated cmd[i] will be NULL */
	if (txq->cmd)
		for (i = 0; i < slots_num; i++)
			kfree(txq->cmd[i]);
	kfree(txq->meta);
	kfree(txq->cmd);
	txq->meta = NULL;
	txq->cmd = NULL;

	return -ENOMEM;

}

static int iwl_trans_txq_init(struct iwl_priv *priv, struct iwl_tx_queue *txq,
		      int slots_num, u32 txq_id)
{
	int ret;

	txq->need_update = 0;
	memset(txq->meta, 0, sizeof(txq->meta[0]) * slots_num);

	/*
	 * For the default queues 0-3, set up the swq_id
	 * already -- all others need to get one later
	 * (if they need one at all).
	 */
	if (txq_id < 4)
		iwl_set_swq_id(txq, txq_id, txq_id);

	/* TFD_QUEUE_SIZE_MAX must be power-of-two size, otherwise
	 * iwl_queue_inc_wrap and iwl_queue_dec_wrap are broken. */
	BUILD_BUG_ON(TFD_QUEUE_SIZE_MAX & (TFD_QUEUE_SIZE_MAX - 1));

	/* Initialize queue's high/low-water marks, and head/tail indexes */
	ret = iwl_queue_init(priv, &txq->q, TFD_QUEUE_SIZE_MAX, slots_num,
			txq_id);
	if (ret)
		return ret;

	/*
	 * Tell nic where to find circular buffer of Tx Frame Descriptors for
	 * given Tx queue, and enable the DMA channel used for that queue.
	 * Circular buffer (TFD queue in DRAM) physical base address */
	iwl_write_direct32(priv, FH_MEM_CBBC_QUEUE(txq_id),
			     txq->q.dma_addr >> 8);

	return 0;
}

/**
 * iwl_tx_queue_unmap -  Unmap any remaining DMA mappings and free skb's
 */
static void iwl_tx_queue_unmap(struct iwl_priv *priv, int txq_id)
{
	struct iwl_tx_queue *txq = &priv->txq[txq_id];
	struct iwl_queue *q = &txq->q;

	if (!q->n_bd)
		return;

	while (q->write_ptr != q->read_ptr) {
		/* The read_ptr needs to bound by q->n_window */
		iwlagn_txq_free_tfd(priv, txq, get_cmd_index(q, q->read_ptr));
		q->read_ptr = iwl_queue_inc_wrap(q->read_ptr, q->n_bd);
	}
}

/**
 * iwl_tx_queue_free - Deallocate DMA queue.
 * @txq: Transmit queue to deallocate.
 *
 * Empty queue by removing and destroying all BD's.
 * Free all buffers.
 * 0-fill, but do not free "txq" descriptor structure.
 */
static void iwl_tx_queue_free(struct iwl_priv *priv, int txq_id)
{
	struct iwl_tx_queue *txq = &priv->txq[txq_id];
	struct device *dev = priv->bus.dev;
	int i;
	if (WARN_ON(!txq))
		return;

	iwl_tx_queue_unmap(priv, txq_id);

	/* De-alloc array of command/tx buffers */
	for (i = 0; i < txq->q.n_window; i++)
		kfree(txq->cmd[i]);

	/* De-alloc circular buffer of TFDs */
	if (txq->q.n_bd) {
		dma_free_coherent(dev, priv->hw_params.tfd_size *
				  txq->q.n_bd, txq->tfds, txq->q.dma_addr);
		memset(&txq->q.dma_addr, 0, sizeof(txq->q.dma_addr));
	}

	/* De-alloc array of per-TFD driver data */
	kfree(txq->txb);
	txq->txb = NULL;

	/* deallocate arrays */
	kfree(txq->cmd);
	kfree(txq->meta);
	txq->cmd = NULL;
	txq->meta = NULL;

	/* 0-fill queue descriptor structure */
	memset(txq, 0, sizeof(*txq));
}

/**
 * iwl_trans_tx_free - Free TXQ Context
 *
 * Destroy all TX DMA queues and structures
 */
static void iwl_trans_tx_free(struct iwl_priv *priv)
{
	int txq_id;

	/* Tx queues */
	if (priv->txq) {
		for (txq_id = 0; txq_id < priv->hw_params.max_txq_num; txq_id++)
			iwl_tx_queue_free(priv, txq_id);
	}

	kfree(priv->txq);
	priv->txq = NULL;

	iwlagn_free_dma_ptr(priv, &priv->kw);

	iwlagn_free_dma_ptr(priv, &priv->scd_bc_tbls);
}

/**
 * iwl_trans_tx_alloc - allocate TX context
 * Allocate all Tx DMA structures and initialize them
 *
 * @param priv
 * @return error code
 */
static int iwl_trans_tx_alloc(struct iwl_priv *priv)
{
	int ret;
	int txq_id, slots_num;

	/*It is not allowed to alloc twice, so warn when this happens.
	 * We cannot rely on the previous allocation, so free and fail */
	if (WARN_ON(priv->txq)) {
		ret = -EINVAL;
		goto error;
	}

	ret = iwlagn_alloc_dma_ptr(priv, &priv->scd_bc_tbls,
				priv->hw_params.scd_bc_tbls_size);
	if (ret) {
		IWL_ERR(priv, "Scheduler BC Table allocation failed\n");
		goto error;
	}

	/* Alloc keep-warm buffer */
	ret = iwlagn_alloc_dma_ptr(priv, &priv->kw, IWL_KW_SIZE);
	if (ret) {
		IWL_ERR(priv, "Keep Warm allocation failed\n");
		goto error;
	}

	priv->txq = kzalloc(sizeof(struct iwl_tx_queue) *
			priv->cfg->base_params->num_of_queues, GFP_KERNEL);
	if (!priv->txq) {
		IWL_ERR(priv, "Not enough memory for txq\n");
		ret = ENOMEM;
		goto error;
	}

	/* Alloc and init all Tx queues, including the command queue (#4/#9) */
	for (txq_id = 0; txq_id < priv->hw_params.max_txq_num; txq_id++) {
		slots_num = (txq_id == priv->cmd_queue) ?
					TFD_CMD_SLOTS : TFD_TX_CMD_SLOTS;
		ret = iwl_trans_txq_alloc(priv, &priv->txq[txq_id], slots_num,
				       txq_id);
		if (ret) {
			IWL_ERR(priv, "Tx %d queue alloc failed\n", txq_id);
			goto error;
		}
	}

	return 0;

error:
	trans_tx_free(priv);

	return ret;
}
static int iwl_trans_tx_init(struct iwl_priv *priv)
{
	int ret;
	int txq_id, slots_num;
	unsigned long flags;
	bool alloc = false;

	if (!priv->txq) {
		ret = iwl_trans_tx_alloc(priv);
		if (ret)
			goto error;
		alloc = true;
	}

	spin_lock_irqsave(&priv->lock, flags);

	/* Turn off all Tx DMA fifos */
	iwl_write_prph(priv, IWLAGN_SCD_TXFACT, 0);

	/* Tell NIC where to find the "keep warm" buffer */
	iwl_write_direct32(priv, FH_KW_MEM_ADDR_REG, priv->kw.dma >> 4);

	spin_unlock_irqrestore(&priv->lock, flags);

	/* Alloc and init all Tx queues, including the command queue (#4/#9) */
	for (txq_id = 0; txq_id < priv->hw_params.max_txq_num; txq_id++) {
		slots_num = (txq_id == priv->cmd_queue) ?
					TFD_CMD_SLOTS : TFD_TX_CMD_SLOTS;
		ret = iwl_trans_txq_init(priv, &priv->txq[txq_id], slots_num,
				       txq_id);
		if (ret) {
			IWL_ERR(priv, "Tx %d queue init failed\n", txq_id);
			goto error;
		}
	}

	return 0;
error:
	/*Upon error, free only if we allocated something */
	if (alloc)
		trans_tx_free(priv);
	return ret;
}

/**
 * iwlagn_txq_ctx_stop - Stop all Tx DMA channels
 */
static int iwl_trans_tx_stop(struct iwl_priv *priv)
{
	int ch, txq_id;
	unsigned long flags;

	/* Turn off all Tx DMA fifos */
	spin_lock_irqsave(&priv->lock, flags);

	iwlagn_txq_set_sched(priv, 0);

	/* Stop each Tx DMA channel, and wait for it to be idle */
	for (ch = 0; ch < FH_TCSR_CHNL_NUM; ch++) {
		iwl_write_direct32(priv, FH_TCSR_CHNL_TX_CONFIG_REG(ch), 0x0);
		if (iwl_poll_direct_bit(priv, FH_TSSR_TX_STATUS_REG,
				    FH_TSSR_TX_STATUS_REG_MSK_CHNL_IDLE(ch),
				    1000))
			IWL_ERR(priv, "Failing on timeout while stopping"
			    " DMA channel %d [0x%08x]", ch,
			    iwl_read_direct32(priv, FH_TSSR_TX_STATUS_REG));
	}
	spin_unlock_irqrestore(&priv->lock, flags);

	if (!priv->txq) {
		IWL_WARN(priv, "Stopping tx queues that aren't allocated...");
		return 0;
	}

	/* Unmap DMA from host system and free skb's */
	for (txq_id = 0; txq_id < priv->hw_params.max_txq_num; txq_id++)
		iwl_tx_queue_unmap(priv, txq_id);

	return 0;
}

static struct iwl_tx_cmd *iwl_trans_get_tx_cmd(struct iwl_priv *priv,
						int txq_id)
{
	struct iwl_tx_queue *txq = &priv->txq[txq_id];
	struct iwl_queue *q = &txq->q;
	struct iwl_device_cmd *dev_cmd;

	if (unlikely(iwl_queue_space(q) < q->high_mark))
		return NULL;

	/*
	 * Set up the Tx-command (not MAC!) header.
	 * Store the chosen Tx queue and TFD index within the sequence field;
	 * after Tx, uCode's Tx response will return this value so driver can
	 * locate the frame within the tx queue and do post-tx processing.
	 */
	dev_cmd = txq->cmd[q->write_ptr];
	memset(dev_cmd, 0, sizeof(*dev_cmd));
	dev_cmd->hdr.cmd = REPLY_TX;
	dev_cmd->hdr.sequence = cpu_to_le16((u16)(QUEUE_TO_SEQ(txq_id) |
				INDEX_TO_SEQ(q->write_ptr)));
	return &dev_cmd->cmd.tx;
}

static int iwl_trans_tx(struct iwl_priv *priv, struct sk_buff *skb,
		struct iwl_tx_cmd *tx_cmd, int txq_id, __le16 fc, bool ampdu,
		struct iwl_rxon_context *ctx)
{
	struct iwl_tx_queue *txq = &priv->txq[txq_id];
	struct iwl_queue *q = &txq->q;
	struct iwl_device_cmd *dev_cmd = txq->cmd[q->write_ptr];
	struct iwl_cmd_meta *out_meta;

	dma_addr_t phys_addr = 0;
	dma_addr_t txcmd_phys;
	dma_addr_t scratch_phys;
	u16 len, firstlen, secondlen;
	u8 wait_write_ptr = 0;
	u8 hdr_len = ieee80211_hdrlen(fc);

	/* Set up driver data for this TFD */
	memset(&(txq->txb[q->write_ptr]), 0, sizeof(struct iwl_tx_info));
	txq->txb[q->write_ptr].skb = skb;
	txq->txb[q->write_ptr].ctx = ctx;

	/* Set up first empty entry in queue's array of Tx/cmd buffers */
	out_meta = &txq->meta[q->write_ptr];

	/*
	 * Use the first empty entry in this queue's command buffer array
	 * to contain the Tx command and MAC header concatenated together
	 * (payload data will be in another buffer).
	 * Size of this varies, due to varying MAC header length.
	 * If end is not dword aligned, we'll have 2 extra bytes at the end
	 * of the MAC header (device reads on dword boundaries).
	 * We'll tell device about this padding later.
	 */
	len = sizeof(struct iwl_tx_cmd) +
		sizeof(struct iwl_cmd_header) + hdr_len;
	firstlen = (len + 3) & ~3;

	/* Tell NIC about any 2-byte padding after MAC header */
	if (firstlen != len)
		tx_cmd->tx_flags |= TX_CMD_FLG_MH_PAD_MSK;

	/* Physical address of this Tx command's header (not MAC header!),
	 * within command buffer array. */
	txcmd_phys = dma_map_single(priv->bus.dev,
				    &dev_cmd->hdr, firstlen,
				    DMA_BIDIRECTIONAL);
	if (unlikely(dma_mapping_error(priv->bus.dev, txcmd_phys)))
		return -1;
	dma_unmap_addr_set(out_meta, mapping, txcmd_phys);
	dma_unmap_len_set(out_meta, len, firstlen);

	if (!ieee80211_has_morefrags(fc)) {
		txq->need_update = 1;
	} else {
		wait_write_ptr = 1;
		txq->need_update = 0;
	}

	/* Set up TFD's 2nd entry to point directly to remainder of skb,
	 * if any (802.11 null frames have no payload). */
	secondlen = skb->len - hdr_len;
	if (secondlen > 0) {
		phys_addr = dma_map_single(priv->bus.dev, skb->data + hdr_len,
					   secondlen, DMA_TO_DEVICE);
		if (unlikely(dma_mapping_error(priv->bus.dev, phys_addr))) {
			dma_unmap_single(priv->bus.dev,
					 dma_unmap_addr(out_meta, mapping),
					 dma_unmap_len(out_meta, len),
					 DMA_BIDIRECTIONAL);
			return -1;
		}
	}

	/* Attach buffers to TFD */
	iwlagn_txq_attach_buf_to_tfd(priv, txq, txcmd_phys, firstlen, 1);
	if (secondlen > 0)
		iwlagn_txq_attach_buf_to_tfd(priv, txq, phys_addr,
					     secondlen, 0);

	scratch_phys = txcmd_phys + sizeof(struct iwl_cmd_header) +
				offsetof(struct iwl_tx_cmd, scratch);

	/* take back ownership of DMA buffer to enable update */
	dma_sync_single_for_cpu(priv->bus.dev, txcmd_phys, firstlen,
			DMA_BIDIRECTIONAL);
	tx_cmd->dram_lsb_ptr = cpu_to_le32(scratch_phys);
	tx_cmd->dram_msb_ptr = iwl_get_dma_hi_addr(scratch_phys);

	IWL_DEBUG_TX(priv, "sequence nr = 0X%x\n",
		     le16_to_cpu(dev_cmd->hdr.sequence));
	IWL_DEBUG_TX(priv, "tx_flags = 0X%x\n", le32_to_cpu(tx_cmd->tx_flags));
	iwl_print_hex_dump(priv, IWL_DL_TX, (u8 *)tx_cmd, sizeof(*tx_cmd));
	iwl_print_hex_dump(priv, IWL_DL_TX, (u8 *)tx_cmd->hdr, hdr_len);

	/* Set up entry for this TFD in Tx byte-count array */
	if (ampdu)
		iwlagn_txq_update_byte_cnt_tbl(priv, txq,
					       le16_to_cpu(tx_cmd->len));

	dma_sync_single_for_device(priv->bus.dev, txcmd_phys, firstlen,
			DMA_BIDIRECTIONAL);

	trace_iwlwifi_dev_tx(priv,
			     &((struct iwl_tfd *)txq->tfds)[txq->q.write_ptr],
			     sizeof(struct iwl_tfd),
			     &dev_cmd->hdr, firstlen,
			     skb->data + hdr_len, secondlen);

	/* Tell device the write index *just past* this latest filled TFD */
	q->write_ptr = iwl_queue_inc_wrap(q->write_ptr, q->n_bd);
	iwl_txq_update_write_ptr(priv, txq);

	/*
	 * At this point the frame is "transmitted" successfully
	 * and we will get a TX status notification eventually,
	 * regardless of the value of ret. "ret" only indicates
	 * whether or not we should update the write pointer.
	 */
	if ((iwl_queue_space(q) < q->high_mark) && priv->mac80211_registered) {
		if (wait_write_ptr) {
			txq->need_update = 1;
			iwl_txq_update_write_ptr(priv, txq);
		} else {
			iwl_stop_queue(priv, txq);
		}
	}
	return 0;
}

static const struct iwl_trans_ops trans_ops = {
	.rx_init = iwl_trans_rx_init,
	.rx_stop = iwl_trans_rx_stop,
	.rx_free = iwl_trans_rx_free,

	.tx_init = iwl_trans_tx_init,
	.tx_stop = iwl_trans_tx_stop,
	.tx_free = iwl_trans_tx_free,

	.send_cmd = iwl_send_cmd,
	.send_cmd_pdu = iwl_send_cmd_pdu,

	.get_tx_cmd = iwl_trans_get_tx_cmd,
	.tx = iwl_trans_tx,
};

void iwl_trans_register(struct iwl_trans *trans)
{
	trans->ops = &trans_ops;
}
