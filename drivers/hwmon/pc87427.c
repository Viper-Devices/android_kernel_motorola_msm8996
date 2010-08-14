/*
 *  pc87427.c - hardware monitoring driver for the
 *              National Semiconductor PC87427 Super-I/O chip
 *  Copyright (C) 2006, 2008  Jean Delvare <khali@linux-fr.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Supports the following chips:
 *
 *  Chip        #vin    #fan    #pwm    #temp   devid
 *  PC87427     -       8       4       -       0xF2
 *
 *  This driver assumes that no more than one chip is present.
 *  Only fans are supported so far, although the chip can do much more.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/ioport.h>
#include <linux/acpi.h>
#include <linux/io.h>

static unsigned short force_id;
module_param(force_id, ushort, 0);
MODULE_PARM_DESC(force_id, "Override the detected device ID");

static struct platform_device *pdev;

#define DRVNAME "pc87427"

/* The lock mutex protects both the I/O accesses (needed because the
   device is using banked registers) and the register cache (needed to keep
   the data in the registers and the cache in sync at any time). */
struct pc87427_data {
	struct device *hwmon_dev;
	struct mutex lock;
	int address[2];
	const char *name;

	unsigned long last_updated;	/* in jiffies */
	u8 fan_enabled;			/* bit vector */
	u16 fan[8];			/* register values */
	u16 fan_min[8];			/* register values */
	u8 fan_status[8];		/* register values */

	u8 pwm_enabled;			/* bit vector */
	u8 pwm_auto_ok;			/* bit vector */
	u8 pwm_enable[4];		/* register values */
	u8 pwm[4];			/* register values */
};

struct pc87427_sio_data {
	u8 has_fanin;
	u8 has_fanout;
};

/*
 * Super-I/O registers and operations
 */

#define SIOREG_LDSEL	0x07	/* Logical device select */
#define SIOREG_DEVID	0x20	/* Device ID */
#define SIOREG_CF2	0x22	/* Configuration 2 */
#define SIOREG_CF3	0x23	/* Configuration 3 */
#define SIOREG_CF4	0x24	/* Configuration 4 */
#define SIOREG_CF5	0x25	/* Configuration 5 */
#define SIOREG_CFB	0x2B	/* Configuration B */
#define SIOREG_CFC	0x2C	/* Configuration C */
#define SIOREG_CFD	0x2D	/* Configuration D */
#define SIOREG_ACT	0x30	/* Device activation */
#define SIOREG_MAP	0x50	/* I/O or memory mapping */
#define SIOREG_IOBASE	0x60	/* I/O base address */

static const u8 logdev[2] = { 0x09, 0x14 };
static const char *logdev_str[2] = { DRVNAME " FMC", DRVNAME " HMC" };
#define LD_FAN		0
#define LD_IN		1
#define LD_TEMP		1

static inline void superio_outb(int sioaddr, int reg, int val)
{
	outb(reg, sioaddr);
	outb(val, sioaddr + 1);
}

static inline int superio_inb(int sioaddr, int reg)
{
	outb(reg, sioaddr);
	return inb(sioaddr + 1);
}

static inline void superio_exit(int sioaddr)
{
	outb(0x02, sioaddr);
	outb(0x02, sioaddr + 1);
}

/*
 * Logical devices
 */

#define REGION_LENGTH		32
#define PC87427_REG_BANK	0x0f
#define BANK_FM(nr)		(nr)
#define BANK_FT(nr)		(0x08 + (nr))
#define BANK_FC(nr)		(0x10 + (nr) * 2)

/*
 * I/O access functions
 */

/* ldi is the logical device index */
static inline int pc87427_read8(struct pc87427_data *data, u8 ldi, u8 reg)
{
	return inb(data->address[ldi] + reg);
}

/* Must be called with data->lock held, except during init */
static inline int pc87427_read8_bank(struct pc87427_data *data, u8 ldi,
				     u8 bank, u8 reg)
{
	outb(bank, data->address[ldi] + PC87427_REG_BANK);
	return inb(data->address[ldi] + reg);
}

/* Must be called with data->lock held, except during init */
static inline void pc87427_write8_bank(struct pc87427_data *data, u8 ldi,
				       u8 bank, u8 reg, u8 value)
{
	outb(bank, data->address[ldi] + PC87427_REG_BANK);
	outb(value, data->address[ldi] + reg);
}

/*
 * Fan registers and conversions
 */

/* fan data registers are 16-bit wide */
#define PC87427_REG_FAN			0x12
#define PC87427_REG_FAN_MIN		0x14
#define PC87427_REG_FAN_STATUS		0x10

