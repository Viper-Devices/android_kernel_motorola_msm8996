/*
 * max14577-private.h - Common API for the Maxim 14577 internal sub chip
 *
 * Copyright (C) 2013 Samsung Electrnoics
 * Chanwoo Choi <cw00.choi@samsung.com>
 * Krzysztof Kozlowski <k.kozlowski@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MAX14577_PRIVATE_H__
#define __MAX14577_PRIVATE_H__

#include <linux/i2c.h>
#include <linux/regmap.h>

/* Slave addr = 0x4A: MUIC and Charger */
enum max14577_reg {
	MAX14577_REG_DEVICEID		= 0x00,
	MAX14577_REG_INT1		= 0x01,
	MAX14577_REG_INT2		= 0x02,
	MAX14577_REG_INT3		= 0x03,
	MAX14577_REG_STATUS1		= 0x04,
	MAX14577_REG_STATUS2		= 0x05,
	MAX14577_REG_STATUS3		= 0x06,
	MAX14577_REG_INTMASK1		= 0x07,
	MAX14577_REG_INTMASK2		= 0x08,
	MAX14577_REG_INTMASK3		= 0x09,
	MAX14577_REG_CDETCTRL1		= 0x0A,
	MAX14577_REG_RFU		= 0x0B,
	MAX14577_REG_CONTROL1		= 0x0C,
	MAX14577_REG_CONTROL2		= 0x0D,
	MAX14577_REG_CONTROL3		= 0x0E,
	MAX14577_REG_CHGCTRL1		= 0x0F,
	MAX14577_REG_CHGCTRL2		= 0x10,
	MAX14577_REG_CHGCTRL3		= 0x11,
	MAX14577_REG_CHGCTRL4		= 0x12,
	MAX14577_REG_CHGCTRL5		= 0x13,
	MAX14577_REG_CHGCTRL6		= 0x14,
	MAX14577_REG_CHGCTRL7		= 0x15,

	MAX14577_REG_END,
};

/* Slave addr = 0x4A: MUIC */
enum max14577_muic_reg {
	MAX14577_MUIC_REG_STATUS1	= 0x04,
	MAX14577_MUIC_REG_STATUS2	= 0x05,
	MAX14577_MUIC_REG_CONTROL1	= 0x0C,
	MAX14577_MUIC_REG_CONTROL3	= 0x0E,

	MAX14577_MUIC_REG_END,
};

enum max14577_muic_charger_type {
	MAX14577_CHARGER_TYPE_NONE = 0,
	MAX14577_CHARGER_TYPE_USB,
	MAX14577_CHARGER_TYPE_DOWNSTREAM_PORT,
	MAX14577_CHARGER_TYPE_DEDICATED_CHG,
	MAX14577_CHARGER_TYPE_SPECIAL_500MA,
	MAX14577_CHARGER_TYPE_SPECIAL_1A,
	MAX14577_CHARGER_TYPE_RESERVED,
	MAX14577_CHARGER_TYPE_DEAD_BATTERY = 7,
};

/* MAX14577 interrupts */
#define INT1_ADC_MASK			(0x1 << 0)
#define INT1_ADCLOW_MASK		(0x1 << 1)
#define INT1_ADCERR_MASK		(0x1 << 2)

#define INT2_CHGTYP_MASK		(0x1 << 0)
#define INT2_CHGDETRUN_MASK		(0x1 << 1)
#define INT2_DCDTMR_MASK		(0x1 << 2)
#define INT2_DBCHG_MASK			(0x1 << 3)
#define INT2_VBVOLT_MASK		(0x1 << 4)

#define INT3_EOC_MASK			(0x1 << 0)
#define INT3_CGMBC_MASK			(0x1 << 1)
#define INT3_OVP_MASK			(0x1 << 2)
#define INT3_MBCCHGERR_MASK		(0x1 << 3)

/* MAX14577 DEVICE ID register */
#define DEVID_VENDORID_SHIFT		0
#define DEVID_DEVICEID_SHIFT		3
#define DEVID_VENDORID_MASK		(0x07 << DEVID_VENDORID_SHIFT)
#define DEVID_DEVICEID_MASK		(0x1f << DEVID_DEVICEID_SHIFT)

