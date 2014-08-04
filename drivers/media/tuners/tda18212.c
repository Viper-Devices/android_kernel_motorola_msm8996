/*
 * NXP TDA18212HN silicon tuner driver
 *
 * Copyright (C) 2011 Antti Palosaari <crope@iki.fi>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "tda18212.h"

/* Max transfer size done by I2C transfer functions */
#define MAX_XFER_SIZE  64

struct tda18212_priv {
	struct tda18212_config cfg;
	struct i2c_client *client;

	u32 if_frequency;
};

/* write multiple registers */
static int tda18212_wr_regs(struct tda18212_priv *priv, u8 reg, u8 *val,
	int len)
{
	int ret;
	u8 buf[MAX_XFER_SIZE];
	struct i2c_msg msg[1] = {
		{
			.addr = priv->client->addr,
			.flags = 0,
			.len = 1 + len,
			.buf = buf,
		}
	};

	if (1 + len > sizeof(buf)) {
		dev_warn(&priv->client->dev,
				"i2c wr reg=%04x: len=%d is too big!\n",
				reg, len);
		return -EINVAL;
	}

	buf[0] = reg;
	memcpy(&buf[1], val, len);

	ret = i2c_transfer(priv->client->adapter, msg, 1);
	if (ret == 1) {
		ret = 0;
	} else {
		dev_warn(&priv->client->dev,
				"i2c wr failed=%d reg=%02x len=%d\n",
				ret, reg, len);
		ret = -EREMOTEIO;
	}
	return ret;
}

/* read multiple registers */
static int tda18212_rd_regs(struct tda18212_priv *priv, u8 reg, u8 *val,
	int len)
{
	int ret;
	u8 buf[MAX_XFER_SIZE];
	struct i2c_msg msg[2] = {
		{
			.addr = priv->client->addr,
			.flags = 0,
			.len = 1,
			.buf = &reg,
		}, {
			.addr = priv->client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf,
		}
	};

	if (len > sizeof(buf)) {
		dev_warn(&priv->client->dev,
				"i2c rd reg=%04x: len=%d is too big!\n",
				reg, len);
		return -EINVAL;
	}

	ret = i2c_transfer(priv->client->adapter, msg, 2);
	if (ret == 2) {
		memcpy(val, buf, len);
		ret = 0;
	} else {
		dev_warn(&priv->client->dev,
				"i2c rd failed=%d reg=%02x len=%d\n",
				ret, reg, len);
		ret = -EREMOTEIO;
	}

	return ret;
}

/* write single register */
static int tda18212_wr_reg(struct tda18212_priv *priv, u8 reg, u8 val)
{
	return tda18212_wr_regs(priv, reg, &val, 1);
}

/* read single register */
static int tda18212_rd_reg(struct tda18212_priv *priv, u8 reg, u8 *val)
{
	return tda18212_rd_regs(priv, reg, val, 1);
}

#if 0 /* keep, useful when developing driver */
static void tda18212_dump_regs(struct tda18212_priv *priv)
{
	int i;
	u8 buf[256];

	#define TDA18212_RD_LEN 32
	for (i = 0; i < sizeof(buf); i += TDA18212_RD_LEN)
		tda18212_rd_regs(priv, i, &buf[i], TDA18212_RD_LEN);

	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 1, buf,
		sizeof(buf), true);

	return;
}
#endif