#define FAN_STATUS_STALL		(1 << 3)
#define FAN_STATUS_LOSPD		(1 << 1)
#define FAN_STATUS_MONEN		(1 << 0)

/* Dedicated function to read all registers related to a given fan input.
   This saves us quite a few locks and bank selections.
   Must be called with data->lock held.
   nr is from 0 to 7 */
static void pc87427_readall_fan(struct pc87427_data *data, u8 nr)
{
	int iobase = data->address[LD_FAN];

	outb(BANK_FM(nr), iobase + PC87427_REG_BANK);
	data->fan[nr] = inw(iobase + PC87427_REG_FAN);
	data->fan_min[nr] = inw(iobase + PC87427_REG_FAN_MIN);
	data->fan_status[nr] = inb(iobase + PC87427_REG_FAN_STATUS);
	/* Clear fan alarm bits */
	outb(data->fan_status[nr], iobase + PC87427_REG_FAN_STATUS);
}

/* The 2 LSB of fan speed registers are used for something different.
   The actual 2 LSB of the measurements are not available. */
static inline unsigned long fan_from_reg(u16 reg)
{
	reg &= 0xfffc;
	if (reg == 0x0000 || reg == 0xfffc)
		return 0;
	return 5400000UL / reg;
}

/* The 2 LSB of the fan speed limit registers are not significant. */
static inline u16 fan_to_reg(unsigned long val)
{
	if (val < 83UL)
		return 0xffff;
	if (val >= 1350000UL)
		return 0x0004;
	return ((1350000UL + val / 2) / val) << 2;
}

/*
 * PWM registers and conversions
 */

#define PC87427_REG_PWM_ENABLE		0x10
#define PC87427_REG_PWM_DUTY		0x12

#define PWM_ENABLE_MODE_MASK		(7 << 4)
#define PWM_ENABLE_CTLEN		(1 << 0)

#define PWM_MODE_MANUAL			(0 << 4)
#define PWM_MODE_AUTO			(1 << 4)
#define PWM_MODE_OFF			(2 << 4)
#define PWM_MODE_ON			(7 << 4)

/* Dedicated function to read all registers related to a given PWM output.
   This saves us quite a few locks and bank selections.
   Must be called with data->lock held.
   nr is from 0 to 3 */
static void pc87427_readall_pwm(struct pc87427_data *data, u8 nr)
{
	int iobase = data->address[LD_FAN];

	outb(BANK_FC(nr), iobase + PC87427_REG_BANK);
	data->pwm_enable[nr] = inb(iobase + PC87427_REG_PWM_ENABLE);
	data->pwm[nr] = inb(iobase + PC87427_REG_PWM_DUTY);
}

static inline int pwm_enable_from_reg(u8 reg)
{
	switch (reg & PWM_ENABLE_MODE_MASK) {
	case PWM_MODE_ON:
		return 0;
	case PWM_MODE_MANUAL:
	case PWM_MODE_OFF:
		return 1;
	case PWM_MODE_AUTO:
		return 2;
	default:
		return -EPROTO;
	}
}

static inline u8 pwm_enable_to_reg(unsigned long val, u8 pwmval)
{
	switch (val) {
	default:
		return PWM_MODE_ON;
	case 1:
		return pwmval ? PWM_MODE_MANUAL : PWM_MODE_OFF;
	case 2:
		return PWM_MODE_AUTO;
	}
}

/*
 * Data interface
 */

static struct pc87427_data *pc87427_update_device(struct device *dev)
{
	struct pc87427_data *data = dev_get_drvdata(dev);
	int i;

	mutex_lock(&data->lock);
	if (!time_after(jiffies, data->last_updated + HZ)
	 && data->last_updated)
		goto done;

	/* Fans */
	for (i = 0; i < 8; i++) {
		if (!(data->fan_enabled & (1 << i)))
			continue;
		pc87427_readall_fan(data, i);
	}

	/* PWM outputs */
	for (i = 0; i < 4; i++) {
		if (!(data->pwm_enabled & (1 << i)))
			continue;
		pc87427_readall_pwm(data, i);
	}

	data->last_updated = jiffies;

done:
	mutex_unlock(&data->lock);
	return data;
}

static ssize_t show_fan_input(struct device *dev, struct device_attribute
			      *devattr, char *buf)
{
	struct pc87427_data *data = pc87427_update_device(dev);
	int nr = to_sensor_dev_attr(devattr)->index;

	return sprintf(buf, "%lu\n", fan_from_reg(data->fan[nr]));
}

