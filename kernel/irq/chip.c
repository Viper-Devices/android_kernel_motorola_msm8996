/*
 * linux/kernel/irq/chip.c
 *
 * Copyright (C) 1992, 1998-2006 Linus Torvalds, Ingo Molnar
 * Copyright (C) 2005-2006, Thomas Gleixner, Russell King
 *
 * This file contains the core interrupt handling code, for irq-chip
 * based architectures.
 *
 * Detailed information is available in Documentation/DocBook/genericirq
 */

#include <linux/irq.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>

#include "internals.h"

/**
 *	set_irq_chip - set the irq chip for an irq
 *	@irq:	irq number
 *	@chip:	pointer to irq chip description structure
 */
int set_irq_chip(unsigned int irq, struct irq_chip *chip)
{
	struct irq_desc *desc;
	unsigned long flags;

	if (irq >= NR_IRQS) {
		printk(KERN_ERR "Trying to install chip for IRQ%d\n", irq);
		WARN_ON(1);
		return -EINVAL;
	}

	if (!chip)
		chip = &no_irq_chip;

	desc = irq_desc + irq;
	spin_lock_irqsave(&desc->lock, flags);
	irq_chip_set_defaults(chip);
	desc->chip = chip;
	/*
	 * For compatibility only:
	 */
	desc->chip = chip;
	spin_unlock_irqrestore(&desc->lock, flags);

	return 0;
}
EXPORT_SYMBOL(set_irq_chip);

/**
 *	set_irq_type - set the irq type for an irq
 *	@irq:	irq number
 *	@type:	interrupt type - see include/linux/interrupt.h
 */
int set_irq_type(unsigned int irq, unsigned int type)
{
	struct irq_desc *desc;
	unsigned long flags;
	int ret = -ENXIO;

	if (irq >= NR_IRQS) {
		printk(KERN_ERR "Trying to set irq type for IRQ%d\n", irq);
		return -ENODEV;
	}

	desc = irq_desc + irq;
	if (desc->chip->set_type) {
		spin_lock_irqsave(&desc->lock, flags);
		ret = desc->chip->set_type(irq, type);
		spin_unlock_irqrestore(&desc->lock, flags);
	}
	return ret;
}
EXPORT_SYMBOL(set_irq_type);

/**
 *	set_irq_data - set irq type data for an irq
 *	@irq:	Interrupt number
 *	@data:	Pointer to interrupt specific data
 *
 *	Set the hardware irq controller data for an irq
 */
int set_irq_data(unsigned int irq, void *data)
{
	struct irq_desc *desc;
	unsigned long flags;

	if (irq >= NR_IRQS) {
		printk(KERN_ERR
		       "Trying to install controller data for IRQ%d\n", irq);
		return -EINVAL;
	}

	desc = irq_desc + irq;
	spin_lock_irqsave(&desc->lock, flags);
	desc->handler_data = data;
	spin_unlock_irqrestore(&desc->lock, flags);
	return 0;
}
EXPORT_SYMBOL(set_irq_data);

/**
 *	set_irq_chip_data - set irq chip data for an irq
 *	@irq:	Interrupt number
 *	@data:	Pointer to chip specific data
 *
 *	Set the hardware irq chip data for an irq
 */
int set_irq_chip_data(unsigned int irq, void *data)
{
	struct irq_desc *desc = irq_desc + irq;
	unsigned long flags;

	if (irq >= NR_IRQS || !desc->chip) {
		printk(KERN_ERR "BUG: bad set_irq_chip_data(IRQ#%d)\n", irq);
		return -EINVAL;
	}

	spin_lock_irqsave(&desc->lock, flags);
	desc->chip_data = data;
	spin_unlock_irqrestore(&desc->lock, flags);

	return 0;
}
EXPORT_SYMBOL(set_irq_chip_data);

/*
 * default enable function
 */
static void default_enable(unsigned int irq)
{
	struct irq_desc *desc = irq_desc + irq;

	desc->chip->unmask(irq);
	desc->status &= ~IRQ_MASKED;
}

/*
 * default disable function
 */
