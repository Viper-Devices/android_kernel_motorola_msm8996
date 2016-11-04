/* Copyright (c) 2008-2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef MDSS_PANEL_H
#define MDSS_PANEL_H

#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/stringify.h>
#include <linux/types.h>
#include <linux/debugfs.h>

#define KHZ_TO_HZ 1000

/* panel id type */
struct panel_id {
	u16 id;
	u16 type;
};

enum fps_resolution {
	FPS_RESOLUTION_DEFAULT,
	FPS_RESOLUTION_HZ,
	FPS_RESOLUTION_KHZ,
};

#define DEFAULT_FRAME_RATE	60
#define DEFAULT_ROTATOR_FRAME_RATE 120
#define ROTATOR_LOW_FRAME_RATE 30
#define MDSS_DSI_RST_SEQ_LEN	10
/* worst case prefill lines for all chipsets including all vertical blank */
#define MDSS_MDP_MAX_PREFILL_FETCH 25

#define OVERRIDE_CFG	"override"
#define SIM_PANEL	"sim"
#define SIM_SW_TE_PANEL	"sim-swte"
#define SIM_HW_TE_PANEL	"sim-hwte"

#define BRIGHTNESS_HBM_ON	0xFFFFFFFE
#define BRIGHTNESS_HBM_OFF	(BRIGHTNESS_HBM_ON - 1)
#define HBM_BRIGHTNESS(value) ((value) == HBM_ON_STATE ?\
			BRIGHTNESS_HBM_ON : BRIGHTNESS_HBM_OFF)
/* HBM implementation is different, depending on display and backlight hardware
 * design, which is classified into the following types:
 * HBM_TYPE_OLED: OLED panel, HBM is controlled by DSI register only, which
 *     is independent on brightness.
 * HBM_TYPE_LCD_DCS_WLED: LCD panel, HBM is controlled by DSI register, and
 *     brightness is decided by WLED IC on I2C/SPI bus.
 * HBM_TYPE_LCD_DCS_ONLY: LCD panel, brightness/HBM is controlled by DSI
 *     register only.
 * HBM_TYPE_LCD_WLED_ONLY: LCD panel, brightness/HBM is controlled by WLED
 *     IC only.
 *
 * Note: brightness must be at maximum while enabling HBM for all LCD panels
 */
#define HBM_TYPE_OLED	0
#define HBM_TYPE_LCD_DCS_WLED	1
#define HBM_TYPE_LCD_DCS_ONLY	2
#define HBM_TYPE_LCD_WLED_ONLY	3

enum hbm_state {
	HBM_OFF_STATE = 0,
	HBM_ON_STATE,
	HBM_STATE_NUM
};

enum acl_state {
	ACL_OFF_STATE = 0,
	ACL_ON_STATE,
	ACL_STATE_NUM
};

enum cabc_mode {
	CABC_UI_MODE = 0,
	CABC_MV_MODE,
	CABC_DIS_MODE,
	CABC_MODE_NUM
};

enum panel_param_id {
	PARAM_HBM_ID = 0,
	PARAM_ACL_ID,
	PARAM_CABC_ID,
	PARAM_ID_NUM
};

struct panel_param_val_map {
	char *name;
	char *prop;
};

struct panel_param {
	const char *param_name;
	const struct panel_param_val_map *val_map;
	const u16 val_max;
	const u16 default_value;
	u16 value;
	bool is_supported;
};

/* panel type list */
#define NO_PANEL		0xffff	/* No Panel */
#define MDDI_PANEL		1	/* MDDI */
#define EBI2_PANEL		2	/* EBI2 */
#define LCDC_PANEL		3	/* internal LCDC type */
#define EXT_MDDI_PANEL		4	/* Ext.MDDI */
#define TV_PANEL		5	/* TV */
#define HDMI_PANEL		6	/* HDMI TV */
#define DTV_PANEL		7	/* DTV */
#define MIPI_VIDEO_PANEL	8	/* MIPI */
#define MIPI_CMD_PANEL		9	/* MIPI */
#define WRITEBACK_PANEL		10	/* Wifi display */
#define LVDS_PANEL		11	/* LVDS */
#define EDP_PANEL		12	/* LVDS */

#define DSC_PPS_LEN		128

/* HDR propeties count */
#define DISPLAY_PRIMARIES_COUNT	8	/* WRGB x and y values*/

static inline const char *mdss_panel2str(u32 panel)
{
	static const char const *names[] = {
#define PANEL_NAME(n) [n ## _PANEL] = __stringify(n)
		PANEL_NAME(MIPI_VIDEO),
		PANEL_NAME(MIPI_CMD),
		PANEL_NAME(EDP),
		PANEL_NAME(HDMI),
		PANEL_NAME(DTV),
		PANEL_NAME(WRITEBACK),
#undef PANEL_NAME
	};

	if (panel >= ARRAY_SIZE(names) || !names[panel])
		return "UNKNOWN";

	return names[panel];
}

/* panel class */
enum {
	DISPLAY_LCD = 0,	/* lcd = ebi2/mddi */
	DISPLAY_LCDC,		/* lcdc */
	DISPLAY_TV,		/* TV Out */
	DISPLAY_EXT_MDDI,	/* External MDDI */
	DISPLAY_WRITEBACK,
};

/* panel device locaiton */
enum {
	DISPLAY_1 = 0,		/* attached as first device */
	DISPLAY_2,		/* attached on second device */
	DISPLAY_3,		/* attached on third device */
	DISPLAY_4,		/* attached on fourth device */
	MAX_PHYS_TARGET_NUM,
};

enum {
	MDSS_PANEL_INTF_INVALID = -1,
	MDSS_PANEL_INTF_DSI,
	MDSS_PANEL_INTF_EDP,
	MDSS_PANEL_INTF_HDMI,
};

enum {
	MDSS_PANEL_POWER_OFF = 0,
	MDSS_PANEL_POWER_ON,
	MDSS_PANEL_POWER_LP1,
	MDSS_PANEL_POWER_LP2,
};

