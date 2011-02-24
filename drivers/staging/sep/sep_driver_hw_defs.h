/*
 *
 *  sep_driver_hw_defs.h - Security Processor Driver hardware definitions
 *
 *  Copyright(c) 2009,2010 Intel Corporation. All rights reserved.
 *  Contributions(c) 2009,2010 Discretix. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program; if not, write to the Free Software Foundation, Inc., 59
 *  Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  CONTACTS:
 *
 *  Mark Allyn		mark.a.allyn@intel.com
 *  Jayant Mangalampalli jayant.mangalampalli@intel.com
 *
 *  CHANGES:
 *
 *  2010.09.20	Upgrade to Medfield
 *
 */

#ifndef SEP_DRIVER_HW_DEFS__H
#define SEP_DRIVER_HW_DEFS__H

/* PCI ID's */
#define MFLD_PCI_DEVICE_ID 0x0826

/*----------------------- */
/* HW Registers Defines.  */
/*                        */
/*---------------------- -*/


/* cf registers */
#define		HW_R0B_ADDR_0_REG_ADDR			0x0000UL
#define		HW_R0B_ADDR_1_REG_ADDR			0x0004UL
#define		HW_R0B_ADDR_2_REG_ADDR			0x0008UL
#define		HW_R0B_ADDR_3_REG_ADDR			0x000cUL
#define		HW_R0B_ADDR_4_REG_ADDR			0x0010UL
#define		HW_R0B_ADDR_5_REG_ADDR			0x0014UL
#define		HW_R0B_ADDR_6_REG_ADDR			0x0018UL
#define		HW_R0B_ADDR_7_REG_ADDR			0x001cUL
#define		HW_R0B_ADDR_8_REG_ADDR			0x0020UL
#define		HW_R2B_ADDR_0_REG_ADDR			0x0080UL
#define		HW_R2B_ADDR_1_REG_ADDR			0x0084UL
#define		HW_R2B_ADDR_2_REG_ADDR			0x0088UL
#define		HW_R2B_ADDR_3_REG_ADDR			0x008cUL
#define		HW_R2B_ADDR_4_REG_ADDR			0x0090UL
#define		HW_R2B_ADDR_5_REG_ADDR			0x0094UL
#define		HW_R2B_ADDR_6_REG_ADDR			0x0098UL
#define		HW_R2B_ADDR_7_REG_ADDR			0x009cUL
#define		HW_R2B_ADDR_8_REG_ADDR			0x00a0UL
#define		HW_R3B_REG_ADDR				0x00C0UL
#define		HW_R4B_REG_ADDR				0x0100UL
#define		HW_CSA_ADDR_0_REG_ADDR			0x0140UL
#define		HW_CSA_ADDR_1_REG_ADDR			0x0144UL
#define		HW_CSA_ADDR_2_REG_ADDR			0x0148UL
#define		HW_CSA_ADDR_3_REG_ADDR			0x014cUL
#define		HW_CSA_ADDR_4_REG_ADDR			0x0150UL
#define		HW_CSA_ADDR_5_REG_ADDR			0x0154UL
#define		HW_CSA_ADDR_6_REG_ADDR			0x0158UL
#define		HW_CSA_ADDR_7_REG_ADDR			0x015cUL
#define		HW_CSA_ADDR_8_REG_ADDR			0x0160UL
#define		HW_CSA_REG_ADDR				0x0140UL
#define		HW_SINB_REG_ADDR			0x0180UL
#define		HW_SOUTB_REG_ADDR			0x0184UL
#define		HW_PKI_CONTROL_REG_ADDR			0x01C0UL
#define		HW_PKI_STATUS_REG_ADDR			0x01C4UL
#define		HW_PKI_BUSY_REG_ADDR			0x01C8UL
#define		HW_PKI_A_1025_REG_ADDR			0x01CCUL
#define		HW_PKI_SDMA_CTL_REG_ADDR		0x01D0UL
#define		HW_PKI_SDMA_OFFSET_REG_ADDR		0x01D4UL
#define		HW_PKI_SDMA_POINTERS_REG_ADDR		0x01D8UL
#define		HW_PKI_SDMA_DLENG_REG_ADDR		0x01DCUL
#define		HW_PKI_SDMA_EXP_POINTERS_REG_ADDR	0x01E0UL
#define		HW_PKI_SDMA_RES_POINTERS_REG_ADDR	0x01E4UL
#define		HW_PKI_CLR_REG_ADDR			0x01E8UL
#define		HW_PKI_SDMA_BUSY_REG_ADDR		0x01E8UL
#define		HW_PKI_SDMA_FIRST_EXP_N_REG_ADDR	0x01ECUL
#define		HW_PKI_SDMA_MUL_BY1_REG_ADDR		0x01F0UL
#define		HW_PKI_SDMA_RMUL_SEL_REG_ADDR		0x01F4UL
#define		HW_DES_KEY_0_REG_ADDR			0x0208UL
#define		HW_DES_KEY_1_REG_ADDR			0x020CUL
#define		HW_DES_KEY_2_REG_ADDR			0x0210UL
#define		HW_DES_KEY_3_REG_ADDR			0x0214UL
#define		HW_DES_KEY_4_REG_ADDR			0x0218UL
#define		HW_DES_KEY_5_REG_ADDR			0x021CUL
#define		HW_DES_CONTROL_0_REG_ADDR		0x0220UL
#define		HW_DES_CONTROL_1_REG_ADDR		0x0224UL
#define		HW_DES_IV_0_REG_ADDR			0x0228UL
#define		HW_DES_IV_1_REG_ADDR			0x022CUL
#define		HW_AES_KEY_0_ADDR_0_REG_ADDR		0x0400UL
#define		HW_AES_KEY_0_ADDR_1_REG_ADDR		0x0404UL
#define		HW_AES_KEY_0_ADDR_2_REG_ADDR		0x0408UL
#define		HW_AES_KEY_0_ADDR_3_REG_ADDR		0x040cUL
#define		HW_AES_KEY_0_ADDR_4_REG_ADDR		0x0410UL
#define		HW_AES_KEY_0_ADDR_5_REG_ADDR		0x0414UL
#define		HW_AES_KEY_0_ADDR_6_REG_ADDR		0x0418UL
#define		HW_AES_KEY_0_ADDR_7_REG_ADDR		0x041cUL
#define		HW_AES_KEY_0_REG_ADDR			0x0400UL
#define		HW_AES_IV_0_ADDR_0_REG_ADDR		0x0440UL
#define		HW_AES_IV_0_ADDR_1_REG_ADDR		0x0444UL
#define		HW_AES_IV_0_ADDR_2_REG_ADDR		0x0448UL
#define		HW_AES_IV_0_ADDR_3_REG_ADDR		0x044cUL
#define		HW_AES_IV_0_REG_ADDR			0x0440UL
#define		HW_AES_CTR1_ADDR_0_REG_ADDR		0x0460UL
#define		HW_AES_CTR1_ADDR_1_REG_ADDR		0x0464UL
#define		HW_AES_CTR1_ADDR_2_REG_ADDR		0x0468UL
#define		HW_AES_CTR1_ADDR_3_REG_ADDR		0x046cUL
#define		HW_AES_CTR1_REG_ADDR			0x0460UL
#define		HW_AES_SK_REG_ADDR			0x0478UL
#define		HW_AES_MAC_OK_REG_ADDR			0x0480UL
#define		HW_AES_PREV_IV_0_ADDR_0_REG_ADDR	0x0490UL
#define		HW_AES_PREV_IV_0_ADDR_1_REG_ADDR	0x0494UL
#define		HW_AES_PREV_IV_0_ADDR_2_REG_ADDR	0x0498UL
#define		HW_AES_PREV_IV_0_ADDR_3_REG_ADDR	0x049cUL
#define		HW_AES_PREV_IV_0_REG_ADDR		0x0490UL
#define		HW_AES_CONTROL_REG_ADDR			0x04C0UL
#define		HW_HASH_H0_REG_ADDR			0x0640UL
#define		HW_HASH_H1_REG_ADDR			0x0644UL
#define		HW_HASH_H2_REG_ADDR			0x0648UL
#define		HW_HASH_H3_REG_ADDR			0x064CUL
#define		HW_HASH_H4_REG_ADDR			0x0650UL
#define		HW_HASH_H5_REG_ADDR			0x0654UL
#define		HW_HASH_H6_REG_ADDR			0x0658UL
#define		HW_HASH_H7_REG_ADDR			0x065CUL
#define		HW_HASH_H8_REG_ADDR			0x0660UL
#define		HW_HASH_H9_REG_ADDR			0x0664UL
#define		HW_HASH_H10_REG_ADDR			0x0668UL
#define		HW_HASH_H11_REG_ADDR			0x066CUL
#define		HW_HASH_H12_REG_ADDR			0x0670UL
#define		HW_HASH_H13_REG_ADDR			0x0674UL
#define		HW_HASH_H14_REG_ADDR			0x0678UL
#define		HW_HASH_H15_REG_ADDR			0x067CUL
#define		HW_HASH_CONTROL_REG_ADDR		0x07C0UL
#define		HW_HASH_PAD_EN_REG_ADDR			0x07C4UL
#define		HW_HASH_PAD_CFG_REG_ADDR		0x07C8UL
#define		HW_HASH_CUR_LEN_0_REG_ADDR		0x07CCUL
#define		HW_HASH_CUR_LEN_1_REG_ADDR		0x07D0UL
#define		HW_HASH_CUR_LEN_2_REG_ADDR		0x07D4UL
#define		HW_HASH_CUR_LEN_3_REG_ADDR		0x07D8UL
#define		HW_HASH_PARAM_REG_ADDR			0x07DCUL
#define		HW_HASH_INT_BUSY_REG_ADDR		0x07E0UL
#define		HW_HASH_SW_RESET_REG_ADDR		0x07E4UL
#define		HW_HASH_ENDIANESS_REG_ADDR		0x07E8UL
#define		HW_HASH_DATA_REG_ADDR			0x07ECUL
#define		HW_DRNG_CONTROL_REG_ADDR		0x0800UL
#define		HW_DRNG_VALID_REG_ADDR			0x0804UL
#define		HW_DRNG_DATA_REG_ADDR			0x0808UL
#define		HW_RND_SRC_EN_REG_ADDR			0x080CUL
#define		HW_AES_CLK_ENABLE_REG_ADDR		0x0810UL
#define		HW_DES_CLK_ENABLE_REG_ADDR		0x0814UL
#define		HW_HASH_CLK_ENABLE_REG_ADDR		0x0818UL
#define		HW_PKI_CLK_ENABLE_REG_ADDR		0x081CUL
#define		HW_CLK_STATUS_REG_ADDR			0x0824UL
#define		HW_CLK_ENABLE_REG_ADDR			0x0828UL
#define		HW_DRNG_SAMPLE_REG_ADDR			0x0850UL
#define		HW_RND_SRC_CTL_REG_ADDR			0x0858UL
#define		HW_CRYPTO_CTL_REG_ADDR			0x0900UL
#define		HW_CRYPTO_STATUS_REG_ADDR		0x090CUL
#define		HW_CRYPTO_BUSY_REG_ADDR			0x0910UL
#define		HW_AES_BUSY_REG_ADDR			0x0914UL
#define		HW_DES_BUSY_REG_ADDR			0x0918UL
#define		HW_HASH_BUSY_REG_ADDR			0x091CUL
#define		HW_CONTENT_REG_ADDR			0x0924UL
#define		HW_VERSION_REG_ADDR			0x0928UL
#define		HW_CONTEXT_ID_REG_ADDR			0x0930UL
#define		HW_DIN_BUFFER_REG_ADDR			0x0C00UL
#define		HW_DIN_MEM_DMA_BUSY_REG_ADDR		0x0c20UL
#define		HW_SRC_LLI_MEM_ADDR_REG_ADDR		0x0c24UL
#define		HW_SRC_LLI_WORD0_REG_ADDR		0x0C28UL
#define		HW_SRC_LLI_WORD1_REG_ADDR		0x0C2CUL
#define		HW_SRAM_SRC_ADDR_REG_ADDR		0x0c30UL
#define		HW_DIN_SRAM_BYTES_LEN_REG_ADDR		0x0c34UL
#define		HW_DIN_SRAM_DMA_BUSY_REG_ADDR		0x0C38UL
#define		HW_WRITE_ALIGN_REG_ADDR			0x0C3CUL
#define		HW_OLD_DATA_REG_ADDR			0x0C48UL
#define		HW_WRITE_ALIGN_LAST_REG_ADDR		0x0C4CUL
#define		HW_DOUT_BUFFER_REG_ADDR			0x0C00UL
#define		HW_DST_LLI_WORD0_REG_ADDR		0x0D28UL
#define		HW_DST_LLI_WORD1_REG_ADDR		0x0D2CUL
#define		HW_DST_LLI_MEM_ADDR_REG_ADDR		0x0D24UL
#define		HW_DOUT_MEM_DMA_BUSY_REG_ADDR		0x0D20UL
#define		HW_SRAM_DEST_ADDR_REG_ADDR		0x0D30UL
#define		HW_DOUT_SRAM_BYTES_LEN_REG_ADDR		0x0D34UL
#define		HW_DOUT_SRAM_DMA_BUSY_REG_ADDR		0x0D38UL
#define		HW_READ_ALIGN_REG_ADDR			0x0D3CUL
#define		HW_READ_LAST_DATA_REG_ADDR		0x0D44UL
#define		HW_RC4_THRU_CPU_REG_ADDR		0x0D4CUL
#define		HW_AHB_SINGLE_REG_ADDR			0x0E00UL
#define		HW_SRAM_DATA_REG_ADDR			0x0F00UL
#define		HW_SRAM_ADDR_REG_ADDR			0x0F04UL
#define		HW_SRAM_DATA_READY_REG_ADDR		0x0F08UL
#define		HW_HOST_IRR_REG_ADDR			0x0A00UL
#define		HW_HOST_IMR_REG_ADDR			0x0A04UL
#define		HW_HOST_ICR_REG_ADDR			0x0A08UL
#define		HW_HOST_SEP_SRAM_THRESHOLD_REG_ADDR	0x0A10UL
#define		HW_HOST_SEP_BUSY_REG_ADDR		0x0A14UL
#define		HW_HOST_SEP_LCS_REG_ADDR		0x0A18UL
#define		HW_HOST_CC_SW_RST_REG_ADDR		0x0A40UL
#define		HW_HOST_SEP_SW_RST_REG_ADDR		0x0A44UL
#define		HW_HOST_FLOW_DMA_SW_INT0_REG_ADDR	0x0A80UL
#define		HW_HOST_FLOW_DMA_SW_INT1_REG_ADDR	0x0A84UL
#define		HW_HOST_FLOW_DMA_SW_INT2_REG_ADDR	0x0A88UL
#define		HW_HOST_FLOW_DMA_SW_INT3_REG_ADDR	0x0A8cUL
#define		HW_HOST_FLOW_DMA_SW_INT4_REG_ADDR	0x0A90UL
#define		HW_HOST_FLOW_DMA_SW_INT5_REG_ADDR	0x0A94UL
#define		HW_HOST_FLOW_DMA_SW_INT6_REG_ADDR	0x0A98UL
#define		HW_HOST_FLOW_DMA_SW_INT7_REG_ADDR	0x0A9cUL
#define		HW_HOST_SEP_HOST_GPR0_REG_ADDR		0x0B00UL
#define		HW_HOST_SEP_HOST_GPR1_REG_ADDR		0x0B04UL
#define		HW_HOST_SEP_HOST_GPR2_REG_ADDR		0x0B08UL
#define		HW_HOST_SEP_HOST_GPR3_REG_ADDR		0x0B0CUL
#define		HW_HOST_HOST_SEP_GPR0_REG_ADDR		0x0B80UL
#define		HW_HOST_HOST_SEP_GPR1_REG_ADDR		0x0B84UL
#define		HW_HOST_HOST_SEP_GPR2_REG_ADDR		0x0B88UL
#define		HW_HOST_HOST_SEP_GPR3_REG_ADDR		0x0B8CUL
#define		HW_HOST_HOST_ENDIAN_REG_ADDR		0x0B90UL
#define		HW_HOST_HOST_COMM_CLK_EN_REG_ADDR	0x0B94UL
#define		HW_CLR_SRAM_BUSY_REG_REG_ADDR		0x0F0CUL
#define		HW_CC_SRAM_BASE_ADDRESS			0x5800UL

#endif		/* ifndef HW_DEFS */
