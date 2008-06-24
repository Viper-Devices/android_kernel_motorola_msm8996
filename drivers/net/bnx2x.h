/* bnx2x.h: Broadcom Everest network driver.
 *
 * Copyright (c) 2007-2008 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 *
 * Maintained by: Eilon Greenstein <eilong@broadcom.com>
 * Written by: Eliezer Tamir
 * Based on code from Michael Chan's bnx2 driver
 */

#ifndef BNX2X_H
#define BNX2X_H

/* compilation time flags */

/* define this to make the driver freeze on error to allow getting debug info
 * (you will need to reboot afterwards) */
/* #define BNX2X_STOP_ON_ERROR */

/* error/debug prints */

#define DRV_MODULE_NAME		"bnx2x"
#define PFX DRV_MODULE_NAME	": "

/* for messages that are currently off */
#define BNX2X_MSG_OFF			0
#define BNX2X_MSG_MCP			0x010000 /* was: NETIF_MSG_HW */
#define BNX2X_MSG_STATS			0x020000 /* was: NETIF_MSG_TIMER */
#define BNX2X_MSG_NVM			0x040000 /* was: NETIF_MSG_HW */
#define BNX2X_MSG_DMAE			0x080000 /* was: NETIF_MSG_HW */
#define BNX2X_MSG_SP			0x100000 /* was: NETIF_MSG_INTR */
#define BNX2X_MSG_FP			0x200000 /* was: NETIF_MSG_INTR */

#define DP_LEVEL			KERN_NOTICE	/* was: KERN_DEBUG */

/* regular debug print */
#define DP(__mask, __fmt, __args...) do { \
	if (bp->msglevel & (__mask)) \
		printk(DP_LEVEL "[%s:%d(%s)]" __fmt, __func__, __LINE__, \
			bp->dev?(bp->dev->name):"?", ##__args); \
	} while (0)

/* errors debug print */
#define BNX2X_DBG_ERR(__fmt, __args...) do { \
	if (bp->msglevel & NETIF_MSG_PROBE) \
		printk(KERN_ERR "[%s:%d(%s)]" __fmt, __func__, __LINE__, \
			bp->dev?(bp->dev->name):"?", ##__args); \
	} while (0)

/* for errors (never masked) */
#define BNX2X_ERR(__fmt, __args...) do { \
	printk(KERN_ERR "[%s:%d(%s)]" __fmt, __func__, __LINE__, \
		bp->dev?(bp->dev->name):"?", ##__args); \
	} while (0)

/* before we have a dev->name use dev_info() */
#define BNX2X_DEV_INFO(__fmt, __args...) do { \
	if (bp->msglevel & NETIF_MSG_PROBE) \
		dev_info(&bp->pdev->dev, __fmt, ##__args); \
	} while (0)


#ifdef BNX2X_STOP_ON_ERROR
#define bnx2x_panic() do { \
		bp->panic = 1; \
		BNX2X_ERR("driver assert\n"); \
		bnx2x_int_disable(bp); \
		bnx2x_panic_dump(bp); \
	} while (0)
#else
#define bnx2x_panic() do { \
		BNX2X_ERR("driver assert\n"); \
		bnx2x_panic_dump(bp); \
	} while (0)
#endif


#ifdef NETIF_F_HW_VLAN_TX
#define BCM_VLAN			1
#endif


#define U64_LO(x)			(u32)(((u64)(x)) & 0xffffffff)
#define U64_HI(x)			(u32)(((u64)(x)) >> 32)
#define HILO_U64(hi, lo)		((((u64)(hi)) << 32) + (lo))


#define REG_ADDR(bp, offset)		(bp->regview + offset)

#define REG_RD(bp, offset)		readl(REG_ADDR(bp, offset))
#define REG_RD8(bp, offset)		readb(REG_ADDR(bp, offset))
#define REG_RD64(bp, offset)		readq(REG_ADDR(bp, offset))

#define REG_WR(bp, offset, val)		writel((u32)val, REG_ADDR(bp, offset))
#define REG_WR8(bp, offset, val)	writeb((u8)val, REG_ADDR(bp, offset))
#define REG_WR16(bp, offset, val)	writew((u16)val, REG_ADDR(bp, offset))
#define REG_WR32(bp, offset, val)	REG_WR(bp, offset, val)

#define REG_RD_IND(bp, offset)		bnx2x_reg_rd_ind(bp, offset)
#define REG_WR_IND(bp, offset, val)	bnx2x_reg_wr_ind(bp, offset, val)

#define REG_RD_DMAE(bp, offset, valp, len32) \
	do { \
		bnx2x_read_dmae(bp, offset, len32);\
		memcpy(valp, bnx2x_sp(bp, wb_data[0]), len32 * 4); \
	} while (0)

#define REG_WR_DMAE(bp, offset, valp, len32) \
	do { \
		memcpy(bnx2x_sp(bp, wb_data[0]), valp, len32 * 4); \
		bnx2x_write_dmae(bp, bnx2x_sp_mapping(bp, wb_data), \
				 offset, len32); \
	} while (0)

#define SHMEM_ADDR(bp, field)		(bp->common.shmem_base + \
					 offsetof(struct shmem_region, field))
#define SHMEM_RD(bp, field)		REG_RD(bp, SHMEM_ADDR(bp, field))
#define SHMEM_WR(bp, field, val)	REG_WR(bp, SHMEM_ADDR(bp, field), val)

#define NIG_WR(reg, val)	REG_WR(bp, reg, val)
#define EMAC_WR(reg, val)	REG_WR(bp, emac_base + reg, val)
#define BMAC_WR(reg, val)	REG_WR(bp, GRCBASE_NIG + bmac_addr + reg, val)


#define for_each_queue(bp, var)	for (var = 0; var < bp->num_queues; var++)

#define for_each_nondefault_queue(bp, var) \
				for (var = 1; var < bp->num_queues; var++)
#define is_multi(bp)		(bp->num_queues > 1)


struct regp {
	u32 lo;
	u32 hi;
};

