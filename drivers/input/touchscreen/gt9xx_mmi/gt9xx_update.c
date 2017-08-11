/*
 * Goodix GT9xx touchscreen driver
 *
 * Copyright  (C)  2010 - 2016 Goodix. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the GOODiX's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Version: 2.4.0.1
 * Release Date: 2016/10/26
 */

#include <linux/kthread.h>
#include "gt9xx.h"

#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/firmware.h>

#define GUP_REG_HW_INFO		0x4220
#define GUP_REG_FW_MSG		0x41E4
#define GUP_REG_PID_VID		0x8140

#define FIRMWARE_NAME_LEN_MAX		256
#define GOODIX_FIRMWARE_FILE_NAME	"_goodix_update_.bin"
#define GOODIX_CONFIG_FILE_NAME		"_goodix_config_.cfg"

#define FW_HEAD_LENGTH			14
#define FW_SECTION_LENGTH		0x2000	/*  8K */
#define FW_DSP_ISP_LENGTH		0x1000	/*  4K */
#define FW_DSP_LENGTH			0x1000	/*  4K */
#define FW_BOOT_LENGTH			0x800	/*  2K */
#define FW_SS51_LENGTH		(4 * FW_SECTION_LENGTH)	/*  32K */
#define FW_BOOT_ISP_LENGTH		0x800	/*  2k */
#define FW_GLINK_LENGTH			0x3000	/*  12k */
#define FW_GWAKE_LENGTH		(4 * FW_SECTION_LENGTH) /*  32k */

#define PACK_SIZE			256
#define MAX_FRAME_CHECK_TIME		5


#define _bRW_MISCTL__SRAM_BANK		0x4048
#define _bRW_MISCTL__MEM_CD_EN		0x4049
#define _bRW_MISCTL__CACHE_EN		0x404B
#define _bRW_MISCTL__TMR0_EN		0x40B0
#define _rRW_MISCTL__SWRST_B0_		0x4180
#define _bWO_MISCTL__CPU_SWRST_PULSE	0x4184
#define _rRW_MISCTL__BOOTCTL_B0_	0x4190
#define _rRW_MISCTL__BOOT_OPT_B0_	0x4218
#define _rRW_MISCTL__BOOT_CTL_		0x5094

#define AUTO_SEARCH_BIN			0x01
#define AUTO_SEARCH_CFG			0x02
#define BIN_FILE_READY			0x80
#define CFG_FILE_READY			0x08
#define HEADER_FW_READY			0x00

#pragma pack(1)
struct st_fw_head {
	u8	hw_info[4];			 /* hardware info */
	u8	pid[8];				 /* product id */
	u16 vid;				 /* version id */
};
#pragma pack()

struct st_update_msg {
	u8 force_update;
	u8 fw_flag;
	u8 *fw_data;
	struct file *cfg_file;
	struct st_fw_head ic_fw_msg;
	mm_segment_t old_fs;
	u32 fw_total_len;
	u32 fw_burned_len; };

struct st_update_msg update_msg;

u16 show_len;
u16 total_len;
u8 got_file_flag;

static u8 gup_burn_fw_gwake_section(struct i2c_client *client,
		u8 *fw_section, u16 start_addr, u32 len, u8 bank_cmd);

#define _CLOSE_FILE(p_file) (if (p_file && !IS_ERR(p_file))\
		{\
		filp_close(p_file, NULL);\
		})

/*******************************************************
Function:
	Read data from the i2c slave device.
Input:
	client:		i2c device.
	buf[0~1]:	read start address.
	buf[2~len-1]:	read data buffer.
	len:	GTP_ADDR_LENGTH + read bytes count
Output:
	numbers of i2c_msgs to transfer:
	  2: succeed, otherwise: failed
*********************************************************/
s32 gup_i2c_read(struct i2c_client *client, u8 *buf, s32 len)
{
	struct i2c_msg msgs[2];
	s32 ret = -1;
	s32 retries = 0;

	GTP_DEBUG_FUNC();

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = client->addr;
	msgs[0].len   = GTP_ADDR_LENGTH;
	msgs[0].buf   = &buf[0];
	/* msgs[0].scl_rate = 300 * 1000;	for Rockchip, etc */

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len   = len - GTP_ADDR_LENGTH;
	msgs[1].buf   = &buf[GTP_ADDR_LENGTH];
	/* msgs[1].scl_rate = 300 * 1000;	for Rockchip, etc. */

	while (retries < RETRY_MAX_TIMES) {
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret == 2)
			break;
		retries++;
	}

	return ret;
}

/*******************************************************
Function:
	Write data to the i2c slave device.
Input:
	client:		i2c device.
	buf[0~1]:	write start address.
	buf[2~len-1]:	data buffer
	len:	GTP_ADDR_LENGTH + write bytes count
Output:
	numbers of i2c_msgs to transfer:
		1: succeed, otherwise: failed
*********************************************************/
s32 gup_i2c_write(struct i2c_client *client, u8 *buf, s32 len)
{
	struct i2c_msg msg;
	s32 ret = -1;
	s32 retries = 0;

	GTP_DEBUG_FUNC();

	msg.flags = !I2C_M_RD;
	msg.addr  = client->addr;
	msg.len   = len;
	msg.buf   = buf;
	/* msg.scl_rate = 300 * 1000;	for Rockchip, etc */

	while (retries < RETRY_MAX_TIMES) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)
			break;
		retries++;
	}

	return ret;
}

static s32 gup_init_panel(struct goodix_ts_data *ts)
{
	s32 ret = 0;
	s32 i = 0;
	u8 check_sum = 0;
	u8 opr_buf[16];
	u8 sensor_id = 0;
	u16 version = 0;
	u8 id[4];
	u8 drv_cfg_version;
	u8 flash_cfg_version;

#ifndef CONFIG_OF
	u8 cfg_info_group0[] = CTP_CFG_GROUP0;
	u8 cfg_info_group1[] = CTP_CFG_GROUP1;
	u8 cfg_info_group2[] = CTP_CFG_GROUP2;
	u8 cfg_info_group3[] = CTP_CFG_GROUP3;
	u8 cfg_info_group4[] = CTP_CFG_GROUP4;
	u8 cfg_info_group5[] = CTP_CFG_GROUP5;
	u8 *send_cfg_buf[] = {cfg_info_group0,
				cfg_info_group1,
				cfg_info_group2,
				cfg_info_group3,
				cfg_info_group4,
				cfg_info_group5};
	u8 cfg_info_len[] = { CFG_GROUP_LEN(cfg_info_group0),
			CFG_GROUP_LEN(cfg_info_group1),
			CFG_GROUP_LEN(cfg_info_group2),
			CFG_GROUP_LEN(cfg_info_group3),
			CFG_GROUP_LEN(cfg_info_group4),
			CFG_GROUP_LEN(cfg_info_group5)};
 #endif

	ret = gtp_i2c_read_dbl_check(
	ts->client, GTP_REG_SENSOR_ID, &sensor_id, 1);
	if (SUCCESS == ret) {
		if (sensor_id >= 0x06) {
			dev_err(&ts->client->dev,
					"Invalid sensor_id(0x%02X), No Config Sent!",
					sensor_id);
			return -EPERM;
		}
	} else {
		dev_err(&ts->client->dev,
				"Failed to get sensor_id, No config sent!");
		return -EPERM;
	}

		/* parse config data*/
#ifdef CONFIG_OF
	dev_dbg(&ts->client->dev, "Get config data from dts file.");
	ret = gtp_parse_dt_cfg(&ts->client->dev,
		&ts->pdata->config[GTP_ADDR_LENGTH],
		&ts->gtp_cfg_len, sensor_id);
	if (ret < 0) {
		dev_err(&ts->client->dev,
				"Failed to parse config data form dts file.");
		ts->pnl_init_error = 1;
		return -EPERM;
	}
#else
	dev_dbg(&ts->client->dev, "Get config data from header file.");
	if ((!cfg_info_len[1]) && (!cfg_info_len[2]) &&
		(!cfg_info_len[3]) && (!cfg_info_len[4]) &&
		(!cfg_info_len[5])) {
		sensor_id = 0;
	}
	ts->gtp_cfg_len = cfg_info_len[sensor_id];
	memset(&ts->pdata->config[GTP_ADDR_LENGTH], 0, GTP_CONFIG_MAX_LENGTH);
	memcpy(&ts->pdata->config[GTP_ADDR_LENGTH],
		send_cfg_buf[sensor_id], ts->gtp_cfg_len);
#endif

	dev_dbg(&ts->client->dev, "Sensor_ID: %d", sensor_id);
	if (ts->gtp_cfg_len < GTP_CONFIG_MIN_LENGTH) {
		dev_err(&ts->client->dev,
				"Sensor_ID(%d) matches with NULL or ",
				sensor_id);
		dev_err(&ts->client->dev,
				"INVALID CONFIG GROUP!");
		dev_err(&ts->client->dev,
				"NO Config Sent!");
		dev_err(&ts->client->dev,
				"You need to check you header ");
		dev_err(&ts->client->dev,
				"file CFG_GROUP section!");
		return -EPERM;
	}

	ret = gtp_i2c_read_dbl_check(
	ts->client, GTP_REG_CONFIG_DATA, &opr_buf[0], 1);
	if (ret == SUCCESS) {
		dev_dbg(&ts->client->dev,
		"CFG_GROUP%d Config Version: %d, IC Config Version: %d",
		sensor_id, ts->pdata->config[GTP_ADDR_LENGTH], opr_buf[0]);

		flash_cfg_version = opr_buf[0];
		drv_cfg_version = ts->pdata->config[GTP_ADDR_LENGTH];

		if (flash_cfg_version < 90 &&
		flash_cfg_version > drv_cfg_version)
			ts->pdata->config[GTP_ADDR_LENGTH] = 0x00;
	} else {
		dev_err(&ts->client->dev,
			"Failed to get ic config version!No config sent!");
		return -EPERM;
	}

	dev_dbg(&ts->client->dev, "X_MAX = %d, Y_MAX = %d, TRIGGER = 0x%02x",
			ts->abs_x_max, ts->abs_y_max, ts->int_trigger_type);

	ts->pdata->config[RESOLUTION_LOC]	   = (u8)GTP_MAX_WIDTH;
	ts->pdata->config[RESOLUTION_LOC + 1] = (u8)(GTP_MAX_WIDTH>>8);
	ts->pdata->config[RESOLUTION_LOC + 2] = (u8)GTP_MAX_HEIGHT;
	ts->pdata->config[RESOLUTION_LOC + 3] = (u8)(GTP_MAX_HEIGHT>>8);

	/* RISING */
	if (GTP_INT_TRIGGER == 0) {
		ts->pdata->config[TRIGGER_LOC] &= 0xfe;
	/* FALLING */
	} else if (GTP_INT_TRIGGER == 1) {
		ts->pdata->config[TRIGGER_LOC] |= 0x01;
	}

	check_sum = 0;
	for (i = GTP_ADDR_LENGTH; i < ts->gtp_cfg_len; i++)
		check_sum += ts->pdata->config[i];
	ts->pdata->config[ts->gtp_cfg_len] = (~check_sum) + 1;

	GTP_DEBUG_FUNC();
	ret = gtp_send_cfg(ts->client);
	if (ret < 0)
		dev_err(&ts->client->dev, "Send config error.");

	if (flash_cfg_version < 90 && flash_cfg_version > drv_cfg_version) {
		check_sum = 0;
		ts->pdata->config[GTP_ADDR_LENGTH] = drv_cfg_version;
		for (i = GTP_ADDR_LENGTH; i < ts->gtp_cfg_len; i++)
			check_sum += ts->pdata->config[i];
		ts->pdata->config[ts->gtp_cfg_len] = (~check_sum) + 1;
	}
	gtp_read_version(ts->client, &version, id);
	usleep_range(10000, 11000);

	return 0;
}


