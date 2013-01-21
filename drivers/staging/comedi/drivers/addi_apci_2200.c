/*
 * addi_apci_2200.c
 * Copyright (C) 2004,2005  ADDI-DATA GmbH for the source code of this module.
 * Project manager: Eric Stolz
 *
 *	ADDI-DATA GmbH
 *	Dieselstrasse 3
 *	D-77833 Ottersweier
 *	Tel: +19(0)7223/9493-0
 *	Fax: +49(0)7223/9493-92
 *	http://www.addi-data.com
 *	info@addi-data.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * You should also find the complete GPL in the COPYING file accompanying
 * this source code.
 */

#include "../comedidev.h"
#include "addi_watchdog.h"

/*
 * I/O Register Map
 */
#define APCI2200_DI_REG			0x00
#define APCI2200_DO_REG			0x04
#define APCI2200_WDOG_REG		0x08

static int apci2200_di_insn_bits(struct comedi_device *dev,
				 struct comedi_subdevice *s,
				 struct comedi_insn *insn,
				 unsigned int *data)
{
	data[1] = inw(dev->iobase + APCI2200_DI_REG);

	return insn->n;
}

static int apci2200_do_insn_bits(struct comedi_device *dev,
				 struct comedi_subdevice *s,
				 struct comedi_insn *insn,
				 unsigned int *data)
{
	unsigned int mask = data[0];
	unsigned int bits = data[1];

	s->state = inw(dev->iobase + APCI2200_DO_REG);
	if (mask) {
		s->state &= ~mask;
		s->state |= (bits & mask);

		outw(s->state, dev->iobase + APCI2200_DO_REG);
	}

	data[1] = s->state;

	return insn->n;
}

static int apci2200_reset(struct comedi_device *dev)
{
	outw(0x0, dev->iobase + APCI2200_DO_REG);

	addi_watchdog_reset(dev->iobase + APCI2200_WDOG_REG);

	return 0;
}

static int apci2200_auto_attach(struct comedi_device *dev,
				unsigned long context_unused)
{
	struct pci_dev *pcidev = comedi_to_pci_dev(dev);
	struct comedi_subdevice *s;
	int ret;

	dev->board_name = dev->driver->driver_name;

	ret = comedi_pci_enable(pcidev, dev->board_name);
	if (ret)
		return ret;

	dev->iobase = pci_resource_start(pcidev, 1);

	ret = comedi_alloc_subdevices(dev, 3);
	if (ret)
		return ret;

	/* Initialize the digital input subdevice */
	s = &dev->subdevices[0];
	s->type		= COMEDI_SUBD_DI;
	s->subdev_flags	= SDF_READABLE;
	s->n_chan	= 8;
	s->maxdata	= 1;
	s->range_table	= &range_digital;
	s->insn_bits	= apci2200_di_insn_bits;

	/* Initialize the digital output subdevice */
	s = &dev->subdevices[1];
	s->type		= COMEDI_SUBD_DO;
	s->subdev_flags	= SDF_WRITEABLE;
	s->n_chan	= 16;
	s->maxdata	= 1;
	s->range_table	= &range_digital;
	s->insn_bits	= apci2200_do_insn_bits;

	/* Initialize the watchdog subdevice */
	s = &dev->subdevices[2];
	ret = addi_watchdog_init(s, dev->iobase + APCI2200_WDOG_REG);
	if (ret)
		return ret;

	apci2200_reset(dev);
	return 0;
}

static void apci2200_detach(struct comedi_device *dev)
{
	struct pci_dev *pcidev = comedi_to_pci_dev(dev);

	if (dev->iobase)
		apci2200_reset(dev);
	if (dev->subdevices)
		addi_watchdog_cleanup(&dev->subdevices[2]);
	if (pcidev) {
		if (dev->iobase)
			comedi_pci_disable(pcidev);
	}
}

static struct comedi_driver apci2200_driver = {
	.driver_name	= "addi_apci_2200",
	.module		= THIS_MODULE,
	.auto_attach	= apci2200_auto_attach,
	.detach		= apci2200_detach,
};

static int apci2200_pci_probe(struct pci_dev *dev,
					const struct pci_device_id *ent)
{
	return comedi_pci_auto_config(dev, &apci2200_driver);
}

static void apci2200_pci_remove(struct pci_dev *dev)
{
	comedi_pci_auto_unconfig(dev);
}

static DEFINE_PCI_DEVICE_TABLE(apci2200_pci_table) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_ADDIDATA, 0x1005) },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, apci2200_pci_table);

static struct pci_driver apci2200_pci_driver = {
	.name		= "addi_apci_2200",
	.id_table	= apci2200_pci_table,
	.probe		= apci2200_pci_probe,
	.remove		= apci2200_pci_remove,
};
module_comedi_pci_driver(apci2200_driver, apci2200_pci_driver);

MODULE_AUTHOR("Comedi http://www.comedi.org");
MODULE_DESCRIPTION("Comedi low-level driver");
MODULE_LICENSE("GPL");