struct bmac_stats {
	struct regp tx_gtpkt;
	struct regp tx_gtxpf;
	struct regp tx_gtfcs;
	struct regp tx_gtmca;
	struct regp tx_gtgca;
	struct regp tx_gtfrg;
	struct regp tx_gtovr;
	struct regp tx_gt64;
	struct regp tx_gt127;
	struct regp tx_gt255;   /* 10 */
	struct regp tx_gt511;
	struct regp tx_gt1023;
	struct regp tx_gt1518;
	struct regp tx_gt2047;
	struct regp tx_gt4095;
	struct regp tx_gt9216;
	struct regp tx_gt16383;
	struct regp tx_gtmax;
	struct regp tx_gtufl;
	struct regp tx_gterr;   /* 20 */
	struct regp tx_gtbyt;

	struct regp rx_gr64;
	struct regp rx_gr127;
	struct regp rx_gr255;
	struct regp rx_gr511;
	struct regp rx_gr1023;
	struct regp rx_gr1518;
	struct regp rx_gr2047;
	struct regp rx_gr4095;
	struct regp rx_gr9216;  /* 30 */
	struct regp rx_gr16383;
	struct regp rx_grmax;
	struct regp rx_grpkt;
	struct regp rx_grfcs;
	struct regp rx_grmca;
	struct regp rx_grbca;
	struct regp rx_grxcf;
	struct regp rx_grxpf;
	struct regp rx_grxuo;
	struct regp rx_grjbr;   /* 40 */
	struct regp rx_grovr;
	struct regp rx_grflr;
	struct regp rx_grmeg;
	struct regp rx_grmeb;
	struct regp rx_grbyt;
	struct regp rx_grund;
	struct regp rx_grfrg;
	struct regp rx_grerb;
	struct regp rx_grfre;
	struct regp rx_gripj;   /* 50 */
};

struct emac_stats {
	u32 rx_ifhcinoctets     		   ;
	u32 rx_ifhcinbadoctets  		   ;
	u32 rx_etherstatsfragments      	   ;
	u32 rx_ifhcinucastpkts  		   ;
	u32 rx_ifhcinmulticastpkts      	   ;
	u32 rx_ifhcinbroadcastpkts      	   ;
	u32 rx_dot3statsfcserrors       	   ;
	u32 rx_dot3statsalignmenterrors 	   ;
	u32 rx_dot3statscarriersenseerrors         ;
	u32 rx_xonpauseframesreceived   	   ;    /* 10 */
	u32 rx_xoffpauseframesreceived  	   ;
	u32 rx_maccontrolframesreceived 	   ;
	u32 rx_xoffstateentered 		   ;
	u32 rx_dot3statsframestoolong   	   ;
	u32 rx_etherstatsjabbers		   ;
	u32 rx_etherstatsundersizepkts  	   ;
	u32 rx_etherstatspkts64octets   	   ;
	u32 rx_etherstatspkts65octetsto127octets   ;
	u32 rx_etherstatspkts128octetsto255octets  ;
	u32 rx_etherstatspkts256octetsto511octets  ;    /* 20 */
	u32 rx_etherstatspkts512octetsto1023octets ;
	u32 rx_etherstatspkts1024octetsto1522octets;
	u32 rx_etherstatspktsover1522octets        ;

	u32 rx_falsecarriererrors       	   ;

	u32 tx_ifhcoutoctets    		   ;
	u32 tx_ifhcoutbadoctets 		   ;
	u32 tx_etherstatscollisions     	   ;
	u32 tx_outxonsent       		   ;
	u32 tx_outxoffsent      		   ;
	u32 tx_flowcontroldone  		   ;    /* 30 */
	u32 tx_dot3statssinglecollisionframes      ;
	u32 tx_dot3statsmultiplecollisionframes    ;
	u32 tx_dot3statsdeferredtransmissions      ;
	u32 tx_dot3statsexcessivecollisions        ;
	u32 tx_dot3statslatecollisions  	   ;
	u32 tx_ifhcoutucastpkts 		   ;
	u32 tx_ifhcoutmulticastpkts     	   ;
	u32 tx_ifhcoutbroadcastpkts     	   ;
	u32 tx_etherstatspkts64octets   	   ;
	u32 tx_etherstatspkts65octetsto127octets   ;    /* 40 */
	u32 tx_etherstatspkts128octetsto255octets  ;
	u32 tx_etherstatspkts256octetsto511octets  ;
	u32 tx_etherstatspkts512octetsto1023octets ;
	u32 tx_etherstatspkts1024octetsto1522octet ;
	u32 tx_etherstatspktsover1522octets        ;
	u32 tx_dot3statsinternalmactransmiterrors  ;    /* 46 */
};

union mac_stats {
	struct emac_stats emac;
	struct bmac_stats bmac;
};

struct nig_stats {
	u32 brb_discard;
	u32 brb_packet;
	u32 brb_truncate;
	u32 flow_ctrl_discard;
	u32 flow_ctrl_octets;
	u32 flow_ctrl_packet;
	u32 mng_discard;
	u32 mng_octet_inp;
	u32 mng_octet_out;
	u32 mng_packet_inp;
	u32 mng_packet_out;
	u32 pbf_octets;
	u32 pbf_packet;
	u32 safc_inp;
	u32 done;
	u32 pad;
};

