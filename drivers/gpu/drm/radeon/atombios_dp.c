/*
 * Copyright 2007-8 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Dave Airlie
 *          Alex Deucher
 *          Jerome Glisse
 */
#include <drm/drmP.h>
#include <drm/radeon_drm.h>
#include "radeon.h"

#include "atom.h"
#include "atom-bits.h"
#include <drm/drm_dp_helper.h>

/* move these to drm_dp_helper.c/h */
#define DP_LINK_CONFIGURATION_SIZE 9
#define DP_DPCD_SIZE DP_RECEIVER_CAP_SIZE

static char *voltage_names[] = {
        "0.4V", "0.6V", "0.8V", "1.2V"
};
static char *pre_emph_names[] = {
        "0dB", "3.5dB", "6dB", "9.5dB"
};

/***** radeon AUX functions *****/

/* Atom needs data in little endian format
 * so swap as appropriate when copying data to
 * or from atom. Note that atom operates on
 * dw units.
 */
static void radeon_copy_swap(u8 *dst, u8 *src, u8 num_bytes, bool to_le)
{
#ifdef __BIG_ENDIAN
	u8 src_tmp[20], dst_tmp[20]; /* used for byteswapping */
	u32 *dst32, *src32;
	int i;

	memcpy(src_tmp, src, num_bytes);
	src32 = (u32 *)src_tmp;
	dst32 = (u32 *)dst_tmp;
	if (to_le) {
		for (i = 0; i < ((num_bytes + 3) / 4); i++)
			dst32[i] = cpu_to_le32(src32[i]);
		memcpy(dst, dst_tmp, num_bytes);
	} else {
		u8 dws = num_bytes & ~3;
		for (i = 0; i < ((num_bytes + 3) / 4); i++)
			dst32[i] = le32_to_cpu(src32[i]);
		memcpy(dst, dst_tmp, dws);
		if (num_bytes % 4) {
			for (i = 0; i < (num_bytes % 4); i++)
				dst[dws+i] = dst_tmp[dws+i];
		}
	}
#else
	memcpy(dst, src, num_bytes);
#endif
}

union aux_channel_transaction {
	PROCESS_AUX_CHANNEL_TRANSACTION_PS_ALLOCATION v1;
	PROCESS_AUX_CHANNEL_TRANSACTION_PARAMETERS_V2 v2;
};

static int radeon_process_aux_ch(struct radeon_i2c_chan *chan,
				 u8 *send, int send_bytes,
				 u8 *recv, int recv_size,
				 u8 delay, u8 *ack)
{
	struct drm_device *dev = chan->dev;
	struct radeon_device *rdev = dev->dev_private;
	union aux_channel_transaction args;
	int index = GetIndexIntoMasterTable(COMMAND, ProcessAuxChannelTransaction);
	unsigned char *base;
	int recv_bytes;

	memset(&args, 0, sizeof(args));

	base = (unsigned char *)(rdev->mode_info.atom_context->scratch + 1);

	radeon_copy_swap(base, send, send_bytes, true);

	args.v1.lpAuxRequest = cpu_to_le16((u16)(0 + 4));
	args.v1.lpDataOut = cpu_to_le16((u16)(16 + 4));
	args.v1.ucDataOutLen = 0;
	args.v1.ucChannelID = chan->rec.i2c_id;
	args.v1.ucDelay = delay / 10;
	if (ASIC_IS_DCE4(rdev))
		args.v2.ucHPD_ID = chan->rec.hpd;

	atom_execute_table(rdev->mode_info.atom_context, index, (uint32_t *)&args);

	*ack = args.v1.ucReplyStatus;

	/* timeout */
	if (args.v1.ucReplyStatus == 1) {
		DRM_DEBUG_KMS("dp_aux_ch timeout\n");
		return -ETIMEDOUT;
	}

	/* flags not zero */
	if (args.v1.ucReplyStatus == 2) {
		DRM_DEBUG_KMS("dp_aux_ch flags not zero\n");
		return -EBUSY;
	}

	/* error */
	if (args.v1.ucReplyStatus == 3) {
		DRM_DEBUG_KMS("dp_aux_ch error\n");
		return -EIO;
	}

	recv_bytes = args.v1.ucDataOutLen;
	if (recv_bytes > recv_size)
		recv_bytes = recv_size;

	if (recv && recv_size)
		radeon_copy_swap(recv, base + 16, recv_bytes, false);

	return recv_bytes;
}

