/*
 * Audio support data for ISDN4Linux.
 *
 * Copyright 2002/2003 by Andreas Eversberg (jolly@eversberg.eu)
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 */

#define DEBUG_DSP_CTRL		0x0001
#define DEBUG_DSP_CORE		0x0002
#define DEBUG_DSP_DTMF		0x0004
#define DEBUG_DSP_CMX		0x0010
#define DEBUG_DSP_TONE		0x0020
#define DEBUG_DSP_BLOWFISH	0x0040
#define DEBUG_DSP_DELAY		0x0100
#define DEBUG_DSP_DTMFCOEFF	0x8000 /* heavy output */

/* options may be:
 *
 * bit 0 = use ulaw instead of alaw
 * bit 1 = enable hfc hardware accelleration for all channels
 *
 */
#define DSP_OPT_ULAW		(1<<0)
#define DSP_OPT_NOHARDWARE	(1<<1)

#include <linux/timer.h>
#include <linux/workqueue.h>

#include "dsp_ecdis.h"

extern int dsp_options;
extern int dsp_debug;
extern int dsp_poll;
extern int dsp_tics;
extern spinlock_t dsp_lock;
extern struct work_struct dsp_workq;
extern u32 dsp_poll_diff; /* calculated fix-comma corrected poll value */

/***************
 * audio stuff *
 ***************/

extern s32 dsp_audio_alaw_to_s32[256];
extern s32 dsp_audio_ulaw_to_s32[256];
extern s32 *dsp_audio_law_to_s32;
extern u8 dsp_audio_s16_to_law[65536];
extern u8 dsp_audio_alaw_to_ulaw[256];
extern u8 dsp_audio_mix_law[65536];
extern u8 dsp_audio_seven2law[128];
extern u8 dsp_audio_law2seven[256];
extern void dsp_audio_generate_law_tables(void);
extern void dsp_audio_generate_s2law_table(void);
extern void dsp_audio_generate_seven(void);
extern void dsp_audio_generate_mix_table(void);
extern void dsp_audio_generate_ulaw_samples(void);
extern void dsp_audio_generate_volume_changes(void);
extern u8 dsp_silence;


/*************
 * cmx stuff *
 *************/

#define MAX_POLL	256	/* maximum number of send-chunks */

#define CMX_BUFF_SIZE	0x8000	/* must be 2**n (0x1000 about 1/2 second) */
#define CMX_BUFF_HALF	0x4000	/* CMX_BUFF_SIZE / 2 */
#define CMX_BUFF_MASK	0x7fff	/* CMX_BUFF_SIZE - 1 */

/* how many seconds will we check the lowest delay until the jitter buffer
   is reduced by that delay */
#define MAX_SECONDS_JITTER_CHECK 5

extern struct timer_list dsp_spl_tl;
extern u32 dsp_spl_jiffies;

/* the structure of conferences:
 *
 * each conference has a unique number, given by user space.
 * the conferences are linked in a chain.
 * each conference has members linked in a chain.
 * each dsplayer points to a member, each member points to a dsplayer.
 */

/* all members within a conference (this is linked 1:1 with the dsp) */
struct dsp;
struct dsp_conf_member {
	struct list_head	list;
	struct dsp		*dsp;
};

/* the list of all conferences */
struct dsp_conf {
	struct list_head	list;
	u32			id;
				/* all cmx stacks with the same ID are
				 connected */
	struct list_head	mlist;
	int			software; /* conf is processed by software */
	int			hardware; /* conf is processed by hardware */
				/* note: if both unset, has only one member */
};


/**************
 * DTMF stuff *
 **************/

#define DSP_DTMF_NPOINTS 102

#define ECHOCAN_BUFLEN (4*128)

struct dsp_dtmf {
	int		treshold; /* above this is dtmf (square of) */
	int		software; /* dtmf uses software decoding */
	int		hardware; /* dtmf uses hardware decoding */
	int		size; /* number of bytes in buffer */
	signed short	buffer[DSP_DTMF_NPOINTS];
		/* buffers one full dtmf frame */
	u8		lastwhat, lastdigit;
	int		count;
	u8		digits[16]; /* just the dtmf result */
};


/******************
 * pipeline stuff *
 ******************/
struct dsp_pipeline {
	rwlock_t  lock;
	struct list_head list;
	int inuse;
};

/***************
 * tones stuff *
 ***************/

struct dsp_tone {
	int		software; /* tones are generated by software */
	int		hardware; /* tones are generated by hardware */
	int		tone;
	void		*pattern;
	int		count;
	int		index;
	struct timer_list tl;
};

/*****************
 * general stuff *
 *****************/