static void default_disable(unsigned int irq)
{
	struct irq_desc *desc = irq_desc + irq;

	if (!(desc->status & IRQ_DELAYED_DISABLE))
		irq_desc[irq].chip->mask(irq);
}

/*
 * default startup function
 */
static unsigned int default_startup(unsigned int irq)
{
	irq_desc[irq].chip->enable(irq);

	return 0;
}

/*
 * Fixup enable/disable function pointers
 */
void irq_chip_set_defaults(struct irq_chip *chip)
{
	if (!chip->enable)
		chip->enable = default_enable;
	if (!chip->disable)
		chip->disable = default_disable;
	if (!chip->startup)
		chip->startup = default_startup;
	if (!chip->shutdown)
		chip->shutdown = chip->disable;
	if (!chip->name)
		chip->name = chip->typename;
}

static inline void mask_ack_irq(struct irq_desc *desc, int irq)
{
	if (desc->chip->mask_ack)
		desc->chip->mask_ack(irq);
	else {
		desc->chip->mask(irq);
		desc->chip->ack(irq);
	}
}

/**
 *	handle_simple_irq - Simple and software-decoded IRQs.
 *	@irq:	the interrupt number
 *	@desc:	the interrupt description structure for this irq
 *	@regs:	pointer to a register structure
 *
 *	Simple interrupts are either sent from a demultiplexing interrupt
 *	handler or come from hardware, where no interrupt hardware control
 *	is necessary.
 *
 *	Note: The caller is expected to handle the ack, clear, mask and
 *	unmask issues if necessary.
 */
void fastcall
handle_simple_irq(unsigned int irq, struct irq_desc *desc, struct pt_regs *regs)
{
	struct irqaction *action;
	irqreturn_t action_ret;
	const unsigned int cpu = smp_processor_id();

	spin_lock(&desc->lock);

	if (unlikely(desc->status & IRQ_INPROGRESS))
		goto out_unlock;
	desc->status &= ~(IRQ_REPLAY | IRQ_WAITING);
	kstat_cpu(cpu).irqs[irq]++;

	action = desc->action;
	if (unlikely(!action || (desc->status & IRQ_DISABLED)))
		goto out_unlock;

	desc->status |= IRQ_INPROGRESS;
	spin_unlock(&desc->lock);

	action_ret = handle_IRQ_event(irq, regs, action);
	if (!noirqdebug)
		note_interrupt(irq, desc, action_ret, regs);

	spin_lock(&desc->lock);
	desc->status &= ~IRQ_INPROGRESS;
out_unlock:
	spin_unlock(&desc->lock);
}

/**
 *	handle_level_irq - Level type irq handler
 *	@irq:	the interrupt number
 *	@desc:	the interrupt description structure for this irq
 *	@regs:	pointer to a register structure
 *
 *	Level type interrupts are active as long as the hardware line has
 *	the active level. This may require to mask the interrupt and unmask
 *	it after the associated handler has acknowledged the device, so the
 *	interrupt line is back to inactive.
 */
void fastcall
handle_level_irq(unsigned int irq, struct irq_desc *desc, struct pt_regs *regs)
{
	unsigned int cpu = smp_processor_id();
	struct irqaction *action;
	irqreturn_t action_ret;

	spin_lock(&desc->lock);
	mask_ack_irq(desc, irq);

	if (unlikely(desc->status & IRQ_INPROGRESS))
		goto out_unlock;
	desc->status &= ~(IRQ_REPLAY | IRQ_WAITING);
	kstat_cpu(cpu).irqs[irq]++;

	/*
	 * If its disabled or no action available
	 * keep it masked and get out of here
	 */
	action = desc->action;
	if (unlikely(!action || (desc->status & IRQ_DISABLED))) {
		desc->status |= IRQ_PENDING;
		goto out_unlock;
	}

	desc->status |= IRQ_INPROGRESS;
	desc->status &= ~IRQ_PENDING;
	spin_unlock(&desc->lock);

	action_ret = handle_IRQ_event(irq, regs, action);
	if (!noirqdebug)
		note_interrupt(irq, desc, action_ret, regs);

	spin_lock(&desc->lock);
	desc->status &= ~IRQ_INPROGRESS;
	if (!(desc->status & IRQ_DISABLED) && desc->chip->unmask)
		desc->chip->unmask(irq);
out_unlock:
	spin_unlock(&desc->lock);
}