static int tda18212_set_params(struct dvb_frontend *fe)
{
	struct tda18212_priv *priv = fe->tuner_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret, i;
	u32 if_khz;
	u8 buf[9];
	#define DVBT_6   0
	#define DVBT_7   1
	#define DVBT_8   2
	#define DVBT2_6  3
	#define DVBT2_7  4
	#define DVBT2_8  5
	#define DVBC_6   6
	#define DVBC_8   7
	#define ATSC_VSB 8
	#define ATSC_QAM 9
	static const u8 bw_params[][3] = {
		     /* reg:   0f    13    23 */
		[DVBT_6]  = { 0xb3, 0x20, 0x03 },
		[DVBT_7]  = { 0xb3, 0x31, 0x01 },
		[DVBT_8]  = { 0xb3, 0x22, 0x01 },
		[DVBT2_6] = { 0xbc, 0x20, 0x03 },
		[DVBT2_7] = { 0xbc, 0x72, 0x03 },
		[DVBT2_8] = { 0xbc, 0x22, 0x01 },
		[DVBC_6]  = { 0x92, 0x50, 0x03 },
		[DVBC_8]  = { 0x92, 0x53, 0x03 },
		[ATSC_VSB] = { 0x7d, 0x20, 0x63 },
		[ATSC_QAM] = { 0x7d, 0x20, 0x63 },
	};

	dev_dbg(&priv->client->dev,
			"delivery_system=%d frequency=%d bandwidth_hz=%d\n",
			c->delivery_system, c->frequency,
			c->bandwidth_hz);

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1); /* open I2C-gate */

	switch (c->delivery_system) {
	case SYS_ATSC:
		if_khz = priv->cfg.if_atsc_vsb;
		i = ATSC_VSB;
		break;
	case SYS_DVBC_ANNEX_B:
		if_khz = priv->cfg.if_atsc_qam;
		i = ATSC_QAM;
		break;
	case SYS_DVBT:
		switch (c->bandwidth_hz) {
		case 6000000:
			if_khz = priv->cfg.if_dvbt_6;
			i = DVBT_6;
			break;
		case 7000000:
			if_khz = priv->cfg.if_dvbt_7;
			i = DVBT_7;
			break;
		case 8000000:
			if_khz = priv->cfg.if_dvbt_8;
			i = DVBT_8;
			break;
		default:
			ret = -EINVAL;
			goto error;
		}
		break;
	case SYS_DVBT2:
		switch (c->bandwidth_hz) {
		case 6000000:
			if_khz = priv->cfg.if_dvbt2_6;
			i = DVBT2_6;
			break;
		case 7000000:
			if_khz = priv->cfg.if_dvbt2_7;
			i = DVBT2_7;
			break;
		case 8000000:
			if_khz = priv->cfg.if_dvbt2_8;
			i = DVBT2_8;
			break;
		default:
			ret = -EINVAL;
			goto error;
		}
		break;
	case SYS_DVBC_ANNEX_A:
	case SYS_DVBC_ANNEX_C:
		if_khz = priv->cfg.if_dvbc;
		i = DVBC_8;
		break;
	default:
		ret = -EINVAL;
		goto error;
	}

	ret = tda18212_wr_reg(priv, 0x23, bw_params[i][2]);
	if (ret)
		goto error;

	ret = tda18212_wr_reg(priv, 0x06, 0x00);
	if (ret)
		goto error;

	ret = tda18212_wr_reg(priv, 0x0f, bw_params[i][0]);
	if (ret)
		goto error;

	buf[0] = 0x02;
	buf[1] = bw_params[i][1];
	buf[2] = 0x03; /* default value */
	buf[3] = DIV_ROUND_CLOSEST(if_khz, 50);
	buf[4] = ((c->frequency / 1000) >> 16) & 0xff;
	buf[5] = ((c->frequency / 1000) >>  8) & 0xff;
	buf[6] = ((c->frequency / 1000) >>  0) & 0xff;
	buf[7] = 0xc1;
	buf[8] = 0x01;
	ret = tda18212_wr_regs(priv, 0x12, buf, sizeof(buf));
	if (ret)
		goto error;

	/* actual IF rounded as it is on register */
	priv->if_frequency = buf[3] * 50 * 1000;

exit:
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0); /* close I2C-gate */

	return ret;

error:
	dev_dbg(&priv->client->dev, "failed=%d\n", ret);
	goto exit;
}

static int tda18212_get_if_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct tda18212_priv *priv = fe->tuner_priv;

	*frequency = priv->if_frequency;

	return 0;
}

static const struct dvb_tuner_ops tda18212_tuner_ops = {
	.info = {
		.name           = "NXP TDA18212",

		.frequency_min  =  48000000,
		.frequency_max  = 864000000,
		.frequency_step =      1000,
	},

	.set_params    = tda18212_set_params,
	.get_if_frequency = tda18212_get_if_frequency,
};

static int tda18212_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct tda18212_config *cfg = client->dev.platform_data;
	struct dvb_frontend *fe = cfg->fe;
	struct tda18212_priv *priv;
	int ret;
	u8 chip_id = chip_id;
	char *version;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		ret = -ENOMEM;
		dev_err(&client->dev, "kzalloc() failed\n");
		goto err;
	}

	memcpy(&priv->cfg, cfg, sizeof(struct tda18212_config));
	priv->client = client;

	/* check if the tuner is there */
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1); /* open I2C-gate */

	ret = tda18212_rd_reg(priv, 0x00, &chip_id);
	dev_dbg(&priv->client->dev, "chip_id=%02x\n", chip_id);

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0); /* close I2C-gate */

	if (ret)
		goto err;

	switch (chip_id) {
	case 0xc7:
		version = "M"; /* master */
		break;
	case 0x47:
		version = "S"; /* slave */
		break;
	default:
		ret = -ENODEV;
		goto err;
	}

	dev_info(&priv->client->dev,
			"NXP TDA18212HN/%s successfully identified\n", version);

	fe->tuner_priv = priv;
	memcpy(&fe->ops.tuner_ops, &tda18212_tuner_ops,
			sizeof(struct dvb_tuner_ops));
	i2c_set_clientdata(client, priv);

	return 0;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	kfree(priv);
	return ret;
}

static int tda18212_remove(struct i2c_client *client)
{
	struct tda18212_priv *priv = i2c_get_clientdata(client);
	struct dvb_frontend *fe = priv->cfg.fe;

	dev_dbg(&client->dev, "\n");

	memset(&fe->ops.tuner_ops, 0, sizeof(struct dvb_tuner_ops));
	fe->tuner_priv = NULL;
	kfree(priv);

	return 0;
}

static const struct i2c_device_id tda18212_id[] = {
	{"tda18212", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, tda18212_id);

static struct i2c_driver tda18212_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "tda18212",
	},
	.probe		= tda18212_probe,
	.remove		= tda18212_remove,
	.id_table	= tda18212_id,
};

module_i2c_driver(tda18212_driver);

MODULE_DESCRIPTION("NXP TDA18212HN silicon tuner driver");
MODULE_AUTHOR("Antti Palosaari <crope@iki.fi>");
MODULE_LICENSE("GPL");