enum {
	MDSS_PANEL_LOW_PERSIST_MODE_OFF = 0,
	MDSS_PANEL_LOW_PERSIST_MODE_ON,
};

enum {
	MODE_GPIO_NOT_VALID = 0,
	MODE_GPIO_HIGH,
	MODE_GPIO_LOW,
};

/*
 * enum sim_panel_modes - Different panel modes for simulator panels
 *
 * @SIM_MODE:		Disables all host reads for video mode simulator panels.
 * @SIM_SW_TE_MODE:	Disables all host reads and genereates the SW TE. Used
 *                      for cmd mode simulator panels.
 * @SIM_HW_TE_MODE:	Disables all host reads and expects TE from hardware
 *                      (terminator card). Used for cmd mode simulator panels.
 */
enum {
	SIM_MODE = 1,
	SIM_SW_TE_MODE,
	SIM_HW_TE_MODE,
};

struct mdss_rect {
	u16 x;
	u16 y;
	u16 w;
	u16 h;
};

#define MDSS_MAX_PANEL_LEN      256
#define MDSS_INTF_MAX_NAME_LEN 5
#define MDSS_DISPLAY_ID_MAX_LEN 16
struct mdss_panel_intf {
	char name[MDSS_INTF_MAX_NAME_LEN];
	int  type;
};

struct mdss_panel_cfg {
	char arg_cfg[MDSS_MAX_PANEL_LEN + 1];
	int  pan_intf;
	bool lk_cfg;
	bool init_done;
};

#define MDP_INTF_DSI_CMD_FIFO_UNDERFLOW		0x0001
#define MDP_INTF_DSI_VIDEO_FIFO_OVERFLOW	0x0002


enum {
	MDP_INTF_CALLBACK_DSI_WAIT,
};

struct mdss_intf_recovery {
	int (*fxn)(void *ctx, int event);
	void *data;
};

/**
 * enum mdss_intf_events - Different events generated by MDP core
 *
 * @MDSS_EVENT_RESET:		MDP control path is being (re)initialized.
 * @MDSS_EVENT_LINK_READY	Interface data path inited to ready state.
 * @MDSS_EVENT_UNBLANK:		Sent before first frame update from MDP is
 *				sent to panel.
 * @MDSS_EVENT_PANEL_ON:	After first frame update from MDP.
 * @MDSS_EVENT_POST_PANEL_ON	send 2nd phase panel on commands to panel
 * @MDSS_EVENT_BLANK:		MDP has no contents to display only blank screen
 *				is shown in panel. Sent before panel off.
 * @MDSS_EVENT_PANEL_OFF:	MDP has suspended frame updates, panel should be
 *				completely shutdown after this call.
 * @MDSS_EVENT_CLOSE:		MDP has tore down entire session.
 * @MDSS_EVENT_SUSPEND:		Propagation of power suspend event.
 * @MDSS_EVENT_RESUME:		Propagation of power resume event.
 * @MDSS_EVENT_CHECK_PARAMS:	Event generated when a panel reconfiguration is
 *				requested including when resolution changes.
 *				The event handler receives pointer to
 *				struct mdss_panel_info and should return one of:
 *				 - negative if the configuration is invalid
 *				 - 0 if there is no panel reconfig needed
 *				 - 1 if reconfig is needed to take effect
 * @MDSS_EVENT_CONT_SPLASH_BEGIN: Special event used to handle transition of
 *				display state from boot loader to panel driver.
 *				The event handler will disable the panel.
 * @MDSS_EVENT_CONT_SPLASH_FINISH: Special event used to handle transition of
 *				display state from boot loader to panel driver.
 *				The event handler will enable the panel and
 *				vote for the display clocks.
 * @MDSS_EVENT_PANEL_UPDATE_FPS: Event to update the frame rate of the panel.
 * @MDSS_EVENT_FB_REGISTERED:	Called after fb dev driver has been registered,
 *				panel driver gets ptr to struct fb_info which
 *				holds fb dev information.
 * @MDSS_EVENT_PANEL_CLK_CTRL:	panel clock control
 *				- 0 clock disable
 *				- 1 clock enable
 * @MDSS_EVENT_DSI_CMDLIST_KOFF: acquire dsi_mdp_busy lock before kickoff.
 * @MDSS_EVENT_ENABLE_PARTIAL_ROI: Event to update ROI of the panel.
 * @MDSS_EVENT_DSC_PPS_SEND: Event to send DSC PPS command to panel.
 * @MDSS_EVENT_DSI_STREAM_SIZE: Event to update DSI controller's stream size
 * @MDSS_EVENT_DSI_UPDATE_PANEL_DATA: Event to update the dsi driver structures
 *				based on the dsi mode passed as argument.
 *				- 0: update to video mode
 *				- 1: update to command mode
 * @MDSS_EVENT_REGISTER_RECOVERY_HANDLER: Event to recover the interface in
 *					case there was any errors detected.
 * @MDSS_EVENT_REGISTER_MDP_CALLBACK: Event to register callback to MDP driver.
 * @MDSS_EVENT_DSI_PANEL_STATUS: Event to check the panel status
 *				<= 0: panel check fail
 *				>  0: panel check success
 * @MDSS_EVENT_DSI_DYNAMIC_SWITCH: Send DCS command to panel to initiate
 *				switching panel to new mode
 *				- MIPI_VIDEO_PANEL: switch to video mode
 *				- MIPI_CMD_PANEL: switch to command mode
 * @MDSS_EVENT_DSI_RECONFIG_CMD: Setup DSI controller in new mode
 *				- MIPI_VIDEO_PANEL: switch to video mode
 *				- MIPI_CMD_PANEL: switch to command mode
 * @MDSS_EVENT_DSI_RESET_WRITE_PTR: Reset the write pointer coordinates on
 *				the panel.
 * @MDSS_EVENT_PANEL_TIMING_SWITCH: Panel timing switch is requested.
 *				Argument provided is new panel timing.
 * @MDSS_EVENT_ENABLE_TE: Change TE state, used for factory testing only
 */