static int radeon_dp_aux_native_write(struct radeon_connector *radeon_connector,
				      u16 address, u8 *send, u8 send_bytes, u8 delay)
{
	struct radeon_connector_atom_dig *dig_connector = radeon_connector->con_priv;
	int ret;
	u8 msg[20];
	int msg_bytes = send_bytes + 4;
	u8 ack;
	unsigned retry;

	if (send_bytes > 16)
		return -1;

	msg[0] = address;
	msg[1] = address >> 8;
	msg[2] = AUX_NATIVE_WRITE << 4;
	msg[3] = (msg_bytes << 4) | (send_bytes - 1);
	memcpy(&msg[4], send, send_bytes);

	for (retry = 0; retry < 4; retry++) {
		ret = radeon_process_aux_ch(dig_connector->dp_i2c_bus,
					    msg, msg_bytes, NULL, 0, delay, &ack);
		if (ret == -EBUSY)
			continue;
		else if (ret < 0)
			return ret;
		if ((ack & AUX_NATIVE_REPLY_MASK) == AUX_NATIVE_REPLY_ACK)
			return send_bytes;
		else if ((ack & AUX_NATIVE_REPLY_MASK) == AUX_NATIVE_REPLY_DEFER)
			udelay(400);
		else
			return -EIO;
	}

	return -EIO;
}

static int radeon_dp_aux_native_read(struct radeon_connector *radeon_connector,
				     u16 address, u8 *recv, int recv_bytes, u8 delay)
{
	struct radeon_connector_atom_dig *dig_connector = radeon_connector->con_priv;
	u8 msg[4];
	int msg_bytes = 4;
	u8 ack;
	int ret;
	unsigned retry;

	msg[0] = address;
	msg[1] = address >> 8;
	msg[2] = AUX_NATIVE_READ << 4;
	msg[3] = (msg_bytes << 4) | (recv_bytes - 1);

	for (retry = 0; retry < 4; retry++) {
		ret = radeon_process_aux_ch(dig_connector->dp_i2c_bus,
					    msg, msg_bytes, recv, recv_bytes, delay, &ack);
		if (ret == -EBUSY)
			continue;
		else if (ret < 0)
			return ret;
		if ((ack & AUX_NATIVE_REPLY_MASK) == AUX_NATIVE_REPLY_ACK)
			return ret;
		else if ((ack & AUX_NATIVE_REPLY_MASK) == AUX_NATIVE_REPLY_DEFER)
			udelay(400);
		else if (ret == 0)
			return -EPROTO;
		else
			return -EIO;
	}

	return -EIO;
}

static void radeon_write_dpcd_reg(struct radeon_connector *radeon_connector,
				 u16 reg, u8 val)
{
	radeon_dp_aux_native_write(radeon_connector, reg, &val, 1, 0);
}

static u8 radeon_read_dpcd_reg(struct radeon_connector *radeon_connector,
			       u16 reg)
{
	u8 val = 0;

	radeon_dp_aux_native_read(radeon_connector, reg, &val, 1, 0);

	return val;
}