static ssize_t show_fan_min(struct device *dev, struct device_attribute
			    *devattr, char *buf)
{
	struct pc87427_data *data = pc87427_update_device(dev);
	int nr = to_sensor_dev_attr(devattr)->index;

	return sprintf(buf, "%lu\n", fan_from_reg(data->fan_min[nr]));
}

static ssize_t show_fan_alarm(struct device *dev, struct device_attribute
			      *devattr, char *buf)
{
	struct pc87427_data *data = pc87427_update_device(dev);
	int nr = to_sensor_dev_attr(devattr)->index;

	return sprintf(buf, "%d\n", !!(data->fan_status[nr]
				       & FAN_STATUS_LOSPD));
}

static ssize_t show_fan_fault(struct device *dev, struct device_attribute
			      *devattr, char *buf)
{
	struct pc87427_data *data = pc87427_update_device(dev);
	int nr = to_sensor_dev_attr(devattr)->index;

	return sprintf(buf, "%d\n", !!(data->fan_status[nr]
				       & FAN_STATUS_STALL));
}

static ssize_t set_fan_min(struct device *dev, struct device_attribute
			   *devattr, const char *buf, size_t count)
{
	struct pc87427_data *data = dev_get_drvdata(dev);
	int nr = to_sensor_dev_attr(devattr)->index;
	unsigned long val;
	int iobase = data->address[LD_FAN];

	if (strict_strtoul(buf, 10, &val) < 0)
		return -EINVAL;

	mutex_lock(&data->lock);
	outb(BANK_FM(nr), iobase + PC87427_REG_BANK);
	/* The low speed limit registers are read-only while monitoring
	   is enabled, so we have to disable monitoring, then change the
	   limit, and finally enable monitoring again. */
	outb(0, iobase + PC87427_REG_FAN_STATUS);
	data->fan_min[nr] = fan_to_reg(val);
	outw(data->fan_min[nr], iobase + PC87427_REG_FAN_MIN);
	outb(FAN_STATUS_MONEN, iobase + PC87427_REG_FAN_STATUS);
	mutex_unlock(&data->lock);

	return count;
}

static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, show_fan_input, NULL, 0);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO, show_fan_input, NULL, 1);
static SENSOR_DEVICE_ATTR(fan3_input, S_IRUGO, show_fan_input, NULL, 2);
static SENSOR_DEVICE_ATTR(fan4_input, S_IRUGO, show_fan_input, NULL, 3);
static SENSOR_DEVICE_ATTR(fan5_input, S_IRUGO, show_fan_input, NULL, 4);
static SENSOR_DEVICE_ATTR(fan6_input, S_IRUGO, show_fan_input, NULL, 5);
static SENSOR_DEVICE_ATTR(fan7_input, S_IRUGO, show_fan_input, NULL, 6);
static SENSOR_DEVICE_ATTR(fan8_input, S_IRUGO, show_fan_input, NULL, 7);

static SENSOR_DEVICE_ATTR(fan1_min, S_IWUSR | S_IRUGO,
			  show_fan_min, set_fan_min, 0);
static SENSOR_DEVICE_ATTR(fan2_min, S_IWUSR | S_IRUGO,
			  show_fan_min, set_fan_min, 1);
static SENSOR_DEVICE_ATTR(fan3_min, S_IWUSR | S_IRUGO,
			  show_fan_min, set_fan_min, 2);
static SENSOR_DEVICE_ATTR(fan4_min, S_IWUSR | S_IRUGO,
			  show_fan_min, set_fan_min, 3);
static SENSOR_DEVICE_ATTR(fan5_min, S_IWUSR | S_IRUGO,
			  show_fan_min, set_fan_min, 4);
static SENSOR_DEVICE_ATTR(fan6_min, S_IWUSR | S_IRUGO,
			  show_fan_min, set_fan_min, 5);
static SENSOR_DEVICE_ATTR(fan7_min, S_IWUSR | S_IRUGO,
			  show_fan_min, set_fan_min, 6);
static SENSOR_DEVICE_ATTR(fan8_min, S_IWUSR | S_IRUGO,
			  show_fan_min, set_fan_min, 7);