enum mdss_intf_events {
	MDSS_EVENT_RESET = 1,
	MDSS_EVENT_LINK_READY,
	MDSS_EVENT_UNBLANK,
	MDSS_EVENT_PANEL_ON,
	MDSS_EVENT_POST_PANEL_ON,
	MDSS_EVENT_BLANK,
	MDSS_EVENT_PANEL_OFF,
	MDSS_EVENT_CLOSE,
	MDSS_EVENT_SUSPEND,
	MDSS_EVENT_RESUME,
	MDSS_EVENT_CHECK_PARAMS,
	MDSS_EVENT_CONT_SPLASH_BEGIN,
	MDSS_EVENT_CONT_SPLASH_FINISH,
	MDSS_EVENT_PANEL_UPDATE_FPS,
	MDSS_EVENT_FB_REGISTERED,
	MDSS_EVENT_PANEL_CLK_CTRL,
	MDSS_EVENT_DSI_CMDLIST_KOFF,
	MDSS_EVENT_ENABLE_PARTIAL_ROI,
	MDSS_EVENT_DSC_PPS_SEND,
	MDSS_EVENT_DSI_STREAM_SIZE,
	MDSS_EVENT_DSI_UPDATE_PANEL_DATA,
	MDSS_EVENT_REGISTER_RECOVERY_HANDLER,
	MDSS_EVENT_REGISTER_MDP_CALLBACK,
	MDSS_EVENT_DSI_PANEL_STATUS,
	MDSS_EVENT_DSI_DYNAMIC_SWITCH,
	MDSS_EVENT_DSI_RECONFIG_CMD,
	MDSS_EVENT_DSI_RESET_WRITE_PTR,
	MDSS_EVENT_PANEL_TIMING_SWITCH,
	MDSS_EVENT_UPDATE_PARAMS,
	MDSS_EVENT_MAX,
	MDSS_EVENT_ENABLE_TE,
};

struct lcd_panel_info {
	u32 h_back_porch;
	u32 h_front_porch;
	u32 h_pulse_width;
	u32 v_back_porch;
	u32 v_front_porch;
	u32 v_pulse_width;
	u32 border_clr;
	u32 underflow_clr;
	u32 hsync_skew;
	u32 border_top;
	u32 border_bottom;
	u32 border_left;
	u32 border_right;
	/* Pad width */
	u32 xres_pad;
	/* Pad height */
	u32 yres_pad;
	u32 frame_rate;
};


/* DSI PHY configuration */
struct mdss_dsi_phy_ctrl {
	char regulator[7];	/* 8996, 1 * 5 */
	char timing[12];
	char ctrl[4];
	char strength[10];	/* 8996, 2 * 5 */
	char bistctrl[6];
	uint32_t pll[21];
	char lanecfg[45];	/* 8996, 4 * 5 */
	bool reg_ldo_mode;

	char timing_8996[40];/* 8996, 8 * 5 */
	char regulator_len;
	char strength_len;
	char lanecfg_len;
};

/**
 * enum dynamic_mode_switch - Dynamic mode switch methods
 * @DYNAMIC_MODE_SWITCH_DISABLED: Dynamic mode switch is not supported
 * @DYNAMIC_MODE_SWITCH_SUSPEND_RESUME: Switch requires panel suspend/resume
 * @DYNAMIC_MODE_SWITCH_IMMEDIATE: Supports video/cmd mode switch immediately
 * @DYNAMIC_MODE_RESOLUTION_SWITCH_IMMEDIATE: Panel supports display resolution
 * switch immediately.
 **/
enum dynamic_mode_switch {
	DYNAMIC_MODE_SWITCH_DISABLED = 0,
	DYNAMIC_MODE_SWITCH_SUSPEND_RESUME,
	DYNAMIC_MODE_SWITCH_IMMEDIATE,
	DYNAMIC_MODE_RESOLUTION_SWITCH_IMMEDIATE,
};

/**
 * enum dynamic_switch_modes - Type of dynamic mode switch to be given as
 * argument to MDSS_EVENT_DSI_DYNAMIC_SWITCH event
 * @SWITCH_TO_CMD_MODE: Switch from DSI video mode to command mode
 * @SWITCH_TO_VIDEO_MODE: Switch from DSI command mode to video mode
 * @SWITCH_RESOLUTION: Switch only display resolution
 **/
enum dynamic_switch_modes {
	SWITCH_MODE_UNKNOWN = 0,
	SWITCH_TO_CMD_MODE,
	SWITCH_TO_VIDEO_MODE,
	SWITCH_RESOLUTION,
};

/**
 * struct mdss_panel_timing - structure for panel timing information
 * @list: List head ptr to track within panel data timings list
 * @name: A unique name of this timing that can be used to identify it
 * @xres: Panel width
 * @yres: Panel height
 * @h_back_porch: Horizontal back porch
 * @h_front_porch: Horizontal front porch
 * @h_pulse_width: Horizontal pulse width
 * @hsync_skew: Horizontal sync skew
 * @v_back_porch: Vertical back porch
 * @v_front_porch: Vertical front porch
 * @v_pulse_width: Vertical pulse width
 * @border_top: Border color on top
 * @border_bottom: Border color on bottom
 * @border_left: Border color on left
 * @border_right: Border color on right
 * @clk_rate: Pixel clock rate of this panel timing
 * @frame_rate: Display refresh rate
 * @fbc: Framebuffer compression parameters for this display timing
 * @te: Tearcheck parameters for this display timing
 **/