struct bnx2x_eth_stats {
	u32 pad;	/* to make long counters u64 aligned */
	u32 mac_stx_start;
	u32 total_bytes_received_hi;
	u32 total_bytes_received_lo;
	u32 total_bytes_transmitted_hi;
	u32 total_bytes_transmitted_lo;
	u32 total_unicast_packets_received_hi;
	u32 total_unicast_packets_received_lo;
	u32 total_multicast_packets_received_hi;
	u32 total_multicast_packets_received_lo;
	u32 total_broadcast_packets_received_hi;
	u32 total_broadcast_packets_received_lo;
	u32 total_unicast_packets_transmitted_hi;
	u32 total_unicast_packets_transmitted_lo;
	u32 total_multicast_packets_transmitted_hi;
	u32 total_multicast_packets_transmitted_lo;
	u32 total_broadcast_packets_transmitted_hi;
	u32 total_broadcast_packets_transmitted_lo;
	u32 crc_receive_errors;
	u32 alignment_errors;
	u32 false_carrier_detections;
	u32 runt_packets_received;
	u32 jabber_packets_received;
	u32 pause_xon_frames_received;
	u32 pause_xoff_frames_received;
	u32 pause_xon_frames_transmitted;
	u32 pause_xoff_frames_transmitted;
	u32 single_collision_transmit_frames;
	u32 multiple_collision_transmit_frames;
	u32 late_collision_frames;
	u32 excessive_collision_frames;
	u32 control_frames_received;
	u32 frames_received_64_bytes;
	u32 frames_received_65_127_bytes;
	u32 frames_received_128_255_bytes;
	u32 frames_received_256_511_bytes;
	u32 frames_received_512_1023_bytes;
	u32 frames_received_1024_1522_bytes;
	u32 frames_received_1523_9022_bytes;
	u32 frames_transmitted_64_bytes;
	u32 frames_transmitted_65_127_bytes;
	u32 frames_transmitted_128_255_bytes;
	u32 frames_transmitted_256_511_bytes;
	u32 frames_transmitted_512_1023_bytes;
	u32 frames_transmitted_1024_1522_bytes;
	u32 frames_transmitted_1523_9022_bytes;
	u32 valid_bytes_received_hi;
	u32 valid_bytes_received_lo;
	u32 error_runt_packets_received;
	u32 error_jabber_packets_received;
	u32 mac_stx_end;

	u32 pad2;
	u32 stat_IfHCInBadOctets_hi;
	u32 stat_IfHCInBadOctets_lo;
	u32 stat_IfHCOutBadOctets_hi;
	u32 stat_IfHCOutBadOctets_lo;
	u32 stat_Dot3statsFramesTooLong;
	u32 stat_Dot3statsInternalMacTransmitErrors;
	u32 stat_Dot3StatsCarrierSenseErrors;
	u32 stat_Dot3StatsDeferredTransmissions;
	u32 stat_FlowControlDone;
	u32 stat_XoffStateEntered;

	u32 x_total_sent_bytes_hi;
	u32 x_total_sent_bytes_lo;
	u32 x_total_sent_pkts;

	u32 t_rcv_unicast_bytes_hi;
	u32 t_rcv_unicast_bytes_lo;
	u32 t_rcv_broadcast_bytes_hi;
	u32 t_rcv_broadcast_bytes_lo;
	u32 t_rcv_multicast_bytes_hi;
	u32 t_rcv_multicast_bytes_lo;
	u32 t_total_rcv_pkt;

	u32 checksum_discard;
	u32 packets_too_big_discard;
	u32 no_buff_discard;
	u32 ttl0_discard;
	u32 mac_discard;
	u32 mac_filter_discard;
	u32 xxoverflow_discard;
	u32 brb_truncate_discard;

	u32 brb_discard;
	u32 brb_packet;
	u32 brb_truncate;
	u32 flow_ctrl_discard;
	u32 flow_ctrl_octets;
	u32 flow_ctrl_packet;
	u32 mng_discard;
	u32 mng_octet_inp;
	u32 mng_octet_out;
	u32 mng_packet_inp;
	u32 mng_packet_out;
	u32 pbf_octets;
	u32 pbf_packet;
	u32 safc_inp;
	u32 driver_xoff;
	u32 number_of_bugs_found_in_stats_spec; /* just kidding */
};

#define bnx2x_sp_check(bp, var) ((bp->slowpath) ? (&bp->slowpath->var) : NULL)
struct sw_rx_bd {
	struct sk_buff	*skb;
	DECLARE_PCI_UNMAP_ADDR(mapping)
};

struct sw_tx_bd {
	struct sk_buff	*skb;
	u16		first_bd;
};

struct bnx2x_fastpath {

	struct napi_struct	napi;

	struct host_status_block *status_blk;
	dma_addr_t		status_blk_mapping;

	struct eth_tx_db_data	*hw_tx_prods;
	dma_addr_t		tx_prods_mapping;

	struct sw_tx_bd		*tx_buf_ring;

	struct eth_tx_bd	*tx_desc_ring;
	dma_addr_t		tx_desc_mapping;

	struct sw_rx_bd 	*rx_buf_ring;

	struct eth_rx_bd	*rx_desc_ring;
	dma_addr_t		rx_desc_mapping;

	union eth_rx_cqe	*rx_comp_ring;
	dma_addr_t		rx_comp_mapping;

	int			state;
#define BNX2X_FP_STATE_CLOSED		0
#define BNX2X_FP_STATE_IRQ		0x80000
#define BNX2X_FP_STATE_OPENING		0x90000
#define BNX2X_FP_STATE_OPEN		0xa0000
#define BNX2X_FP_STATE_HALTING		0xb0000
#define BNX2X_FP_STATE_HALTED		0xc0000

	u8			index;	/* number in fp array */
	u8			cl_id;	/* eth client id */
	u8			sb_id;	/* status block number in HW */
#define FP_IDX(fp)			(fp->index)
#define FP_CL_ID(fp)			(fp->cl_id)
#define BP_CL_ID(bp)			(bp->fp[0].cl_id)
#define FP_SB_ID(fp)			(fp->sb_id)
#define CNIC_SB_ID			0

	u16			tx_pkt_prod;
	u16			tx_pkt_cons;
	u16			tx_bd_prod;
	u16			tx_bd_cons;
	u16			*tx_cons_sb;

	u16			fp_c_idx;
	u16			fp_u_idx;

	u16			rx_bd_prod;
	u16			rx_bd_cons;
	u16			rx_comp_prod;
	u16			rx_comp_cons;
	u16			*rx_cons_sb;

	unsigned long		tx_pkt,
				rx_pkt,
				rx_calls;

	struct bnx2x		*bp; /* parent */
};

#define bnx2x_fp(bp, nr, var)		(bp->fp[nr].var)
/* This is needed for determening of last_max */
#define SUB_S16(a, b)			(s16)((s16)(a) - (s16)(b))

/* stuff added to make the code fit 80Col */

#define CQE_TYPE(cqe_fp_flags)	((cqe_fp_flags) & ETH_FAST_PATH_RX_CQE_TYPE)

#define ETH_RX_ERROR_FALGS	(ETH_FAST_PATH_RX_CQE_PHY_DECODE_ERR_FLG | \
				 ETH_FAST_PATH_RX_CQE_IP_BAD_XSUM_FLG | \
				 ETH_FAST_PATH_RX_CQE_L4_BAD_XSUM_FLG)


