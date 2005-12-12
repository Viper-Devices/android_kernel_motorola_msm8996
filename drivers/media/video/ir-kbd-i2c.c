/*
 *
 * keyboard input driver for i2c IR remote controls
 *
 * Copyright (c) 2000-2003 Gerd Knorr <kraxel@bytesex.org>
 * modified for PixelView (BT878P+W/FM) by
 *      Michal Kochanowicz <mkochano@pld.org.pl>
 *      Christoph Bartelmus <lirc@bartelmus.de>
 * modified for KNC ONE TV Station/Anubis Typhoon TView Tuner by
 *      Ulrich Mueller <ulrich.mueller42@web.de>
 * modified for em2820 based USB TV tuners by
 *      Markus Rechberger <mrechberger@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <asm/semaphore.h>
#include <media/ir-common.h>
#include <media/ir-kbd-i2c.h>

/* Mark Phalan <phalanm@o2.ie> */
static IR_KEYTAB_TYPE ir_codes_pv951[IR_KEYTAB_SIZE] = {
	[  0 ] = KEY_KP0,
	[  1 ] = KEY_KP1,
	[  2 ] = KEY_KP2,
	[  3 ] = KEY_KP3,
	[  4 ] = KEY_KP4,
	[  5 ] = KEY_KP5,
	[  6 ] = KEY_KP6,
	[  7 ] = KEY_KP7,
	[  8 ] = KEY_KP8,
	[  9 ] = KEY_KP9,

	[ 18 ] = KEY_POWER,
	[ 16 ] = KEY_MUTE,
	[ 31 ] = KEY_VOLUMEDOWN,
	[ 27 ] = KEY_VOLUMEUP,
	[ 26 ] = KEY_CHANNELUP,
	[ 30 ] = KEY_CHANNELDOWN,
	[ 14 ] = KEY_PAGEUP,
	[ 29 ] = KEY_PAGEDOWN,
	[ 19 ] = KEY_SOUND,

	[ 24 ] = KEY_KPPLUSMINUS,	/* CH +/- */
	[ 22 ] = KEY_SUBTITLE,		/* CC */
	[ 13 ] = KEY_TEXT,		/* TTX */
	[ 11 ] = KEY_TV,		/* AIR/CBL */
	[ 17 ] = KEY_PC,		/* PC/TV */
	[ 23 ] = KEY_OK,		/* CH RTN */
	[ 25 ] = KEY_MODE, 		/* FUNC */
	[ 12 ] = KEY_SEARCH, 		/* AUTOSCAN */

	/* Not sure what to do with these ones! */
	[ 15 ] = KEY_SELECT, 		/* SOURCE */
	[ 10 ] = KEY_KPPLUS,		/* +100 */
	[ 20 ] = KEY_KPEQUAL,		/* SYNC */
	[ 28 ] = KEY_MEDIA,             /* PC/TV */
};

/* ----------------------------------------------------------------------- */
/* insmod parameters                                                       */

static int debug;
module_param(debug, int, 0644);    /* debug level (0,1,2) */