/**
 *	handle_fasteoi_irq - irq handler for transparent controllers
 *	@irq:	the interrupt number
 *	@desc:	the interrupt description structure for this irq
 *	@regs:	pointer to a register structure
 *
 *	Only a single callback will be issued to the chip: an ->eoi()
 *	call when the interrupt has been serviced. This enables support
 *	for modern forms of interrupt handlers, which handle the flow
 *	details in hardware, transparently.
 */
void fastcall
handle_fasteoi_irq(unsigned int irq, struct irq_desc *desc,
		   struct pt_regs *regs)
{
	unsigned int cpu = smp_processor_id();
	struct irqaction *action;
	irqreturn_t action_ret;

	spin_lock(&desc->lock);

	if (unlikely(desc->status & IRQ_INPROGRESS))
		goto out;

	desc->status &= ~(IRQ_REPLAY | IRQ_WAITING);
	kstat_cpu(cpu).irqs[irq]++;

	/*
	 * If its disabled or no action available
	 * keep it masked and get out of here
	 */
	action = desc->action;
	if (unlikely(!action || (desc->status & IRQ_DISABLED))) {
		desc->status |= IRQ_PENDING;
		goto out;
	}

	desc->status |= IRQ_INPROGRESS;
	desc->status &= ~IRQ_PENDING;
	spin_unlock(&desc->lock);

	action_ret = handle_IRQ_event(irq, regs, action);
	if (!noirqdebug)
		note_interrupt(irq, desc, action_ret, regs);

	spin_lock(&desc->lock);
	desc->status &= ~IRQ_INPROGRESS;
out:
	desc->chip->eoi(irq);

	spin_unlock(&desc->lock);
}

/**
 *	handle_edge_irq - edge type IRQ handler
 *	@irq:	the interrupt number
 *	@desc:	the interrupt description structure for this irq
 *	@regs:	pointer to a register structure
 *
 *	Interrupt occures on the falling and/or rising edge of a hardware
 *	signal. The occurence is latched into the irq controller hardware
 *	and must be acked in order to be reenabled. After the ack another
 *	interrupt can happen on the same source even before the first one
 *	is handled by the assosiacted event handler. If this happens it
 *	might be necessary to disable (mask) the interrupt depending on the
 *	controller hardware. This requires to reenable the interrupt inside
 *	of the loop which handles the interrupts which have arrived while
 *	the handler was running. If all pending interrupts are handled, the
 *	loop is left.
 */
void fastcall
handle_edge_irq(unsigned int irq, struct irq_desc *desc, struct pt_regs *regs)
{
	const unsigned int cpu = smp_processor_id();

	spin_lock(&desc->lock);

	desc->status &= ~(IRQ_REPLAY | IRQ_WAITING);

	/*
	 * If we're currently running this IRQ, or its disabled,
	 * we shouldn't process the IRQ. Mark it pending, handle
	 * the necessary masking and go out
	 */
	if (unlikely((desc->status & (IRQ_INPROGRESS | IRQ_DISABLED)) ||
		    !desc->action)) {
		desc->status |= (IRQ_PENDING | IRQ_MASKED);
		mask_ack_irq(desc, irq);
		goto out_unlock;
	}

	kstat_cpu(cpu).irqs[irq]++;

	/* Start handling the irq */
	desc->chip->ack(irq);

	/* Mark the IRQ currently in progress.*/
	desc->status |= IRQ_INPROGRESS;

	do {
		struct irqaction *action = desc->action;
		irqreturn_t action_ret;

		if (unlikely(!action)) {
			desc->chip->mask(irq);
			goto out_unlock;
		}

		/*
		 * When another irq arrived while we were handling
		 * one, we could have masked the irq.
		 * Renable it, if it was not disabled in meantime.
		 */
		if (unlikely((desc->status &
			       (IRQ_PENDING | IRQ_MASKED | IRQ_DISABLED)) ==
			      (IRQ_PENDING | IRQ_MASKED))) {
			desc->chip->unmask(irq);
			desc->status &= ~IRQ_MASKED;
		}

		desc->status &= ~IRQ_PENDING;
		spin_unlock(&desc->lock);
		action_ret = handle_IRQ_event(irq, regs, action);
		if (!noirqdebug)
			note_interrupt(irq, desc, action_ret, regs);
		spin_lock(&desc->lock);

	} while ((desc->status & (IRQ_PENDING | IRQ_DISABLED)) == IRQ_PENDING);

	desc->status &= ~IRQ_INPROGRESS;
out_unlock:
	spin_unlock(&desc->lock);
}