#define U_SB_ETH_RX_CQ_INDEX		HC_INDEX_U_ETH_RX_CQ_CONS
#define U_SB_ETH_RX_BD_INDEX		HC_INDEX_U_ETH_RX_BD_CONS
#define C_SB_ETH_TX_CQ_INDEX		HC_INDEX_C_ETH_TX_CQ_CONS

#define BNX2X_RX_SB_INDEX \
	(&fp->status_blk->u_status_block.index_values[U_SB_ETH_RX_CQ_INDEX])

#define BNX2X_RX_SB_BD_INDEX \
	(&fp->status_blk->u_status_block.index_values[U_SB_ETH_RX_BD_INDEX])

#define BNX2X_RX_SB_INDEX_NUM \
		(((U_SB_ETH_RX_CQ_INDEX << \
		   USTORM_ETH_ST_CONTEXT_CONFIG_CQE_SB_INDEX_NUMBER_SHIFT) & \
		  USTORM_ETH_ST_CONTEXT_CONFIG_CQE_SB_INDEX_NUMBER) | \
		 ((U_SB_ETH_RX_BD_INDEX << \
		   USTORM_ETH_ST_CONTEXT_CONFIG_BD_SB_INDEX_NUMBER_SHIFT) & \
		  USTORM_ETH_ST_CONTEXT_CONFIG_BD_SB_INDEX_NUMBER))

#define BNX2X_TX_SB_INDEX \
	(&fp->status_blk->c_status_block.index_values[C_SB_ETH_TX_CQ_INDEX])

/* common */

struct bnx2x_common {

	u32			chip_id;
/* chip num:16-31, rev:12-15, metal:4-11, bond_id:0-3 */
#define CHIP_ID(bp)			(bp->common.chip_id & 0xfffffff0)

#define CHIP_NUM(bp)			(bp->common.chip_id >> 16)
#define CHIP_NUM_57710			0x164e
#define CHIP_NUM_57711			0x164f
#define CHIP_NUM_57711E			0x1650
#define CHIP_IS_E1(bp)			(CHIP_NUM(bp) == CHIP_NUM_57710)
#define CHIP_IS_57711(bp)		(CHIP_NUM(bp) == CHIP_NUM_57711)
#define CHIP_IS_57711E(bp)		(CHIP_NUM(bp) == CHIP_NUM_57711E)
#define CHIP_IS_E1H(bp)			(CHIP_IS_57711(bp) || \
					 CHIP_IS_57711E(bp))
#define IS_E1H_OFFSET			CHIP_IS_E1H(bp)

#define CHIP_REV(bp)			(bp->common.chip_id & 0x0000f000)
#define CHIP_REV_Ax			0x00000000
/* assume maximum 5 revisions */
#define CHIP_REV_IS_SLOW(bp)		(CHIP_REV(bp) > 0x00005000)
/* Emul versions are A=>0xe, B=>0xc, C=>0xa, D=>8, E=>6 */
#define CHIP_REV_IS_EMUL(bp)		((CHIP_REV_IS_SLOW(bp)) && \
					 !(CHIP_REV(bp) & 0x00001000))
/* FPGA versions are A=>0xf, B=>0xd, C=>0xb, D=>9, E=>7 */
#define CHIP_REV_IS_FPGA(bp)		((CHIP_REV_IS_SLOW(bp)) && \
					 (CHIP_REV(bp) & 0x00001000))

#define CHIP_TIME(bp)			((CHIP_REV_IS_EMUL(bp)) ? 2000 : \
					((CHIP_REV_IS_FPGA(bp)) ? 200 : 1))

#define CHIP_METAL(bp)			(bp->common.chip_id & 0x00000ff0)
#define CHIP_BOND_ID(bp)		(bp->common.chip_id & 0x0000000f)

	int			flash_size;
#define NVRAM_1MB_SIZE			0x20000	/* 1M bit in bytes */
#define NVRAM_TIMEOUT_COUNT		30000
#define NVRAM_PAGE_SIZE			256

	u32			shmem_base;

	u32			hw_config;
	u32			board;

	u32			bc_ver;

	char			*name;
};


/* end of common */

/* port */

struct bnx2x_port {
	u32			pmf;

	u32			link_config;

	u32			supported;
/* link settings - missing defines */
#define SUPPORTED_2500baseX_Full	(1 << 15)

	u32			advertising;
/* link settings - missing defines */
#define ADVERTISED_2500baseX_Full	(1 << 15)

	u32			phy_addr;

	/* used to synchronize phy accesses */
	struct mutex		phy_mutex;

	u32			port_stx;

	struct nig_stats	old_nig_stats;
};

/* end of port */

#define MAC_STX_NA      		0xffffffff

#ifdef BNX2X_MULTI
#define MAX_CONTEXT			16
#else
#define MAX_CONTEXT			1
#endif

union cdu_context {
	struct eth_context eth;
	char pad[1024];
};

#define MAX_DMAE_C			6

/* DMA memory not used in fastpath */
struct bnx2x_slowpath {
	union cdu_context		context[MAX_CONTEXT];
	struct eth_stats_query		fw_stats;
	struct mac_configuration_cmd	mac_config;
	struct mac_configuration_cmd	mcast_config;

	/* used by dmae command executer */
	struct dmae_command		dmae[MAX_DMAE_C];

	union mac_stats 		mac_stats;
	struct nig_stats		nig;
	struct bnx2x_eth_stats  	eth_stats;

	u32				wb_comp;
#define BNX2X_WB_COMP_VAL       	0xe0d0d0ae
	u32				wb_data[4];
};

#define bnx2x_sp(bp, var)		(&bp->slowpath->var)
#define bnx2x_sp_mapping(bp, var) \
		(bp->slowpath_mapping + offsetof(struct bnx2x_slowpath, var))


/* attn group wiring */
#define MAX_DYNAMIC_ATTN_GRPS		8

struct attn_route {
	u32	sig[4];
};