struct mipi_panel_info {
	char boot_mode;	/* identify if mode switched from starting mode */
	char mode;		/* video/cmd */
	char interleave_mode;
	char crc_check;
	char ecc_check;
	char dst_format;	/* shared by video and command */
	char data_lane0;
	char data_lane1;
	char data_lane2;
	char data_lane3;
	char rgb_swap;
	char b_sel;
	char g_sel;
	char r_sel;
	char rx_eot_ignore;
	char tx_eot_append;
	char t_clk_post; /* 0xc0, DSI_CLKOUT_TIMING_CTRL */
	char t_clk_pre;  /* 0xc0, DSI_CLKOUT_TIMING_CTRL */
	char vc;	/* virtual channel */
	struct mdss_dsi_phy_ctrl dsi_phy_db;
	/* video mode */
	char pulse_mode_hsa_he;
	char hfp_power_stop;
	char hbp_power_stop;
	char hsa_power_stop;
	char eof_bllp_power_stop;
	char last_line_interleave_en;
	char bllp_power_stop;
	char traffic_mode;
	char frame_rate;
	/* command mode */
	char frame_rate_idle;
	char interleave_max;
	char insert_dcs_cmd;
	char wr_mem_continue;
	char wr_mem_start;
	char te_sel;
	char stream;	/* 0 or 1 */
	char mdp_trigger;
	char dma_trigger;
	/* Dynamic Switch Support */
	enum dynamic_mode_switch dms_mode;

	u32 pixel_packing;
	u32 dsi_pclk_rate;
	/* The packet-size should not bet changed */
	char no_max_pkt_size;
	/* Clock required during LP commands */
	bool force_clk_lane_hs;

	char vsync_enable;
	char hw_vsync_mode;

	char lp11_init;
	u32  init_delay;
	u32  post_init_delay;
	u8 default_lanes;
};

struct edp_panel_info {
	char frame_rate;	/* fps */
};

/**
 * struct dynamic_fps_data - defines dynamic fps related data
 * @hfp: horizontal front porch
 * @hbp: horizontal back porch
 * @hpw: horizontal pulse width
 * @clk_rate: panel clock rate in HZ
 * @fps: frames per second
 */
struct dynamic_fps_data {
	u32 hfp;
	u32 hbp;
	u32 hpw;
	u32 clk_rate;
	u32 fps;
};

/**
 * enum dynamic_fps_update - defines fps update modes
 * @DFPS_SUSPEND_RESUME_MODE: suspend/resume mode
 * @DFPS_IMMEDIATE_CLK_UPDATE_MODE: update fps using clock
 * @DFPS_IMMEDIATE_PORCH_UPDATE_MODE_VFP: update fps using vertical timings
 * @DFPS_IMMEDIATE_PORCH_UPDATE_MODE_HFP: update fps using horizontal timings
 * @DFPS_IMMEDIATE_MULTI_UPDATE_MODE_CLK_HFP: update fps using both horizontal
 *  timings and clock.
 * @DFPS_IMMEDIATE_MULTI_MODE_HFP_CALC_CLK: update fps using both
 *  horizontal timings, clock need to be caculate base on new clock and
 *  porches.
 * @DFPS_MODE_MAX: defines maximum limit of supported modes.
 */
enum dynamic_fps_update {
	DFPS_SUSPEND_RESUME_MODE,
	DFPS_IMMEDIATE_CLK_UPDATE_MODE,
	DFPS_IMMEDIATE_PORCH_UPDATE_MODE_VFP,
	DFPS_IMMEDIATE_PORCH_UPDATE_MODE_HFP,
	DFPS_IMMEDIATE_MULTI_UPDATE_MODE_CLK_HFP,
	DFPS_IMMEDIATE_MULTI_MODE_HFP_CALC_CLK,
	DFPS_MODE_MAX
};

enum lvds_mode {
	LVDS_SINGLE_CHANNEL_MODE,
	LVDS_DUAL_CHANNEL_MODE,
};

struct lvds_panel_info {
	enum lvds_mode channel_mode;
	/* Channel swap in dual mode */
	char channel_swap;
};

enum {
	COMPRESSION_NONE,
	COMPRESSION_DSC,
	COMPRESSION_FBC
};

struct dsc_desc {
	u8 version; /* top 4 bits major and lower 4 bits minor version */
	u8 scr_rev; /* 8 bit value for dsc scr revision */

	/*
	 * Following parameters can change per frame if partial update is on
	 */
	int pic_height;
	int pic_width;
	int initial_lines;

	/*
	 * Following parameters are used for DSI and not for MDP. They can
	 * change per frame if partial update is enabled.
	 */
	int pkt_per_line;
	int bytes_in_slice;
	int bytes_per_pkt;
	int eol_byte_num;
	int pclk_per_line;	/* width */

	/*
	 * Following parameters only changes when slice dimensions are changed.
	 */
	int full_frame_slices; /* denotes number of slice per intf */
	int slice_height;
	int slice_width;
	int chunk_size;

	int slice_last_group_size;
	int bpp;	/* target bits per pixel */
	int bpc;	/* uncompressed bits per component */
	int line_buf_depth;
	bool config_by_manufacture_cmd;
	bool block_pred_enable;
	int vbr_enable;
	int enable_422;
	int convert_rgb;
	int input_10_bits;
	int slice_per_pkt;

	int initial_dec_delay;
	int initial_xmit_delay;

	int initial_scale_value;
	int scale_decrement_interval;
	int scale_increment_interval;

	int first_line_bpg_offset;
	int nfl_bpg_offset;
	int slice_bpg_offset;

	int initial_offset;
	int final_offset;

	int rc_model_size;	/* rate_buffer_size */

	int det_thresh_flatness;
	int max_qp_flatness;
	int min_qp_flatness;
	int edge_factor;
	int quant_incr_limit0;
	int quant_incr_limit1;
	int tgt_offset_hi;
	int tgt_offset_lo;
	u32 *buf_thresh;
	char *range_min_qp;
	char *range_max_qp;
	char *range_bpg_offset;
};

struct fbc_panel_info {
	u32 enabled;
	u32 target_bpp;
	u32 comp_mode;
	u32 qerr_enable;
	u32 cd_bias;
	u32 pat_enable;
	u32 vlc_enable;
	u32 bflc_enable;

	u32 line_x_budget;
	u32 block_x_budget;
	u32 block_budget;

	u32 lossless_mode_thd;
	u32 lossy_mode_thd;
	u32 lossy_rgb_thd;
	u32 lossy_mode_idx;

	u32 slice_height;
	bool pred_mode;
	bool enc_mode;
	u32 max_pred_err;
};

