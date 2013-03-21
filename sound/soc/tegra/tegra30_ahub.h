/*
 * tegra30_ahub.h - Definitions for Tegra30 AHUB driver
 *
 * Copyright (c) 2011,2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TEGRA30_AHUB_H__
#define __TEGRA30_AHUB_H__

/* Fields in *_CIF_RX/TX_CTRL; used by AHUB FIFOs, and all other audio modules */

#define TEGRA30_AUDIOCIF_CTRL_FIFO_THRESHOLD_SHIFT	28
#define TEGRA30_AUDIOCIF_CTRL_FIFO_THRESHOLD_MASK_US	0xf
#define TEGRA30_AUDIOCIF_CTRL_FIFO_THRESHOLD_MASK	(TEGRA30_AUDIOCIF_CTRL_FIFO_THRESHOLD_MASK_US << TEGRA30_AUDIOCIF_CTRL_FIFO_THRESHOLD_SHIFT)

/* Channel count minus 1 */
#define TEGRA30_AUDIOCIF_CTRL_AUDIO_CHANNELS_SHIFT	24
#define TEGRA30_AUDIOCIF_CTRL_AUDIO_CHANNELS_MASK_US	7
#define TEGRA30_AUDIOCIF_CTRL_AUDIO_CHANNELS_MASK	(TEGRA30_AUDIOCIF_CTRL_AUDIO_CHANNELS_MASK_US << TEGRA30_AUDIOCIF_CTRL_AUDIO_CHANNELS_SHIFT)

/* Channel count minus 1 */
#define TEGRA30_AUDIOCIF_CTRL_CLIENT_CHANNELS_SHIFT	16
#define TEGRA30_AUDIOCIF_CTRL_CLIENT_CHANNELS_MASK_US	7
#define TEGRA30_AUDIOCIF_CTRL_CLIENT_CHANNELS_MASK	(TEGRA30_AUDIOCIF_CTRL_CLIENT_CHANNELS_MASK_US << TEGRA30_AUDIOCIF_CTRL_CLIENT_CHANNELS_SHIFT)

#define TEGRA30_AUDIOCIF_BITS_4				0
#define TEGRA30_AUDIOCIF_BITS_8				1
#define TEGRA30_AUDIOCIF_BITS_12			2
#define TEGRA30_AUDIOCIF_BITS_16			3
#define TEGRA30_AUDIOCIF_BITS_20			4
#define TEGRA30_AUDIOCIF_BITS_24			5
#define TEGRA30_AUDIOCIF_BITS_28			6
#define TEGRA30_AUDIOCIF_BITS_32			7

#define TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_SHIFT		12
#define TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_MASK		(7                        << TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_4		(TEGRA30_AUDIOCIF_BITS_4  << TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_8		(TEGRA30_AUDIOCIF_BITS_8  << TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_12		(TEGRA30_AUDIOCIF_BITS_12 << TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_16		(TEGRA30_AUDIOCIF_BITS_16 << TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_20		(TEGRA30_AUDIOCIF_BITS_20 << TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_24		(TEGRA30_AUDIOCIF_BITS_24 << TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_28		(TEGRA30_AUDIOCIF_BITS_28 << TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_32		(TEGRA30_AUDIOCIF_BITS_32 << TEGRA30_AUDIOCIF_CTRL_AUDIO_BITS_SHIFT)

#define TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_SHIFT		8
#define TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_MASK		(7                        << TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_4		(TEGRA30_AUDIOCIF_BITS_4  << TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_8		(TEGRA30_AUDIOCIF_BITS_8  << TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_12		(TEGRA30_AUDIOCIF_BITS_12 << TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_16		(TEGRA30_AUDIOCIF_BITS_16 << TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_20		(TEGRA30_AUDIOCIF_BITS_20 << TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_24		(TEGRA30_AUDIOCIF_BITS_24 << TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_28		(TEGRA30_AUDIOCIF_BITS_28 << TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_32		(TEGRA30_AUDIOCIF_BITS_32 << TEGRA30_AUDIOCIF_CTRL_CLIENT_BITS_SHIFT)

#define TEGRA30_AUDIOCIF_EXPAND_ZERO			0
#define TEGRA30_AUDIOCIF_EXPAND_ONE			1
#define TEGRA30_AUDIOCIF_EXPAND_LFSR			2