int radeon_dp_i2c_aux_ch(struct i2c_adapter *adapter, int mode,
			 u8 write_byte, u8 *read_byte)
{
	struct i2c_algo_dp_aux_data *algo_data = adapter->algo_data;
	struct radeon_i2c_chan *auxch = (struct radeon_i2c_chan *)adapter;
	u16 address = algo_data->address;
	u8 msg[5];
	u8 reply[2];
	unsigned retry;
	int msg_bytes;
	int reply_bytes = 1;
	int ret;
	u8 ack;

	/* Set up the command byte */
	if (mode & MODE_I2C_READ)
		msg[2] = AUX_I2C_READ << 4;
	else
		msg[2] = AUX_I2C_WRITE << 4;

	if (!(mode & MODE_I2C_STOP))
		msg[2] |= AUX_I2C_MOT << 4;

	msg[0] = address;
	msg[1] = address >> 8;

	switch (mode) {
	case MODE_I2C_WRITE:
		msg_bytes = 5;
		msg[3] = msg_bytes << 4;
		msg[4] = write_byte;
		break;
	case MODE_I2C_READ:
		msg_bytes = 4;
		msg[3] = msg_bytes << 4;
		break;
	default:
		msg_bytes = 4;
		msg[3] = 3 << 4;
		break;
	}

	for (retry = 0; retry < 4; retry++) {
		ret = radeon_process_aux_ch(auxch,
					    msg, msg_bytes, reply, reply_bytes, 0, &ack);
		if (ret == -EBUSY)
			continue;
		else if (ret < 0) {
			DRM_DEBUG_KMS("aux_ch failed %d\n", ret);
			return ret;
		}

		switch (ack & AUX_NATIVE_REPLY_MASK) {
		case AUX_NATIVE_REPLY_ACK:
			/* I2C-over-AUX Reply field is only valid
			 * when paired with AUX ACK.
			 */
			break;
		case AUX_NATIVE_REPLY_NACK:
			DRM_DEBUG_KMS("aux_ch native nack\n");
			return -EREMOTEIO;
		case AUX_NATIVE_REPLY_DEFER:
			DRM_DEBUG_KMS("aux_ch native defer\n");
			udelay(400);
			continue;
		default:
			DRM_ERROR("aux_ch invalid native reply 0x%02x\n", ack);
			return -EREMOTEIO;
		}

		switch (ack & AUX_I2C_REPLY_MASK) {
		case AUX_I2C_REPLY_ACK:
			if (mode == MODE_I2C_READ)
				*read_byte = reply[0];
			return ret;
		case AUX_I2C_REPLY_NACK:
			DRM_DEBUG_KMS("aux_i2c nack\n");
			return -EREMOTEIO;
		case AUX_I2C_REPLY_DEFER:
			DRM_DEBUG_KMS("aux_i2c defer\n");
			udelay(400);
			break;
		default:
			DRM_ERROR("aux_i2c invalid reply 0x%02x\n", ack);
			return -EREMOTEIO;
		}
	}

	DRM_DEBUG_KMS("aux i2c too many retries, giving up\n");
	return -EREMOTEIO;
}

/***** general DP utility functions *****/

#define DP_VOLTAGE_MAX         DP_TRAIN_VOLTAGE_SWING_1200
#define DP_PRE_EMPHASIS_MAX    DP_TRAIN_PRE_EMPHASIS_9_5

static void dp_get_adjust_train(u8 link_status[DP_LINK_STATUS_SIZE],
				int lane_count,
				u8 train_set[4])
{
	u8 v = 0;
	u8 p = 0;
	int lane;

	for (lane = 0; lane < lane_count; lane++) {
		u8 this_v = drm_dp_get_adjust_request_voltage(link_status, lane);
		u8 this_p = drm_dp_get_adjust_request_pre_emphasis(link_status, lane);

		DRM_DEBUG_KMS("requested signal parameters: lane %d voltage %s pre_emph %s\n",
			  lane,
			  voltage_names[this_v >> DP_TRAIN_VOLTAGE_SWING_SHIFT],
			  pre_emph_names[this_p >> DP_TRAIN_PRE_EMPHASIS_SHIFT]);

		if (this_v > v)
			v = this_v;
		if (this_p > p)
			p = this_p;
	}

	if (v >= DP_VOLTAGE_MAX)
		v |= DP_TRAIN_MAX_SWING_REACHED;

	if (p >= DP_PRE_EMPHASIS_MAX)
		p |= DP_TRAIN_MAX_PRE_EMPHASIS_REACHED;

	DRM_DEBUG_KMS("using signal parameters: voltage %s pre_emph %s\n",
		  voltage_names[(v & DP_TRAIN_VOLTAGE_SWING_MASK) >> DP_TRAIN_VOLTAGE_SWING_SHIFT],
		  pre_emph_names[(p & DP_TRAIN_PRE_EMPHASIS_MASK) >> DP_TRAIN_PRE_EMPHASIS_SHIFT]);

	for (lane = 0; lane < 4; lane++)
		train_set[lane] = v | p;
}

/* convert bits per color to bits per pixel */
/* get bpc from the EDID */
static int convert_bpc_to_bpp(int bpc)
{
	if (bpc == 0)
		return 24;
	else
		return bpc * 3;
}

/* get the max pix clock supported by the link rate and lane num */
static int dp_get_max_dp_pix_clock(int link_rate,
				   int lane_num,
				   int bpp)
{
	return (link_rate * lane_num * 8) / bpp;
}

/***** radeon specific DP functions *****/

/* First get the min lane# when low rate is used according to pixel clock
 * (prefer low rate), second check max lane# supported by DP panel,
 * if the max lane# < low rate lane# then use max lane# instead.
 */