#define DEVNAME "ir-kbd-i2c"
#define dprintk(level, fmt, arg...)	if (debug >= level) \
	printk(KERN_DEBUG DEVNAME ": " fmt , ## arg)

/* ----------------------------------------------------------------------- */

static int get_key_haup(struct IR_i2c *ir, u32 *ir_key, u32 *ir_raw)
{
	unsigned char buf[3];
	int start, toggle, dev, code;

	/* poll IR chip */
	if (3 != i2c_master_recv(&ir->c,buf,3))
		return -EIO;

	/* split rc5 data block ... */
	start  = (buf[0] >> 6) &    3;
	toggle = (buf[0] >> 5) &    1;
	dev    =  buf[0]       & 0x1f;
	code   = (buf[1] >> 2) & 0x3f;

	if (3 != start)
		/* no key pressed */
		return 0;
	dprintk(1,"ir hauppauge (rc5): s%d t%d dev=%d code=%d\n",
		start, toggle, dev, code);

	/* return key */
	*ir_key = code;
	*ir_raw = (start << 12) | (toggle << 11) | (dev << 6) | code;
	return 1;
}

static int get_key_pixelview(struct IR_i2c *ir, u32 *ir_key, u32 *ir_raw)
{
	unsigned char b;

	/* poll IR chip */
	if (1 != i2c_master_recv(&ir->c,&b,1)) {
		dprintk(1,"read error\n");
		return -EIO;
	}
	*ir_key = b;
	*ir_raw = b;
	return 1;
}

static int get_key_pv951(struct IR_i2c *ir, u32 *ir_key, u32 *ir_raw)
{
	unsigned char b;

	/* poll IR chip */
	if (1 != i2c_master_recv(&ir->c,&b,1)) {
		dprintk(1,"read error\n");
		return -EIO;
	}

	/* ignore 0xaa */
	if (b==0xaa)
		return 0;
	dprintk(2,"key %02x\n", b);

	*ir_key = b;
	*ir_raw = b;
	return 1;
}

static int get_key_knc1(struct IR_i2c *ir, u32 *ir_key, u32 *ir_raw)
{
	unsigned char b;

	/* poll IR chip */
	if (1 != i2c_master_recv(&ir->c,&b,1)) {
		dprintk(1,"read error\n");
		return -EIO;
	}

	/* it seems that 0xFE indicates that a button is still hold
	   down, while 0xff indicates that no button is hold
	   down. 0xfe sequences are sometimes interrupted by 0xFF */

	dprintk(2,"key %02x\n", b);

	if (b == 0xff)
		return 0;

	if (b == 0xfe)
		/* keep old data */
		return 1;

	*ir_key = b;
	*ir_raw = b;
	return 1;
}

/* The new pinnacle PCTV remote (with the colored buttons)
 *
 * Ricardo Cerqueira <v4l@cerqueira.org>
 */

int get_key_pinnacle(struct IR_i2c *ir, u32 *ir_key, u32 *ir_raw)
{
	unsigned char b[4];
	unsigned int start = 0,parity = 0,code = 0;

	/* poll IR chip */
	if (4 != i2c_master_recv(&ir->c,b,4)) {
		dprintk(2,"read error\n");
		return -EIO;
	}

	for (start = 0; start<4; start++) {
		if (b[start] == 0x80) {
			code=b[(start+3)%4];
			parity=b[(start+2)%4];
		}
	}

	/* Empty Request */
	if (parity==0)
		return 0;

	/* Repeating... */
	if (ir->old == parity)
		return 0;


	ir->old = parity;

	/* Reduce code value to fit inside IR_KEYTAB_SIZE
	 *
	 * this is the only value that results in 42 unique
	 * codes < 128
	 */

	code %= 0x88;

	*ir_raw = code;
	*ir_key = code;

	dprintk(1,"Pinnacle PCTV key %02x\n", code);

	return 1;
}

EXPORT_SYMBOL_GPL(get_key_pinnacle);

/* ----------------------------------------------------------------------- */

static void ir_key_poll(struct IR_i2c *ir)
{
	static u32 ir_key, ir_raw;
	int rc;

	dprintk(2,"ir_poll_key\n");
	rc = ir->get_key(ir, &ir_key, &ir_raw);
	if (rc < 0) {
		dprintk(2,"error\n");
		return;
	}

	if (0 == rc) {
		ir_input_nokey(ir->input, &ir->ir);
	} else {
		ir_input_keydown(ir->input, &ir->ir, ir_key, ir_raw);
	}
}

static void ir_timer(unsigned long data)
{
	struct IR_i2c *ir = (struct IR_i2c*)data;
	schedule_work(&ir->work);
}

static void ir_work(void *data)
{
	struct IR_i2c *ir = data;
	ir_key_poll(ir);
	mod_timer(&ir->timer, jiffies+HZ/10);
}

/* ----------------------------------------------------------------------- */

static int ir_attach(struct i2c_adapter *adap, int addr,
		      unsigned short flags, int kind);
static int ir_detach(struct i2c_client *client);
static int ir_probe(struct i2c_adapter *adap);

static struct i2c_driver driver = {
	.name           = "ir remote kbd driver",
	.id             = I2C_DRIVERID_I2C_IR,
	.flags          = I2C_DF_NOTIFY,
	.attach_adapter = ir_probe,
	.detach_client  = ir_detach,
};

static struct i2c_client client_template =
{
	.name = "unset",
	.driver = &driver
};

static int ir_attach(struct i2c_adapter *adap, int addr,
		     unsigned short flags, int kind)
{
	IR_KEYTAB_TYPE *ir_codes = NULL;
	char *name;
	int ir_type;
	struct IR_i2c *ir;
	struct input_dev *input_dev;

	ir = kzalloc(sizeof(struct IR_i2c), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!ir || !input_dev) {
		kfree(ir);
		input_free_device(input_dev);
		return -ENOMEM;
	}

	ir->c = client_template;
	ir->input = input_dev;

	i2c_set_clientdata(&ir->c, ir);
	ir->c.adapter = adap;
	ir->c.addr    = addr;

	switch(addr) {
	case 0x64:
		name        = "Pixelview";
		ir->get_key = get_key_pixelview;
		ir_type     = IR_TYPE_OTHER;
		ir_codes    = ir_codes_empty;
		break;
	case 0x4b:
		name        = "PV951";
		ir->get_key = get_key_pv951;
		ir_type     = IR_TYPE_OTHER;
		ir_codes    = ir_codes_pv951;
		break;
	case 0x18:
	case 0x1a:
		name        = "Hauppauge";
		ir->get_key = get_key_haup;
		ir_type     = IR_TYPE_RC5;
		ir_codes    = ir_codes_rc5_tv;
		break;
	case 0x30:
		name        = "KNC One";
		ir->get_key = get_key_knc1;
		ir_type     = IR_TYPE_OTHER;
		ir_codes    = ir_codes_empty;
		break;
	case 0x7a:
	case 0x47:
		/* Handled by saa7134-input */
		name        = "SAA713x remote";
		ir_type     = IR_TYPE_OTHER;
		break;
	default:
		/* shouldn't happen */
		printk(DEVNAME ": Huh? unknown i2c address (0x%02x)?\n",addr);
		kfree(ir);
		return -1;
	}

	/* Sets name */
	snprintf(ir->c.name, sizeof(ir->c.name), "i2c IR (%s)", name);
	ir->ir_codes=ir_codes;

	/* register i2c device
	 * At device register, IR codes may be changed to be
	 * board dependent.
	*/
	i2c_attach_client(&ir->c);

	/* If IR not supported or disabled, unregisters driver */
	if (ir->get_key == NULL) {
		i2c_detach_client(&ir->c);
		kfree(ir);
		return -1;
	}

	/* Phys addr can only be set after attaching (for ir->c.dev.bus_id) */
	snprintf(ir->phys, sizeof(ir->phys), "%s/%s/ir0",
		 ir->c.adapter->dev.bus_id,
		 ir->c.dev.bus_id);

	/* init + register input device */
	ir_input_init(input_dev, &ir->ir, ir_type, ir_codes);
	input_dev->id.bustype	= BUS_I2C;
	input_dev->name		= ir->c.name;
	input_dev->phys		= ir->phys;

	/* register event device */
	input_register_device(ir->input);

	/* start polling via eventd */
	INIT_WORK(&ir->work, ir_work, ir);
	init_timer(&ir->timer);
	ir->timer.function = ir_timer;
	ir->timer.data     = (unsigned long)ir;
	schedule_work(&ir->work);

	return 0;
}

static int ir_detach(struct i2c_client *client)
{
	struct IR_i2c *ir = i2c_get_clientdata(client);

	/* kill outstanding polls */
	del_timer(&ir->timer);
	flush_scheduled_work();

	/* unregister devices */
	input_unregister_device(ir->input);
	i2c_detach_client(&ir->c);

	/* free memory */
	kfree(ir);
	return 0;
}

static int ir_probe(struct i2c_adapter *adap)
{

	/* The external IR receiver is at i2c address 0x34 (0x35 for
	   reads).  Future Hauppauge cards will have an internal
	   receiver at 0x30 (0x31 for reads).  In theory, both can be
	   fitted, and Hauppauge suggest an external overrides an
	   internal.

	   That's why we probe 0x1a (~0x34) first. CB
	*/

	static const int probe_bttv[] = { 0x1a, 0x18, 0x4b, 0x64, 0x30, -1};
	static const int probe_saa7134[] = { 0x7a, 0x47, -1 };
	static const int probe_em28XX[] = { 0x30, 0x47, -1 };
	const int *probe = NULL;
	struct i2c_client c;
	unsigned char buf;
	int i,rc;

	switch (adap->id) {
	case I2C_HW_B_BT848:
		probe = probe_bttv;
		break;
	case I2C_HW_SAA7134:
		probe = probe_saa7134;
		break;
	case I2C_HW_B_EM28XX:
		probe = probe_em28XX;
		break;
	}
	if (NULL == probe)
		return 0;

	memset(&c,0,sizeof(c));
	c.adapter = adap;
	for (i = 0; -1 != probe[i]; i++) {
		c.addr = probe[i];
		rc = i2c_master_recv(&c,&buf,0);
		dprintk(1,"probe 0x%02x @ %s: %s\n",
			probe[i], adap->name,
			(0 == rc) ? "yes" : "no");
		if (0 == rc) {
			ir_attach(adap,probe[i],0,0);
			break;
		}
	}
	return 0;
}

/* ----------------------------------------------------------------------- */

MODULE_AUTHOR("Gerd Knorr, Michal Kochanowicz, Christoph Bartelmus, Ulrich Mueller");
MODULE_DESCRIPTION("input driver for i2c IR remote controls");
MODULE_LICENSE("GPL");

static int __init ir_init(void)
{
	return i2c_add_driver(&driver);
}

static void __exit ir_fini(void)
{
	i2c_del_driver(&driver);
}

module_init(ir_init);
module_exit(ir_fini);

/*
 * Overrides for Emacs so that we follow Linus's tabbing style.
 * ---------------------------------------------------------------------------
 * Local variables:
 * c-basic-offset: 8
 * End:
 */