struct bnx2x {
	/* Fields used in the tx and intr/napi performance paths
	 * are grouped together in the beginning of the structure
	 */
	struct bnx2x_fastpath	fp[MAX_CONTEXT];
	void __iomem		*regview;
	void __iomem		*doorbells;
#define BNX2X_DB_SIZE		(16*2048)

	struct net_device	*dev;
	struct pci_dev		*pdev;

	atomic_t		intr_sem;
	struct msix_entry       msix_table[MAX_CONTEXT+1];

	int			tx_ring_size;

#ifdef BCM_VLAN
	struct vlan_group	*vlgrp;
#endif

	u32			rx_csum;
	u32			rx_offset;
	u32			rx_buf_use_size;	/* useable size */
	u32			rx_buf_size;		/* with alignment */
#define ETH_OVREHEAD			(ETH_HLEN + 8)	/* 8 for CRC + VLAN */
#define ETH_MIN_PACKET_SIZE		60
#define ETH_MAX_PACKET_SIZE		1500
#define ETH_MAX_JUMBO_PACKET_SIZE	9600

	struct host_def_status_block *def_status_blk;
#define DEF_SB_ID			16
	u16			def_c_idx;
	u16			def_u_idx;
	u16			def_x_idx;
	u16			def_t_idx;
	u16			def_att_idx;
	u32			attn_state;
	struct attn_route	attn_group[MAX_DYNAMIC_ATTN_GRPS];
	u32			aeu_mask;
	u32			nig_mask;

	/* slow path ring */
	struct eth_spe		*spq;
	dma_addr_t		spq_mapping;
	u16			spq_prod_idx;
	struct eth_spe		*spq_prod_bd;
	struct eth_spe		*spq_last_bd;
	u16			*dsb_sp_prod;
	u16			spq_left; /* serialize spq */
	/* used to synchronize spq accesses */
	spinlock_t		spq_lock;

	/* Flag for marking that there is either
	 * STAT_QUERY or CFC DELETE ramrod pending
	 */
	u8      		stat_pending;

	/* End of fileds used in the performance code paths */

	int			panic;
	int			msglevel;

	u32			flags;
#define PCIX_FLAG			1
#define PCI_32BIT_FLAG			2
#define ONE_TDMA_FLAG			4	/* no longer used */
#define NO_WOL_FLAG			8
#define USING_DAC_FLAG			0x10
#define USING_MSIX_FLAG			0x20
#define ASF_ENABLE_FLAG			0x40
#define NO_MCP_FLAG			0x100
#define BP_NOMCP(bp)			(bp->flags & NO_MCP_FLAG)

	int			func;
#define BP_PORT(bp)			(bp->func % PORT_MAX)
#define BP_FUNC(bp)			(bp->func)
#define BP_E1HVN(bp)			(bp->func >> 1)
#define BP_L_ID(bp)			(BP_E1HVN(bp) << 2)
/* assorted E1HVN */
#define IS_E1HMF(bp)			(bp->e1hmf != 0)
#define BP_MAX_QUEUES(bp)		(IS_E1HMF(bp) ? 4 : 16)

	int			pm_cap;
	int			pcie_cap;

	struct work_struct	sp_task;
	struct work_struct	reset_task;

	struct timer_list	timer;
	int			timer_interval;
	int			current_interval;

	u16			fw_seq;
	u16			fw_drv_pulse_wr_seq;
	u32			func_stx;

	struct link_params	link_params;
	struct link_vars	link_vars;

	struct bnx2x_common	common;
	struct bnx2x_port	port;

	u32			mf_config;
	u16			e1hov;
	u8			e1hmf;

	u8			wol;

	int			rx_ring_size;

	u16			tx_quick_cons_trip_int;
	u16			tx_quick_cons_trip;
	u16			tx_ticks_int;
	u16			tx_ticks;

	u16			rx_quick_cons_trip_int;
	u16			rx_quick_cons_trip;
	u16			rx_ticks_int;
	u16			rx_ticks;

	u32			stats_ticks;
	u32			lin_cnt;

	int			state;
#define BNX2X_STATE_CLOSED		0x0
#define BNX2X_STATE_OPENING_WAIT4_LOAD	0x1000
#define BNX2X_STATE_OPENING_WAIT4_PORT	0x2000
#define BNX2X_STATE_OPEN		0x3000
#define BNX2X_STATE_CLOSING_WAIT4_HALT	0x4000
#define BNX2X_STATE_CLOSING_WAIT4_DELETE 0x5000
#define BNX2X_STATE_CLOSING_WAIT4_UNLOAD 0x6000
#define BNX2X_STATE_DISABLED		0xd000
#define BNX2X_STATE_DIAG		0xe000
#define BNX2X_STATE_ERROR		0xf000

	int			num_queues;

	u32			rx_mode;
#define BNX2X_RX_MODE_NONE		0
#define BNX2X_RX_MODE_NORMAL		1
#define BNX2X_RX_MODE_ALLMULTI		2
#define BNX2X_RX_MODE_PROMISC		3
#define BNX2X_MAX_MULTICAST		64
#define BNX2X_MAX_EMUL_MULTI		16

	dma_addr_t		def_status_blk_mapping;

	struct bnx2x_slowpath	*slowpath;
	dma_addr_t		slowpath_mapping;

#ifdef BCM_ISCSI
	void    		*t1;
	dma_addr_t      	t1_mapping;
	void    		*t2;
	dma_addr_t      	t2_mapping;
	void    		*timers;
	dma_addr_t      	timers_mapping;
	void    		*qm;
	dma_addr_t      	qm_mapping;
#endif

	char    		*name;

	/* used to synchronize stats collecting */
	int     		stats_state;
#define STATS_STATE_DISABLE     	0
#define STATS_STATE_ENABLE      	1
#define STATS_STATE_STOP		2 /* stop stats on next iteration */

	/* used by dmae command loader */
	struct dmae_command     dmae;
	int     		executer_idx;

	int			dmae_ready;
	/* used to synchronize dmae accesses */
	struct mutex		dmae_mutex;
	struct dmae_command	init_dmae;



	u32     		old_brb_discard;
	struct bmac_stats       old_bmac;
	struct tstorm_per_client_stats old_tclient;
	struct z_stream_s       *strm;
	void    		*gunzip_buf;
	dma_addr_t      	gunzip_mapping;
	int     		gunzip_outlen;
#define FW_BUF_SIZE			0x8000

};