static u8 gup_get_ic_msg(struct i2c_client *client, u16 addr, u8 *msg, s32 len)
{
	s32 i = 0;

	msg[0] = (addr >> 8) & 0xff;
	msg[1] = addr & 0xff;

	for (i = 0; i < 5; i++) {
		if (gup_i2c_read(client, msg, GTP_ADDR_LENGTH + len) > 0)
			break;
	}

	if (i >= 5) {
		dev_err(&client->dev,
				"Read data from 0x%02x%02x failed!",
				msg[0], msg[1]);
		return FAIL;
	}

	return SUCCESS;
}

static u8 gup_set_ic_msg(struct i2c_client *client, u16 addr, u8 val)
{
	s32 i = 0;
	u8 msg[3];

	msg[0] = (addr >> 8) & 0xff;
	msg[1] = addr & 0xff;
	msg[2] = val;

	for (i = 0; i < 5; i++) {
		if (gup_i2c_write(client, msg, GTP_ADDR_LENGTH + 1) > 0)
			break;
	}

	if (i >= 5) {
		dev_err(&client->dev,
			"Set data to 0x%02x%02x failed!", msg[0], msg[1]);
		return FAIL;
	}

	return SUCCESS;
}

static u8 gup_get_ic_fw_msg(struct i2c_client *client)
{
	s32 ret = -1;
	u8	retry = 0;
	u8	buf[16];
	u8	i;

	/*  step1:get hardware info */
	ret = gtp_i2c_read_dbl_check(
	client, GUP_REG_HW_INFO, &buf[GTP_ADDR_LENGTH], 4);
	if (FAIL == ret) {
		dev_err(&client->dev, "[get_ic_fw_msg]get hw_info failed,exit");
		return FAIL;
	}

	/*  buf[2~5]: 00 06 90 00 */
	/*  hw_info: 00 90 06 00 */
	for (i = 0; i < 4; i++)
		update_msg.ic_fw_msg.hw_info[i] = buf[GTP_ADDR_LENGTH + 3 - i];
		dev_dbg(&client->dev,
		"IC Hardware info:%02x%02x%02x%02x",
		update_msg.ic_fw_msg.hw_info[0],
		update_msg.ic_fw_msg.hw_info[1],
		update_msg.ic_fw_msg.hw_info[2],
		update_msg.ic_fw_msg.hw_info[3]);
	/*  step2:get firmware message */
	for (retry = 0; retry < 2; retry++) {
		ret = gup_get_ic_msg(client, GUP_REG_FW_MSG, buf, 1);
		if (FAIL == ret) {
			dev_err(&client->dev, "Read firmware message fail.");
			return ret;
		}

		update_msg.force_update = buf[GTP_ADDR_LENGTH];
		if ((0xBE != update_msg.force_update) && (!retry)) {
			dev_info(&client->dev, "The check sum in ic is error.");
			dev_info(&client->dev, "The IC will be updated by force.");
			continue;
		}
		break;
	}
	dev_dbg(&client->dev,
		"IC force update flag:0x%x", update_msg.force_update);

	/*  step3:get pid & vid */
	ret = gtp_i2c_read_dbl_check(
	client, GUP_REG_PID_VID, &buf[GTP_ADDR_LENGTH], 6);
	if (FAIL == ret) {
		dev_err(&client->dev, "[get_ic_fw_msg]get pid & vid failed,exit");
		return FAIL;
	}

		memset(update_msg.ic_fw_msg.pid, 0,
			sizeof(update_msg.ic_fw_msg.pid));
		memcpy(update_msg.ic_fw_msg.pid, &buf[GTP_ADDR_LENGTH], 4);
		dev_dbg(&client->dev,
			"IC Product id:%s", update_msg.ic_fw_msg.pid);

	/* GT9XX PID MAPPING */
	/*|-----FLASH-----RAM-----|
	  |------918------918-----|
	  |------968------968-----|
	  |------913------913-----|
	  |------913P-----913P----|
	  |------927------927-----|
	  |------927P-----927P----|
	  |------9110-----9110----|
	  |------9110P----9111----|*/
	if (update_msg.ic_fw_msg.pid[0] != 0) {
		if (!memcmp(update_msg.ic_fw_msg.pid, "9111", 4)) {
			dev_dbg(&client->dev, "IC Mapping Product id:%s",
					update_msg.ic_fw_msg.pid);
			memcpy(update_msg.ic_fw_msg.pid, "9110P", 5);
		}
	}

	update_msg.ic_fw_msg.vid = buf[GTP_ADDR_LENGTH+4]
	+ (buf[GTP_ADDR_LENGTH + 5]<<8);
	dev_dbg(&client->dev, "IC version id:%04x", update_msg.ic_fw_msg.vid);

	return SUCCESS;
}

s32 gup_enter_update_mode(struct i2c_client *client)
{
	s32 ret = -1;
	s32 retry = 0;
	u8 rd_buf[3];

	struct goodix_ts_data *ts = i2c_get_clientdata(client);

	/* step1:RST output low last at least 2ms */
	GTP_GPIO_OUTPUT(ts->pdata->rst_gpio, 0);
	usleep_range(2000, 3000);

	/* step2:select I2C slave addr,INT:0--0xBA;1--0x28. */
	GTP_GPIO_OUTPUT(ts->pdata->irq_gpio, (client->addr == 0x14));
	usleep_range(2000, 3000);

	/* step3:RST output high reset guitar */
	GTP_GPIO_OUTPUT(ts->pdata->rst_gpio, 1);

	/* 20121211 modify start */
	usleep_range(5000, 6000);
	while (retry++ < 200) {
		/* step4:Hold ss51 & dsp */
		ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x0C);
		if (ret <= 0) {
			dev_dbg(&client->dev,
					"Hold ss51 & dsp I2C error,retry:%d",
					retry);
			continue;
		}

		/* step5:Confirm hold */
		ret = gup_get_ic_msg(client, _rRW_MISCTL__SWRST_B0_, rd_buf, 1);
		if (ret <= 0) {
			dev_dbg(&client->dev,
					"Hold ss51 & dsp I2C error,retry:%d",
					retry);
			continue;
		}
		if (0x0C == rd_buf[GTP_ADDR_LENGTH]) {
			dev_dbg(&client->dev, "Hold ss51 & dsp confirm SUCCESS");
			break;
		}
		dev_dbg(&client->dev,
				"Hold ss51 & dsp confirm 0x4180 failed,value:%d",
				rd_buf[GTP_ADDR_LENGTH]);
	}
	if (retry >= 200) {
		dev_err(&client->dev, "Enter update Hold ss51 failed.");
		return FAIL;
	}

	/* step6:DSP_CK and DSP_ALU_CK PowerOn */
	ret = gup_set_ic_msg(client, 0x4010, 0x00);

	/* 20121211 modify end */
	return ret;
}

void gup_leave_update_mode(struct i2c_client *client)
{
	struct goodix_ts_data *ts = i2c_get_clientdata(client);

	GTP_GPIO_AS_INT(ts->pdata->irq_gpio);

	dev_dbg(&client->dev, "[leave_update_mode]reset chip.");
	gtp_reset_guitar(i2c_connect_client, 20);
}

/*  Get the correct nvram data */
/*  The correct conditions: */
/*	1. the hardware info is the same */
/*	2. the product id is the same */
/*	3. the firmware version in update file is
greater than the firmware version in ic */
/*	or the check sum in ic is wrong */
/* Update Conditions:
	1. Same hardware info
	2. Same PID
	3. File VID > IC VID
   Force Update Conditions:
	1. Wrong ic firmware checksum
	2. INVALID IC PID or VID
	3. (IC PID == 91XX || File PID == 91XX) && (File VID > IC VID)
*/

static u8 gup_enter_update_judge(struct i2c_client *client,
		struct st_fw_head *fw_head)
{
	u16 u16_tmp;
	s32 i = 0;
	u32 fw_len = 0;
	s32 pid_cmp_len = 0;

	u16_tmp = fw_head->vid;
	fw_head->vid = (u16)(u16_tmp>>8) + (u16)(u16_tmp<<8);

	dev_info(&client->dev,
			"FILE HARDWARE INFO:%02x%02x%02x%02x",
			fw_head->hw_info[0],
	fw_head->hw_info[1], fw_head->hw_info[2], fw_head->hw_info[3]);
	dev_info(&client->dev, "FILE PID:%s", fw_head->pid);
	dev_info(&client->dev, "FILE VID:%04x", fw_head->vid);
	dev_info(&client->dev,
			"IC HARDWARE INFO:%02x%02x%02x%02x",
			update_msg.ic_fw_msg.hw_info[0],
			update_msg.ic_fw_msg.hw_info[1],
			update_msg.ic_fw_msg.hw_info[2],
			update_msg.ic_fw_msg.hw_info[3]);
	dev_info(&client->dev, "IC PID:%s", update_msg.ic_fw_msg.pid);
	dev_info(&client->dev, "IC VID:%04x", update_msg.ic_fw_msg.vid);