static SENSOR_DEVICE_ATTR(fan1_alarm, S_IRUGO, show_fan_alarm, NULL, 0);
static SENSOR_DEVICE_ATTR(fan2_alarm, S_IRUGO, show_fan_alarm, NULL, 1);
static SENSOR_DEVICE_ATTR(fan3_alarm, S_IRUGO, show_fan_alarm, NULL, 2);
static SENSOR_DEVICE_ATTR(fan4_alarm, S_IRUGO, show_fan_alarm, NULL, 3);
static SENSOR_DEVICE_ATTR(fan5_alarm, S_IRUGO, show_fan_alarm, NULL, 4);
static SENSOR_DEVICE_ATTR(fan6_alarm, S_IRUGO, show_fan_alarm, NULL, 5);
static SENSOR_DEVICE_ATTR(fan7_alarm, S_IRUGO, show_fan_alarm, NULL, 6);
static SENSOR_DEVICE_ATTR(fan8_alarm, S_IRUGO, show_fan_alarm, NULL, 7);

static SENSOR_DEVICE_ATTR(fan1_fault, S_IRUGO, show_fan_fault, NULL, 0);
static SENSOR_DEVICE_ATTR(fan2_fault, S_IRUGO, show_fan_fault, NULL, 1);
static SENSOR_DEVICE_ATTR(fan3_fault, S_IRUGO, show_fan_fault, NULL, 2);
static SENSOR_DEVICE_ATTR(fan4_fault, S_IRUGO, show_fan_fault, NULL, 3);
static SENSOR_DEVICE_ATTR(fan5_fault, S_IRUGO, show_fan_fault, NULL, 4);
static SENSOR_DEVICE_ATTR(fan6_fault, S_IRUGO, show_fan_fault, NULL, 5);
static SENSOR_DEVICE_ATTR(fan7_fault, S_IRUGO, show_fan_fault, NULL, 6);
static SENSOR_DEVICE_ATTR(fan8_fault, S_IRUGO, show_fan_fault, NULL, 7);

static struct attribute *pc87427_attributes_fan[8][5] = {
	{
		&sensor_dev_attr_fan1_input.dev_attr.attr,
		&sensor_dev_attr_fan1_min.dev_attr.attr,
		&sensor_dev_attr_fan1_alarm.dev_attr.attr,
		&sensor_dev_attr_fan1_fault.dev_attr.attr,
		NULL
	}, {
		&sensor_dev_attr_fan2_input.dev_attr.attr,
		&sensor_dev_attr_fan2_min.dev_attr.attr,
		&sensor_dev_attr_fan2_alarm.dev_attr.attr,
		&sensor_dev_attr_fan2_fault.dev_attr.attr,
		NULL
	}, {
		&sensor_dev_attr_fan3_input.dev_attr.attr,
		&sensor_dev_attr_fan3_min.dev_attr.attr,
		&sensor_dev_attr_fan3_alarm.dev_attr.attr,
		&sensor_dev_attr_fan3_fault.dev_attr.attr,
		NULL
	}, {
		&sensor_dev_attr_fan4_input.dev_attr.attr,
		&sensor_dev_attr_fan4_min.dev_attr.attr,
		&sensor_dev_attr_fan4_alarm.dev_attr.attr,
		&sensor_dev_attr_fan4_fault.dev_attr.attr,
		NULL
	}, {
		&sensor_dev_attr_fan5_input.dev_attr.attr,
		&sensor_dev_attr_fan5_min.dev_attr.attr,
		&sensor_dev_attr_fan5_alarm.dev_attr.attr,
		&sensor_dev_attr_fan5_fault.dev_attr.attr,
		NULL
	}, {
		&sensor_dev_attr_fan6_input.dev_attr.attr,
		&sensor_dev_attr_fan6_min.dev_attr.attr,
		&sensor_dev_attr_fan6_alarm.dev_attr.attr,
		&sensor_dev_attr_fan6_fault.dev_attr.attr,
		NULL
	}, {
		&sensor_dev_attr_fan7_input.dev_attr.attr,
		&sensor_dev_attr_fan7_min.dev_attr.attr,
		&sensor_dev_attr_fan7_alarm.dev_attr.attr,
		&sensor_dev_attr_fan7_fault.dev_attr.attr,
		NULL
	}, {
		&sensor_dev_attr_fan8_input.dev_attr.attr,
		&sensor_dev_attr_fan8_min.dev_attr.attr,
		&sensor_dev_attr_fan8_alarm.dev_attr.attr,
		&sensor_dev_attr_fan8_fault.dev_attr.attr,
		NULL
	}
};

static const struct attribute_group pc87427_group_fan[8] = {
	{ .attrs = pc87427_attributes_fan[0] },
	{ .attrs = pc87427_attributes_fan[1] },
	{ .attrs = pc87427_attributes_fan[2] },
	{ .attrs = pc87427_attributes_fan[3] },
	{ .attrs = pc87427_attributes_fan[4] },
	{ .attrs = pc87427_attributes_fan[5] },
	{ .attrs = pc87427_attributes_fan[6] },
	{ .attrs = pc87427_attributes_fan[7] },
};