static int radeon_dp_get_dp_lane_number(struct drm_connector *connector,
					u8 dpcd[DP_DPCD_SIZE],
					int pix_clock)
{
	int bpp = convert_bpc_to_bpp(radeon_get_monitor_bpc(connector));
	int max_link_rate = drm_dp_max_link_rate(dpcd);
	int max_lane_num = drm_dp_max_lane_count(dpcd);
	int lane_num;
	int max_dp_pix_clock;

	for (lane_num = 1; lane_num < max_lane_num; lane_num <<= 1) {
		max_dp_pix_clock = dp_get_max_dp_pix_clock(max_link_rate, lane_num, bpp);
		if (pix_clock <= max_dp_pix_clock)
			break;
	}

	return lane_num;
}

static int radeon_dp_get_dp_link_clock(struct drm_connector *connector,
				       u8 dpcd[DP_DPCD_SIZE],
				       int pix_clock)
{
	int bpp = convert_bpc_to_bpp(radeon_get_monitor_bpc(connector));
	int lane_num, max_pix_clock;

	if (radeon_connector_encoder_get_dp_bridge_encoder_id(connector) ==
	    ENCODER_OBJECT_ID_NUTMEG)
		return 270000;

	lane_num = radeon_dp_get_dp_lane_number(connector, dpcd, pix_clock);
	max_pix_clock = dp_get_max_dp_pix_clock(162000, lane_num, bpp);
	if (pix_clock <= max_pix_clock)
		return 162000;
	max_pix_clock = dp_get_max_dp_pix_clock(270000, lane_num, bpp);
	if (pix_clock <= max_pix_clock)
		return 270000;
	if (radeon_connector_is_dp12_capable(connector)) {
		max_pix_clock = dp_get_max_dp_pix_clock(540000, lane_num, bpp);
		if (pix_clock <= max_pix_clock)
			return 540000;
	}

	return drm_dp_max_link_rate(dpcd);
}

static u8 radeon_dp_encoder_service(struct radeon_device *rdev,
				    int action, int dp_clock,
				    u8 ucconfig, u8 lane_num)
{
	DP_ENCODER_SERVICE_PARAMETERS args;
	int index = GetIndexIntoMasterTable(COMMAND, DPEncoderService);

	memset(&args, 0, sizeof(args));
	args.ucLinkClock = dp_clock / 10;
	args.ucConfig = ucconfig;
	args.ucAction = action;
	args.ucLaneNum = lane_num;
	args.ucStatus = 0;

	atom_execute_table(rdev->mode_info.atom_context, index, (uint32_t *)&args);
	return args.ucStatus;
}

u8 radeon_dp_getsinktype(struct radeon_connector *radeon_connector)
{
	struct radeon_connector_atom_dig *dig_connector = radeon_connector->con_priv;
	struct drm_device *dev = radeon_connector->base.dev;
	struct radeon_device *rdev = dev->dev_private;

	return radeon_dp_encoder_service(rdev, ATOM_DP_ACTION_GET_SINK_TYPE, 0,
					 dig_connector->dp_i2c_bus->rec.i2c_id, 0);
}

static void radeon_dp_probe_oui(struct radeon_connector *radeon_connector)
{
	struct radeon_connector_atom_dig *dig_connector = radeon_connector->con_priv;
	u8 buf[3];

	if (!(dig_connector->dpcd[DP_DOWN_STREAM_PORT_COUNT] & DP_OUI_SUPPORT))
		return;

	if (radeon_dp_aux_native_read(radeon_connector, DP_SINK_OUI, buf, 3, 0))
		DRM_DEBUG_KMS("Sink OUI: %02hx%02hx%02hx\n",
			      buf[0], buf[1], buf[2]);

	if (radeon_dp_aux_native_read(radeon_connector, DP_BRANCH_OUI, buf, 3, 0))
		DRM_DEBUG_KMS("Branch OUI: %02hx%02hx%02hx\n",
			      buf[0], buf[1], buf[2]);
}