	if (!memcmp(fw_head->pid, "9158", 4) &&
	!memcmp(update_msg.ic_fw_msg.pid, "915S", 4)) {
		dev_info(&client->dev, "Update GT915S to GT9158 directly!");
		return SUCCESS;
	}
	/* First two conditions */
	if (!memcmp(fw_head->hw_info, update_msg.ic_fw_msg.hw_info,
	sizeof(update_msg.ic_fw_msg.hw_info))) {
		fw_len = 42 * 1024;
	} else {
		fw_len = fw_head->hw_info[3];
		fw_len += (((u32)fw_head->hw_info[2]) << 8);
		fw_len += (((u32)fw_head->hw_info[1]) << 16);
		fw_len += (((u32)fw_head->hw_info[0]) << 24);
	}
	if (update_msg.fw_total_len != fw_len) {
		dev_err(&client->dev,
			"Inconsistent firmware size, Update aborted!");
		dev_err(&client->dev,
			" Default size: %d(%dK), actual size: %d(%dK)",
			fw_len, fw_len/1024, update_msg.fw_total_len,
			update_msg.fw_total_len/1024);
		return FAIL;
	}
	dev_info(&client->dev, "Firmware length:%d(%dK)",
			update_msg.fw_total_len,
			update_msg.fw_total_len/1024);

	if (update_msg.force_update != 0xBE) {
		dev_info(&client->dev, "FW chksum error,need enter update.");
		return SUCCESS;
	}

	/*  20130523 start */
	if (strlen(update_msg.ic_fw_msg.pid) < 3) {
		dev_info(&client->dev, "Illegal IC pid, need enter update");
		return SUCCESS;
	} else {
		for (i = 0; i < 3; i++) {
			if ((update_msg.ic_fw_msg.pid[i] < 0x30)
			|| (update_msg.ic_fw_msg.pid[i] > 0x39)) {
				dev_info(&client->dev,
			"Illegal IC pid, out of bound, need enter update");
				return SUCCESS;
			}
		}
	}
	/*  20130523 end */

	pid_cmp_len = strlen(fw_head->pid);
	if (pid_cmp_len < strlen(update_msg.ic_fw_msg.pid))
		pid_cmp_len = strlen(update_msg.ic_fw_msg.pid);

	if ((!memcmp(fw_head->pid, update_msg.ic_fw_msg.pid, pid_cmp_len)) ||
			(!memcmp(update_msg.ic_fw_msg.pid, "91XX", 4)) ||
			(!memcmp(fw_head->pid, "91XX", 4))) {
		if (!memcmp(fw_head->pid, "91XX", 4))
			dev_dbg(&client->dev, "Force none same pid update mode.");
		else
			dev_dbg(&client->dev, "Get the same pid.");

		/* The third condition */
		if (fw_head->vid > update_msg.ic_fw_msg.vid) {
			dev_info(&client->dev, "Need enter update.");
			return SUCCESS;
		}
		dev_err(&client->dev, "Don't meet the third condition.");
		dev_err(&client->dev, "File VID <= Ic VID, update aborted!");
	} else {
		dev_err(&client->dev, "File PID != Ic PID, update aborted!");
	}

	return FAIL;
}

static u8 ascii2hex(u8 a)
{
	s8 value = 0;

	if (a >= '0' && a <= '9')
		value = a - '0';
	else if (a >= 'A' && a <= 'F')
		value = a - 'A' + 0x0A;
	else if (a >= 'a' && a <= 'f')
		value = a - 'a' + 0x0A;
	else
		value = 0xff;

	return value;
}