#define TEGRA30_AUDIOCIF_CTRL_EXPAND_SHIFT		6
#define TEGRA30_AUDIOCIF_CTRL_EXPAND_MASK		(3                            << TEGRA30_AUDIOCIF_CTRL_EXPAND_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_EXPAND_ZERO		(TEGRA30_AUDIOCIF_EXPAND_ZERO << TEGRA30_AUDIOCIF_CTRL_EXPAND_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_EXPAND_ONE		(TEGRA30_AUDIOCIF_EXPAND_ONE  << TEGRA30_AUDIOCIF_CTRL_EXPAND_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_EXPAND_LFSR		(TEGRA30_AUDIOCIF_EXPAND_LFSR << TEGRA30_AUDIOCIF_CTRL_EXPAND_SHIFT)

#define TEGRA30_AUDIOCIF_STEREO_CONV_CH0		0
#define TEGRA30_AUDIOCIF_STEREO_CONV_CH1		1
#define TEGRA30_AUDIOCIF_STEREO_CONV_AVG		2

#define TEGRA30_AUDIOCIF_CTRL_STEREO_CONV_SHIFT		4
#define TEGRA30_AUDIOCIF_CTRL_STEREO_CONV_MASK		(3                                << TEGRA30_AUDIOCIF_CTRL_STEREO_CONV_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_STEREO_CONV_CH0		(TEGRA30_AUDIOCIF_STEREO_CONV_CH0 << TEGRA30_AUDIOCIF_CTRL_STEREO_CONV_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_STEREO_CONV_CH1		(TEGRA30_AUDIOCIF_STEREO_CONV_CH1 << TEGRA30_AUDIOCIF_CTRL_STEREO_CONV_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_STEREO_CONV_AVG		(TEGRA30_AUDIOCIF_STEREO_CONV_AVG << TEGRA30_AUDIOCIF_CTRL_STEREO_CONV_SHIFT)

#define TEGRA30_AUDIOCIF_CTRL_REPLICATE			3

#define TEGRA30_AUDIOCIF_DIRECTION_TX			0
#define TEGRA30_AUDIOCIF_DIRECTION_RX			1

#define TEGRA30_AUDIOCIF_CTRL_DIRECTION_SHIFT		2
#define TEGRA30_AUDIOCIF_CTRL_DIRECTION_MASK		(1                             << TEGRA30_AUDIOCIF_CTRL_DIRECTION_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_DIRECTION_TX		(TEGRA30_AUDIOCIF_DIRECTION_TX << TEGRA30_AUDIOCIF_CTRL_DIRECTION_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_DIRECTION_RX		(TEGRA30_AUDIOCIF_DIRECTION_RX << TEGRA30_AUDIOCIF_CTRL_DIRECTION_SHIFT)

#define TEGRA30_AUDIOCIF_TRUNCATE_ROUND			0
#define TEGRA30_AUDIOCIF_TRUNCATE_CHOP			1

#define TEGRA30_AUDIOCIF_CTRL_TRUNCATE_SHIFT		1
#define TEGRA30_AUDIOCIF_CTRL_TRUNCATE_MASK		(1                               << TEGRA30_AUDIOCIF_CTRL_TRUNCATE_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_TRUNCATE_ROUND		(TEGRA30_AUDIOCIF_TRUNCATE_ROUND << TEGRA30_AUDIOCIF_CTRL_TRUNCATE_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_TRUNCATE_CHOP		(TEGRA30_AUDIOCIF_TRUNCATE_CHOP  << TEGRA30_AUDIOCIF_CTRL_TRUNCATE_SHIFT)

#define TEGRA30_AUDIOCIF_MONO_CONV_ZERO			0
#define TEGRA30_AUDIOCIF_MONO_CONV_COPY			1

#define TEGRA30_AUDIOCIF_CTRL_MONO_CONV_SHIFT		0
#define TEGRA30_AUDIOCIF_CTRL_MONO_CONV_MASK		(1                               << TEGRA30_AUDIOCIF_CTRL_MONO_CONV_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_MONO_CONV_ZERO		(TEGRA30_AUDIOCIF_MONO_CONV_ZERO << TEGRA30_AUDIOCIF_CTRL_MONO_CONV_SHIFT)
#define TEGRA30_AUDIOCIF_CTRL_MONO_CONV_COPY		(TEGRA30_AUDIOCIF_MONO_CONV_COPY << TEGRA30_AUDIOCIF_CTRL_MONO_CONV_SHIFT)

/* Registers within TEGRA30_AUDIO_CLUSTER_BASE */