/* MAX14577 STATUS1 register */
#define STATUS1_ADC_SHIFT		0
#define STATUS1_ADCLOW_SHIFT		5
#define STATUS1_ADCERR_SHIFT		6
#define STATUS1_ADC_MASK		(0x1f << STATUS1_ADC_SHIFT)
#define STATUS1_ADCLOW_MASK		(0x1 << STATUS1_ADCLOW_SHIFT)
#define STATUS1_ADCERR_MASK		(0x1 << STATUS1_ADCERR_SHIFT)

/* MAX14577 STATUS2 register */
#define STATUS2_CHGTYP_SHIFT		0
#define STATUS2_CHGDETRUN_SHIFT		3
#define STATUS2_DCDTMR_SHIFT		4
#define STATUS2_DBCHG_SHIFT		5
#define STATUS2_VBVOLT_SHIFT		6
#define STATUS2_CHGTYP_MASK		(0x7 << STATUS2_CHGTYP_SHIFT)
#define STATUS2_CHGDETRUN_MASK		(0x1 << STATUS2_CHGDETRUN_SHIFT)
#define STATUS2_DCDTMR_MASK		(0x1 << STATUS2_DCDTMR_SHIFT)
#define STATUS2_DBCHG_MASK		(0x1 << STATUS2_DBCHG_SHIFT)
#define STATUS2_VBVOLT_MASK		(0x1 << STATUS2_VBVOLT_SHIFT)

/* MAX14577 CONTROL1 register */
#define COMN1SW_SHIFT			0
#define COMP2SW_SHIFT			3
#define MICEN_SHIFT			6
#define IDBEN_SHIFT			7
#define COMN1SW_MASK			(0x7 << COMN1SW_SHIFT)
#define COMP2SW_MASK			(0x7 << COMP2SW_SHIFT)
#define MICEN_MASK			(0x1 << MICEN_SHIFT)
#define IDBEN_MASK			(0x1 << IDBEN_SHIFT)
#define CLEAR_IDBEN_MICEN_MASK		(COMN1SW_MASK | COMP2SW_MASK)
#define CTRL1_SW_USB			((1 << COMP2SW_SHIFT) \
						| (1 << COMN1SW_SHIFT))
#define CTRL1_SW_AUDIO			((2 << COMP2SW_SHIFT) \
						| (2 << COMN1SW_SHIFT))
#define CTRL1_SW_UART			((3 << COMP2SW_SHIFT) \
						| (3 << COMN1SW_SHIFT))
#define CTRL1_SW_OPEN			((0 << COMP2SW_SHIFT) \
						| (0 << COMN1SW_SHIFT))

/* MAX14577 CONTROL2 register */
#define CTRL2_LOWPWR_SHIFT		(0)
#define CTRL2_ADCEN_SHIFT		(1)
#define CTRL2_CPEN_SHIFT		(2)
#define CTRL2_SFOUTASRT_SHIFT		(3)
#define CTRL2_SFOUTORD_SHIFT		(4)
#define CTRL2_ACCDET_SHIFT		(5)
#define CTRL2_USBCPINT_SHIFT		(6)
#define CTRL2_RCPS_SHIFT		(7)
#define CTRL2_LOWPWR_MASK		(0x1 << CTRL2_LOWPWR_SHIFT)
#define CTRL2_ADCEN_MASK		(0x1 << CTRL2_ADCEN_SHIFT)
#define CTRL2_CPEN_MASK			(0x1 << CTRL2_CPEN_SHIFT)
#define CTRL2_SFOUTASRT_MASK		(0x1 << CTRL2_SFOUTASRT_SHIFT)
#define CTRL2_SFOUTORD_MASK		(0x1 << CTRL2_SFOUTORD_SHIFT)
#define CTRL2_ACCDET_MASK		(0x1 << CTRL2_ACCDET_SHIFT)
#define CTRL2_USBCPINT_MASK		(0x1 << CTRL2_USBCPINT_SHIFT)
#define CTRL2_RCPS_MASK			(0x1 << CTR2_RCPS_SHIFT)

#define CTRL2_CPEN1_LOWPWR0 ((1 << CTRL2_CPEN_SHIFT) | \
				(0 << CTRL2_LOWPWR_SHIFT))
#define CTRL2_CPEN0_LOWPWR1 ((0 << CTRL2_CPEN_SHIFT) | \
				(1 << CTRL2_LOWPWR_SHIFT))