bool radeon_dp_getdpcd(struct radeon_connector *radeon_connector)
{
	struct radeon_connector_atom_dig *dig_connector = radeon_connector->con_priv;
	u8 msg[DP_DPCD_SIZE];
	int ret, i;

	ret = radeon_dp_aux_native_read(radeon_connector, DP_DPCD_REV, msg,
					DP_DPCD_SIZE, 0);
	if (ret > 0) {
		memcpy(dig_connector->dpcd, msg, DP_DPCD_SIZE);
		DRM_DEBUG_KMS("DPCD: ");
		for (i = 0; i < DP_DPCD_SIZE; i++)
			DRM_DEBUG_KMS("%02x ", msg[i]);
		DRM_DEBUG_KMS("\n");

		radeon_dp_probe_oui(radeon_connector);

		return true;
	}
	dig_connector->dpcd[0] = 0;
	return false;
}

int radeon_dp_get_panel_mode(struct drm_encoder *encoder,
			     struct drm_connector *connector)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_connector *radeon_connector = to_radeon_connector(connector);
	int panel_mode = DP_PANEL_MODE_EXTERNAL_DP_MODE;
	u16 dp_bridge = radeon_connector_encoder_get_dp_bridge_encoder_id(connector);
	u8 tmp;

	if (!ASIC_IS_DCE4(rdev))
		return panel_mode;

	if (dp_bridge != ENCODER_OBJECT_ID_NONE) {
		/* DP bridge chips */
		tmp = radeon_read_dpcd_reg(radeon_connector, DP_EDP_CONFIGURATION_CAP);
		if (tmp & 1)
			panel_mode = DP_PANEL_MODE_INTERNAL_DP2_MODE;
		else if ((dp_bridge == ENCODER_OBJECT_ID_NUTMEG) ||
			 (dp_bridge == ENCODER_OBJECT_ID_TRAVIS))
			panel_mode = DP_PANEL_MODE_INTERNAL_DP1_MODE;
		else
			panel_mode = DP_PANEL_MODE_EXTERNAL_DP_MODE;
	} else if (connector->connector_type == DRM_MODE_CONNECTOR_eDP) {
		/* eDP */
		tmp = radeon_read_dpcd_reg(radeon_connector, DP_EDP_CONFIGURATION_CAP);
		if (tmp & 1)
			panel_mode = DP_PANEL_MODE_INTERNAL_DP2_MODE;
	}

	return panel_mode;
}

void radeon_dp_set_link_config(struct drm_connector *connector,
			       const struct drm_display_mode *mode)
{
	struct radeon_connector *radeon_connector = to_radeon_connector(connector);
	struct radeon_connector_atom_dig *dig_connector;

	if (!radeon_connector->con_priv)
		return;
	dig_connector = radeon_connector->con_priv;

	if ((dig_connector->dp_sink_type == CONNECTOR_OBJECT_ID_DISPLAYPORT) ||
	    (dig_connector->dp_sink_type == CONNECTOR_OBJECT_ID_eDP)) {
		dig_connector->dp_clock =
			radeon_dp_get_dp_link_clock(connector, dig_connector->dpcd, mode->clock);
		dig_connector->dp_lane_count =
			radeon_dp_get_dp_lane_number(connector, dig_connector->dpcd, mode->clock);
	}
}

int radeon_dp_mode_valid_helper(struct drm_connector *connector,
				struct drm_display_mode *mode)
{
	struct radeon_connector *radeon_connector = to_radeon_connector(connector);
	struct radeon_connector_atom_dig *dig_connector;
	int dp_clock;

	if (!radeon_connector->con_priv)
		return MODE_CLOCK_HIGH;
	dig_connector = radeon_connector->con_priv;

	dp_clock =
		radeon_dp_get_dp_link_clock(connector, dig_connector->dpcd, mode->clock);