struct mdss_mdp_pp_tear_check {
	u32 tear_check_en;
	u32 sync_cfg_height;
	u32 vsync_init_val;
	u32 sync_threshold_start;
	u32 sync_threshold_continue;
	u32 start_pos;
	u32 rd_ptr_irq;
	u32 wr_ptr_irq;
	u32 refx100;
};

struct mdss_panel_roi_alignment {
	u32 xstart_pix_align;
	u32 width_pix_align;
	u32 ystart_pix_align;
	u32 height_pix_align;
	u32 min_width;
	u32 min_height;
};

struct mdss_panel_hdr_properties {
	bool hdr_enabled;

	/* WRGB X and y values arrayed in format */
	/* [WX, WY, RX, RY, GX, GY, BX, BY] */
	u32 display_primaries[DISPLAY_PRIMARIES_COUNT];

	/* peak brightness supported by panel */
	u32 peak_brightness;
	/* Blackness level supported by panel */
	u32 blackness_level;
};

struct mdss_panel_info {
	u32 xres;
	u32 yres;
	u32 physical_width;
	u32 physical_height;
	u32 bpp;
	u32 type;
	u32 wait_cycle;
	u32 pdest;
	u32 brightness_max;
	u32 bl_max;
	u32 bl_min;
	u32 fb_num;
	u64 clk_rate;
	u32 clk_min;
	u64 clk_max;
	u32 mdp_transfer_time_us;
	u32 frame_count;
	u32 is_3d_panel;
	u32 out_format;
	u32 rst_seq[MDSS_DSI_RST_SEQ_LEN];
	u32 rst_seq_len;
	u32 vic; /* video identification code */
	struct mdss_rect roi;
	int pwm_pmic_gpio;
	int pwm_lpg_chan;
	int pwm_period;
	bool dynamic_fps;
	bool ulps_feature_enabled;
	bool ulps_suspend_enabled;
	bool panel_ack_disabled;
	bool esd_check_enabled;
	bool allow_phy_power_off;
	char dfps_update;
	/* new requested fps before it is updated in hw */
	int new_fps;
	/* stores initial fps after boot */
	u32 default_fps;
	/* stores initial vtotal (vfp-method) or htotal (hfp-method) */
	u32 saved_total;
	/* stores initial vfp (vfp-method) or hfp (hfp-method) */
	u32 saved_fporch;
	/* current fps, once is programmed in hw */
	int current_fps;

	int panel_max_fps;
	int panel_max_vtotal;
	u32 mode_gpio_state;
	u32 min_fps;
	u32 max_fps;
	u32 prg_fet;
	struct mdss_panel_roi_alignment roi_alignment;

	u32 cont_splash_enabled;
	bool esd_rdy;
	bool partial_update_supported; /* value from dts if pu is supported */
	bool partial_update_enabled; /* is pu currently allowed */
	u32 dcs_cmd_by_left;
	u32 partial_update_roi_merge;
	struct ion_handle *splash_ihdl;
	int panel_power_state;
	int compression_mode;

	uint32_t panel_dead;
	u32 panel_force_dead;
	u32 panel_orientation;
	bool dynamic_switch_pending;
	bool is_lpm_mode;
	bool is_split_display; /* two DSIs in one display, pp split or not */
	bool use_pingpong_split;

	/*
	 * index[0] = left layer mixer, value of 0 not valid
	 * index[1] = right layer mixer, 0 is possible
	 *
	 * Ex(1): 1080x1920 display using single DSI and single lm, [1080 0]
	 * Ex(2): 1440x2560 display using two DSIs and two lms,
	 *        each with 720x2560, [720 0]
	 * Ex(3): 1440x2560 display using single DSI w/ compression and
	 *        single lm, [1440 0]
	 * Ex(4): 1440x2560 display using single DSI w/ compression and
	 *        two lms, [720 720]
	 * Ex(5): 1080x1920 display using single DSI and two lm, [540 540]
	 * Ex(6): 1080x1920 display using single DSI and two lm,
	 *        [880 400] - not practical but possible
	 */
	u32 lm_widths[2];

	bool is_prim_panel;
	bool is_pluggable;
	char display_id[MDSS_DISPLAY_ID_MAX_LEN];
	bool is_cec_supported;

	/* refer sim_panel_modes enum for different modes */
	u8 sim_panel_mode;

	void *edid_data;
	void *dba_data;
	void *cec_data;

	char panel_name[MDSS_MAX_PANEL_LEN];
	char panel_family_name[MDSS_MAX_PANEL_LEN];
	u32 panel_ver;
	char panel_supplier[8];
	struct mdss_mdp_pp_tear_check te;

	/*
	 * Value of 2 only when single DSI is configured with 2 DSC
	 * encoders. When 2 encoders are used, currently both use
	 * same configuration.
	 */
	u8 dsc_enc_total; /* max 2 */
	struct dsc_desc dsc;

	/*
	 * To determine, if DSC panel requires the pps to be sent
	 * before or after the switch, during dynamic resolution switching
	 */
	bool send_pps_before_switch;

	struct lcd_panel_info lcdc;
	struct fbc_panel_info fbc;
	struct mipi_panel_info mipi;
	struct lvds_panel_info lvds;
	struct edp_panel_info edp;

	bool is_dba_panel;

	/*
	 * Delay(in ms) to accommodate s/w delay while
	 * configuring the event timer wakeup logic.
	 */
	u32 adjust_timer_delay_ms;

	/* debugfs structure for the panel */
	struct mdss_panel_debugfs_info *debugfs_info;

	/* persistence mode on/off */
	bool persist_mode;

	/* HDR properties of display panel*/
	struct mdss_panel_hdr_properties hdr_properties;

	u32 disp_on_check_val;
	bool no_panel_read_support;
	bool no_panel_on_read_support;
	struct panel_param *param[PARAM_ID_NUM];
	bool hbm_restore;
	u32 hbm_type;
	u32 bl_hbm_off;
	u32 forced_tx_mode_ftr_enabled;
	u32 forced_tx_mode_state;
	bool opr_stats_enabled;
};

struct mdss_panel_timing {
	struct list_head list;
	const char *name;

	u32 xres;
	u32 yres;