/* Must be called with data->lock held and pc87427_readall_pwm() freshly
   called */
static void update_pwm_enable(struct pc87427_data *data, int nr, u8 mode)
{
	int iobase = data->address[LD_FAN];
	data->pwm_enable[nr] &= ~PWM_ENABLE_MODE_MASK;
	data->pwm_enable[nr] |= mode;
	outb(data->pwm_enable[nr], iobase + PC87427_REG_PWM_ENABLE);
}

static ssize_t show_pwm_enable(struct device *dev, struct device_attribute
			       *devattr, char *buf)
{
	struct pc87427_data *data = pc87427_update_device(dev);
	int nr = to_sensor_dev_attr(devattr)->index;
	int pwm_enable;

	pwm_enable = pwm_enable_from_reg(data->pwm_enable[nr]);
	if (pwm_enable < 0)
		return pwm_enable;
	return sprintf(buf, "%d\n", pwm_enable);
}

static ssize_t set_pwm_enable(struct device *dev, struct device_attribute
			      *devattr, const char *buf, size_t count)
{
	struct pc87427_data *data = dev_get_drvdata(dev);
	int nr = to_sensor_dev_attr(devattr)->index;
	unsigned long val;

	if (strict_strtoul(buf, 10, &val) < 0 || val > 2)
		return -EINVAL;
	/* Can't go to automatic mode if it isn't configured */
	if (val == 2 && !(data->pwm_auto_ok & (1 << nr)))
		return -EINVAL;

	mutex_lock(&data->lock);
	pc87427_readall_pwm(data, nr);
	update_pwm_enable(data, nr, pwm_enable_to_reg(val, data->pwm[nr]));
	mutex_unlock(&data->lock);

	return count;
}

static ssize_t show_pwm(struct device *dev, struct device_attribute
			*devattr, char *buf)
{
	struct pc87427_data *data = pc87427_update_device(dev);
	int nr = to_sensor_dev_attr(devattr)->index;

	return sprintf(buf, "%d\n", (int)data->pwm[nr]);
}

static ssize_t set_pwm(struct device *dev, struct device_attribute
		       *devattr, const char *buf, size_t count)
{
	struct pc87427_data *data = dev_get_drvdata(dev);
	int nr = to_sensor_dev_attr(devattr)->index;
	unsigned long val;
	int iobase = data->address[LD_FAN];
	u8 mode;

	if (strict_strtoul(buf, 10, &val) < 0 || val > 0xff)
		return -EINVAL;

	mutex_lock(&data->lock);
	pc87427_readall_pwm(data, nr);
	mode = data->pwm_enable[nr] & PWM_ENABLE_MODE_MASK;
	if (mode != PWM_MODE_MANUAL && mode != PWM_MODE_OFF) {
		dev_notice(dev, "Can't set PWM%d duty cycle while not in "
			   "manual mode\n", nr + 1);
		mutex_unlock(&data->lock);
		return -EPERM;
	}

	/* We may have to change the mode */
	if (mode == PWM_MODE_MANUAL && val == 0) {
		/* Transition from Manual to Off */
		update_pwm_enable(data, nr, PWM_MODE_OFF);
		mode = PWM_MODE_OFF;
		dev_dbg(dev, "Switching PWM%d from %s to %s\n", nr + 1,
			"manual", "off");
	} else if (mode == PWM_MODE_OFF && val != 0) {
		/* Transition from Off to Manual */
		update_pwm_enable(data, nr, PWM_MODE_MANUAL);
		mode = PWM_MODE_MANUAL;
		dev_dbg(dev, "Switching PWM%d from %s to %s\n", nr + 1,
			"off", "manual");
	}

	data->pwm[nr] = val;
	if (mode == PWM_MODE_MANUAL)
		outb(val, iobase + PC87427_REG_PWM_DUTY);
	mutex_unlock(&data->lock);

	return count;
}

static SENSOR_DEVICE_ATTR(pwm1_enable, S_IWUSR | S_IRUGO,
			  show_pwm_enable, set_pwm_enable, 0);
static SENSOR_DEVICE_ATTR(pwm2_enable, S_IWUSR | S_IRUGO,
			  show_pwm_enable, set_pwm_enable, 1);
static SENSOR_DEVICE_ATTR(pwm3_enable, S_IWUSR | S_IRUGO,
			  show_pwm_enable, set_pwm_enable, 2);
static SENSOR_DEVICE_ATTR(pwm4_enable, S_IWUSR | S_IRUGO,
			  show_pwm_enable, set_pwm_enable, 3);