/* TEGRA30_AHUB_CHANNEL_CTRL */

#define TEGRA30_AHUB_CHANNEL_CTRL			0x0
#define TEGRA30_AHUB_CHANNEL_CTRL_STRIDE		0x20
#define TEGRA30_AHUB_CHANNEL_CTRL_COUNT			4
#define TEGRA30_AHUB_CHANNEL_CTRL_TX_EN			(1 << 31)
#define TEGRA30_AHUB_CHANNEL_CTRL_RX_EN			(1 << 30)
#define TEGRA30_AHUB_CHANNEL_CTRL_LOOPBACK		(1 << 29)

#define TEGRA30_AHUB_CHANNEL_CTRL_TX_THRESHOLD_SHIFT	16
#define TEGRA30_AHUB_CHANNEL_CTRL_TX_THRESHOLD_MASK_US	0xff
#define TEGRA30_AHUB_CHANNEL_CTRL_TX_THRESHOLD_MASK	(TEGRA30_AHUB_CHANNEL_CTRL_TX_THRESHOLD_MASK_US << TEGRA30_AHUB_CHANNEL_CTRL_TX_THRESHOLD_SHIFT)

#define TEGRA30_AHUB_CHANNEL_CTRL_RX_THRESHOLD_SHIFT	8
#define TEGRA30_AHUB_CHANNEL_CTRL_RX_THRESHOLD_MASK_US	0xff
#define TEGRA30_AHUB_CHANNEL_CTRL_RX_THRESHOLD_MASK	(TEGRA30_AHUB_CHANNEL_CTRL_RX_THRESHOLD_MASK_US << TEGRA30_AHUB_CHANNEL_CTRL_RX_THRESHOLD_SHIFT)

#define TEGRA30_AHUB_CHANNEL_CTRL_TX_PACK_EN		(1 << 6)

#define TEGRA30_PACK_8_4				2
#define TEGRA30_PACK_16					3

#define TEGRA30_AHUB_CHANNEL_CTRL_TX_PACK_SHIFT		4
#define TEGRA30_AHUB_CHANNEL_CTRL_TX_PACK_MASK_US	3
#define TEGRA30_AHUB_CHANNEL_CTRL_TX_PACK_MASK		(TEGRA30_AHUB_CHANNEL_CTRL_TX_PACK_MASK_US << TEGRA30_AHUB_CHANNEL_CTRL_TX_PACK_SHIFT)
#define TEGRA30_AHUB_CHANNEL_CTRL_TX_PACK_8_4		(TEGRA30_PACK_8_4                          << TEGRA30_AHUB_CHANNEL_CTRL_TX_PACK_SHIFT)
#define TEGRA30_AHUB_CHANNEL_CTRL_TX_PACK_16		(TEGRA30_PACK_16                           << TEGRA30_AHUB_CHANNEL_CTRL_TX_PACK_SHIFT)

#define TEGRA30_AHUB_CHANNEL_CTRL_RX_PACK_EN		(1 << 2)

#define TEGRA30_AHUB_CHANNEL_CTRL_RX_PACK_SHIFT		0
#define TEGRA30_AHUB_CHANNEL_CTRL_RX_PACK_MASK_US	3
#define TEGRA30_AHUB_CHANNEL_CTRL_RX_PACK_MASK		(TEGRA30_AHUB_CHANNEL_CTRL_RX_PACK_MASK_US << TEGRA30_AHUB_CHANNEL_CTRL_RX_PACK_SHIFT)
#define TEGRA30_AHUB_CHANNEL_CTRL_RX_PACK_8_4		(TEGRA30_PACK_8_4                          << TEGRA30_AHUB_CHANNEL_CTRL_RX_PACK_SHIFT)
#define TEGRA30_AHUB_CHANNEL_CTRL_RX_PACK_16		(TEGRA30_PACK_16                           << TEGRA30_AHUB_CHANNEL_CTRL_RX_PACK_SHIFT)

/* TEGRA30_AHUB_CHANNEL_CLEAR */

#define TEGRA30_AHUB_CHANNEL_CLEAR			0x4
#define TEGRA30_AHUB_CHANNEL_CLEAR_STRIDE		0x20
#define TEGRA30_AHUB_CHANNEL_CLEAR_COUNT		4
#define TEGRA30_AHUB_CHANNEL_CLEAR_TX_SOFT_RESET	(1 << 31)
#define TEGRA30_AHUB_CHANNEL_CLEAR_RX_SOFT_RESET	(1 << 30)