static s8 gup_update_config(struct i2c_client *client,
		const struct firmware *cfg)
{
	s32 ret = 0;
	s32 i = 0;
	s32 file_cfg_len = 0;
	s32 chip_cfg_len = 0;
	s32 count = 0;
	u8 *buf;
	u8 *file_config;
	/* u8 checksum = 0; */
	struct goodix_ts_data *ts = i2c_get_clientdata(client);

	if (!cfg || !cfg->data) {
		dev_err(&ts->client->dev,
				"[update_cfg]No need to upgrade config!");
		return FAIL;
	}

	chip_cfg_len = ts->gtp_cfg_len;

	dev_dbg(&client->dev,
			"[update_cfg]config file ASCII len:%d", cfg->size);
	dev_dbg(&client->dev, "[update_cfg]need config len:%d", chip_cfg_len);
	if ((cfg->size+5) < chip_cfg_len*5) {
		dev_err(&ts->client->dev, "[update_cfg]Config length error");
		return -EPERM;
	}

	buf = kzalloc(cfg->size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	file_config = kzalloc(chip_cfg_len + GTP_ADDR_LENGTH, GFP_KERNEL);
	if (!file_config)
		return -ENOMEM;

	dev_dbg(&client->dev, "[update_cfg]Delete illgal charactor.");
	for (i = 0, count = 0; i < cfg->size; i++) {
		if (cfg->data[i] == ' ' || cfg->data[i] == '\r'
					|| cfg->data[i] == '\n')
			continue;
		buf[count++] = cfg->data[i];
	}

	dev_dbg(&client->dev, "[update_cfg]Ascii to hex.");
	file_config[0] = GTP_REG_CONFIG_DATA >> 8;
	file_config[1] = GTP_REG_CONFIG_DATA & 0xff;
	for (i = 0, file_cfg_len = GTP_ADDR_LENGTH; i < count; i += 5) {
		if ((buf[i] == '0') && ((buf[i+1] == 'x') ||
					(buf[i+1] == 'X'))) {
			u8 high, low;

			high = ascii2hex(buf[i+2]);
			low = ascii2hex(buf[i+3]);

			if ((high == 0xFF) || (low == 0xFF)) {
				ret = 0;
				dev_err(&ts->client->dev,
					"[update_cfg]Illegal config file.");
				goto update_cfg_file_failed;
			}
			file_config[file_cfg_len++] = (high<<4) + low;
		} else {
			ret = 0;
			dev_err(&ts->client->dev,
				"[update_cfg]Illegal config file.");
			goto update_cfg_file_failed;
		}
	}

	dev_dbg(&client->dev, "config:");
	GTP_DEBUG_ARRAY(file_config+2, file_cfg_len);

	i = 0;
	while (i++ < 5) {
		ret = gup_i2c_write(client, file_config, file_cfg_len);
		if (ret > 0) {
			dev_info(&client->dev, "[update_cfg]Send config SUCCESS.");
			break;
		}
		dev_err(&ts->client->dev, "[update_cfg]Send config i2c error.");
	}

update_cfg_file_failed:
	kfree(buf);
	kfree(file_config);

	return ret;
}

static u8 gup_check_firmware_name(struct i2c_client *client,
		u8 **path_p)
{
	u8 len;
	u8 *fname;

	if (!(*path_p)) {
		*path_p = GOODIX_FIRMWARE_FILE_NAME;
		return 0;
	}

	len = strnlen(*path_p, FIRMWARE_NAME_LEN_MAX);
	if (len >= FIRMWARE_NAME_LEN_MAX) {
		dev_err(&client->dev, "firmware name too long!");
		return -EINVAL;
	}

	fname = strrchr(*path_p, '/');
	if (fname) {
		fname = fname + 1;
		*path_p = fname;
	}

	return 0;
}

static s32 gup_get_firmware_file(struct i2c_client *client,
		struct st_update_msg *msg, u8 *path)
{
	s32 ret;
	const struct firmware *fw = NULL;

	ret = request_firmware(&fw, path, &client->dev);
	if (ret < 0) {
		dev_info(&client->dev, "Cannot get firmware - %s (%d)\n",
				path, ret);
		return -EEXIST;
	}

	dev_dbg(&client->dev, "FW File: %s size=%d", path, fw->size);
	msg->fw_data =
		devm_kzalloc(&client->dev, fw->size, GFP_KERNEL);
	if (!msg->fw_data) {
		dev_err(&client->dev,
				"Not enough memory for firmware data.");
		release_firmware(fw);
		return -ENOMEM;
	}

	memcpy(msg->fw_data, fw->data, fw->size);
	msg->fw_total_len = fw->size;
	release_firmware(fw);

	return 0;
}

static s32 gup_get_update_config_file(struct i2c_client *client)
{
	s32 ret = 0;
	const struct firmware *fw = NULL;

	ret = request_firmware(&fw, GOODIX_CONFIG_FILE_NAME,
		&client->dev);
	if (ret < 0) {
		dev_info(&client->dev,
				"Cannot get config file - %s (%d)\n",
				GOODIX_CONFIG_FILE_NAME, ret);
	} else {
		dev_dbg(&client->dev, "Update config File: %s",
				GOODIX_CONFIG_FILE_NAME);
		ret = gup_update_config(client, fw);
		if (ret <= 0)
			dev_err(&client->dev, "Update config failed.");
		release_firmware(fw);
	}

	return ret;
}

static u8 gup_check_update_file(struct i2c_client *client,
		struct st_fw_head *fw_head, u8 *path)
{
	s32 ret = 0;
	s32 i = 0;
	s32 fw_checksum = 0;
	struct goodix_ts_data *ts = i2c_get_clientdata(i2c_connect_client);

	got_file_flag = 0x00;

	if (ts->pdata->auto_update_cfg) {
		ret = gup_get_update_config_file(client);
		if (ret < 0) {
			dev_info(&client->dev, "Cannot get config file");
			return -EEXIST;
		}
	}

	ret = gup_check_firmware_name(client, &path);
	if (ret < 0) {
		dev_info(&client->dev, "firmware name too long! - %s (%d)\n",
				path, ret);
		return -EEXIST;
	}

	ret = gup_get_firmware_file(client, &update_msg, path);
	if (ret < 0) {
		dev_info(&client->dev, "Cannot get firmware - %s (%d)\n",
				path, ret);
		return -EEXIST;
	}

	got_file_flag = BIN_FILE_READY;

	if (update_msg.fw_total_len < (FW_HEAD_LENGTH
				+ FW_SECTION_LENGTH*4 + FW_DSP_ISP_LENGTH +
				FW_DSP_LENGTH + FW_BOOT_LENGTH)) {
		dev_err(&client->dev,
				"INVALID bin file(size: %d), update aborted.",
				update_msg.fw_total_len);
		return FAIL;
	}

	update_msg.fw_total_len -= FW_HEAD_LENGTH;

	dev_dbg(&client->dev, "Bin firmware actual size: %d(%dK)",
	update_msg.fw_total_len, update_msg.fw_total_len/1024);

	memcpy(fw_head, update_msg.fw_data, FW_HEAD_LENGTH);

	/* check firmware legality */
	fw_checksum = 0;
	for (i = 0; i < update_msg.fw_total_len; i += 2) {
		u16 temp;

		temp = (update_msg.fw_data[FW_HEAD_LENGTH + i] << 8) +
			update_msg.fw_data[FW_HEAD_LENGTH + i + 1];
		fw_checksum += temp;
	}

	dev_dbg(&client->dev, "firmware checksum:%x", fw_checksum&0xFFFF);
	if (fw_checksum&0xFFFF) {
		dev_err(&client->dev, "Illegal firmware file.");
		return FAIL;
	}

	return SUCCESS;
}

static u8 gup_burn_proc(struct i2c_client *client, u8 *burn_buf,
		u16 start_addr, u16 total_length)
{
	s32 ret = 0;
	u16 burn_addr = start_addr;
	u16 frame_length = 0;
	u16 burn_length = 0;
	u8 wr_buf[PACK_SIZE + GTP_ADDR_LENGTH];
	u8 rd_buf[PACK_SIZE + GTP_ADDR_LENGTH];
	u8 retry = 0;

	dev_dbg(&client->dev, "Begin burn %dk data to addr 0x%x",
			(total_length/1024), start_addr);
	while (burn_length < total_length) {
		dev_dbg(&client->dev,
				"B/T:%04d/%04d", burn_length, total_length);
		frame_length = ((total_length - burn_length)
		> PACK_SIZE) ? PACK_SIZE : (total_length - burn_length);
		wr_buf[0] = (u8)(burn_addr>>8);
		rd_buf[0] = wr_buf[0];
		wr_buf[1] = (u8)burn_addr;
		rd_buf[1] = wr_buf[1];
		memcpy(&wr_buf[GTP_ADDR_LENGTH],
		&burn_buf[burn_length], frame_length);

		for (retry = 0; retry < MAX_FRAME_CHECK_TIME; retry++) {
			ret = gup_i2c_write(client,
			wr_buf, GTP_ADDR_LENGTH + frame_length);
			if (ret <= 0) {
				dev_err(&client->dev, "Write frame data i2c error.");
				continue;
			}
			ret = gup_i2c_read(client,
			rd_buf, GTP_ADDR_LENGTH + frame_length);
			if (ret <= 0) {
				dev_err(&client->dev,
					"Read back frame data i2c error.");
				continue;
			}
			if (memcmp(&wr_buf[GTP_ADDR_LENGTH],
				&rd_buf[GTP_ADDR_LENGTH], frame_length)) {
				dev_err(&client->dev,
					"Check frame data fail,not equal.");
				dev_dbg(&client->dev, "write array:");
				GTP_DEBUG_ARRAY(&wr_buf[GTP_ADDR_LENGTH],
				frame_length);
				dev_dbg(&client->dev, "read array:");
				GTP_DEBUG_ARRAY(&rd_buf[GTP_ADDR_LENGTH],
				frame_length);
				continue;
			} else {
				/* dev_dbg(&client->dev,
					"Check frame data success."); */
				break;
			}
		}
		if (retry >= MAX_FRAME_CHECK_TIME) {
			dev_err(&client->dev, "Burn frame data time out,exit.");
			return FAIL;
		}
		burn_length += frame_length;
		burn_addr += frame_length;
	}

	return SUCCESS;
}

static u8 gup_load_section_file(u8 *buf,
		u32 offset, u16 length, u8 set_or_end)
{
	if (!update_msg.fw_data ||
			update_msg.fw_total_len < FW_HEAD_LENGTH +
			offset + length) {
		pr_err("<<-GTP->> cannot load section data. fw_len=%d read end=%d\n",
		update_msg.fw_total_len ,
		FW_HEAD_LENGTH + offset + length);
		return FAIL;
	}


	if (SEEK_SET == set_or_end) {
		memcpy(buf, &update_msg.fw_data[FW_HEAD_LENGTH + offset],
				length);
	} else {
		/* seek end */
		memcpy(buf, &update_msg.fw_data[update_msg.fw_total_len
				+ FW_HEAD_LENGTH - offset], length);
	}

	return SUCCESS;
}

static u8 gup_recall_check(struct i2c_client *client,
		u8 *chk_src, u16 start_rd_addr, u16 chk_length)
{
	u8 rd_buf[PACK_SIZE + GTP_ADDR_LENGTH];
	s32 ret = 0;
	u16 recall_addr = start_rd_addr;
	u16 recall_length = 0;
	u16 frame_length = 0;

	while (recall_length < chk_length) {
		frame_length = ((chk_length - recall_length)
				> PACK_SIZE) ? PACK_SIZE :
				(chk_length - recall_length);
		ret = gup_get_ic_msg(client, recall_addr, rd_buf, frame_length);
		if (ret <= 0) {
			dev_err(&client->dev, "recall i2c error,exit");
			return FAIL;
		}

		if (memcmp(&rd_buf[GTP_ADDR_LENGTH],
		&chk_src[recall_length], frame_length)) {
			dev_err(&client->dev, "Recall frame data fail,not equal.");
			dev_dbg(&client->dev, "chk_src array:");
			GTP_DEBUG_ARRAY(&chk_src[recall_length], frame_length);
			dev_dbg(&client->dev, "recall array:");
			GTP_DEBUG_ARRAY(&rd_buf[GTP_ADDR_LENGTH], frame_length);
			return FAIL;
		}

		recall_length += frame_length;
		recall_addr += frame_length;
	}
	dev_dbg(&client->dev,
			"Recall check %dk firmware success.",
			(chk_length/1024));

	return SUCCESS;
}

static u8 gup_burn_fw_section(struct i2c_client *client,
		u8 *fw_section, u16 start_addr, u8 bank_cmd)
{
	s32 ret = 0;
	u8	rd_buf[5];

	/* step1:hold ss51 & dsp */
	ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x0C);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_section]hold ss51 & dsp fail.");
		return FAIL;
	}

	/* step2:set scramble */
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_OPT_B0_, 0x00);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_section]set scramble fail.");
		return FAIL;
	}

	/* step3:select bank */
	ret = gup_set_ic_msg(client, _bRW_MISCTL__SRAM_BANK,
	(bank_cmd >> 4)&0x0F);
	if (ret <= 0) {
		dev_err(&client->dev,
				"[burn_fw_section]select bank %d fail.",
				(bank_cmd >> 4)&0x0F);
		return FAIL;
	}

	/* step4:enable accessing code */
	ret = gup_set_ic_msg(client, _bRW_MISCTL__MEM_CD_EN, 0x01);
	if (ret <= 0) {
		dev_err(&client->dev,
				"[burn_fw_section]enable accessing code fail.");
		return FAIL;
	}

	/* step5:burn 8k fw section */
	ret = gup_burn_proc(client, fw_section, start_addr, FW_SECTION_LENGTH);
	if (FAIL == ret) {
		dev_err(&client->dev, "[burn_fw_section]burn fw_section fail.");
		return FAIL;
	}

	/* step6:hold ss51 & release dsp */
	ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x04);
	if (ret <= 0) {
		dev_err(&client->dev,
				"[burn_fw_section]hold ss51 & release dsp fail.");
		return FAIL;
	}
	/* must delay */
	usleep_range(1000, 2000);

	/* step7:send burn cmd to move data to flash from sram */
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, bank_cmd&0x0f);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_section]send burn cmd fail.");
		return FAIL;
	}
	dev_dbg(&client->dev,
		"[burn_fw_section]Wait for the burn is complete......");
	do {
		ret = gup_get_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, rd_buf, 1);
		if (ret <= 0) {
			dev_err(&client->dev,
					"[burn_fw_section]Get burn state fail");
			return FAIL;
		}
		usleep_range(10000, 11000);
	/* dev_dbg(&client->dev, "[burn_fw_section]Get burn state:%d.",
	rd_buf[GTP_ADDR_LENGTH]); */
	} while (rd_buf[GTP_ADDR_LENGTH]);

	/* step8:select bank */
	ret = gup_set_ic_msg(client,
	_bRW_MISCTL__SRAM_BANK, (bank_cmd >> 4)&0x0F);
	if (ret <= 0) {
		dev_err(&client->dev,
				"[burn_fw_section]select bank %d fail.",
				(bank_cmd >> 4)&0x0F);
		return FAIL;
	}

	/* step9:enable accessing code */
	ret = gup_set_ic_msg(client, _bRW_MISCTL__MEM_CD_EN, 0x01);
	if (ret <= 0) {
		dev_err(&client->dev,
			"[burn_fw_section]enable accessing code fail.");
		return FAIL;
	}

	/* step10:recall 8k fw section */
	ret = gup_recall_check(client,
	fw_section, start_addr, FW_SECTION_LENGTH);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_section]recall check %dk firmware fail.",
			FW_SECTION_LENGTH/1024);
		return FAIL;
	}

	/* step11:disable accessing code */
	ret = gup_set_ic_msg(client, _bRW_MISCTL__MEM_CD_EN, 0x00);
	if (ret <= 0) {
		dev_err(&client->dev,
			"[burn_fw_section]disable accessing code fail.");
		return FAIL;
	}

	return SUCCESS;
}