#ifdef CONFIG_SMP
/**
 *	handle_percpu_IRQ - Per CPU local irq handler
 *	@irq:	the interrupt number
 *	@desc:	the interrupt description structure for this irq
 *	@regs:	pointer to a register structure
 *
 *	Per CPU interrupts on SMP machines without locking requirements
 */
void fastcall
handle_percpu_irq(unsigned int irq, struct irq_desc *desc, struct pt_regs *regs)
{
	irqreturn_t action_ret;

	kstat_this_cpu.irqs[irq]++;

	if (desc->chip->ack)
		desc->chip->ack(irq);

	action_ret = handle_IRQ_event(irq, regs, desc->action);
	if (!noirqdebug)
		note_interrupt(irq, desc, action_ret, regs);

	if (desc->chip->eoi)
		desc->chip->eoi(irq);
}

#endif /* CONFIG_SMP */

void
__set_irq_handler(unsigned int irq,
		  void fastcall (*handle)(unsigned int, irq_desc_t *,
					  struct pt_regs *),
		  int is_chained)
{
	struct irq_desc *desc;
	unsigned long flags;

	if (irq >= NR_IRQS) {
		printk(KERN_ERR
		       "Trying to install type control for IRQ%d\n", irq);
		return;
	}

	desc = irq_desc + irq;

	if (!handle)
		handle = handle_bad_irq;

	if (desc->chip == &no_irq_chip) {
		printk(KERN_WARNING "Trying to install %sinterrupt handler "
		       "for IRQ%d\n", is_chained ? "chained " : " ", irq);
		/*
		 * Some ARM implementations install a handler for really dumb
		 * interrupt hardware without setting an irq_chip. This worked
		 * with the ARM no_irq_chip but the check in setup_irq would
		 * prevent us to setup the interrupt at all. Switch it to
		 * dummy_irq_chip for easy transition.
		 */
		desc->chip = &dummy_irq_chip;
	}

	spin_lock_irqsave(&desc->lock, flags);

	/* Uninstall? */
	if (handle == handle_bad_irq) {
		if (desc->chip != &no_irq_chip) {
			desc->chip->mask(irq);
			desc->chip->ack(irq);
		}
		desc->status |= IRQ_DISABLED;
		desc->depth = 1;
	}
	desc->handle_irq = handle;

	if (handle != handle_bad_irq && is_chained) {
		desc->status &= ~IRQ_DISABLED;
		desc->status |= IRQ_NOREQUEST | IRQ_NOPROBE;
		desc->depth = 0;
		desc->chip->unmask(irq);
	}
	spin_unlock_irqrestore(&desc->lock, flags);
}

void
set_irq_chip_and_handler(unsigned int irq, struct irq_chip *chip,
			 void fastcall (*handle)(unsigned int,
						 struct irq_desc *,
						 struct pt_regs *))
{
	set_irq_chip(irq, chip);
	__set_irq_handler(irq, handle, 0);
}

/*
 * Get a descriptive string for the highlevel handler, for
 * /proc/interrupts output:
 */
const char *
handle_irq_name(void fastcall (*handle)(unsigned int, struct irq_desc *,
					struct pt_regs *))
{
	if (handle == handle_level_irq)
		return "level  ";
	if (handle == handle_fasteoi_irq)
		return "fasteoi";
	if (handle == handle_edge_irq)
		return "edge   ";
	if (handle == handle_simple_irq)
		return "simple ";
#ifdef CONFIG_SMP
	if (handle == handle_percpu_irq)
		return "percpu ";
#endif
	if (handle == handle_bad_irq)
		return "bad    ";

	return NULL;
}