/* TEGRA30_AHUB_CHANNEL_STATUS */

#define TEGRA30_AHUB_CHANNEL_STATUS			0x8
#define TEGRA30_AHUB_CHANNEL_STATUS_STRIDE		0x20
#define TEGRA30_AHUB_CHANNEL_STATUS_COUNT		4
#define TEGRA30_AHUB_CHANNEL_STATUS_TX_FREE_SHIFT	24
#define TEGRA30_AHUB_CHANNEL_STATUS_TX_FREE_MASK_US	0xff
#define TEGRA30_AHUB_CHANNEL_STATUS_TX_FREE_MASK	(TEGRA30_AHUB_CHANNEL_STATUS_TX_FREE_MASK_US << TEGRA30_AHUB_CHANNEL_STATUS_TX_FREE_SHIFT)
#define TEGRA30_AHUB_CHANNEL_STATUS_RX_FREE_SHIFT	16
#define TEGRA30_AHUB_CHANNEL_STATUS_RX_FREE_MASK_US	0xff
#define TEGRA30_AHUB_CHANNEL_STATUS_RX_FREE_MASK	(TEGRA30_AHUB_CHANNEL_STATUS_RX_FREE_MASK_US << TEGRA30_AHUB_CHANNEL_STATUS_RX_FREE_SHIFT)
#define TEGRA30_AHUB_CHANNEL_STATUS_TX_TRIG		(1 << 1)
#define TEGRA30_AHUB_CHANNEL_STATUS_RX_TRIG		(1 << 0)

/* TEGRA30_AHUB_CHANNEL_TXFIFO */

#define TEGRA30_AHUB_CHANNEL_TXFIFO			0xc
#define TEGRA30_AHUB_CHANNEL_TXFIFO_STRIDE		0x20
#define TEGRA30_AHUB_CHANNEL_TXFIFO_COUNT		4

/* TEGRA30_AHUB_CHANNEL_RXFIFO */

#define TEGRA30_AHUB_CHANNEL_RXFIFO			0x10
#define TEGRA30_AHUB_CHANNEL_RXFIFO_STRIDE		0x20
#define TEGRA30_AHUB_CHANNEL_RXFIFO_COUNT		4

/* TEGRA30_AHUB_CIF_TX_CTRL */

#define TEGRA30_AHUB_CIF_TX_CTRL			0x14
#define TEGRA30_AHUB_CIF_TX_CTRL_STRIDE			0x20
#define TEGRA30_AHUB_CIF_TX_CTRL_COUNT			4
/* Uses field from TEGRA30_AUDIOCIF_CTRL_* */

/* TEGRA30_AHUB_CIF_RX_CTRL */

#define TEGRA30_AHUB_CIF_RX_CTRL			0x18
#define TEGRA30_AHUB_CIF_RX_CTRL_STRIDE			0x20
#define TEGRA30_AHUB_CIF_RX_CTRL_COUNT			4
/* Uses field from TEGRA30_AUDIOCIF_CTRL_* */

/* TEGRA30_AHUB_CONFIG_LINK_CTRL */

#define TEGRA30_AHUB_CONFIG_LINK_CTRL					0x80
#define TEGRA30_AHUB_CONFIG_LINK_CTRL_MASTER_FIFO_FULL_CNT_SHIFT	28
#define TEGRA30_AHUB_CONFIG_LINK_CTRL_MASTER_FIFO_FULL_CNT_MASK_US	0xf
#define TEGRA30_AHUB_CONFIG_LINK_CTRL_MASTER_FIFO_FULL_CNT_MASK		(TEGRA30_AHUB_CONFIG_LINK_CTRL_MASTER_FIFO_FULL_CNT_MASK_US << TEGRA30_AHUB_CONFIG_LINK_CTRL_MASTER_FIFO_FULL_CNT_SHIFT)
#define TEGRA30_AHUB_CONFIG_LINK_CTRL_TIMEOUT_CNT_SHIFT			16
#define TEGRA30_AHUB_CONFIG_LINK_CTRL_TIMEOUT_CNT_MASK_US		0xfff
#define TEGRA30_AHUB_CONFIG_LINK_CTRL_TIMEOUT_CNT_MASK			(TEGRA30_AHUB_CONFIG_LINK_CTRL_TIMEOUT_CNT_MASK_US << TEGRA30_AHUB_CONFIG_LINK_CTRL_TIMEOUT_CNT_SHIFT)
#define TEGRA30_AHUB_CONFIG_LINK_CTRL_IDLE_CNT_SHIFT			4
#define TEGRA30_AHUB_CONFIG_LINK_CTRL_IDLE_CNT_MASK_US			0xfff
#define TEGRA30_AHUB_CONFIG_LINK_CTRL_IDLE_CNT_MASK			(TEGRA30_AHUB_CONFIG_LINK_CTRL_IDLE_CNT_MASK_US << TEGRA30_AHUB_CONFIG_LINK_CTRL_IDLE_CNT_SHIFT)
#define TEGRA30_AHUB_CONFIG_LINK_CTRL_CG_EN				(1 << 2)
#define TEGRA30_AHUB_CONFIG_LINK_CTRL_CLEAR_TIMEOUT_CNTR		(1 << 1)
#define TEGRA30_AHUB_CONFIG_LINK_CTRL_SOFT_RESET			(1 << 0)