/* DMAE command defines */
#define DMAE_CMD_SRC_PCI		0
#define DMAE_CMD_SRC_GRC		DMAE_COMMAND_SRC

#define DMAE_CMD_DST_PCI		(1 << DMAE_COMMAND_DST_SHIFT)
#define DMAE_CMD_DST_GRC		(2 << DMAE_COMMAND_DST_SHIFT)

#define DMAE_CMD_C_DST_PCI      	0
#define DMAE_CMD_C_DST_GRC      	(1 << DMAE_COMMAND_C_DST_SHIFT)

#define DMAE_CMD_C_ENABLE       	DMAE_COMMAND_C_TYPE_ENABLE

#define DMAE_CMD_ENDIANITY_NO_SWAP      (0 << DMAE_COMMAND_ENDIANITY_SHIFT)
#define DMAE_CMD_ENDIANITY_B_SWAP       (1 << DMAE_COMMAND_ENDIANITY_SHIFT)
#define DMAE_CMD_ENDIANITY_DW_SWAP      (2 << DMAE_COMMAND_ENDIANITY_SHIFT)
#define DMAE_CMD_ENDIANITY_B_DW_SWAP    (3 << DMAE_COMMAND_ENDIANITY_SHIFT)

#define DMAE_CMD_PORT_0 		0
#define DMAE_CMD_PORT_1 		DMAE_COMMAND_PORT

#define DMAE_CMD_SRC_RESET      	DMAE_COMMAND_SRC_RESET
#define DMAE_CMD_DST_RESET      	DMAE_COMMAND_DST_RESET

#define DMAE_LEN32_MAX  		0x400

void bnx2x_read_dmae(struct bnx2x *bp, u32 src_addr, u32 len32);
void bnx2x_write_dmae(struct bnx2x *bp, dma_addr_t dma_addr, u32 dst_addr,
		      u32 len32);
int bnx2x_set_gpio(struct bnx2x *bp, int gpio_num, u32 mode);


/* MC hsi */
#define RX_COPY_THRESH  		92
#define BCM_PAGE_SHIFT			12
#define BCM_PAGE_SIZE			(1 << BCM_PAGE_SHIFT)
#define BCM_PAGE_MASK			(~(BCM_PAGE_SIZE - 1))
#define BCM_PAGE_ALIGN(addr)	(((addr) + BCM_PAGE_SIZE - 1) & BCM_PAGE_MASK)

#define NUM_TX_RINGS    		16
#define TX_DESC_CNT     	(BCM_PAGE_SIZE / sizeof(struct eth_tx_bd))
#define MAX_TX_DESC_CNT 		(TX_DESC_CNT - 1)
#define NUM_TX_BD       		(TX_DESC_CNT * NUM_TX_RINGS)
#define MAX_TX_BD       		(NUM_TX_BD - 1)
#define MAX_TX_AVAIL    		(MAX_TX_DESC_CNT * NUM_TX_RINGS - 2)
#define NEXT_TX_IDX(x)  	((((x) & MAX_TX_DESC_CNT) == \
				 (MAX_TX_DESC_CNT - 1)) ? (x) + 2 : (x) + 1)
#define TX_BD(x)			((x) & MAX_TX_BD)
#define TX_BD_POFF(x)   		((x) & MAX_TX_DESC_CNT)

/* The RX BD ring is special, each bd is 8 bytes but the last one is 16 */
#define NUM_RX_RINGS    		8
#define RX_DESC_CNT     	(BCM_PAGE_SIZE / sizeof(struct eth_rx_bd))
#define MAX_RX_DESC_CNT 		(RX_DESC_CNT - 2)
#define RX_DESC_MASK    		(RX_DESC_CNT - 1)
#define NUM_RX_BD       		(RX_DESC_CNT * NUM_RX_RINGS)
#define MAX_RX_BD       		(NUM_RX_BD - 1)
#define MAX_RX_AVAIL    		(MAX_RX_DESC_CNT * NUM_RX_RINGS - 2)
#define NEXT_RX_IDX(x)  	((((x) & RX_DESC_MASK) == \
				 (MAX_RX_DESC_CNT - 1)) ? (x) + 3 : (x) + 1)
#define RX_BD(x)			((x) & MAX_RX_BD)

#define NUM_RCQ_RINGS   		(NUM_RX_RINGS * 2)
#define RCQ_DESC_CNT    	(BCM_PAGE_SIZE / sizeof(union eth_rx_cqe))
#define MAX_RCQ_DESC_CNT		(RCQ_DESC_CNT - 1)
#define NUM_RCQ_BD      		(RCQ_DESC_CNT * NUM_RCQ_RINGS)
#define MAX_RCQ_BD      		(NUM_RCQ_BD - 1)
#define MAX_RCQ_AVAIL   		(MAX_RCQ_DESC_CNT * NUM_RCQ_RINGS - 2)
#define NEXT_RCQ_IDX(x) 	((((x) & MAX_RCQ_DESC_CNT) == \
				 (MAX_RCQ_DESC_CNT - 1)) ? (x) + 2 : (x) + 1)
#define RCQ_BD(x)       		((x) & MAX_RCQ_BD)


/* used on a CID received from the HW */
#define SW_CID(x)       		(le32_to_cpu(x) & \
					 (COMMON_RAMROD_ETH_RX_CQE_CID >> 1))
#define CQE_CMD(x)      		(le32_to_cpu(x) >> \
					COMMON_RAMROD_ETH_RX_CQE_CMD_ID_SHIFT)

#define BD_UNMAP_ADDR(bd)       	HILO_U64(le32_to_cpu((bd)->addr_hi), \
						 le32_to_cpu((bd)->addr_lo))
#define BD_UNMAP_LEN(bd)		(le16_to_cpu((bd)->nbytes))


#define STROM_ASSERT_ARRAY_SIZE 	50



/* must be used on a CID before placing it on a HW ring */
#define HW_CID(bp, x)		((BP_PORT(bp) << 23) | (BP_E1HVN(bp) << 17) | x)