static SENSOR_DEVICE_ATTR(pwm1, S_IWUSR | S_IRUGO, show_pwm, set_pwm, 0);
static SENSOR_DEVICE_ATTR(pwm2, S_IWUSR | S_IRUGO, show_pwm, set_pwm, 1);
static SENSOR_DEVICE_ATTR(pwm3, S_IWUSR | S_IRUGO, show_pwm, set_pwm, 2);
static SENSOR_DEVICE_ATTR(pwm4, S_IWUSR | S_IRUGO, show_pwm, set_pwm, 3);

static struct attribute *pc87427_attributes_pwm[4][3] = {
	{
		&sensor_dev_attr_pwm1_enable.dev_attr.attr,
		&sensor_dev_attr_pwm1.dev_attr.attr,
		NULL
	}, {
		&sensor_dev_attr_pwm2_enable.dev_attr.attr,
		&sensor_dev_attr_pwm2.dev_attr.attr,
		NULL
	}, {
		&sensor_dev_attr_pwm3_enable.dev_attr.attr,
		&sensor_dev_attr_pwm3.dev_attr.attr,
		NULL
	}, {
		&sensor_dev_attr_pwm4_enable.dev_attr.attr,
		&sensor_dev_attr_pwm4.dev_attr.attr,
		NULL
	}
};

static const struct attribute_group pc87427_group_pwm[4] = {
	{ .attrs = pc87427_attributes_pwm[0] },
	{ .attrs = pc87427_attributes_pwm[1] },
	{ .attrs = pc87427_attributes_pwm[2] },
	{ .attrs = pc87427_attributes_pwm[3] },
};

static ssize_t show_name(struct device *dev, struct device_attribute
			 *devattr, char *buf)
{
	struct pc87427_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", data->name);
}
static DEVICE_ATTR(name, S_IRUGO, show_name, NULL);


/*
 * Device detection, attach and detach
 */

static void __devinit pc87427_init_device(struct device *dev)
{
	struct pc87427_sio_data *sio_data = dev->platform_data;
	struct pc87427_data *data = dev_get_drvdata(dev);
	int i;
	u8 reg;

	/* The FMC module should be ready */
	reg = pc87427_read8(data, LD_FAN, PC87427_REG_BANK);
	if (!(reg & 0x80))
		dev_warn(dev, "FMC module not ready!\n");

	/* Check which fans are enabled */
	for (i = 0; i < 8; i++) {
		if (!(sio_data->has_fanin & (1 << i)))	/* Not wired */
			continue;
		reg = pc87427_read8_bank(data, LD_FAN, BANK_FM(i),
					 PC87427_REG_FAN_STATUS);
		if (reg & FAN_STATUS_MONEN)
			data->fan_enabled |= (1 << i);
	}

	if (!data->fan_enabled) {
		dev_dbg(dev, "Enabling monitoring of all fans\n");
		for (i = 0; i < 8; i++) {
			if (!(sio_data->has_fanin & (1 << i)))	/* Not wired */
				continue;
			pc87427_write8_bank(data, LD_FAN, BANK_FM(i),
					    PC87427_REG_FAN_STATUS,
					    FAN_STATUS_MONEN);
		}
		data->fan_enabled = sio_data->has_fanin;
	}

	/* Check which PWM outputs are enabled */
	for (i = 0; i < 4; i++) {
		if (!(sio_data->has_fanout & (1 << i)))	/* Not wired */
			continue;
		reg = pc87427_read8_bank(data, LD_FAN, BANK_FC(i),
					 PC87427_REG_PWM_ENABLE);
		if (reg & PWM_ENABLE_CTLEN)
			data->pwm_enabled |= (1 << i);

		/* We don't expose an interface to reconfigure the automatic
		   fan control mode, so only allow to return to this mode if
		   it was originally set. */
		if ((reg & PWM_ENABLE_MODE_MASK) == PWM_MODE_AUTO) {
			dev_dbg(dev, "PWM%d is in automatic control mode\n",
				i + 1);
			data->pwm_auto_ok |= (1 << i);
		}
	}
}