static u8 gup_burn_dsp_isp(struct i2c_client *client)
{
	s32 ret = 0;
	u8 *fw_dsp_isp = NULL;
	u8	retry = 0;

	dev_info(&client->dev, "[burn_dsp_isp]Begin burn dsp isp---->>");

	/* step1:alloc memory */
	dev_dbg(&client->dev, "[burn_dsp_isp]step1:alloc memory");
	while (retry++ < 5) {
		fw_dsp_isp = kzalloc(FW_DSP_ISP_LENGTH, GFP_KERNEL);
		if (fw_dsp_isp == NULL) {
			continue;
		} else {
			dev_info(&client->dev,
				"[burn_dsp_isp]Alloc %dk byte memory success.",
				(FW_DSP_ISP_LENGTH/1024));
			break;
		}
	}
	if (retry >= 5) {
		dev_err(&client->dev, "[burn_dsp_isp]Alloc memory fail,exit.");
		return FAIL;
	}

	/* step2:load dsp isp file data */
	dev_dbg(&client->dev, "[burn_dsp_isp]step2:load dsp isp file data");
	ret = gup_load_section_file(fw_dsp_isp,
	FW_DSP_ISP_LENGTH, FW_DSP_ISP_LENGTH, SEEK_END);
	if (FAIL == ret) {
		dev_err(&client->dev,
				"[burn_dsp_isp]load firmware dsp_isp fail.");
		goto exit_burn_dsp_isp;
	}

	/* step3:disable wdt,clear cache enable */
	dev_dbg(&client->dev,
			"[burn_dsp_isp]step3:disable wdt,clear cache enable");
	ret = gup_set_ic_msg(client, _bRW_MISCTL__TMR0_EN, 0x00);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_dsp_isp]disable wdt fail.");
		ret = FAIL;
		goto exit_burn_dsp_isp;
	}
	ret = gup_set_ic_msg(client, _bRW_MISCTL__CACHE_EN, 0x00);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_dsp_isp]clear cache enable fail.");
		ret = FAIL;
		goto exit_burn_dsp_isp;
	}

	/* step4:hold ss51 & dsp */
	dev_dbg(&client->dev, "[burn_dsp_isp]step4:hold ss51 & dsp");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x0C);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_dsp_isp]hold ss51 & dsp fail.");
		ret = FAIL;
		goto exit_burn_dsp_isp;
	}

	/* step5:set boot from sram */
	dev_dbg(&client->dev, "[burn_dsp_isp]step5:set boot from sram");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOTCTL_B0_, 0x02);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_dsp_isp]set boot from sram fail.");
		ret = FAIL;
		goto exit_burn_dsp_isp;
	}

	/* step6:software reboot */
	dev_dbg(&client->dev, "[burn_dsp_isp]step6:software reboot");
	ret = gup_set_ic_msg(client, _bWO_MISCTL__CPU_SWRST_PULSE, 0x01);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_dsp_isp]software reboot fail.");
		ret = FAIL;
		goto exit_burn_dsp_isp;
	}

	/* step7:select bank2 */
	dev_dbg(&client->dev, "[burn_dsp_isp]step7:select bank2");
	ret = gup_set_ic_msg(client, _bRW_MISCTL__SRAM_BANK, 0x02);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_dsp_isp]select bank2 fail.");
		ret = FAIL;
		goto exit_burn_dsp_isp;
	}

	/* step8:enable accessing code */
	dev_dbg(&client->dev, "[burn_dsp_isp]step8:enable accessing code");
	ret = gup_set_ic_msg(client, _bRW_MISCTL__MEM_CD_EN, 0x01);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_dsp_isp]enable accessing code fail.");
		ret = FAIL;
		goto exit_burn_dsp_isp;
	}

	/* step9:burn 4k dsp_isp */
	dev_dbg(&client->dev, "[burn_dsp_isp]step9:burn 4k dsp_isp");
	ret = gup_burn_proc(client, fw_dsp_isp, 0xC000, FW_DSP_ISP_LENGTH);
	if (FAIL == ret) {
		dev_err(&client->dev, "[burn_dsp_isp]burn dsp_isp fail.");
		goto exit_burn_dsp_isp;
	}

	/* step10:set scramble */
	dev_dbg(&client->dev, "[burn_dsp_isp]step10:set scramble");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_OPT_B0_, 0x00);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_dsp_isp]set scramble fail.");
		ret = FAIL;
		goto exit_burn_dsp_isp;
	}
	update_msg.fw_burned_len += FW_DSP_ISP_LENGTH;
	dev_dbg(&client->dev, "[burn_dsp_isp]Burned length:%d",
			update_msg.fw_burned_len);
	ret = SUCCESS;

exit_burn_dsp_isp:
	kfree(fw_dsp_isp);

	return ret;
}

static u8 gup_burn_fw_ss51(struct i2c_client *client)
{
	u8 *fw_ss51 = NULL;
	u8	retry = 0;
	s32 ret = 0;

	dev_info(&client->dev, "[burn_fw_ss51]Begin burn ss51 firmware---->>");

	/* step1:alloc memory */
	dev_dbg(&client->dev, "[burn_fw_ss51]step1:alloc memory");
	while (retry++ < 5) {
		fw_ss51 = kzalloc(FW_SECTION_LENGTH, GFP_KERNEL);
		if (fw_ss51 == NULL) {
			continue;
		} else {
			dev_dbg(&client->dev,
				"[burn_fw_ss51]Alloc %dk byte memory success.",
				(FW_SECTION_LENGTH / 1024));
			break;
		}
	}
	if (retry >= 5) {
		dev_err(&client->dev, "[burn_fw_ss51]Alloc memory fail,exit.");
		return FAIL;
	}

/*	step2:load ss51 firmware section 1 file data */
/*	dev_dbg(&client->dev, "[burn_fw_ss51]step2:
	load ss51 firmware section 1 file data"); */
/*	ret = gup_load_section_file(fw_ss51,
	0, FW_SECTION_LENGTH, SEEK_SET); */
/*	if (FAIL == ret) */
/*	{ */
/*	dev_err(&client->dev,
		"[burn_fw_ss51]load ss51 firmware section 1 fail."); */
/*	goto exit_burn_fw_ss51; */
/*	} */

	dev_info(&client->dev, "[burn_fw_ss51]Reset first 8K of ss51 to 0xFF.");
	dev_dbg(&client->dev, "[burn_fw_ss51]step2: reset bank0 0xC000~0xD000");
	memset(fw_ss51, 0xFF, FW_SECTION_LENGTH);

	/* step3:clear control flag */
	dev_dbg(&client->dev, "[burn_fw_ss51]step3:clear control flag");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, 0x00);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_ss51]clear control flag fail.");
		ret = FAIL;
		goto exit_burn_fw_ss51;
	}

	/* step4:burn ss51 firmware section 1 */
	dev_dbg(&client->dev, "[burn_fw_ss51]step4:burn ss51 firmware section 1");
	ret = gup_burn_fw_section(client, fw_ss51, 0xC000, 0x01);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_ss51]burn ss51 firmware section 1 fail.");
		goto exit_burn_fw_ss51;
	}

	/* step5:load ss51 firmware section 2 file data */
	dev_dbg(&client->dev,
		"[burn_fw_ss51]step5:load ss51 firmware section 2 file data");
	ret = gup_load_section_file(fw_ss51,
	FW_SECTION_LENGTH, FW_SECTION_LENGTH, SEEK_SET);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_ss51]load ss51 firmware section 2 fail.");
		goto exit_burn_fw_ss51;
	}

	/* step6:burn ss51 firmware section 2 */
	dev_dbg(&client->dev, "[burn_fw_ss51]step6:burn ss51 firmware section 2");
	ret = gup_burn_fw_section(client, fw_ss51, 0xE000, 0x02);
	if (FAIL == ret) {
		dev_err(&client->dev,
				"[burn_fw_ss51]burn ss51 firmware section 2 fail.");
		goto exit_burn_fw_ss51;
	}

	/* step7:load ss51 firmware section 3 file data */
	dev_dbg(&client->dev,
		"[burn_fw_ss51]step7:load ss51 firmware section 3 file data");
	ret = gup_load_section_file(fw_ss51,
	2 * FW_SECTION_LENGTH, FW_SECTION_LENGTH, SEEK_SET);
	if (FAIL == ret) {
		dev_err(&client->dev,
				"[burn_fw_ss51]load ss51 firmware section 3 fail.");
		goto exit_burn_fw_ss51;
	}

	/* step8:burn ss51 firmware section 3 */
	dev_dbg(&client->dev,
		"[burn_fw_ss51]step8:burn ss51 firmware section 3");
	ret = gup_burn_fw_section(client, fw_ss51, 0xC000, 0x13);
	if (FAIL == ret) {
		dev_err(&client->dev,
				"[burn_fw_ss51]burn ss51 firmware section 3 fail.");
		goto exit_burn_fw_ss51;
	}

	/* step9:load ss51 firmware section 4 file data */
	dev_dbg(&client->dev,
		"[burn_fw_ss51]step9:load ss51 firmware section 4 file data");
	ret = gup_load_section_file(fw_ss51,
	3 * FW_SECTION_LENGTH, FW_SECTION_LENGTH, SEEK_SET);
	if (FAIL == ret) {
		dev_err(&client->dev,
				"[burn_fw_ss51]load ss51 firmware section 4 fail.");
		goto exit_burn_fw_ss51;
	}

	/* step10:burn ss51 firmware section 4 */
	dev_dbg(&client->dev, "[burn_fw_ss51]step10:burn ss51 firmware section 4");
	ret = gup_burn_fw_section(client, fw_ss51, 0xE000, 0x14);
	if (FAIL == ret) {
		dev_err(&client->dev,
				"[burn_fw_ss51]burn ss51 firmware section 4 fail.");
		goto exit_burn_fw_ss51;
	}

	update_msg.fw_burned_len += (FW_SECTION_LENGTH*4);
	dev_dbg(&client->dev, "[burn_fw_ss51]Burned length:%d",
			update_msg.fw_burned_len);
	ret = SUCCESS;

exit_burn_fw_ss51:
	kfree(fw_ss51);
	return ret;
}