	u32 h_back_porch;
	u32 h_front_porch;
	u32 h_pulse_width;
	u32 hsync_skew;
	u32 v_back_porch;
	u32 v_front_porch;
	u32 v_pulse_width;

	u32 border_top;
	u32 border_bottom;
	u32 border_left;
	u32 border_right;

	u32 lm_widths[2];

	u64 clk_rate;
	char frame_rate;

	u8 dsc_enc_total;
	struct dsc_desc dsc;
	struct fbc_panel_info fbc;
	u32 compression_mode;

	struct mdss_mdp_pp_tear_check te;
	struct mdss_panel_roi_alignment roi_alignment;
};

struct mdss_panel_data {
	struct mdss_panel_info panel_info;
	void (*set_backlight) (struct mdss_panel_data *pdata, u32 bl_level);
	int (*apply_display_setting)(struct mdss_panel_data *pdata, u32 mode);
	int (*set_param)(struct mdss_panel_data *pdata, u16 id, u16 value);
	unsigned char *mmss_cc_base;

	/**
	 * event_handler() - callback handler for MDP core events
	 * @pdata:	Pointer refering to the panel struct associated to this
	 *		event. Can be used to retrieve panel info.
	 * @e:		Event being generated, see enum mdss_intf_events
	 * @arg:	Optional argument to pass some info from some events.
	 *
	 * Used to register handler to be used to propagate different events
	 * happening in MDP core driver. Panel driver can listen for any of
	 * these events to perform appropriate actions for panel initialization
	 * and teardown.
	 */
	int (*event_handler) (struct mdss_panel_data *pdata, int e, void *arg);
	struct device_node *(*get_fb_node)(struct platform_device *pdev);
	bool (*get_idle)(struct mdss_panel_data *pdata);

	struct list_head timings_list;
	struct mdss_panel_timing *current_timing;
	bool active;

	/* To store dsc cfg name passed by bootloader */
	char dsc_cfg_np_name[MDSS_MAX_PANEL_LEN];
	struct mdss_panel_data *next;

	int panel_te_gpio;
	struct completion te_done;
};

/*
 * Start OPR buffer size to hold 3 hours of screen on time with 8 second
 * polling.  Max size is 24 hours of data
 */
#define OPR_REC_NUM_PER_HR 450
#define OPR_REC_NUM_INIT (OPR_REC_NUM_PER_HR * 3)
#define OPR_REC_NUM_MAX (OPR_REC_NUM_PER_HR * 24)

struct __attribute__((__packed__)) mdss_panel_debugfs_stats_opr_record {
	time_t time_secs;
	u8 w;
	u8 r;
	u8 g;
	u8 b;
	u8 brightness;
};

struct mdss_panel_debugfs_stats_opr {
	struct mutex opr_lock;
	int collected_recs;
	int max_recs;
	int alloc_recs;
	struct mdss_panel_debugfs_stats_opr_record *recs;
};

struct mdss_panel_debugfs_stats {
	struct mdss_panel_debugfs_stats_opr *opr;
	u32 wait4pingpong_timeout_cnt;
	u32 wait4pingpong_timeout_cnt_prev;
	u32 fifo_err_cnt_prev;
	u32 phy_err_cnt_prev;
	u32 err_cnt_prev;
};
int mdss_panel_debufs_stats_opr_alloc(struct mdss_panel_debugfs_stats_opr *opr,
				int new_alloc, bool copy);

struct mdss_panel_debugfs_info {
	struct dentry *root;
	struct dentry *parent;
	struct mdss_panel_info panel_info;
	u32 override_flag;
	struct mdss_panel_debugfs_info *next;
	struct mdss_panel_data *rt_pdata;
	struct mdss_panel_debugfs_stats stats;
};

/**
 * mdss_get_panel_framerate() - get panel frame rate based on panel information
 * @panel_info:	Pointer to panel info containing all panel information
 */
static inline u32 mdss_panel_get_framerate(struct mdss_panel_info *panel_info,
					   u32 flags)
{
	u32 frame_rate, pixel_total;
	u64 rate;
	struct mdss_panel_data *panel_data =
			container_of(panel_info, typeof(*panel_data),
					panel_info);
	bool idle = false;

	if (panel_info == NULL) {
		frame_rate = DEFAULT_FRAME_RATE;
		goto end;
	}

	switch (panel_info->type) {
	case MIPI_VIDEO_PANEL:
	case MIPI_CMD_PANEL:
		frame_rate = panel_info->mipi.frame_rate;
		if (panel_data->get_idle)
			idle = panel_data->get_idle(panel_data);
		if (idle)
			frame_rate = panel_info->mipi.frame_rate_idle;
		else
			frame_rate = panel_info->mipi.frame_rate;
		break;
	case EDP_PANEL:
		frame_rate = panel_info->edp.frame_rate;
		break;
	case WRITEBACK_PANEL:
		frame_rate = DEFAULT_FRAME_RATE;
		break;
	case DTV_PANEL:
		if (panel_info->dynamic_fps) {
			frame_rate = panel_info->lcdc.frame_rate;
			break;
		}
	default:
		pixel_total = (panel_info->lcdc.h_back_porch +
			  panel_info->lcdc.h_front_porch +
			  panel_info->lcdc.h_pulse_width +
			  panel_info->xres) *
			 (panel_info->lcdc.v_back_porch +
			  panel_info->lcdc.v_front_porch +
			  panel_info->lcdc.v_pulse_width +
			  panel_info->yres);
		if (pixel_total) {
			rate = panel_info->clk_rate * KHZ_TO_HZ;
			do_div(rate, pixel_total);
			frame_rate = (u32)rate;
		} else {
			frame_rate = DEFAULT_FRAME_RATE;
		}
		break;
	}
end:
	if (flags == FPS_RESOLUTION_KHZ) {
		if (!(frame_rate / KHZ_TO_HZ))
			frame_rate *= KHZ_TO_HZ;
	} else if (flags == FPS_RESOLUTION_HZ) {
		if (frame_rate / KHZ_TO_HZ)
			frame_rate /= KHZ_TO_HZ;
	}

	return frame_rate;
}