/* TEGRA30_AHUB_MISC_CTRL */

#define TEGRA30_AHUB_MISC_CTRL				0x84
#define TEGRA30_AHUB_MISC_CTRL_AUDIO_ACTIVE		(1 << 31)
#define TEGRA30_AHUB_MISC_CTRL_AUDIO_CG_EN		(1 << 8)
#define TEGRA30_AHUB_MISC_CTRL_AUDIO_OBS_SEL_SHIFT	0
#define TEGRA30_AHUB_MISC_CTRL_AUDIO_OBS_SEL_MASK	(0x1f << TEGRA30_AHUB_MISC_CTRL_AUDIO_OBS_SEL_SHIFT)

/* TEGRA30_AHUB_APBDMA_LIVE_STATUS */

#define TEGRA30_AHUB_APBDMA_LIVE_STATUS				0x88
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH3_RX_CIF_FIFO_FULL	(1 << 31)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH3_TX_CIF_FIFO_FULL	(1 << 30)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH2_RX_CIF_FIFO_FULL	(1 << 29)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH2_TX_CIF_FIFO_FULL	(1 << 28)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH1_RX_CIF_FIFO_FULL	(1 << 27)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH1_TX_CIF_FIFO_FULL	(1 << 26)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH0_RX_CIF_FIFO_FULL	(1 << 25)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH0_TX_CIF_FIFO_FULL	(1 << 24)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH3_RX_CIF_FIFO_EMPTY	(1 << 23)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH3_TX_CIF_FIFO_EMPTY	(1 << 22)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH2_RX_CIF_FIFO_EMPTY	(1 << 21)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH2_TX_CIF_FIFO_EMPTY	(1 << 20)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH1_RX_CIF_FIFO_EMPTY	(1 << 19)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH1_TX_CIF_FIFO_EMPTY	(1 << 18)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH0_RX_CIF_FIFO_EMPTY	(1 << 17)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH0_TX_CIF_FIFO_EMPTY	(1 << 16)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH3_RX_DMA_FIFO_FULL	(1 << 15)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH3_TX_DMA_FIFO_FULL	(1 << 14)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH2_RX_DMA_FIFO_FULL	(1 << 13)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH2_TX_DMA_FIFO_FULL	(1 << 12)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH1_RX_DMA_FIFO_FULL	(1 << 11)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH1_TX_DMA_FIFO_FULL	(1 << 10)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH0_RX_DMA_FIFO_FULL	(1 << 9)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH0_TX_DMA_FIFO_FULL	(1 << 8)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH3_RX_DMA_FIFO_EMPTY	(1 << 7)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH3_TX_DMA_FIFO_EMPTY	(1 << 6)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH2_RX_DMA_FIFO_EMPTY	(1 << 5)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH2_TX_DMA_FIFO_EMPTY	(1 << 4)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH1_RX_DMA_FIFO_EMPTY	(1 << 3)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH1_TX_DMA_FIFO_EMPTY	(1 << 2)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH0_RX_DMA_FIFO_EMPTY	(1 << 1)
#define TEGRA30_AHUB_APBDMA_LIVE_STATUS_CH0_TX_DMA_FIFO_EMPTY	(1 << 0)

/* TEGRA30_AHUB_I2S_LIVE_STATUS */