static u8 gup_burn_fw_dsp(struct i2c_client *client)
{
	s32 ret = 0;
	u8 *fw_dsp = NULL;
	u8	retry = 0;
	u8	rd_buf[5];

	dev_info(&client->dev, "[burn_fw_dsp]Begin burn dsp firmware---->>");
	/* step1:alloc memory */
	dev_dbg(&client->dev, "[burn_fw_dsp]step1:alloc memory");
	while (retry++ < 5) {
		fw_dsp = kzalloc(FW_DSP_LENGTH, GFP_KERNEL);
		if (fw_dsp == NULL) {
			continue;
		} else {
		dev_dbg(&client->dev, "[burn_fw_dsp]Alloc %dk byte memory success.",
		(FW_SECTION_LENGTH / 1024));
			break;
		}
	}
	if (retry >= 5) {
		dev_err(&client->dev, "[burn_fw_dsp]Alloc memory fail,exit.");
		return FAIL;
	}

	/* step2:load firmware dsp */
	dev_dbg(&client->dev, "[burn_fw_dsp]step2:load firmware dsp");
	ret = gup_load_section_file(fw_dsp,
	4 * FW_SECTION_LENGTH, FW_DSP_LENGTH, SEEK_SET);
	if (FAIL == ret) {
		dev_err(&client->dev, "[burn_fw_dsp]load firmware dsp fail.");
		goto exit_burn_fw_dsp;
	}

	/* step3:select bank3 */
	dev_dbg(&client->dev, "[burn_fw_dsp]step3:select bank3");
	ret = gup_set_ic_msg(client, _bRW_MISCTL__SRAM_BANK, 0x03);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_dsp]select bank3 fail.");
		ret = FAIL;
		goto exit_burn_fw_dsp;
	}

	/* step4:hold ss51 & dsp */
	dev_dbg(&client->dev, "[burn_fw_dsp]step4:hold ss51 & dsp");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x0C);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_dsp]hold ss51 & dsp fail.");
		ret = FAIL;
		goto exit_burn_fw_dsp;
	}

	/* step5:set scramble */
	dev_dbg(&client->dev, "[burn_fw_dsp]step5:set scramble");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_OPT_B0_, 0x00);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_dsp]set scramble fail.");
		ret = FAIL;
		goto exit_burn_fw_dsp;
	}

	/* step6:release ss51 & dsp */
	dev_dbg(&client->dev, "[burn_fw_dsp]step6:release ss51 & dsp");
	ret = gup_set_ic_msg(
	client, _rRW_MISCTL__SWRST_B0_, 0x04);/* 20121211 */
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_dsp]release ss51 & dsp fail.");
		ret = FAIL;
		goto exit_burn_fw_dsp;
	}
	/* must delay */
	usleep_range(1000, 1100);

	/* step7:burn 4k dsp firmware */
	dev_dbg(&client->dev, "[burn_fw_dsp]step7:burn 4k dsp firmware");
	ret = gup_burn_proc(client, fw_dsp, 0x9000, FW_DSP_LENGTH);
	if (FAIL == ret) {
		dev_err(&client->dev, "[burn_fw_dsp]burn fw_section fail.");
		goto exit_burn_fw_dsp;
	}

	/* step8:send burn cmd to move data to flash from sram */
	dev_dbg(&client->dev,
	"[burn_fw_dsp]step8:send burn cmd to move data to flash from sram");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, 0x05);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_dsp]send burn cmd fail.");
		goto exit_burn_fw_dsp;
	}
	dev_dbg(&client->dev, "[burn_fw_dsp]Wait for the burn is complete......");
	do {
		ret = gup_get_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, rd_buf, 1);
		if (ret <= 0) {
			dev_err(&client->dev, "[burn_fw_dsp]Get burn state fail");
			goto exit_burn_fw_dsp;
		}
		usleep_range(10000, 11000);
	/* dev_dbg(&client->dev, "[burn_fw_dsp]Get burn state:%d.",
	rd_buf[GTP_ADDR_LENGTH]); */
	} while (rd_buf[GTP_ADDR_LENGTH]);

	/* step9:recall check 4k dsp firmware */
	dev_dbg(&client->dev, "[burn_fw_dsp]step9:recall check 4k dsp firmware");
	ret = gup_recall_check(client, fw_dsp, 0x9000, FW_DSP_LENGTH);
	if (FAIL == ret) {
		dev_err(&client->dev,
				"[burn_fw_dsp]recall check 4k dsp firmware fail.");
		goto exit_burn_fw_dsp;
	}

	update_msg.fw_burned_len += FW_DSP_LENGTH;
	dev_dbg(&client->dev, "[burn_fw_dsp]Burned length:%d",
			update_msg.fw_burned_len);
	ret = SUCCESS;

exit_burn_fw_dsp:
	kfree(fw_dsp);

	return ret;
}

static u8 gup_burn_fw_boot(struct i2c_client *client)
{
	s32 ret = 0;
	u8 *fw_boot = NULL;
	u8	retry = 0;
	u8	rd_buf[5];

	dev_info(&client->dev, "[burn_fw_boot]Begin burn bootloader firmware---->>");

	/* step1:Alloc memory */
	dev_dbg(&client->dev, "[burn_fw_boot]step1:Alloc memory");
	while (retry++ < 5) {
		fw_boot = kzalloc(FW_BOOT_LENGTH, GFP_KERNEL);
		if (fw_boot == NULL) {
			continue;
		} else {
			dev_dbg(&client->dev,
				"[burn_fw_boot]Alloc %dk byte memory success.",
				(FW_BOOT_LENGTH/1024));
			break;
		}
	}
	if (retry >= 5) {
		dev_err(&client->dev, "[burn_fw_boot]Alloc memory fail,exit.");
		return FAIL;
	}

	/* step2:load firmware bootloader */
	dev_dbg(&client->dev, "[burn_fw_boot]step2:load firmware bootloader");
	ret = gup_load_section_file(fw_boot,
	(4 * FW_SECTION_LENGTH + FW_DSP_LENGTH), FW_BOOT_LENGTH, SEEK_SET);
	if (FAIL == ret) {
		dev_err(&client->dev, "[burn_fw_boot]load firmware bootcode fail.");
		goto exit_burn_fw_boot;
	}

	/* step3:hold ss51 & dsp */
	dev_dbg(&client->dev, "[burn_fw_boot]step3:hold ss51 & dsp");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x0C);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_boot]hold ss51 & dsp fail.");
		ret = FAIL;
		goto exit_burn_fw_boot;
	}

	/* step4:set scramble */
	dev_dbg(&client->dev, "[burn_fw_boot]step4:set scramble");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_OPT_B0_, 0x00);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_boot]set scramble fail.");
		ret = FAIL;
		goto exit_burn_fw_boot;
	}

	/* step5:hold ss51 & release dsp */
	dev_dbg(&client->dev, "[burn_fw_boot]step5:hold ss51 & release dsp");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x04);
	/* 20121211 */
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_boot]release ss51 & dsp fail.");
		ret = FAIL;
		goto exit_burn_fw_boot;
	}
	/* must delay */
	usleep_range(1000, 1100);

	/* step6:select bank3 */
	dev_dbg(&client->dev, "[burn_fw_boot]step6:select bank3");
	ret = gup_set_ic_msg(client, _bRW_MISCTL__SRAM_BANK, 0x03);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_boot]select bank3 fail.");
		ret = FAIL;
		goto exit_burn_fw_boot;
	}

	/* step6:burn 2k bootloader firmware */
	dev_dbg(&client->dev, "[burn_fw_boot]step6:burn 2k bootloader firmware");
	ret = gup_burn_proc(client, fw_boot, 0x9000, FW_BOOT_LENGTH);
	if (FAIL == ret) {
		dev_err(&client->dev, "[burn_fw_boot]burn fw_boot fail.");
		goto exit_burn_fw_boot;
	}

	/* step7:send burn cmd to move data to flash from sram */
	dev_dbg(&client->dev,
	"[burn_fw_boot]step7:send burn cmd to move data to flash from sram");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, 0x06);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_boot]send burn cmd fail.");
		goto exit_burn_fw_boot;
	}
	dev_dbg(&client->dev, "[burn_fw_boot]Wait for the burn is complete......");
	do {
		ret = gup_get_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, rd_buf, 1);
		if (ret <= 0) {
			dev_err(&client->dev, "[burn_fw_boot]Get burn state fail");
			goto exit_burn_fw_boot;
		}
		usleep_range(10000, 11000);
	/* dev_dbg(&client->dev, "[burn_fw_boot]Get burn state:%d.",
	rd_buf[GTP_ADDR_LENGTH]); */
	} while (rd_buf[GTP_ADDR_LENGTH]);

	/* step8:recall check 2k bootloader firmware */
	dev_dbg(&client->dev,
			"[burn_fw_boot]step8:recall check 2k bootloader firmware");
	ret = gup_recall_check(client, fw_boot, 0x9000, FW_BOOT_LENGTH);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_boot]recall check 2k bootcode firmware fail.");
		goto exit_burn_fw_boot;
	}

	update_msg.fw_burned_len += FW_BOOT_LENGTH;
	dev_dbg(&client->dev, "[burn_fw_boot]Burned length:%d",
			update_msg.fw_burned_len);
	ret = SUCCESS;

exit_burn_fw_boot:
	kfree(fw_boot);

	return ret;
}
static u8 gup_burn_fw_boot_isp(struct i2c_client *client)
{
	s32 ret = 0;
	u8 *fw_boot_isp = NULL;
	u8	retry = 0;
	u8	rd_buf[5];

	if (update_msg.fw_burned_len >= update_msg.fw_total_len) {
		dev_dbg(&client->dev, "No need to upgrade the boot_isp code!");
		return SUCCESS;
	}
	dev_info(&client->dev,
			"[burn_fw_boot_isp]Begin burn boot_isp firmware---->>");

	/* step1:Alloc memory */
	dev_dbg(&client->dev, "[burn_fw_boot_isp]step1:Alloc memory");
	while (retry++ < 5) {
		fw_boot_isp = kzalloc(FW_BOOT_ISP_LENGTH, GFP_KERNEL);
		if (fw_boot_isp == NULL) {
			continue;
		} else {
			dev_dbg(&client->dev,
				"[burn_fw_boot_isp]Alloc %dk byte memory success.",
				(FW_BOOT_ISP_LENGTH/1024));
			break;
		}
	}
	if (retry >= 5) {
		dev_err(&client->dev, "[burn_fw_boot_isp]Alloc memory fail,exit.");
		return FAIL;
	}

	/* step2:load firmware bootloader */
	dev_dbg(&client->dev,
			"[burn_fw_boot_isp]step2:load firmware bootloader isp");
	/* ret = gup_load_section_file(fw_boot_isp,
	(4*FW_SECTION_LENGTH+FW_DSP_LENGTH +
	FW_BOOT_LENGTH+FW_DSP_ISP_LENGTH), FW_BOOT_ISP_LENGTH, SEEK_SET);*/
	ret = gup_load_section_file(fw_boot_isp,
	(update_msg.fw_burned_len - FW_DSP_ISP_LENGTH),
	FW_BOOT_ISP_LENGTH, SEEK_SET);
	if (FAIL == ret) {
		dev_err(&client->dev, "[burn_fw_boot_isp]load firmware boot_isp fail.");
		goto exit_burn_fw_boot_isp;
	}

	/* step3:hold ss51 & dsp */
	dev_dbg(&client->dev, "[burn_fw_boot_isp]step3:hold ss51 & dsp");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x0C);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_boot_isp]hold ss51 & dsp fail.");
		ret = FAIL;
		goto exit_burn_fw_boot_isp;
	}

	/* step4:set scramble */
	dev_dbg(&client->dev, "[burn_fw_boot_isp]step4:set scramble");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_OPT_B0_, 0x00);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_boot_isp]set scramble fail.");
		ret = FAIL;
		goto exit_burn_fw_boot_isp;
	}


	/* step5:hold ss51 & release dsp */
	dev_dbg(&client->dev, "[burn_fw_boot_isp]step5:hold ss51 & release dsp");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x04);
	/* 20121211 */
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_boot_isp]release ss51 & dsp fail.");
		ret = FAIL;
		goto exit_burn_fw_boot_isp;
	}
	/* must delay */
	usleep_range(1000, 2000);

	/* step6:select bank3 */
	dev_dbg(&client->dev, "[burn_fw_boot_isp]step6:select bank3");
	ret = gup_set_ic_msg(client, _bRW_MISCTL__SRAM_BANK, 0x03);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_boot_isp]select bank3 fail.");
		ret = FAIL;
		goto exit_burn_fw_boot_isp;
	}

	/* step7:burn 2k bootload_isp firmware */
	dev_dbg(&client->dev, "[burn_fw_boot_isp]step7:burn 2k bootloader firmware");
	ret = gup_burn_proc(client, fw_boot_isp, 0x9000, FW_BOOT_ISP_LENGTH);
	if (FAIL == ret) {
		dev_err(&client->dev, "[burn_fw_boot_isp]burn fw_section fail.");
		goto exit_burn_fw_boot_isp;
	}

	/* step7:send burn cmd to move data to flash from sram */
	dev_dbg(&client->dev,
	"[burn_fw_boot_isp]step8:send burn cmd to move data to flash from sram");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, 0x07);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_boot_isp]send burn cmd fail.");
		goto exit_burn_fw_boot_isp;
	}
	dev_dbg(&client->dev,
			"[burn_fw_boot_isp]Wait for the burn is complete......");
	do {
		ret = gup_get_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, rd_buf, 1);
		if (ret <= 0) {
			dev_err(&client->dev,
					"[burn_fw_boot_isp]Get burn state fail");
			goto exit_burn_fw_boot_isp;
		}
		usleep_range(10000, 11000);
	/* dev_dbg(&client->dev, "[burn_fw_boot_isp]Get
	burn state:%d.", rd_buf[GTP_ADDR_LENGTH]); */
	} while (rd_buf[GTP_ADDR_LENGTH]);

	/* step8:recall check 2k bootload_isp firmware */
	dev_dbg(&client->dev,
		"[burn_fw_boot_isp]step9:recall check 2k bootloader firmware");
	ret = gup_recall_check(client, fw_boot_isp, 0x9000, FW_BOOT_ISP_LENGTH);
	if (FAIL == ret) {
		dev_err(&client->dev,
		"[burn_fw_boot_isp]recall check 2k bootcode_isp firmware fail.");
		goto exit_burn_fw_boot_isp;
	}

	update_msg.fw_burned_len += FW_BOOT_ISP_LENGTH;
	dev_dbg(&client->dev,
		"[burn_fw_boot_isp]Burned length:%d", update_msg.fw_burned_len);
	ret = SUCCESS;