/* MAX14577 CONTROL3 register */
#define CTRL3_JIGSET_SHIFT		0
#define CTRL3_BOOTSET_SHIFT		2
#define CTRL3_ADCDBSET_SHIFT		4
#define CTRL3_JIGSET_MASK		(0x3 << CTRL3_JIGSET_SHIFT)
#define CTRL3_BOOTSET_MASK		(0x3 << CTRL3_BOOTSET_SHIFT)
#define CTRL3_ADCDBSET_MASK		(0x3 << CTRL3_ADCDBSET_SHIFT)

/* Slave addr = 0x4A: Charger */
enum max14577_charger_reg {
	MAX14577_CHG_REG_STATUS3	= 0x06,
	MAX14577_CHG_REG_CHG_CTRL1	= 0x0F,
	MAX14577_CHG_REG_CHG_CTRL2	= 0x10,
	MAX14577_CHG_REG_CHG_CTRL3	= 0x11,
	MAX14577_CHG_REG_CHG_CTRL4	= 0x12,
	MAX14577_CHG_REG_CHG_CTRL5	= 0x13,
	MAX14577_CHG_REG_CHG_CTRL6	= 0x14,
	MAX14577_CHG_REG_CHG_CTRL7	= 0x15,

	MAX14577_CHG_REG_END,
};

/* MAX14577 STATUS3 register */
#define STATUS3_EOC_SHIFT		0
#define STATUS3_CGMBC_SHIFT		1
#define STATUS3_OVP_SHIFT		2
#define STATUS3_MBCCHGERR_SHIFT		3
#define STATUS3_EOC_MASK		(0x1 << STATUS3_EOC_SHIFT)
#define STATUS3_CGMBC_MASK		(0x1 << STATUS3_CGMBC_SHIFT)
#define STATUS3_OVP_MASK		(0x1 << STATUS3_OVP_SHIFT)
#define STATUS3_MBCCHGERR_MASK		(0x1 << STATUS3_MBCCHGERR_SHIFT)

/* MAX14577 CDETCTRL1 register */
#define CDETCTRL1_CHGDETEN_SHIFT	0
#define CDETCTRL1_CHGTYPMAN_SHIFT	1
#define CDETCTRL1_DCDEN_SHIFT		2
#define CDETCTRL1_DCD2SCT_SHIFT		3
#define CDETCTRL1_DCHKTM_SHIFT		4
#define CDETCTRL1_DBEXIT_SHIFT		5
#define CDETCTRL1_DBIDLE_SHIFT		6
#define CDETCTRL1_CDPDET_SHIFT		7
#define CDETCTRL1_CHGDETEN_MASK		(0x1 << CDETCTRL1_CHGDETEN_SHIFT)
#define CDETCTRL1_CHGTYPMAN_MASK	(0x1 << CDETCTRL1_CHGTYPMAN_SHIFT)
#define CDETCTRL1_DCDEN_MASK		(0x1 << CDETCTRL1_DCDEN_SHIFT)
#define CDETCTRL1_DCD2SCT_MASK		(0x1 << CDETCTRL1_DCD2SCT_SHIFT)
#define CDETCTRL1_DCHKTM_MASK		(0x1 << CDETCTRL1_DCHKTM_SHIFT)
#define CDETCTRL1_DBEXIT_MASK		(0x1 << CDETCTRL1_DBEXIT_SHIFT)
#define CDETCTRL1_DBIDLE_MASK		(0x1 << CDETCTRL1_DBIDLE_SHIFT)
#define CDETCTRL1_CDPDET_MASK		(0x1 << CDETCTRL1_CDPDET_SHIFT)

/* MAX14577 CHGCTRL1 register */
#define CHGCTRL1_TCHW_SHIFT		4
#define CHGCTRL1_TCHW_MASK		(0x7 << CHGCTRL1_TCHW_SHIFT)

/* MAX14577 CHGCTRL2 register */
#define CHGCTRL2_MBCHOSTEN_SHIFT	6
#define CHGCTRL2_MBCHOSTEN_MASK		(0x1 << CHGCTRL2_MBCHOSTEN_SHIFT)
#define CHGCTRL2_VCHGR_RC_SHIFT		7
#define CHGCTRL2_VCHGR_RC_MASK		(0x1 << CHGCTRL2_VCHGR_RC_SHIFT)