#define TEGRA30_AHUB_I2S_LIVE_STATUS				0x8c
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S4_RX_FIFO_FULL		(1 << 29)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S4_TX_FIFO_FULL		(1 << 28)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S3_RX_FIFO_FULL		(1 << 27)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S3_TX_FIFO_FULL		(1 << 26)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S2_RX_FIFO_FULL		(1 << 25)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S2_TX_FIFO_FULL		(1 << 24)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S1_RX_FIFO_FULL		(1 << 23)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S1_TX_FIFO_FULL		(1 << 22)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S0_RX_FIFO_FULL		(1 << 21)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S0_TX_FIFO_FULL		(1 << 20)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S4_RX_FIFO_ENABLED	(1 << 19)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S4_TX_FIFO_ENABLED	(1 << 18)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S3_RX_FIFO_ENABLED	(1 << 17)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S3_TX_FIFO_ENABLED	(1 << 16)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S2_RX_FIFO_ENABLED	(1 << 15)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S2_TX_FIFO_ENABLED	(1 << 14)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S1_RX_FIFO_ENABLED	(1 << 13)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S1_TX_FIFO_ENABLED	(1 << 12)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S0_RX_FIFO_ENABLED	(1 << 11)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S0_TX_FIFO_ENABLED	(1 << 10)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S4_RX_FIFO_EMPTY		(1 << 9)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S4_TX_FIFO_EMPTY		(1 << 8)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S3_RX_FIFO_EMPTY		(1 << 7)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S3_TX_FIFO_EMPTY		(1 << 6)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S2_RX_FIFO_EMPTY		(1 << 5)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S2_TX_FIFO_EMPTY		(1 << 4)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S1_RX_FIFO_EMPTY		(1 << 3)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S1_TX_FIFO_EMPTY		(1 << 2)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S0_RX_FIFO_EMPTY		(1 << 1)
#define TEGRA30_AHUB_I2S_LIVE_STATUS_I2S0_TX_FIFO_EMPTY		(1 << 0)

/* TEGRA30_AHUB_DAM0_LIVE_STATUS */

#define TEGRA30_AHUB_DAM_LIVE_STATUS				0x90
#define TEGRA30_AHUB_DAM_LIVE_STATUS_STRIDE			0x8
#define TEGRA30_AHUB_DAM_LIVE_STATUS_COUNT			3
#define TEGRA30_AHUB_DAM_LIVE_STATUS_TX_ENABLED			(1 << 26)
#define TEGRA30_AHUB_DAM_LIVE_STATUS_RX1_ENABLED		(1 << 25)
#define TEGRA30_AHUB_DAM_LIVE_STATUS_RX0_ENABLED		(1 << 24)
#define TEGRA30_AHUB_DAM_LIVE_STATUS_TXFIFO_FULL		(1 << 15)
#define TEGRA30_AHUB_DAM_LIVE_STATUS_RX1FIFO_FULL		(1 << 9)
#define TEGRA30_AHUB_DAM_LIVE_STATUS_RX0FIFO_FULL		(1 << 8)
#define TEGRA30_AHUB_DAM_LIVE_STATUS_TXFIFO_EMPTY		(1 << 7)
#define TEGRA30_AHUB_DAM_LIVE_STATUS_RX1FIFO_EMPTY		(1 << 1)
#define TEGRA30_AHUB_DAM_LIVE_STATUS_RX0FIFO_EMPTY		(1 << 0)

/* TEGRA30_AHUB_SPDIF_LIVE_STATUS */

#define TEGRA30_AHUB_SPDIF_LIVE_STATUS				0xa8
#define TEGRA30_AHUB_SPDIF_LIVE_STATUS_USER_TX_ENABLED		(1 << 11)
#define TEGRA30_AHUB_SPDIF_LIVE_STATUS_USER_RX_ENABLED		(1 << 10)
#define TEGRA30_AHUB_SPDIF_LIVE_STATUS_DATA_TX_ENABLED		(1 << 9)
#define TEGRA30_AHUB_SPDIF_LIVE_STATUS_DATA_RX_ENABLED		(1 << 8)
#define TEGRA30_AHUB_SPDIF_LIVE_STATUS_USER_TXFIFO_FULL		(1 << 7)
#define TEGRA30_AHUB_SPDIF_LIVE_STATUS_USER_RXFIFO_FULL		(1 << 6)
#define TEGRA30_AHUB_SPDIF_LIVE_STATUS_DATA_TXFIFO_FULL		(1 << 5)
#define TEGRA30_AHUB_SPDIF_LIVE_STATUS_DATA_RXFIFO_FULL		(1 << 4)
#define TEGRA30_AHUB_SPDIF_LIVE_STATUS_USER_TXFIFO_EMPTY	(1 << 3)
#define TEGRA30_AHUB_SPDIF_LIVE_STATUS_USER_RXFIFO_EMPTY	(1 << 2)
#define TEGRA30_AHUB_SPDIF_LIVE_STATUS_DATA_TXFIFO_EMPTY	(1 << 1)
#define TEGRA30_AHUB_SPDIF_LIVE_STATUS_DATA_RXFIFO_EMPTY	(1 << 0)