struct dsp {
	struct list_head list;
	struct mISDNchannel	ch;
	struct mISDNchannel	*up;
	unsigned char	name[64];
	int		b_active;
	int		echo; /* echo is enabled */
	int		rx_disabled; /* what the user wants */
	int		rx_is_off; /* what the card is */
	int		tx_mix;
	struct dsp_tone	tone;
	struct dsp_dtmf	dtmf;
	int		tx_volume, rx_volume;

	/* queue for sending frames */
	struct work_struct	workq;
	struct sk_buff_head	sendq;
	int		hdlc;	/* if mode is hdlc */
	int		data_pending;	/* currently an unconfirmed frame */

	/* conference stuff */
	u32		conf_id;
	struct dsp_conf	*conf;
	struct dsp_conf_member
			*member;

	/* buffer stuff */
	int		rx_W; /* current write pos for data without timestamp */
	int		rx_R; /* current read pos for transmit clock */
	int		rx_init; /* if set, pointers will be adjusted first */
	int		tx_W; /* current write pos for transmit data */
	int		tx_R; /* current read pos for transmit clock */
	int		rx_delay[MAX_SECONDS_JITTER_CHECK];
	int		tx_delay[MAX_SECONDS_JITTER_CHECK];
	u8		tx_buff[CMX_BUFF_SIZE];
	u8		rx_buff[CMX_BUFF_SIZE];
	int		last_tx; /* if set, we transmitted last poll interval */
	int		cmx_delay; /* initial delay of buffers,
				or 0 for dynamic jitter buffer */
	int		tx_dejitter; /* if set, dejitter tx buffer */
	int		tx_data; /* enables tx-data of CMX to upper layer */

	/* hardware stuff */
	struct dsp_features features;
	int		features_rx_off; /* set if rx_off is featured */
	int		pcm_slot_rx; /* current PCM slot (or -1) */
	int		pcm_bank_rx;
	int		pcm_slot_tx;
	int		pcm_bank_tx;
	int		hfc_conf; /* unique id of current conference (or -1) */

	/* encryption stuff */
	int		bf_enable;
	u32		bf_p[18];
	u32		bf_s[1024];
	int		bf_crypt_pos;
	u8		bf_data_in[9];
	u8		bf_crypt_out[9];
	int		bf_decrypt_in_pos;
	int		bf_decrypt_out_pos;
	u8		bf_crypt_inring[16];
	u8		bf_data_out[9];
	int		bf_sync;

	struct dsp_pipeline
			pipeline;
};

/* functions */

extern void dsp_change_volume(struct sk_buff *skb, int volume);

extern struct list_head dsp_ilist;
extern struct list_head conf_ilist;
extern void dsp_cmx_debug(struct dsp *dsp);
extern void dsp_cmx_hardware(struct dsp_conf *conf, struct dsp *dsp);
extern int dsp_cmx_conf(struct dsp *dsp, u32 conf_id);
extern void dsp_cmx_receive(struct dsp *dsp, struct sk_buff *skb);
extern void dsp_cmx_hdlc(struct dsp *dsp, struct sk_buff *skb);
extern void dsp_cmx_send(void *arg);
extern void dsp_cmx_transmit(struct dsp *dsp, struct sk_buff *skb);
extern int dsp_cmx_del_conf_member(struct dsp *dsp);
extern int dsp_cmx_del_conf(struct dsp_conf *conf);

extern void dsp_dtmf_goertzel_init(struct dsp *dsp);
extern void dsp_dtmf_hardware(struct dsp *dsp);
extern u8 *dsp_dtmf_goertzel_decode(struct dsp *dsp, u8 *data, int len,
		int fmt);

extern int dsp_tone(struct dsp *dsp, int tone);
extern void dsp_tone_copy(struct dsp *dsp, u8 *data, int len);
extern void dsp_tone_timeout(void *arg);

extern void dsp_bf_encrypt(struct dsp *dsp, u8 *data, int len);
extern void dsp_bf_decrypt(struct dsp *dsp, u8 *data, int len);
extern int dsp_bf_init(struct dsp *dsp, const u8 *key, unsigned int keylen);
extern void dsp_bf_cleanup(struct dsp *dsp);

extern int  dsp_pipeline_module_init(void);
extern void dsp_pipeline_module_exit(void);
extern int  dsp_pipeline_init(struct dsp_pipeline *pipeline);
extern void dsp_pipeline_destroy(struct dsp_pipeline *pipeline);
extern int  dsp_pipeline_build(struct dsp_pipeline *pipeline, const char *cfg);
extern void dsp_pipeline_process_tx(struct dsp_pipeline *pipeline, u8 *data,
		int len);
extern void dsp_pipeline_process_rx(struct dsp_pipeline *pipeline, u8 *data,
		int len);

