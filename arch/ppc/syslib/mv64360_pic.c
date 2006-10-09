/*
 * Interrupt controller support for Marvell's MV64360.
 *
 * Author: Rabeeh Khoury <rabeeh@galileo.co.il>
 * Based on MV64360 PIC written by
 * Chris Zankel <chris@mvista.com>
 * Mark A. Greer <mgreer@mvista.com>
 *
 * Copyright 2004 MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/*
 * This file contains the specific functions to support the MV64360
 * interrupt controller.
 *
 * The MV64360 has two main interrupt registers (high and low) that
 * summarizes the interrupts generated by the units of the MV64360.
 * Each bit is assigned to an interrupt number, where the low register
 * are assigned from IRQ0 to IRQ31 and the high cause register
 * from IRQ32 to IRQ63
 * The GPP (General Purpose Pins) interrupts are assigned from IRQ64 (GPP0)
 * to IRQ95 (GPP31).
 * get_irq() returns the lowest interrupt number that is currently asserted.
 *
 * Note:
 *  - This driver does not initialize the GPP when used as an interrupt
 *    input.
 */

#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/stddef.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/processor.h>
#include <asm/system.h>
#include <asm/irq.h>
#include <asm/mv64x60.h>
#include <asm/machdep.h>

#ifdef CONFIG_IRQ_ALL_CPUS
#error "The mv64360 does not support distribution of IRQs on all CPUs"
#endif
/* ========================== forward declaration ========================== */

static void mv64360_unmask_irq(unsigned int);
static void mv64360_mask_irq(unsigned int);
static irqreturn_t mv64360_cpu_error_int_handler(int, void *, struct pt_regs *);
static irqreturn_t mv64360_sram_error_int_handler(int, void *,
						  struct pt_regs *);
static irqreturn_t mv64360_pci_error_int_handler(int, void *, struct pt_regs *);

/* ========================== local declarations =========================== */

struct hw_interrupt_type mv64360_pic = {
	.typename = " mv64360  ",
	.enable   = mv64360_unmask_irq,
	.disable  = mv64360_mask_irq,
	.ack      = mv64360_mask_irq,
	.end      = mv64360_unmask_irq,
};

#define CPU_INTR_STR	"mv64360 cpu interface error"
#define SRAM_INTR_STR	"mv64360 internal sram error"
#define PCI0_INTR_STR	"mv64360 pci 0 error"
#define PCI1_INTR_STR	"mv64360 pci 1 error"

static struct mv64x60_handle bh;

u32 mv64360_irq_base = 0;	/* MV64360 handles the next 96 IRQs from here */

/* mv64360_init_irq()
 *
 * This function initializes the interrupt controller. It assigns
 * all interrupts from IRQ0 to IRQ95 to the mv64360 interrupt controller.
 *
 * Input Variable(s):
 *  None.
 *
 * Outpu. Variable(s):
 *  None.
 *
 * Returns:
 *  void
 *
 * Note:
 *  We register all GPP inputs as interrupt source, but disable them.
 */
void __init
mv64360_init_irq(void)
{
	int i;

	if (ppc_md.progress)
		ppc_md.progress("mv64360_init_irq: enter", 0x0);

	bh.v_base = mv64x60_get_bridge_vbase();

	ppc_cached_irq_mask[0] = 0;
	ppc_cached_irq_mask[1] = 0x0f000000;	/* Enable GPP intrs */
	ppc_cached_irq_mask[2] = 0;

	/* disable all interrupts and clear current interrupts */
	mv64x60_write(&bh, MV64x60_GPP_INTR_CAUSE, 0);
	mv64x60_write(&bh, MV64x60_GPP_INTR_MASK, ppc_cached_irq_mask[2]);
	mv64x60_write(&bh, MV64360_IC_CPU0_INTR_MASK_LO,ppc_cached_irq_mask[0]);
	mv64x60_write(&bh, MV64360_IC_CPU0_INTR_MASK_HI,ppc_cached_irq_mask[1]);

	/* All interrupts are level interrupts */
	for (i = mv64360_irq_base; i < (mv64360_irq_base + 96); i++) {
		irq_desc[i].status |= IRQ_LEVEL;
		irq_desc[i].chip = &mv64360_pic;
	}

	if (ppc_md.progress)
		ppc_md.progress("mv64360_init_irq: exit", 0x0);
}

/* mv64360_get_irq()
 *
 * This function returns the lowest interrupt number of all interrupts that
 * are currently asserted.
 *
 * Input Variable(s):
 *  struct pt_regs*	not used
 *
 * Output Variable(s):
 *  None.
 *
 * Returns:
 *  int	<interrupt number> or -2 (bogus interrupt)
 *
 */