/* TEGRA30_AHUB_I2S_INT_MASK */

#define TEGRA30_AHUB_I2S_INT_MASK				0xb0

/* TEGRA30_AHUB_DAM_INT_MASK */

#define TEGRA30_AHUB_DAM_INT_MASK				0xb4

/* TEGRA30_AHUB_SPDIF_INT_MASK */

#define TEGRA30_AHUB_SPDIF_INT_MASK				0xbc

/* TEGRA30_AHUB_APBIF_INT_MASK */

#define TEGRA30_AHUB_APBIF_INT_MASK				0xc0

/* TEGRA30_AHUB_I2S_INT_STATUS */

#define TEGRA30_AHUB_I2S_INT_STATUS				0xc8

/* TEGRA30_AHUB_DAM_INT_STATUS */

#define TEGRA30_AHUB_DAM_INT_STATUS				0xcc

/* TEGRA30_AHUB_SPDIF_INT_STATUS */

#define TEGRA30_AHUB_SPDIF_INT_STATUS				0xd4

/* TEGRA30_AHUB_APBIF_INT_STATUS */

#define TEGRA30_AHUB_APBIF_INT_STATUS				0xd8

/* TEGRA30_AHUB_I2S_INT_SOURCE */

#define TEGRA30_AHUB_I2S_INT_SOURCE				0xe0

/* TEGRA30_AHUB_DAM_INT_SOURCE */

#define TEGRA30_AHUB_DAM_INT_SOURCE				0xe4

/* TEGRA30_AHUB_SPDIF_INT_SOURCE */

#define TEGRA30_AHUB_SPDIF_INT_SOURCE				0xec

/* TEGRA30_AHUB_APBIF_INT_SOURCE */

#define TEGRA30_AHUB_APBIF_INT_SOURCE				0xf0

/* TEGRA30_AHUB_I2S_INT_SET */

#define TEGRA30_AHUB_I2S_INT_SET				0xf8

/* TEGRA30_AHUB_DAM_INT_SET */

#define TEGRA30_AHUB_DAM_INT_SET				0xfc

/* TEGRA30_AHUB_SPDIF_INT_SET */

#define TEGRA30_AHUB_SPDIF_INT_SET				0x100

/* TEGRA30_AHUB_APBIF_INT_SET */

#define TEGRA30_AHUB_APBIF_INT_SET				0x104

/* Registers within TEGRA30_AHUB_BASE */

#define TEGRA30_AHUB_AUDIO_RX					0x0
#define TEGRA30_AHUB_AUDIO_RX_STRIDE				0x4
#define TEGRA30_AHUB_AUDIO_RX_COUNT				17
/* This register repeats once for each entry in enum tegra30_ahub_rxcif */
/* The fields in this register are 1 bit per entry in tegra30_ahub_txcif */

/*
 * Terminology:
 * AHUB: Audio Hub; a cross-bar switch between the audio devices: DMA FIFOs,
 *       I2S controllers, SPDIF controllers, and DAMs.
 * XBAR: The core cross-bar component of the AHUB.
 * CIF:  Client Interface; the HW module connecting an audio device to the
 *       XBAR.
 * DAM:  Digital Audio Mixer: A HW module that mixes multiple audio streams,
 *       possibly including sample-rate conversion.
 *
 * Each TX CIF transmits data into the XBAR. Each RX CIF can receive audio
 * transmitted by a particular TX CIF.
 *
 * This driver is currently very simplistic; many HW features are not
 * exposed; DAMs are not supported, only 16-bit stereo audio is supported,
 * etc.
 */