/* MAX14577 CHGCTRL3 register */
#define CHGCTRL3_MBCCVWRC_SHIFT		0
#define CHGCTRL3_MBCCVWRC_MASK		(0xf << CHGCTRL3_MBCCVWRC_SHIFT)

/* MAX14577 CHGCTRL4 register */
#define CHGCTRL4_MBCICHWRCH_SHIFT	0
#define CHGCTRL4_MBCICHWRCH_MASK	(0xf << CHGCTRL4_MBCICHWRCH_SHIFT)
#define CHGCTRL4_MBCICHWRCL_SHIFT	4
#define CHGCTRL4_MBCICHWRCL_MASK	(0x1 << CHGCTRL4_MBCICHWRCL_SHIFT)

/* MAX14577 CHGCTRL5 register */
#define CHGCTRL5_EOCS_SHIFT		0
#define CHGCTRL5_EOCS_MASK		(0xf << CHGCTRL5_EOCS_SHIFT)

/* MAX14577 CHGCTRL6 register */
#define CHGCTRL6_AUTOSTOP_SHIFT		5
#define CHGCTRL6_AUTOSTOP_MASK		(0x1 << CHGCTRL6_AUTOSTOP_SHIFT)

/* MAX14577 CHGCTRL7 register */
#define CHGCTRL7_OTPCGHCVS_SHIFT	0
#define CHGCTRL7_OTPCGHCVS_MASK		(0x3 << CHGCTRL7_OTPCGHCVS_SHIFT)

/* MAX14577 regulator current limits (as in CHGCTRL4 register), uA */
#define MAX14577_REGULATOR_CURRENT_LIMIT_MIN		 90000
#define MAX14577_REGULATOR_CURRENT_LIMIT_HIGH_START	200000
#define MAX14577_REGULATOR_CURRENT_LIMIT_HIGH_STEP	 50000
#define MAX14577_REGULATOR_CURRENT_LIMIT_MAX		950000

/* MAX14577 regulator SFOUT LDO voltage, fixed, uV */
#define MAX14577_REGULATOR_SAFEOUT_VOLTAGE		4900000

enum max14577_irq {
	/* INT1 */
	MAX14577_IRQ_INT1_ADC,
	MAX14577_IRQ_INT1_ADCLOW,
	MAX14577_IRQ_INT1_ADCERR,

	/* INT2 */
	MAX14577_IRQ_INT2_CHGTYP,
	MAX14577_IRQ_INT2_CHGDETRUN,
	MAX14577_IRQ_INT2_DCDTMR,
	MAX14577_IRQ_INT2_DBCHG,
	MAX14577_IRQ_INT2_VBVOLT,

	/* INT3 */
	MAX14577_IRQ_INT3_EOC,
	MAX14577_IRQ_INT3_CGMBC,
	MAX14577_IRQ_INT3_OVP,
	MAX14577_IRQ_INT3_MBCCHGERR,

	MAX14577_IRQ_NUM,
};

struct max14577 {
	struct device *dev;
	struct i2c_client *i2c; /* Slave addr = 0x4A */

	struct regmap *regmap;

	struct regmap_irq_chip_data *irq_data;
	int irq;

	/* Device ID */
	u8 vendor_id;	/* Vendor Identification */
	u8 device_id;	/* Chip Version */
};

/* MAX14577 shared regmap API function */
static inline int max14577_read_reg(struct regmap *map, u8 reg, u8 *dest)
{
	unsigned int val;
	int ret;

	ret = regmap_read(map, reg, &val);
	*dest = val;

	return ret;
}

static inline int max14577_bulk_read(struct regmap *map, u8 reg, u8 *buf,
		int count)
{
	return regmap_bulk_read(map, reg, buf, count);
}

static inline int max14577_write_reg(struct regmap *map, u8 reg, u8 value)
{
	return regmap_write(map, reg, value);
}

static inline int max14577_bulk_write(struct regmap *map, u8 reg, u8 *buf,
		int count)
{
	return regmap_bulk_write(map, reg, buf, count);
}

static inline int max14577_update_reg(struct regmap *map, u8 reg, u8 mask,
		u8 val)
{
	return regmap_update_bits(map, reg, mask, val);
}

#endif /* __MAX14577_PRIVATE_H__ */