exit_burn_fw_boot_isp:
	kfree(fw_boot_isp);

	return ret;
}

static u8 gup_burn_fw_link(struct i2c_client *client)
{
	u8 *fw_link = NULL;
	u8	retry = 0;
	s32 ret = 0;
	u32 offset;

	if (update_msg.fw_burned_len >= update_msg.fw_total_len) {
		dev_dbg(&client->dev, "No need to upgrade the link code!");
		return SUCCESS;
	}
	dev_info(&client->dev, "[burn_fw_link]Begin burn link firmware---->>");

	/* step1:Alloc memory */
	dev_dbg(&client->dev, "[burn_fw_link]step1:Alloc memory");
	while (retry++ < 5) {
		fw_link = kzalloc(FW_SECTION_LENGTH, GFP_KERNEL);
		if (fw_link == NULL) {
			continue;
		} else {
			dev_dbg(&client->dev,
				"[burn_fw_link]Alloc %dk byte memory success.",
				(FW_SECTION_LENGTH/1024));
			break;
		}
	}
	if (retry >= 5) {
		dev_err(&client->dev, "[burn_fw_link]Alloc memory fail,exit.");
		return FAIL;
	}

	/* step2:load firmware link section 1 */
	dev_dbg(&client->dev, "[burn_fw_link]step2:load firmware link section 1");
	offset = update_msg.fw_burned_len - FW_DSP_ISP_LENGTH;
	ret = gup_load_section_file(
	fw_link, offset, FW_SECTION_LENGTH, SEEK_SET);
	if (FAIL == ret) {
		dev_err(&client->dev,
				"[burn_fw_link]load firmware link section 1 fail.");
		goto exit_burn_fw_link;
	}

	/* step3:burn link firmware section 1 */
	dev_dbg(&client->dev, "[burn_fw_link]step3:burn link firmware section 1");
	ret = gup_burn_fw_gwake_section(
	client, fw_link, 0x9000, FW_SECTION_LENGTH, 0x38);

	if (FAIL == ret) {
		dev_err(&client->dev,
				"[burn_fw_link]burn link firmware section 1 fail.");
		goto exit_burn_fw_link;
	}

	/* step4:load link firmware section 2 file data */
	dev_dbg(&client->dev,
		"[burn_fw_link]step4:load link firmware section 2 file data");
	offset += FW_SECTION_LENGTH;
	ret = gup_load_section_file(
	fw_link, offset, FW_GLINK_LENGTH - FW_SECTION_LENGTH, SEEK_SET);

	if (FAIL == ret) {
		dev_err(&client->dev,
				"[burn_fw_link]load link firmware section 2 fail.");
		goto exit_burn_fw_link;
	}

	/* step5:burn link firmware section 2 */
	dev_dbg(&client->dev, "[burn_fw_link]step4:burn link firmware section 2");
	ret = gup_burn_fw_gwake_section(client,
	fw_link, 0x9000, FW_GLINK_LENGTH - FW_SECTION_LENGTH, 0x39);

	if (FAIL == ret) {
		dev_err(&client->dev,
				"[burn_fw_link]burn link firmware section 2 fail.");
		goto exit_burn_fw_link;
	}

	update_msg.fw_burned_len += FW_GLINK_LENGTH;
	dev_dbg(&client->dev,
			"[burn_fw_link]Burned length:%d",
			update_msg.fw_burned_len);
	ret = SUCCESS;

exit_burn_fw_link:
	kfree(fw_link);

	return ret;
}

static u8 gup_burn_fw_gwake_section(struct i2c_client *client,
		u8 *fw_section, u16 start_addr, u32 len, u8 bank_cmd)
{
	s32 ret = 0;
	u8	rd_buf[5];

	/* step1:hold ss51 & dsp */
	ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x0C);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_app_section]hold ss51 & dsp fail.");
		return FAIL;
	}

	/* step2:set scramble */
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_OPT_B0_, 0x00);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_app_section]set scramble fail.");
		return FAIL;
	}

	/* step3:hold ss51 & release dsp */
	ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x04);
	if (ret <= 0) {
		dev_err(&client->dev,
			"[burn_fw_app_section]hold ss51 & release dsp fail.");
		return FAIL;
	}
	/* must delay */
	usleep_range(100000, 200000);

	/* step4:select bank */
	ret = gup_set_ic_msg(
	client, _bRW_MISCTL__SRAM_BANK, (bank_cmd >> 4)&0x0F);
	if (ret <= 0) {
		dev_err(&client->dev,
				"[burn_fw_section]select bank %d fail.",
				(bank_cmd >> 4)&0x0F);
		return FAIL;
	}

	/* step5:burn fw section */
	ret = gup_burn_proc(client, fw_section, start_addr, len);
	if (FAIL == ret) {
		dev_err(&client->dev,
				"[burn_fw_app_section]burn fw_section fail.");
		return FAIL;
	}

	/* step6:send burn cmd to move data to flash from sram */
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, bank_cmd&0x0F);
	if (ret <= 0) {
		dev_err(&client->dev,
				"[burn_fw_app_section]send burn cmd fail.");
		return FAIL;
	}
	dev_dbg(&client->dev, "[burn_fw_section]Wait for the burn is complete......");
	do {
		ret = gup_get_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, rd_buf, 1);
		if (ret <= 0) {
			dev_err(&client->dev,
				"[burn_fw_app_section]Get burn state fail");
			return FAIL;
		}
		usleep_range(10000, 11000);
	/* dev_dbg(&client->dev, "[burn_fw_app_section]Get burn state:%d."
	, rd_buf[GTP_ADDR_LENGTH]); */
	} while (rd_buf[GTP_ADDR_LENGTH]);

	/* step7:recall fw section */
	ret = gup_recall_check(client, fw_section, start_addr, len);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_app_section]recall check %dk firmware fail.",
			len/1024);
		return FAIL;
	}

	return SUCCESS;
}

static u8 gup_burn_fw_gwake(struct i2c_client *client)
{
	u8 *fw_gwake = NULL;
	u8	retry = 0;
	s32 ret = 0;
	u16 start_index = 4*FW_SECTION_LENGTH +
	FW_DSP_LENGTH + FW_BOOT_LENGTH +
	FW_BOOT_ISP_LENGTH + FW_GLINK_LENGTH;/* 32 + 4 + 2 + 4 = 42K */
	/* u16 start_index; */

	if (update_msg.fw_burned_len >= update_msg.fw_total_len) {
		dev_dbg(&client->dev, "No need to upgrade the gwake code!");
		return SUCCESS;
	}
	/* start_index = update_msg.fw_burned_len - FW_DSP_ISP_LENGTH; */
	dev_info(&client->dev, "[burn_fw_gwake]Begin burn gwake firmware---->>");

	/* step1:alloc memory */
	dev_dbg(&client->dev, "[burn_fw_gwake]step1:alloc memory");
	while (retry++ < 5) {
		fw_gwake =
	kzalloc(FW_SECTION_LENGTH, GFP_KERNEL);
		if (fw_gwake == NULL) {
			continue;
		} else {
			dev_dbg(&client->dev,
				"[burn_fw_gwake]Alloc %dk byte memory success.",
				(FW_SECTION_LENGTH/1024));
			break;
		}
	}
	if (retry >= 5) {
		dev_err(&client->dev, "[burn_fw_gwake]Alloc memory fail,exit.");
		return FAIL;
	}

	/* clear control flag */
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, 0x00);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_finish]clear control flag fail.");
		goto exit_burn_fw_gwake;
	}

	/* step2:load app_code firmware section 1 file data */
	dev_dbg(&client->dev,
		"[burn_fw_gwake]step2:load app_code firmware section 1 file data");
	ret = gup_load_section_file(fw_gwake,
	start_index, FW_SECTION_LENGTH, SEEK_SET);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_gwake]load app_code firmware section 1 fail.");
		goto exit_burn_fw_gwake;
	}

	/* step3:burn app_code firmware section 1 */
	dev_dbg(&client->dev,
			"[burn_fw_gwake]step3:burn app_code firmware section 1");
	ret = gup_burn_fw_gwake_section(client,
	fw_gwake, 0x9000, FW_SECTION_LENGTH, 0x3A);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_gwake]burn app_code firmware section 1 fail.");
		goto exit_burn_fw_gwake;
	}

	/* step5:load app_code firmware section 2 file data */
	dev_dbg(&client->dev,
		"[burn_fw_gwake]step5:load app_code firmware section 2 file data");
	ret = gup_load_section_file(
	fw_gwake, start_index+FW_SECTION_LENGTH, FW_SECTION_LENGTH, SEEK_SET);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_gwake]load app_code firmware section 2 fail.");
		goto exit_burn_fw_gwake;
	}

	/* step6:burn app_code firmware section 2 */
	dev_dbg(&client->dev,
			"[burn_fw_gwake]step6:burn app_code firmware section 2");
	ret = gup_burn_fw_gwake_section(client,
	fw_gwake, 0x9000, FW_SECTION_LENGTH, 0x3B);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_gwake]burn app_code firmware section 2 fail.");
		goto exit_burn_fw_gwake;
	}

	/* step7:load app_code firmware section 3 file data */
	dev_dbg(&client->dev,
		"[burn_fw_gwake]step7:load app_code firmware section 3 file data");
	ret = gup_load_section_file(
	fw_gwake, start_index + 2*FW_SECTION_LENGTH,
	FW_SECTION_LENGTH, SEEK_SET);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_gwake]load app_code firmware section 3 fail.");
		goto exit_burn_fw_gwake;
	}

	/* step8:burn app_code firmware section 3 */
	dev_dbg(&client->dev,
			"[burn_fw_gwake]step8:burn app_code firmware section 3");
	ret = gup_burn_fw_gwake_section(
	client, fw_gwake, 0x9000, FW_SECTION_LENGTH, 0x3C);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_gwake]burn app_code firmware section 3 fail.");
		goto exit_burn_fw_gwake;
	}

	/* step9:load app_code firmware section 4 file data */
	dev_dbg(&client->dev,
		"[burn_fw_gwake]step9:load app_code firmware section 4 file data");
	ret = gup_load_section_file(fw_gwake,
	start_index + 3*FW_SECTION_LENGTH, FW_SECTION_LENGTH, SEEK_SET);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_gwake]load app_code firmware section 4 fail.");
		goto exit_burn_fw_gwake;
	}

	/* step10:burn app_code firmware section 4 */
	dev_dbg(&client->dev,
		"[burn_fw_gwake]step10:burn app_code firmware section 4");
	ret = gup_burn_fw_gwake_section(
	client, fw_gwake, 0x9000, FW_SECTION_LENGTH, 0x3D);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_gwake]burn app_code firmware section 4 fail.");
		goto exit_burn_fw_gwake;
	}

	/* update_msg.fw_burned_len += FW_GWAKE_LENGTH; */
	dev_dbg(&client->dev,
		"[burn_fw_gwake]Burned length:%d", update_msg.fw_burned_len);
	ret = SUCCESS;