/*
 * mdss_panel_get_vtotal() - return panel vertical height
 * @pinfo:	Pointer to panel info containing all panel information
 *
 * Returns the total height of the panel including any blanking regions
 * which are not visible to user but used to calculate panel pixel clock.
 */
static inline int mdss_panel_get_vtotal(struct mdss_panel_info *pinfo)
{
	return pinfo->yres + pinfo->lcdc.v_back_porch +
			pinfo->lcdc.v_front_porch +
			pinfo->lcdc.v_pulse_width+
			pinfo->lcdc.border_top +
			pinfo->lcdc.border_bottom;
}

/*
 * mdss_panel_get_htotal() - return panel horizontal width
 * @pinfo:	Pointer to panel info containing all panel information
 * @compression: true to factor fbc settings, false to ignore.
 *
 * Returns the total width of the panel including any blanking regions
 * which are not visible to user but used for calculations. For certain
 * usescases where the fbc parameters need to be ignored like bandwidth
 * calculation, the appropriate flag can be passed.
 */
static inline int mdss_panel_get_htotal(struct mdss_panel_info *pinfo, bool
		compression)
{
	struct dsc_desc *dsc = NULL;

	int adj_xres = pinfo->xres + pinfo->lcdc.border_left +
				pinfo->lcdc.border_right;

	if (compression) {
		if (pinfo->compression_mode == COMPRESSION_DSC) {
			dsc = &pinfo->dsc;
			adj_xres = dsc->pclk_per_line;
		} else if (pinfo->fbc.enabled) {
			adj_xres = mult_frac(adj_xres,
				pinfo->fbc.target_bpp, pinfo->bpp);
		}
	}

	return adj_xres + pinfo->lcdc.h_back_porch +
		pinfo->lcdc.h_front_porch +
		pinfo->lcdc.h_pulse_width;
}

static inline bool is_dsc_compression(struct mdss_panel_info *pinfo)
{
	if (pinfo)
		return (pinfo->compression_mode == COMPRESSION_DSC);

	return false;
}

int mdss_register_panel(struct platform_device *pdev,
	struct mdss_panel_data *pdata);

/*
 * mdss_panel_is_power_off: - checks if a panel is off
 * @panel_power_state: enum identifying the power state to be checked
 */
static inline bool mdss_panel_is_power_off(int panel_power_state)
{
	return (panel_power_state == MDSS_PANEL_POWER_OFF);
}

/**
 * mdss_panel_is_power_on_interactive: - checks if a panel is on and interactive
 * @panel_power_state: enum identifying the power state to be checked
 *
 * This function returns true only is the panel is fully interactive and
 * opertaing in normal mode.
 */
static inline bool mdss_panel_is_power_on_interactive(int panel_power_state)
{
	return (panel_power_state == MDSS_PANEL_POWER_ON);
}

/**
 * mdss_panel_is_panel_power_on: - checks if a panel is on
 * @panel_power_state: enum identifying the power state to be checked
 *
 * A panel is considered to be on as long as it can accept any commands
 * or data. Sometimes it is posible to program the panel to be in a low
 * power non-interactive state. This function returns false only if panel
 * has explicitly been turned off.
 */
static inline bool mdss_panel_is_power_on(int panel_power_state)
{
	return !mdss_panel_is_power_off(panel_power_state);
}

/**
 * mdss_panel_is_panel_power_on_lp: - checks if a panel is in a low power mode
 * @pdata: pointer to the panel struct associated to the panel
 * @panel_power_state: enum identifying the power state to be checked
 *
 * This function returns true if the panel is in an intermediate low power
 * state where it is still on but not fully interactive. It may or may not
 * accept any commands and display updates.
 */
static inline bool mdss_panel_is_power_on_lp(int panel_power_state)
{
	return !mdss_panel_is_power_off(panel_power_state) &&
		!mdss_panel_is_power_on_interactive(panel_power_state);
}

/**
 * mdss_panel_is_panel_power_on_ulp: - checks if panel is in ultra low power mode
 * @pdata: pointer to the panel struct associated to the panel
 * @panel_power_state: enum identifying the power state to be checked
 *
 * This function returns true if the panel is in a ultra low power
 * state where it is still on but cannot recieve any display updates.
 */
static inline bool mdss_panel_is_power_on_ulp(int panel_power_state)
{
	return panel_power_state == MDSS_PANEL_POWER_LP2;
}

/**
 * mdss_panel_update_clk_rate() - update the clock rate based on panel timing
 *				information.
 * @panel_info:	Pointer to panel info containing all panel information
 * @fps: frame rate of the panel
 */
static inline void mdss_panel_update_clk_rate(struct mdss_panel_info *pinfo,
	u32 fps)
{
	struct lcd_panel_info *lcdc = &pinfo->lcdc;
	u32 htotal, vtotal;

	if (pinfo->type == DTV_PANEL) {
		htotal = pinfo->xres + lcdc->h_front_porch +
				lcdc->h_back_porch + lcdc->h_pulse_width;
		vtotal = pinfo->yres + lcdc->v_front_porch +
				lcdc->v_back_porch + lcdc->v_pulse_width;

		pinfo->clk_rate = mult_frac(htotal * vtotal, fps, 1000);

		pr_debug("vtotal %d, htotal %d, rate %llu\n",
			vtotal, htotal, pinfo->clk_rate);
	}
}

/**
 * mdss_panel_calc_frame_rate() - calculate panel frame rate based on panel timing
 *				information.
 * @panel_info:	Pointer to panel info containing all panel information
 */