#define SP_DESC_CNT     	(BCM_PAGE_SIZE / sizeof(struct eth_spe))
#define MAX_SP_DESC_CNT 		(SP_DESC_CNT - 1)


#define BNX2X_BTR       		3
#define MAX_SPQ_PENDING 		8


#define BNX2X_NUM_STATS			34
#define BNX2X_NUM_TESTS			1


#define DPM_TRIGER_TYPE 		0x40
#define DOORBELL(bp, cid, val) \
	do { \
		writel((u32)val, (bp)->doorbells + (BCM_PAGE_SIZE * cid) + \
		       DPM_TRIGER_TYPE); \
	} while (0)

static inline u32 reg_poll(struct bnx2x *bp, u32 reg, u32 expected, int ms,
			   int wait)
{
	u32 val;

	do {
		val = REG_RD(bp, reg);
		if (val == expected)
			break;
		ms -= wait;
		msleep(wait);

	} while (ms > 0);

	return val;
}


/* load/unload mode */
#define LOAD_NORMAL			0
#define LOAD_OPEN			1
#define LOAD_DIAG			2
#define UNLOAD_NORMAL			0
#define UNLOAD_CLOSE			1

/* DMAE command defines */
#define DMAE_CMD_SRC_PCI		0
#define DMAE_CMD_SRC_GRC		DMAE_COMMAND_SRC

#define DMAE_CMD_DST_PCI		(1 << DMAE_COMMAND_DST_SHIFT)
#define DMAE_CMD_DST_GRC		(2 << DMAE_COMMAND_DST_SHIFT)

#define DMAE_CMD_C_DST_PCI		0
#define DMAE_CMD_C_DST_GRC		(1 << DMAE_COMMAND_C_DST_SHIFT)

#define DMAE_CMD_C_ENABLE		DMAE_COMMAND_C_TYPE_ENABLE

#define DMAE_CMD_ENDIANITY_NO_SWAP	(0 << DMAE_COMMAND_ENDIANITY_SHIFT)
#define DMAE_CMD_ENDIANITY_B_SWAP	(1 << DMAE_COMMAND_ENDIANITY_SHIFT)
#define DMAE_CMD_ENDIANITY_DW_SWAP	(2 << DMAE_COMMAND_ENDIANITY_SHIFT)
#define DMAE_CMD_ENDIANITY_B_DW_SWAP	(3 << DMAE_COMMAND_ENDIANITY_SHIFT)

#define DMAE_CMD_PORT_0			0
#define DMAE_CMD_PORT_1			DMAE_COMMAND_PORT

#define DMAE_CMD_SRC_RESET		DMAE_COMMAND_SRC_RESET
#define DMAE_CMD_DST_RESET		DMAE_COMMAND_DST_RESET
#define DMAE_CMD_E1HVN_SHIFT		DMAE_COMMAND_E1HVN_SHIFT

#define DMAE_LEN32_RD_MAX		0x80
#define DMAE_LEN32_WR_MAX		0x400

#define DMAE_COMP_VAL			0xe0d0d0ae

#define MAX_DMAE_C_PER_PORT		8
#define INIT_DMAE_C(bp)			(BP_PORT(bp)*MAX_DMAE_C_PER_PORT + \
					 BP_E1HVN(bp))
#define PMF_DMAE_C(bp)			(BP_PORT(bp)*MAX_DMAE_C_PER_PORT + \
					 E1HVN_MAX)


/* PCIE link and speed */
#define PCICFG_LINK_WIDTH		0x1f00000
#define PCICFG_LINK_WIDTH_SHIFT		20
#define PCICFG_LINK_SPEED		0xf0000
#define PCICFG_LINK_SPEED_SHIFT		16

#define BMAC_CONTROL_RX_ENABLE		2

#define pbd_tcp_flags(skb)  	(ntohl(tcp_flag_word(tcp_hdr(skb)))>>16 & 0xff)

/* must be used on a CID before placing it on a HW ring */

#define BNX2X_RX_SUM_OK(cqe) \
			(!(cqe->fast_path_cqe.status_flags & \
			 (ETH_FAST_PATH_RX_CQE_IP_XSUM_NO_VALIDATION_FLG | \
			  ETH_FAST_PATH_RX_CQE_L4_XSUM_NO_VALIDATION_FLG)))

/* CMNG constants
   derived from lab experiments, and not from system spec calculations !!! */
#define DEF_MIN_RATE			100
/* resolution of the rate shaping timer - 100 usec */
#define RS_PERIODIC_TIMEOUT_USEC	100
/* resolution of fairness algorithm in usecs -
   coefficient for clauclating the actuall t fair */
#define T_FAIR_COEF			10000000
/* number of bytes in single QM arbitration cycle -
   coeffiecnt for calculating the fairness timer */
#define QM_ARB_BYTES			40000
#define FAIR_MEM			2


#define ATTN_NIG_FOR_FUNC		(1L << 8)
#define ATTN_SW_TIMER_4_FUNC		(1L << 9)
#define GPIO_2_FUNC			(1L << 10)
#define GPIO_3_FUNC			(1L << 11)
#define GPIO_4_FUNC			(1L << 12)
#define ATTN_GENERAL_ATTN_1		(1L << 13)
#define ATTN_GENERAL_ATTN_2		(1L << 14)
#define ATTN_GENERAL_ATTN_3		(1L << 15)
#define ATTN_GENERAL_ATTN_4		(1L << 13)
#define ATTN_GENERAL_ATTN_5		(1L << 14)
#define ATTN_GENERAL_ATTN_6		(1L << 15)

#define ATTN_HARD_WIRED_MASK		0xff00
#define ATTENTION_ID			4


/* stuff added to make the code fit 80Col */

#define BNX2X_PMF_LINK_ASSERT \
	GENERAL_ATTEN_OFFSET(LINK_SYNC_ATTENTION_BIT_FUNC_0 + BP_FUNC(bp))

#define BNX2X_MC_ASSERT_BITS \
	(GENERAL_ATTEN_OFFSET(TSTORM_FATAL_ASSERT_ATTENTION_BIT) | \
	 GENERAL_ATTEN_OFFSET(USTORM_FATAL_ASSERT_ATTENTION_BIT) | \
	 GENERAL_ATTEN_OFFSET(CSTORM_FATAL_ASSERT_ATTENTION_BIT) | \
	 GENERAL_ATTEN_OFFSET(XSTORM_FATAL_ASSERT_ATTENTION_BIT))