int
mv64360_get_irq(struct pt_regs *regs)
{
	int irq;
	int irq_gpp;

#ifdef CONFIG_SMP
	/*
	 * Second CPU gets only doorbell (message) interrupts.
	 * The doorbell interrupt is BIT28 in the main interrupt low cause reg.
	 */
	int cpu_nr = smp_processor_id();
	if (cpu_nr == 1) {
		if (!(mv64x60_read(&bh, MV64360_IC_MAIN_CAUSE_LO) &
		      (1 << MV64x60_IRQ_DOORBELL)))
			return -1;
		return mv64360_irq_base + MV64x60_IRQ_DOORBELL;
	}
#endif

	irq = mv64x60_read(&bh, MV64360_IC_MAIN_CAUSE_LO);
	irq = __ilog2((irq & 0x3dfffffe) & ppc_cached_irq_mask[0]);

	if (irq == -1) {
		irq = mv64x60_read(&bh, MV64360_IC_MAIN_CAUSE_HI);
		irq = __ilog2((irq & 0x1f0003f7) & ppc_cached_irq_mask[1]);

		if (irq == -1)
			irq = -2; /* bogus interrupt, should never happen */
		else {
			if ((irq >= 24) && (irq < MV64x60_IRQ_DOORBELL)) {
				irq_gpp = mv64x60_read(&bh,
					MV64x60_GPP_INTR_CAUSE);
				irq_gpp = __ilog2(irq_gpp &
					ppc_cached_irq_mask[2]);

				if (irq_gpp == -1)
					irq = -2;
				else {
					irq = irq_gpp + 64;
					mv64x60_write(&bh,
						MV64x60_GPP_INTR_CAUSE,
						~(1 << (irq - 64)));
				}
			}
			else
				irq += 32;
		}
	}

	(void)mv64x60_read(&bh, MV64x60_GPP_INTR_CAUSE);

	if (irq < 0)
		return (irq);
	else
		return (mv64360_irq_base + irq);
}

/* mv64360_unmask_irq()
 *
 * This function enables an interrupt.
 *
 * Input Variable(s):
 *  unsigned int	interrupt number (IRQ0...IRQ95).
 *
 * Output Variable(s):
 *  None.
 *
 * Returns:
 *  void
 */
static void
mv64360_unmask_irq(unsigned int irq)
{
#ifdef CONFIG_SMP
	/* second CPU gets only doorbell interrupts */
	if ((irq - mv64360_irq_base) == MV64x60_IRQ_DOORBELL) {
		mv64x60_set_bits(&bh, MV64360_IC_CPU1_INTR_MASK_LO,
				 (1 << MV64x60_IRQ_DOORBELL));
		return;
	}
#endif
	irq -= mv64360_irq_base;

	if (irq > 31) {
		if (irq > 63) /* unmask GPP irq */
			mv64x60_write(&bh, MV64x60_GPP_INTR_MASK,
				ppc_cached_irq_mask[2] |= (1 << (irq - 64)));
		else /* mask high interrupt register */
			mv64x60_write(&bh, MV64360_IC_CPU0_INTR_MASK_HI,
				ppc_cached_irq_mask[1] |= (1 << (irq - 32)));
	}
	else /* mask low interrupt register */
		mv64x60_write(&bh, MV64360_IC_CPU0_INTR_MASK_LO,
			ppc_cached_irq_mask[0] |= (1 << irq));

	(void)mv64x60_read(&bh, MV64x60_GPP_INTR_MASK);
	return;
}

/* mv64360_mask_irq()
 *
 * This function disables the requested interrupt.
 *
 * Input Variable(s):
 *  unsigned int	interrupt number (IRQ0...IRQ95).
 *
 * Output Variable(s):
 *  None.
 *
 * Returns:
 *  void
 */
static void
mv64360_mask_irq(unsigned int irq)
{
#ifdef CONFIG_SMP
	if ((irq - mv64360_irq_base) == MV64x60_IRQ_DOORBELL) {
		mv64x60_clr_bits(&bh, MV64360_IC_CPU1_INTR_MASK_LO,
				 (1 << MV64x60_IRQ_DOORBELL));
		return;
	}
#endif
	irq -= mv64360_irq_base;

	if (irq > 31) {
		if (irq > 63) /* mask GPP irq */
			mv64x60_write(&bh, MV64x60_GPP_INTR_MASK,
				ppc_cached_irq_mask[2] &= ~(1 << (irq - 64)));
		else /* mask high interrupt register */
			mv64x60_write(&bh, MV64360_IC_CPU0_INTR_MASK_HI,
				ppc_cached_irq_mask[1] &= ~(1 << (irq - 32)));
	}
	else /* mask low interrupt register */
		mv64x60_write(&bh, MV64360_IC_CPU0_INTR_MASK_LO,
			ppc_cached_irq_mask[0] &= ~(1 << irq));

	(void)mv64x60_read(&bh, MV64x60_GPP_INTR_MASK);
	return;
}