	if ((dp_clock == 540000) &&
	    (!radeon_connector_is_dp12_capable(connector)))
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static bool radeon_dp_get_link_status(struct radeon_connector *radeon_connector,
				      u8 link_status[DP_LINK_STATUS_SIZE])
{
	int ret;
	ret = radeon_dp_aux_native_read(radeon_connector, DP_LANE0_1_STATUS,
					link_status, DP_LINK_STATUS_SIZE, 100);
	if (ret <= 0) {
		return false;
	}

	DRM_DEBUG_KMS("link status %6ph\n", link_status);
	return true;
}

bool radeon_dp_needs_link_train(struct radeon_connector *radeon_connector)
{
	u8 link_status[DP_LINK_STATUS_SIZE];
	struct radeon_connector_atom_dig *dig = radeon_connector->con_priv;

	if (!radeon_dp_get_link_status(radeon_connector, link_status))
		return false;
	if (drm_dp_channel_eq_ok(link_status, dig->dp_lane_count))
		return false;
	return true;
}

struct radeon_dp_link_train_info {
	struct radeon_device *rdev;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct radeon_connector *radeon_connector;
	int enc_id;
	int dp_clock;
	int dp_lane_count;
	bool tp3_supported;
	u8 dpcd[DP_RECEIVER_CAP_SIZE];
	u8 train_set[4];
	u8 link_status[DP_LINK_STATUS_SIZE];
	u8 tries;
	bool use_dpencoder;
};

static void radeon_dp_update_vs_emph(struct radeon_dp_link_train_info *dp_info)
{
	/* set the initial vs/emph on the source */
	atombios_dig_transmitter_setup(dp_info->encoder,
				       ATOM_TRANSMITTER_ACTION_SETUP_VSEMPH,
				       0, dp_info->train_set[0]); /* sets all lanes at once */

	/* set the vs/emph on the sink */
	radeon_dp_aux_native_write(dp_info->radeon_connector, DP_TRAINING_LANE0_SET,
				   dp_info->train_set, dp_info->dp_lane_count, 0);
}

static void radeon_dp_set_tp(struct radeon_dp_link_train_info *dp_info, int tp)
{
	int rtp = 0;

	/* set training pattern on the source */
	if (ASIC_IS_DCE4(dp_info->rdev) || !dp_info->use_dpencoder) {
		switch (tp) {
		case DP_TRAINING_PATTERN_1:
			rtp = ATOM_ENCODER_CMD_DP_LINK_TRAINING_PATTERN1;
			break;
		case DP_TRAINING_PATTERN_2:
			rtp = ATOM_ENCODER_CMD_DP_LINK_TRAINING_PATTERN2;
			break;
		case DP_TRAINING_PATTERN_3:
			rtp = ATOM_ENCODER_CMD_DP_LINK_TRAINING_PATTERN3;
			break;
		}
		atombios_dig_encoder_setup(dp_info->encoder, rtp, 0);
	} else {
		switch (tp) {
		case DP_TRAINING_PATTERN_1:
			rtp = 0;
			break;
		case DP_TRAINING_PATTERN_2:
			rtp = 1;
			break;
		}
		radeon_dp_encoder_service(dp_info->rdev, ATOM_DP_ACTION_TRAINING_PATTERN_SEL,
					  dp_info->dp_clock, dp_info->enc_id, rtp);
	}

	/* enable training pattern on the sink */
	radeon_write_dpcd_reg(dp_info->radeon_connector, DP_TRAINING_PATTERN_SET, tp);
}

static int radeon_dp_link_train_init(struct radeon_dp_link_train_info *dp_info)
{
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(dp_info->encoder);
	struct radeon_encoder_atom_dig *dig = radeon_encoder->enc_priv;
	u8 tmp;

	/* power up the sink */
	if (dp_info->dpcd[0] >= 0x11)
		radeon_write_dpcd_reg(dp_info->radeon_connector,
				      DP_SET_POWER, DP_SET_POWER_D0);

	/* possibly enable downspread on the sink */
	if (dp_info->dpcd[3] & 0x1)
		radeon_write_dpcd_reg(dp_info->radeon_connector,
				      DP_DOWNSPREAD_CTRL, DP_SPREAD_AMP_0_5);
	else
		radeon_write_dpcd_reg(dp_info->radeon_connector,
				      DP_DOWNSPREAD_CTRL, 0);

	if ((dp_info->connector->connector_type == DRM_MODE_CONNECTOR_eDP) &&
	    (dig->panel_mode == DP_PANEL_MODE_INTERNAL_DP2_MODE)) {
		radeon_write_dpcd_reg(dp_info->radeon_connector, DP_EDP_CONFIGURATION_SET, 1);
	}

	/* set the lane count on the sink */
	tmp = dp_info->dp_lane_count;
	if (dp_info->dpcd[DP_DPCD_REV] >= 0x11 &&
	    dp_info->dpcd[DP_MAX_LANE_COUNT] & DP_ENHANCED_FRAME_CAP)
		tmp |= DP_LANE_COUNT_ENHANCED_FRAME_EN;
	radeon_write_dpcd_reg(dp_info->radeon_connector, DP_LANE_COUNT_SET, tmp);

	/* set the link rate on the sink */
	tmp = drm_dp_link_rate_to_bw_code(dp_info->dp_clock);
	radeon_write_dpcd_reg(dp_info->radeon_connector, DP_LINK_BW_SET, tmp);

	/* start training on the source */
	if (ASIC_IS_DCE4(dp_info->rdev) || !dp_info->use_dpencoder)
		atombios_dig_encoder_setup(dp_info->encoder,
					   ATOM_ENCODER_CMD_DP_LINK_TRAINING_START, 0);
	else
		radeon_dp_encoder_service(dp_info->rdev, ATOM_DP_ACTION_TRAINING_START,
					  dp_info->dp_clock, dp_info->enc_id, 0);

	/* disable the training pattern on the sink */
	radeon_write_dpcd_reg(dp_info->radeon_connector,
			      DP_TRAINING_PATTERN_SET,
			      DP_TRAINING_PATTERN_DISABLE);

	return 0;
}

static int radeon_dp_link_train_finish(struct radeon_dp_link_train_info *dp_info)
{
	udelay(400);

	/* disable the training pattern on the sink */
	radeon_write_dpcd_reg(dp_info->radeon_connector,
			      DP_TRAINING_PATTERN_SET,
			      DP_TRAINING_PATTERN_DISABLE);

	/* disable the training pattern on the source */
	if (ASIC_IS_DCE4(dp_info->rdev) || !dp_info->use_dpencoder)
		atombios_dig_encoder_setup(dp_info->encoder,
					   ATOM_ENCODER_CMD_DP_LINK_TRAINING_COMPLETE, 0);
	else
		radeon_dp_encoder_service(dp_info->rdev, ATOM_DP_ACTION_TRAINING_COMPLETE,
					  dp_info->dp_clock, dp_info->enc_id, 0);

	return 0;
}

static int radeon_dp_link_train_cr(struct radeon_dp_link_train_info *dp_info)
{
	bool clock_recovery;
 	u8 voltage;
	int i;

	radeon_dp_set_tp(dp_info, DP_TRAINING_PATTERN_1);
	memset(dp_info->train_set, 0, 4);
	radeon_dp_update_vs_emph(dp_info);

	udelay(400);

	/* clock recovery loop */
	clock_recovery = false;
	dp_info->tries = 0;
	voltage = 0xff;
	while (1) {
		drm_dp_link_train_clock_recovery_delay(dp_info->dpcd);

		if (!radeon_dp_get_link_status(dp_info->radeon_connector, dp_info->link_status)) {
			DRM_ERROR("displayport link status failed\n");
			break;
		}

		if (drm_dp_clock_recovery_ok(dp_info->link_status, dp_info->dp_lane_count)) {
			clock_recovery = true;
			break;
		}

		for (i = 0; i < dp_info->dp_lane_count; i++) {
			if ((dp_info->train_set[i] & DP_TRAIN_MAX_SWING_REACHED) == 0)
				break;
		}
		if (i == dp_info->dp_lane_count) {
			DRM_ERROR("clock recovery reached max voltage\n");
			break;
		}

		if ((dp_info->train_set[0] & DP_TRAIN_VOLTAGE_SWING_MASK) == voltage) {
			++dp_info->tries;
			if (dp_info->tries == 5) {
				DRM_ERROR("clock recovery tried 5 times\n");
				break;
			}
		} else
			dp_info->tries = 0;

		voltage = dp_info->train_set[0] & DP_TRAIN_VOLTAGE_SWING_MASK;

		/* Compute new train_set as requested by sink */
		dp_get_adjust_train(dp_info->link_status, dp_info->dp_lane_count, dp_info->train_set);

		radeon_dp_update_vs_emph(dp_info);
	}
	if (!clock_recovery) {
		DRM_ERROR("clock recovery failed\n");
		return -1;
	} else {
		DRM_DEBUG_KMS("clock recovery at voltage %d pre-emphasis %d\n",
			  dp_info->train_set[0] & DP_TRAIN_VOLTAGE_SWING_MASK,
			  (dp_info->train_set[0] & DP_TRAIN_PRE_EMPHASIS_MASK) >>
			  DP_TRAIN_PRE_EMPHASIS_SHIFT);
		return 0;
	}
}

static int radeon_dp_link_train_ce(struct radeon_dp_link_train_info *dp_info)
{
	bool channel_eq;

	if (dp_info->tp3_supported)
		radeon_dp_set_tp(dp_info, DP_TRAINING_PATTERN_3);
	else
		radeon_dp_set_tp(dp_info, DP_TRAINING_PATTERN_2);

	/* channel equalization loop */
	dp_info->tries = 0;
	channel_eq = false;
	while (1) {
		drm_dp_link_train_channel_eq_delay(dp_info->dpcd);

		if (!radeon_dp_get_link_status(dp_info->radeon_connector, dp_info->link_status)) {
			DRM_ERROR("displayport link status failed\n");
			break;
		}

		if (drm_dp_channel_eq_ok(dp_info->link_status, dp_info->dp_lane_count)) {
			channel_eq = true;
			break;
		}

		/* Try 5 times */
		if (dp_info->tries > 5) {
			DRM_ERROR("channel eq failed: 5 tries\n");
			break;
		}

		/* Compute new train_set as requested by sink */
		dp_get_adjust_train(dp_info->link_status, dp_info->dp_lane_count, dp_info->train_set);

		radeon_dp_update_vs_emph(dp_info);
		dp_info->tries++;
	}

	if (!channel_eq) {
		DRM_ERROR("channel eq failed\n");
		return -1;
	} else {
		DRM_DEBUG_KMS("channel eq at voltage %d pre-emphasis %d\n",
			  dp_info->train_set[0] & DP_TRAIN_VOLTAGE_SWING_MASK,
			  (dp_info->train_set[0] & DP_TRAIN_PRE_EMPHASIS_MASK)
			  >> DP_TRAIN_PRE_EMPHASIS_SHIFT);
		return 0;
	}
}

void radeon_dp_link_train(struct drm_encoder *encoder,
			  struct drm_connector *connector)
{
	struct drm_device *dev = encoder->dev;
	struct radeon_device *rdev = dev->dev_private;
	struct radeon_encoder *radeon_encoder = to_radeon_encoder(encoder);
	struct radeon_encoder_atom_dig *dig;
	struct radeon_connector *radeon_connector;
	struct radeon_connector_atom_dig *dig_connector;
	struct radeon_dp_link_train_info dp_info;
	int index;
	u8 tmp, frev, crev;