#define BNX2X_MCP_ASSERT \
	GENERAL_ATTEN_OFFSET(MCP_FATAL_ASSERT_ATTENTION_BIT)

#define BNX2X_DOORQ_ASSERT \
	AEU_INPUTS_ATTN_BITS_DOORBELLQ_HW_INTERRUPT

#define BNX2X_GRC_TIMEOUT	GENERAL_ATTEN_OFFSET(LATCHED_ATTN_TIMEOUT_GRC)
#define BNX2X_GRC_RSV		(GENERAL_ATTEN_OFFSET(LATCHED_ATTN_RBCR) | \
				 GENERAL_ATTEN_OFFSET(LATCHED_ATTN_RBCT) | \
				 GENERAL_ATTEN_OFFSET(LATCHED_ATTN_RBCN) | \
				 GENERAL_ATTEN_OFFSET(LATCHED_ATTN_RBCU) | \
				 GENERAL_ATTEN_OFFSET(LATCHED_ATTN_RBCP) | \
				 GENERAL_ATTEN_OFFSET(LATCHED_ATTN_RSVD_GRC))

#define HW_INTERRUT_ASSERT_SET_0 \
				(AEU_INPUTS_ATTN_BITS_TSDM_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_TCM_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_TSEMI_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_PBF_HW_INTERRUPT)
#define HW_PRTY_ASSERT_SET_0	(AEU_INPUTS_ATTN_BITS_BRB_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_PARSER_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_TSDM_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_SEARCHER_PARITY_ERROR |\
				 AEU_INPUTS_ATTN_BITS_TSEMI_PARITY_ERROR)
#define HW_INTERRUT_ASSERT_SET_1 \
				(AEU_INPUTS_ATTN_BITS_QM_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_TIMERS_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_XSDM_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_XCM_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_XSEMI_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_USDM_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_UCM_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_USEMI_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_UPB_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_CSDM_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_CCM_HW_INTERRUPT)
#define HW_PRTY_ASSERT_SET_1	(AEU_INPUTS_ATTN_BITS_PBCLIENT_PARITY_ERROR |\
				 AEU_INPUTS_ATTN_BITS_QM_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_XSDM_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_XSEMI_PARITY_ERROR | \
				AEU_INPUTS_ATTN_BITS_DOORBELLQ_PARITY_ERROR |\
			    AEU_INPUTS_ATTN_BITS_VAUX_PCI_CORE_PARITY_ERROR |\
				 AEU_INPUTS_ATTN_BITS_DEBUG_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_USDM_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_USEMI_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_UPB_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_CSDM_PARITY_ERROR)
#define HW_INTERRUT_ASSERT_SET_2 \
				(AEU_INPUTS_ATTN_BITS_CSEMI_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_CDU_HW_INTERRUPT | \
				 AEU_INPUTS_ATTN_BITS_DMAE_HW_INTERRUPT | \
			AEU_INPUTS_ATTN_BITS_PXPPCICLOCKCLIENT_HW_INTERRUPT |\
				 AEU_INPUTS_ATTN_BITS_MISC_HW_INTERRUPT)
#define HW_PRTY_ASSERT_SET_2	(AEU_INPUTS_ATTN_BITS_CSEMI_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_PXP_PARITY_ERROR | \
			AEU_INPUTS_ATTN_BITS_PXPPCICLOCKCLIENT_PARITY_ERROR |\
				 AEU_INPUTS_ATTN_BITS_CFC_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_CDU_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_IGU_PARITY_ERROR | \
				 AEU_INPUTS_ATTN_BITS_MISC_PARITY_ERROR)


#define MULTI_FLAGS \
		(TSTORM_ETH_FUNCTION_COMMON_CONFIG_RSS_IPV4_CAPABILITY | \
		 TSTORM_ETH_FUNCTION_COMMON_CONFIG_RSS_IPV4_TCP_CAPABILITY | \
		 TSTORM_ETH_FUNCTION_COMMON_CONFIG_RSS_IPV6_CAPABILITY | \
		 TSTORM_ETH_FUNCTION_COMMON_CONFIG_RSS_IPV6_TCP_CAPABILITY | \
		 TSTORM_ETH_FUNCTION_COMMON_CONFIG_RSS_ENABLE)

#define MULTI_MASK			0x7f


#define DEF_USB_FUNC_OFF		(2 + 2*HC_USTORM_DEF_SB_NUM_INDICES)
#define DEF_CSB_FUNC_OFF		(2 + 2*HC_CSTORM_DEF_SB_NUM_INDICES)
#define DEF_XSB_FUNC_OFF		(2 + 2*HC_XSTORM_DEF_SB_NUM_INDICES)
#define DEF_TSB_FUNC_OFF		(2 + 2*HC_TSTORM_DEF_SB_NUM_INDICES)

#define C_DEF_SB_SP_INDEX		HC_INDEX_DEF_C_ETH_SLOW_PATH

#define BNX2X_SP_DSB_INDEX \
(&bp->def_status_blk->c_def_status_block.index_values[C_DEF_SB_SP_INDEX])


#define CAM_IS_INVALID(x) \
(x.target_table_entry.flags == TSTORM_CAM_TARGET_TABLE_ENTRY_ACTION_TYPE)

#define CAM_INVALIDATE(x) \
	(x.target_table_entry.flags = TSTORM_CAM_TARGET_TABLE_ENTRY_ACTION_TYPE)


/* Number of u32 elements in MC hash array */
#define MC_HASH_SIZE			8
#define MC_HASH_OFFSET(bp, i)		(BAR_TSTRORM_INTMEM + \
	TSTORM_APPROXIMATE_MATCH_MULTICAST_FILTERING_OFFSET(BP_FUNC(bp)) + i*4)


#ifndef PXP2_REG_PXP2_INT_STS
#define PXP2_REG_PXP2_INT_STS		PXP2_REG_PXP2_INT_STS_0
#endif

/* MISC_REG_RESET_REG - this is here for the hsi to work don't touch */

#endif /* bnx2x.h */