static irqreturn_t
mv64360_cpu_error_int_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	printk(KERN_ERR "mv64360_cpu_error_int_handler: %s 0x%08x\n",
		"Error on CPU interface - Cause regiser",
		mv64x60_read(&bh, MV64x60_CPU_ERR_CAUSE));
	printk(KERN_ERR "\tCPU error register dump:\n");
	printk(KERN_ERR "\tAddress low  0x%08x\n",
	       mv64x60_read(&bh, MV64x60_CPU_ERR_ADDR_LO));
	printk(KERN_ERR "\tAddress high 0x%08x\n",
	       mv64x60_read(&bh, MV64x60_CPU_ERR_ADDR_HI));
	printk(KERN_ERR "\tData low     0x%08x\n",
	       mv64x60_read(&bh, MV64x60_CPU_ERR_DATA_LO));
	printk(KERN_ERR "\tData high    0x%08x\n",
	       mv64x60_read(&bh, MV64x60_CPU_ERR_DATA_HI));
	printk(KERN_ERR "\tParity       0x%08x\n",
	       mv64x60_read(&bh, MV64x60_CPU_ERR_PARITY));
	mv64x60_write(&bh, MV64x60_CPU_ERR_CAUSE, 0);
	return IRQ_HANDLED;
}

static irqreturn_t
mv64360_sram_error_int_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	printk(KERN_ERR "mv64360_sram_error_int_handler: %s 0x%08x\n",
		"Error in internal SRAM - Cause register",
		mv64x60_read(&bh, MV64360_SRAM_ERR_CAUSE));
	printk(KERN_ERR "\tSRAM error register dump:\n");
	printk(KERN_ERR "\tAddress Low  0x%08x\n",
	       mv64x60_read(&bh, MV64360_SRAM_ERR_ADDR_LO));
	printk(KERN_ERR "\tAddress High 0x%08x\n",
	       mv64x60_read(&bh, MV64360_SRAM_ERR_ADDR_HI));
	printk(KERN_ERR "\tData Low     0x%08x\n",
	       mv64x60_read(&bh, MV64360_SRAM_ERR_DATA_LO));
	printk(KERN_ERR "\tData High    0x%08x\n",
	       mv64x60_read(&bh, MV64360_SRAM_ERR_DATA_HI));
	printk(KERN_ERR "\tParity       0x%08x\n",
		mv64x60_read(&bh, MV64360_SRAM_ERR_PARITY));
	mv64x60_write(&bh, MV64360_SRAM_ERR_CAUSE, 0);
	return IRQ_HANDLED;
}

static irqreturn_t
mv64360_pci_error_int_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	u32 val;
	unsigned int pci_bus = (unsigned int)dev_id;

	if (pci_bus == 0) {	/* Error on PCI 0 */
		val = mv64x60_read(&bh, MV64x60_PCI0_ERR_CAUSE);
		printk(KERN_ERR "%s: Error in PCI %d Interface\n",
			"mv64360_pci_error_int_handler", pci_bus);
		printk(KERN_ERR "\tPCI %d error register dump:\n", pci_bus);
		printk(KERN_ERR "\tCause register 0x%08x\n", val);
		printk(KERN_ERR "\tAddress Low    0x%08x\n",
		       mv64x60_read(&bh, MV64x60_PCI0_ERR_ADDR_LO));
		printk(KERN_ERR "\tAddress High   0x%08x\n",
		       mv64x60_read(&bh, MV64x60_PCI0_ERR_ADDR_HI));
		printk(KERN_ERR "\tAttribute      0x%08x\n",
		       mv64x60_read(&bh, MV64x60_PCI0_ERR_DATA_LO));
		printk(KERN_ERR "\tCommand        0x%08x\n",
		       mv64x60_read(&bh, MV64x60_PCI0_ERR_CMD));
		mv64x60_write(&bh, MV64x60_PCI0_ERR_CAUSE, ~val);
	}
	if (pci_bus == 1) {	/* Error on PCI 1 */
		val = mv64x60_read(&bh, MV64x60_PCI1_ERR_CAUSE);
		printk(KERN_ERR "%s: Error in PCI %d Interface\n",
			"mv64360_pci_error_int_handler", pci_bus);
		printk(KERN_ERR "\tPCI %d error register dump:\n", pci_bus);
		printk(KERN_ERR "\tCause register 0x%08x\n", val);
		printk(KERN_ERR "\tAddress Low    0x%08x\n",
		       mv64x60_read(&bh, MV64x60_PCI1_ERR_ADDR_LO));
		printk(KERN_ERR "\tAddress High   0x%08x\n",
		       mv64x60_read(&bh, MV64x60_PCI1_ERR_ADDR_HI));
		printk(KERN_ERR "\tAttribute      0x%08x\n",
		       mv64x60_read(&bh, MV64x60_PCI1_ERR_DATA_LO));
		printk(KERN_ERR "\tCommand        0x%08x\n",
		       mv64x60_read(&bh, MV64x60_PCI1_ERR_CMD));
		mv64x60_write(&bh, MV64x60_PCI1_ERR_CAUSE, ~val);
	}
	return IRQ_HANDLED;
}