enum tegra30_ahub_txcif {
	TEGRA30_AHUB_TXCIF_APBIF_TX0,
	TEGRA30_AHUB_TXCIF_APBIF_TX1,
	TEGRA30_AHUB_TXCIF_APBIF_TX2,
	TEGRA30_AHUB_TXCIF_APBIF_TX3,
	TEGRA30_AHUB_TXCIF_I2S0_TX0,
	TEGRA30_AHUB_TXCIF_I2S1_TX0,
	TEGRA30_AHUB_TXCIF_I2S2_TX0,
	TEGRA30_AHUB_TXCIF_I2S3_TX0,
	TEGRA30_AHUB_TXCIF_I2S4_TX0,
	TEGRA30_AHUB_TXCIF_DAM0_TX0,
	TEGRA30_AHUB_TXCIF_DAM1_TX0,
	TEGRA30_AHUB_TXCIF_DAM2_TX0,
	TEGRA30_AHUB_TXCIF_SPDIF_TX0,
	TEGRA30_AHUB_TXCIF_SPDIF_TX1,
};

enum tegra30_ahub_rxcif {
	TEGRA30_AHUB_RXCIF_APBIF_RX0,
	TEGRA30_AHUB_RXCIF_APBIF_RX1,
	TEGRA30_AHUB_RXcIF_APBIF_RX2,
	TEGRA30_AHUB_RXCIF_APBIF_RX3,
	TEGRA30_AHUB_RXCIF_I2S0_RX0,
	TEGRA30_AHUB_RXCIF_I2S1_RX0,
	TEGRA30_AHUB_RXCIF_I2S2_RX0,
	TEGRA30_AHUB_RXCIF_I2S3_RX0,
	TEGRA30_AHUB_RXCIF_I2S4_RX0,
	TEGRA30_AHUB_RXCIF_DAM0_RX0,
	TEGRA30_AHUB_RXCIF_DAM0_RX1,
	TEGRA30_AHUB_RXCIF_DAM1_RX0,
	TEGRA30_AHUB_RXCIF_DAM2_RX1,
	TEGRA30_AHUB_RXCIF_DAM3_RX0,
	TEGRA30_AHUB_RXCIF_DAM3_RX1,
	TEGRA30_AHUB_RXCIF_SPDIF_RX0,
	TEGRA30_AHUB_RXCIF_SPDIF_RX1,
};

extern int tegra30_ahub_allocate_rx_fifo(enum tegra30_ahub_rxcif *rxcif,
					 unsigned long *fiforeg,
					 unsigned long *reqsel);
extern int tegra30_ahub_enable_rx_fifo(enum tegra30_ahub_rxcif rxcif);
extern int tegra30_ahub_disable_rx_fifo(enum tegra30_ahub_rxcif rxcif);
extern int tegra30_ahub_free_rx_fifo(enum tegra30_ahub_rxcif rxcif);

extern int tegra30_ahub_allocate_tx_fifo(enum tegra30_ahub_txcif *txcif,
					 unsigned long *fiforeg,
					 unsigned long *reqsel);
extern int tegra30_ahub_enable_tx_fifo(enum tegra30_ahub_txcif txcif);
extern int tegra30_ahub_disable_tx_fifo(enum tegra30_ahub_txcif txcif);
extern int tegra30_ahub_free_tx_fifo(enum tegra30_ahub_txcif txcif);

extern int tegra30_ahub_set_rx_cif_source(enum tegra30_ahub_rxcif rxcif,
					  enum tegra30_ahub_txcif txcif);
extern int tegra30_ahub_unset_rx_cif_source(enum tegra30_ahub_rxcif rxcif);

struct tegra30_ahub_soc_data {
	u32 clk_list_mask;
	/*
	 * FIXME: There are many more differences in HW, such as:
	 * - More APBIF channels.
	 * - Extra separate chunks of register address space to represent
	 *   the extra APBIF channels.
	 * - More units connected to the AHUB, so that tegra30_ahub_[rt]xcif
	 *   need expansion, coupled with there being more defined bits in
	 *   the AHUB routing registers.
	 * However, the driver doesn't support those new features yet, so we
	 * don't represent them here yet.
	 */
};

struct tegra30_ahub {
	const struct tegra30_ahub_soc_data *soc_data;
	struct device *dev;
	struct clk *clk_d_audio;
	struct clk *clk_apbif;
	int dma_sel;
	resource_size_t apbif_addr;
	struct regmap *regmap_apbif;
	struct regmap *regmap_ahub;
	DECLARE_BITMAP(rx_usage, TEGRA30_AHUB_CHANNEL_CTRL_COUNT);
	DECLARE_BITMAP(tx_usage, TEGRA30_AHUB_CHANNEL_CTRL_COUNT);
};

#endif