static int __devinit pc87427_probe(struct platform_device *pdev)
{
	struct pc87427_data *data;
	struct resource *res;
	int i, err;

	data = kzalloc(sizeof(struct pc87427_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		printk(KERN_ERR DRVNAME ": Out of memory\n");
		goto exit;
	}

	/* This will need to be revisited when we add support for
	   temperature and voltage monitoring. */
	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (!request_region(res->start, resource_size(res), DRVNAME)) {
		err = -EBUSY;
		dev_err(&pdev->dev, "Failed to request region 0x%lx-0x%lx\n",
			(unsigned long)res->start, (unsigned long)res->end);
		goto exit_kfree;
	}
	data->address[0] = res->start;

	mutex_init(&data->lock);
	data->name = "pc87427";
	platform_set_drvdata(pdev, data);
	pc87427_init_device(&pdev->dev);

	/* Register sysfs hooks */
	err = device_create_file(&pdev->dev, &dev_attr_name);
	if (err)
		goto exit_release_region;
	for (i = 0; i < 8; i++) {
		if (!(data->fan_enabled & (1 << i)))
			continue;
		err = sysfs_create_group(&pdev->dev.kobj,
					 &pc87427_group_fan[i]);
		if (err)
			goto exit_remove_files;
	}
	for (i = 0; i < 4; i++) {
		if (!(data->pwm_enabled & (1 << i)))
			continue;
		err = sysfs_create_group(&pdev->dev.kobj,
					 &pc87427_group_pwm[i]);
		if (err)
			goto exit_remove_files;
	}

	data->hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		dev_err(&pdev->dev, "Class registration failed (%d)\n", err);
		goto exit_remove_files;
	}

	return 0;

exit_remove_files:
	for (i = 0; i < 8; i++) {
		if (!(data->fan_enabled & (1 << i)))
			continue;
		sysfs_remove_group(&pdev->dev.kobj, &pc87427_group_fan[i]);
	}
	for (i = 0; i < 4; i++) {
		if (!(data->pwm_enabled & (1 << i)))
			continue;
		sysfs_remove_group(&pdev->dev.kobj, &pc87427_group_pwm[i]);
	}
exit_release_region:
	release_region(res->start, resource_size(res));
exit_kfree:
	platform_set_drvdata(pdev, NULL);
	kfree(data);
exit:
	return err;
}

static int __devexit pc87427_remove(struct platform_device *pdev)
{
	struct pc87427_data *data = platform_get_drvdata(pdev);
	struct resource *res;
	int i;

	hwmon_device_unregister(data->hwmon_dev);
	device_remove_file(&pdev->dev, &dev_attr_name);
	for (i = 0; i < 8; i++) {
		if (!(data->fan_enabled & (1 << i)))
			continue;
		sysfs_remove_group(&pdev->dev.kobj, &pc87427_group_fan[i]);
	}
	for (i = 0; i < 4; i++) {
		if (!(data->pwm_enabled & (1 << i)))
			continue;
		sysfs_remove_group(&pdev->dev.kobj, &pc87427_group_pwm[i]);
	}
	platform_set_drvdata(pdev, NULL);
	kfree(data);

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	release_region(res->start, resource_size(res));

	return 0;
}


static struct platform_driver pc87427_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= DRVNAME,
	},
	.probe		= pc87427_probe,
	.remove		= __devexit_p(pc87427_remove),
};

static int __init pc87427_device_add(unsigned short address,
				     const struct pc87427_sio_data *sio_data)
{
	struct resource res = {
		.start	= address,
		.end	= address + REGION_LENGTH - 1,
		.name	= logdev_str[0],
		.flags	= IORESOURCE_IO,
	};
	int err;

	err = acpi_check_resource_conflict(&res);
	if (err)
		goto exit;

	pdev = platform_device_alloc(DRVNAME, address);
	if (!pdev) {
		err = -ENOMEM;
		printk(KERN_ERR DRVNAME ": Device allocation failed\n");
		goto exit;
	}

	err = platform_device_add_resources(pdev, &res, 1);
	if (err) {
		printk(KERN_ERR DRVNAME ": Device resource addition failed "
		       "(%d)\n", err);
		goto exit_device_put;
	}

	err = platform_device_add_data(pdev, sio_data,
				       sizeof(struct pc87427_sio_data));
	if (err) {
		printk(KERN_ERR DRVNAME ": Platform data allocation failed\n");
		goto exit_device_put;
	}

	err = platform_device_add(pdev);
	if (err) {
		printk(KERN_ERR DRVNAME ": Device addition failed (%d)\n",
		       err);
		goto exit_device_put;
	}

	return 0;

exit_device_put:
	platform_device_put(pdev);
exit:
	return err;
}

static int __init pc87427_find(int sioaddr, unsigned short *address,
			       struct pc87427_sio_data *sio_data)
{
	u16 val;
	u8 cfg, cfg_b;
	int i, err = 0;

	/* Identify device */
	val = force_id ? force_id : superio_inb(sioaddr, SIOREG_DEVID);
	if (val != 0xf2) {	/* PC87427 */
		err = -ENODEV;
		goto exit;
	}