static inline u8 mdss_panel_calc_frame_rate(struct mdss_panel_info *pinfo)
{
		u32 pixel_total = 0;
		u8 frame_rate = 0;
		unsigned long pclk_rate = pinfo->mipi.dsi_pclk_rate;
		u32 xres;

		xres = pinfo->xres;
		if (pinfo->compression_mode == COMPRESSION_DSC)
			xres /= 3;

		pixel_total = (pinfo->lcdc.h_back_porch +
			  pinfo->lcdc.h_front_porch +
			  pinfo->lcdc.h_pulse_width +
			  xres) *
			 (pinfo->lcdc.v_back_porch +
			  pinfo->lcdc.v_front_porch +
			  pinfo->lcdc.v_pulse_width +
			  pinfo->yres);

		if (pclk_rate && pixel_total)
			frame_rate =
				DIV_ROUND_CLOSEST(pclk_rate, pixel_total);
		else
			frame_rate = pinfo->panel_max_fps;

		return frame_rate;
}

/**
 * mdss_panel_intf_type: - checks if a given intf type is primary
 * @intf_val: panel interface type of the individual controller
 *
 * Individual controller queries with MDP to check if it is
 * configured as the primary interface.
 *
 * returns a pointer to the configured structure mdss_panel_cfg
 * to the controller that's configured as the primary panel interface.
 * returns NULL on error or if @intf_val is not the configured
 * controller.
 */
struct mdss_panel_cfg *mdss_panel_intf_type(int intf_val);

/**
 * mdss_is_ready() - checks if mdss is probed and ready
 *
 * Checks if mdss resources have been initialized
 *
 * returns true if mdss is ready, else returns false.
 */
bool mdss_is_ready(void);
int mdss_rect_cmp(struct mdss_rect *rect1, struct mdss_rect *rect2);

/**
 * mdss_panel_override_te_params() - overrides TE params to enable SW TE
 * @pinfo: panel info
 */
void mdss_panel_override_te_params(struct mdss_panel_info *pinfo);

/**
 * mdss_panel_dsc_parameters_calc: calculate DSC parameters
 * @dsc: pointer to DSC structure associated with panel_info
 */
void mdss_panel_dsc_parameters_calc(struct dsc_desc *dsc);

/**
 * mdss_panel_dsc_update_pic_dim: update DSC structure with picture dimension
 * @dsc: pointer to DSC structure associated with panel_info
 * @pic_width: Picture width
 * @pic_height: Picture height
 */
void mdss_panel_dsc_update_pic_dim(struct dsc_desc *dsc,
	int pic_width, int pic_height);

/**
 * mdss_panel_dsc_initial_line_calc: update DSC initial line buffering
 * @dsc: pointer to DSC structure associated with panel_info
 * @enc_ip_width: uncompressed input width for DSC enc represented by @dsc
 *              i.e.
 *                 * 720 for full frame single_display_dual_lm: 1440x2560
 *                 * 1080 for full frame dual_display_dual_lm: 2160x3840
 *                 * 360 for partial frame single_display_dual_lm: 360x2560
 */
void mdss_panel_dsc_initial_line_calc(struct dsc_desc *dsc, int enc_ip_width);

/**
 * mdss_panel_dsc_pclk_param_calc: calculate DSC params related to DSI
 * @dsc: pointer to DSC structure associated with panel_info
 * @intf_width: Uncompressed width per interface
 *              i.e.
 *                 * 1440 for full frame single_display_dual_lm: 1440x2560
 *                 * 1080 for full frame dual_display_dual_lm: 2160x3840
 *                 * 720 for partial frame single_display_dual_lm: 720x2560
 */
void mdss_panel_dsc_pclk_param_calc(struct dsc_desc *dsc, int intf_width);

/**
 * mdss_panel_dsc_prepare_pps_buf - prepares Picture Parameter Set to be sent to panel
 * @dsc: pointer to DSC structure associated with panel_info
 * @buf: buffer that holds PPS
 * @pps_id: pps_identifier
 *
 * returns length of the PPS buffer.
 */
int mdss_panel_dsc_prepare_pps_buf(struct dsc_desc *dsc, char *buf,
	int pps_id);
#ifdef CONFIG_FB_MSM_MDSS
int mdss_panel_debugfs_init(struct mdss_panel_info *panel_info,
		char const *panel_name);
void mdss_panel_debugfs_cleanup(struct mdss_panel_info *panel_info);
void mdss_panel_debugfsinfo_to_panelinfo(struct mdss_panel_info *panel_info);

/*
 * mdss_panel_info_from_timing() - populate panel info from panel timing
 * @pt:		pointer to source panel timing
 * @pinfo:	pointer to destination panel info
 *
 * Populates relevant data from panel timing into panel info
 */
void mdss_panel_info_from_timing(struct mdss_panel_timing *pt,
		struct mdss_panel_info *pinfo);

/**
 * mdss_panel_get_timing_by_name() - return panel timing with matching name
 * @pdata:	pointer to panel data struct containing list of panel timings
 * @name:	name of the panel timing to be returned
 *
 * Looks through list of timings provided in panel data and returns pointer
 * to panel timing matching it. If none is found, NULL is returned.
 */
struct mdss_panel_timing *mdss_panel_get_timing_by_name(
		struct mdss_panel_data *pdata,
		const char *name);
#else
static inline int mdss_panel_debugfs_init(
		struct mdss_panel_info *panel_info,
		char const *panel_name) { return 0; };
static inline void mdss_panel_debugfs_cleanup(
		struct mdss_panel_info *panel_info) { };
static inline void mdss_panel_debugfsinfo_to_panelinfo(
		struct mdss_panel_info *panel_info) { };
static inline void mdss_panel_info_from_timing(struct mdss_panel_timing *pt,
		struct mdss_panel_info *pinfo) { };
static inline struct mdss_panel_timing *mdss_panel_get_timing_by_name(
		struct mdss_panel_data *pdata,
		const char *name) { return NULL; };
#endif

static inline bool mdss_panel_param_is_supported(struct mdss_panel_info *p,
	u16 id)
{
	if (id < PARAM_ID_NUM && p && p->param[id] &&
		p->param[id]->is_supported)
		return true;

	return false;
};

static inline bool mdss_panel_param_is_hbm_on(struct mdss_panel_info *p)
{
	u16 id = PARAM_HBM_ID;

	if (mdss_panel_param_is_supported(p, id) &&
		p->param[id]->value == HBM_ON_STATE)
		return true;

	return false;
};
#endif /* MDSS_PANEL_H */