exit_burn_fw_gwake:
	kfree(fw_gwake);

	return ret;
}

static u8 gup_burn_fw_finish(struct i2c_client *client)
{
	u8 *fw_ss51 = NULL;
	u8	retry = 0;
	s32 ret = 0;

	dev_info(&client->dev,
			"[burn_fw_finish]burn first 8K of ss51 and finish update.");
	/* step1:alloc memory */
	dev_dbg(&client->dev, "[burn_fw_finish]step1:alloc memory");
	while (retry++ < 5) {
		fw_ss51 = kzalloc(FW_SECTION_LENGTH, GFP_KERNEL);
		if (fw_ss51 == NULL) {
			continue;
		} else {
			dev_dbg(&client->dev,
				"[burn_fw_finish]Alloc %dk byte memory success.",
				(FW_SECTION_LENGTH/1024));
			break;
		}
	}
	if (retry >= 5) {
		dev_err(&client->dev, "[burn_fw_finish]Alloc memory fail,exit.");
		return FAIL;
	}

	dev_dbg(&client->dev, "[burn_fw_finish]step2: burn ss51 first 8K.");
	ret = gup_load_section_file(fw_ss51, 0, FW_SECTION_LENGTH, SEEK_SET);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_finish]load ss51 firmware section 1 fail.");
		goto exit_burn_fw_finish;
	}

	dev_dbg(&client->dev, "[burn_fw_finish]step3:clear control flag");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, 0x00);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_finish]clear control flag fail.");
		goto exit_burn_fw_finish;
	}

	dev_dbg(&client->dev, "[burn_fw_finish]step4:burn ss51 firmware section 1");
	ret = gup_burn_fw_section(client, fw_ss51, 0xC000, 0x01);
	if (FAIL == ret) {
		dev_err(&client->dev,
			"[burn_fw_finish]burn ss51 firmware section 1 fail.");
		goto exit_burn_fw_finish;
	}

	/* step11:enable download DSP code */
	dev_dbg(&client->dev, "[burn_fw_finish]step5:enable download DSP code ");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, 0x99);
	if (ret <= 0) {
		dev_err(&client->dev, "[burn_fw_finish]enable download DSP code fail.");
		goto exit_burn_fw_finish;
	}

	/* step12:release ss51 & hold dsp */
	dev_dbg(&client->dev, "[burn_fw_finish]step6:release ss51 & hold dsp");
	ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x08);
	if (ret <= 0) {
		dev_err(&client->dev,
				"[burn_fw_finish]release ss51 & hold dsp fail.");
		goto exit_burn_fw_finish;
	}

	if (fw_ss51 != NULL)
		kfree(fw_ss51);
	return SUCCESS;

exit_burn_fw_finish:
	if (fw_ss51 != NULL)
		kfree(fw_ss51);

	return FAIL;
}

s32 gup_update_proc(void *dir)
{
	s32 ret = 0;
	s32 update_ret = FAIL;
	u8	retry = 0;
	struct st_fw_head fw_head;
	struct goodix_ts_data *ts = NULL;

	ts = i2c_get_clientdata(i2c_connect_client);

	dev_dbg(&ts->client->dev, "[update_proc]Begin update ......");

	show_len = 1;
	total_len = 100;

	ret = gup_check_update_file(
	i2c_connect_client, &fw_head, (u8 *)dir);	 /* 20121211 */
	if (FAIL == ret) {
		dev_err(&ts->client->dev, "[update_proc]check update file fail.");
		goto file_fail;
	}

	ret = gup_get_ic_fw_msg(i2c_connect_client);
	if (FAIL == ret) {
		dev_err(&ts->client->dev, "[update_proc]get ic message fail.");
		goto file_fail;
	}

	if (ts->force_update) {
		dev_dbg(&ts->client->dev, "Enter force update.");
	} else {
		ret = gup_enter_update_judge(i2c_connect_client, &fw_head);
		if (FAIL == ret) {
			dev_err(&ts->client->dev,
					"[update_proc]Check *.bin file fail.");
			goto file_fail;
		}
	}

	ts->enter_update = 1;
	gtp_irq_control_enable(ts, false);
	if (ts->pdata->esd_protect)
		gtp_esd_switch(ts->client, SWITCH_OFF);
	ret = gup_enter_update_mode(i2c_connect_client);
	if (FAIL == ret) {
		dev_err(&ts->client->dev, "[update_proc]enter update mode fail.");
		goto update_fail;
	}

	while (retry++ < 5) {
		show_len = 10;
		total_len = 100;
		update_msg.fw_burned_len = 0;
		ret = gup_burn_dsp_isp(i2c_connect_client);
		if (FAIL == ret) {
			dev_err(&ts->client->dev, "[update_proc]burn dsp isp fail.");
			continue;
		}

		show_len = 20;
		ret = gup_burn_fw_gwake(i2c_connect_client);
		if (FAIL == ret) {
			dev_err(&ts->client->dev,
					"[update_proc]burn app_code firmware fail.");
			continue;
		}

		show_len = 30;
		ret = gup_burn_fw_ss51(i2c_connect_client);
		if (FAIL == ret) {
			dev_err(&ts->client->dev,
					"[update_proc]burn ss51 firmware fail.");
			continue;
		}

		show_len = 40;
		ret = gup_burn_fw_dsp(i2c_connect_client);
		if (FAIL == ret) {
			dev_err(&ts->client->dev,
					"[update_proc]burn dsp firmware fail.");
			continue;
		}

		show_len = 50;
		ret = gup_burn_fw_boot(i2c_connect_client);
		if (FAIL == ret) {
			dev_err(&ts->client->dev,
				"[update_proc]burn bootloader firmware fail.");
			continue;
		}
		show_len = 60;

		ret = gup_burn_fw_boot_isp(i2c_connect_client);
		if (FAIL == ret) {
			dev_err(&ts->client->dev,
				"[update_proc]burn boot_isp firmware fail.");
			continue;
		}

		show_len = 70;
		ret = gup_burn_fw_link(i2c_connect_client);
		if (FAIL == ret) {
			dev_err(&ts->client->dev,
				"[update_proc]burn link firmware fail.");
			continue;
		}

		show_len = 80;
		ret = gup_burn_fw_finish(i2c_connect_client);
		if (FAIL == ret) {
			dev_err(&ts->client->dev,
				"[update_proc]burn finish fail.");
			continue;
		}
		show_len = 90;
		dev_info(&ts->client->dev, "[update_proc]UPDATE SUCCESS.");
		retry = 0;
		break;
	}

	if (retry >= 5) {
		dev_err(&ts->client->dev,
				"[update_proc]retry timeout,UPDATE FAIL.");
		update_ret = FAIL;
	} else {
		update_ret = SUCCESS;
	}

update_fail:
	dev_dbg(&ts->client->dev, "[update_proc]leave update mode.");
	gup_leave_update_mode(i2c_connect_client);

	msleep(GTP_100_DLY_MS);

	if (SUCCESS == update_ret) {
		if (ts->fw_error) {
			dev_info(&ts->client->dev,
				"firmware error auto update, resent config!");
			gup_init_panel(ts);
		} else {
			dev_dbg(&ts->client->dev, "[update_proc]send config.");
			ret = gtp_send_cfg(i2c_connect_client);
			if (ret < 0)
				dev_err(&ts->client->dev,
						"[update_proc]send config fail.");
			 else
				msleep(GTP_100_DLY_MS);
		}
	}
	ts->enter_update = 0;
	gtp_irq_control_enable(ts, true);

	if (ts->charger_detection_enabled)
		goodix_resume_ps_chg_cm_state(ts);

	if (ts->pdata->esd_protect)
		gtp_esd_switch(ts->client, SWITCH_ON);

file_fail:
	total_len = 100;
	if (SUCCESS == update_ret) {
		show_len = 100;
		return SUCCESS;
	} else {
		show_len = 200;
		return FAIL;
	}
}

u8 gup_init_update_proc(struct goodix_ts_data *ts)
{
	struct task_struct *thread = NULL;

	dev_info(&ts->client->dev, "Ready to run update thread.");

	thread = kthread_run(gup_update_proc,
			(void *)NULL, "guitar_update");

	if (IS_ERR(thread)) {
		dev_err(&ts->client->dev,
				"Failed to create update thread.\n");
		return -EPERM;
	}

	return 0;
}