	if (!radeon_encoder->enc_priv)
		return;
	dig = radeon_encoder->enc_priv;

	radeon_connector = to_radeon_connector(connector);
	if (!radeon_connector->con_priv)
		return;
	dig_connector = radeon_connector->con_priv;

	if ((dig_connector->dp_sink_type != CONNECTOR_OBJECT_ID_DISPLAYPORT) &&
	    (dig_connector->dp_sink_type != CONNECTOR_OBJECT_ID_eDP))
		return;

	/* DPEncoderService newer than 1.1 can't program properly the
	 * training pattern. When facing such version use the
	 * DIGXEncoderControl (X== 1 | 2)
	 */
	dp_info.use_dpencoder = true;
	index = GetIndexIntoMasterTable(COMMAND, DPEncoderService);
	if (atom_parse_cmd_header(rdev->mode_info.atom_context, index, &frev, &crev)) {
		if (crev > 1) {
			dp_info.use_dpencoder = false;
		}
	}

	dp_info.enc_id = 0;
	if (dig->dig_encoder)
		dp_info.enc_id |= ATOM_DP_CONFIG_DIG2_ENCODER;
	else
		dp_info.enc_id |= ATOM_DP_CONFIG_DIG1_ENCODER;
	if (dig->linkb)
		dp_info.enc_id |= ATOM_DP_CONFIG_LINK_B;
	else
		dp_info.enc_id |= ATOM_DP_CONFIG_LINK_A;

	tmp = radeon_read_dpcd_reg(radeon_connector, DP_MAX_LANE_COUNT);
	if (ASIC_IS_DCE5(rdev) && (tmp & DP_TPS3_SUPPORTED))
		dp_info.tp3_supported = true;
	else
		dp_info.tp3_supported = false;

	memcpy(dp_info.dpcd, dig_connector->dpcd, DP_RECEIVER_CAP_SIZE);
	dp_info.rdev = rdev;
	dp_info.encoder = encoder;
	dp_info.connector = connector;
	dp_info.radeon_connector = radeon_connector;
	dp_info.dp_lane_count = dig_connector->dp_lane_count;
	dp_info.dp_clock = dig_connector->dp_clock;

	if (radeon_dp_link_train_init(&dp_info))
		goto done;
	if (radeon_dp_link_train_cr(&dp_info))
		goto done;
	if (radeon_dp_link_train_ce(&dp_info))
		goto done;
done:
	if (radeon_dp_link_train_finish(&dp_info))
		return;
}