/*
 * Bit 0 of MV64x60_PCIx_ERR_MASK does not exist on the 64360 and because of
 * errata FEr-#11 and FEr-##16 for the 64460, it should be 0 on that chip as
 * well.  IOW, don't set bit 0.
 */
#define MV64360_PCI0_ERR_MASK_VAL	0x00a50c24

static int __init
mv64360_register_hdlrs(void)
{
	int	rc;

	/* Clear old errors and register CPU interface error intr handler */
	mv64x60_write(&bh, MV64x60_CPU_ERR_CAUSE, 0);
	if ((rc = request_irq(MV64x60_IRQ_CPU_ERR + mv64360_irq_base,
		mv64360_cpu_error_int_handler, IRQF_DISABLED, CPU_INTR_STR, NULL)))
		printk(KERN_WARNING "Can't register cpu error handler: %d", rc);

	mv64x60_write(&bh, MV64x60_CPU_ERR_MASK, 0);
	mv64x60_write(&bh, MV64x60_CPU_ERR_MASK, 0x000000ff);

	/* Clear old errors and register internal SRAM error intr handler */
	mv64x60_write(&bh, MV64360_SRAM_ERR_CAUSE, 0);
	if ((rc = request_irq(MV64360_IRQ_SRAM_PAR_ERR + mv64360_irq_base,
		mv64360_sram_error_int_handler,IRQF_DISABLED,SRAM_INTR_STR, NULL)))
		printk(KERN_WARNING "Can't register SRAM error handler: %d",rc);

	/* Clear old errors and register PCI 0 error intr handler */
	mv64x60_write(&bh, MV64x60_PCI0_ERR_CAUSE, 0);
	if ((rc = request_irq(MV64360_IRQ_PCI0 + mv64360_irq_base,
			mv64360_pci_error_int_handler,
			IRQF_DISABLED, PCI0_INTR_STR, (void *)0)))
		printk(KERN_WARNING "Can't register pci 0 error handler: %d",
			rc);

	mv64x60_write(&bh, MV64x60_PCI0_ERR_MASK, 0);
	mv64x60_write(&bh, MV64x60_PCI0_ERR_MASK, MV64360_PCI0_ERR_MASK_VAL);

	/* Erratum FEr PCI-#16 says to clear bit 0 of PCI SERRn Mask reg. */
	mv64x60_write(&bh, MV64x60_PCI0_ERR_SERR_MASK,
		mv64x60_read(&bh, MV64x60_PCI0_ERR_SERR_MASK) & ~0x1UL);

	/* Clear old errors and register PCI 1 error intr handler */
	mv64x60_write(&bh, MV64x60_PCI1_ERR_CAUSE, 0);
	if ((rc = request_irq(MV64360_IRQ_PCI1 + mv64360_irq_base,
			mv64360_pci_error_int_handler,
			IRQF_DISABLED, PCI1_INTR_STR, (void *)1)))
		printk(KERN_WARNING "Can't register pci 1 error handler: %d",
			rc);

	mv64x60_write(&bh, MV64x60_PCI1_ERR_MASK, 0);
	mv64x60_write(&bh, MV64x60_PCI1_ERR_MASK, MV64360_PCI0_ERR_MASK_VAL);

	/* Erratum FEr PCI-#16 says to clear bit 0 of PCI Intr Mask reg. */
	mv64x60_write(&bh, MV64x60_PCI1_ERR_SERR_MASK,
		mv64x60_read(&bh, MV64x60_PCI1_ERR_SERR_MASK) & ~0x1UL);

	return 0;
}

arch_initcall(mv64360_register_hdlrs);