	for (i = 0; i < 2; i++) {
		address[i] = 0;
		/* Select logical device */
		superio_outb(sioaddr, SIOREG_LDSEL, logdev[i]);

		val = superio_inb(sioaddr, SIOREG_ACT);
		if (!(val & 0x01)) {
			printk(KERN_INFO DRVNAME ": Logical device 0x%02x "
			       "not activated\n", logdev[i]);
			continue;
		}

		val = superio_inb(sioaddr, SIOREG_MAP);
		if (val & 0x01) {
			printk(KERN_WARNING DRVNAME ": Logical device 0x%02x "
			       "is memory-mapped, can't use\n", logdev[i]);
			continue;
		}

		val = (superio_inb(sioaddr, SIOREG_IOBASE) << 8)
		    | superio_inb(sioaddr, SIOREG_IOBASE + 1);
		if (!val) {
			printk(KERN_INFO DRVNAME ": I/O base address not set "
			       "for logical device 0x%02x\n", logdev[i]);
			continue;
		}
		address[i] = val;
	}

	/* Check which fan inputs are wired */
	sio_data->has_fanin = (1 << 2) | (1 << 3);	/* FANIN2, FANIN3 */

	cfg = superio_inb(sioaddr, SIOREG_CF2);
	if (!(cfg & (1 << 3)))
		sio_data->has_fanin |= (1 << 0);	/* FANIN0 */
	if (!(cfg & (1 << 2)))
		sio_data->has_fanin |= (1 << 4);	/* FANIN4 */

	cfg = superio_inb(sioaddr, SIOREG_CFD);
	if (!(cfg & (1 << 0)))
		sio_data->has_fanin |= (1 << 1);	/* FANIN1 */

	cfg = superio_inb(sioaddr, SIOREG_CF4);
	if (!(cfg & (1 << 0)))
		sio_data->has_fanin |= (1 << 7);	/* FANIN7 */
	cfg_b = superio_inb(sioaddr, SIOREG_CFB);
	if (!(cfg & (1 << 1)) && (cfg_b & (1 << 3)))
		sio_data->has_fanin |= (1 << 5);	/* FANIN5 */
	cfg = superio_inb(sioaddr, SIOREG_CF3);
	if ((cfg & (1 << 3)) && !(cfg_b & (1 << 5)))
		sio_data->has_fanin |= (1 << 6);	/* FANIN6 */

	/* Check which fan outputs are wired */
	sio_data->has_fanout = (1 << 0);		/* FANOUT0 */
	if (cfg_b & (1 << 0))
		sio_data->has_fanout |= (1 << 3);	/* FANOUT3 */

	cfg = superio_inb(sioaddr, SIOREG_CFC);
	if (!(cfg & (1 << 4))) {
		if (cfg_b & (1 << 1))
			sio_data->has_fanout |= (1 << 1); /* FANOUT1 */
		if (cfg_b & (1 << 2))
			sio_data->has_fanout |= (1 << 2); /* FANOUT2 */
	}

	/* FANOUT1 and FANOUT2 can each be routed to 2 different pins */
	cfg = superio_inb(sioaddr, SIOREG_CF5);
	if (cfg & (1 << 6))
		sio_data->has_fanout |= (1 << 1);	/* FANOUT1 */
	if (cfg & (1 << 5))
		sio_data->has_fanout |= (1 << 2);	/* FANOUT2 */

exit:
	superio_exit(sioaddr);
	return err;
}

static int __init pc87427_init(void)
{
	int err;
	unsigned short address[2];
	struct pc87427_sio_data sio_data;

	if (pc87427_find(0x2e, address, &sio_data)
	 && pc87427_find(0x4e, address, &sio_data))
		return -ENODEV;

	/* For now the driver only handles fans so we only care about the
	   first address. */
	if (!address[0])
		return -ENODEV;

	err = platform_driver_register(&pc87427_driver);
	if (err)
		goto exit;

	/* Sets global pdev as a side effect */
	err = pc87427_device_add(address[0], &sio_data);
	if (err)
		goto exit_driver;

	return 0;

exit_driver:
	platform_driver_unregister(&pc87427_driver);
exit:
	return err;
}

static void __exit pc87427_exit(void)
{
	platform_device_unregister(pdev);
	platform_driver_unregister(&pc87427_driver);
}

MODULE_AUTHOR("Jean Delvare <khali@linux-fr.org>");
MODULE_DESCRIPTION("PC87427 hardware monitoring driver");
MODULE_LICENSE("GPL");

module_init(pc87427_init);
module_exit(pc87427_exit);
