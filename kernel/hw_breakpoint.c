/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2007 Alan Stern
 * Copyright (C) IBM Corporation, 2009
 * Copyright (C) 2009, Frederic Weisbecker <fweisbec@gmail.com>
 *
 * Thanks to Ingo Molnar for his many suggestions.
 *
 * Authors: Alan Stern <stern@rowland.harvard.edu>
 *          K.Prasad <prasad@linux.vnet.ibm.com>
 *          Frederic Weisbecker <fweisbec@gmail.com>
 */

/*
 * HW_breakpoint: a unified kernel/user-space hardware breakpoint facility,
 * using the CPU's debug registers.
 * This file contains the arch-independent routines.
 */

#include <linux/irqflags.h>
#include <linux/kallsyms.h>
#include <linux/notifier.h>
#include <linux/kprobes.h>
#include <linux/kdebug.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/smp.h>

#include <linux/hw_breakpoint.h>

/*
 * Constraints data
 */

/* Number of pinned cpu breakpoints in a cpu */
static DEFINE_PER_CPU(unsigned int, nr_cpu_bp_pinned);

/* Number of pinned task breakpoints in a cpu */
static DEFINE_PER_CPU(unsigned int, task_bp_pinned[HBP_NUM]);

/* Number of non-pinned cpu/task breakpoints in a cpu */
static DEFINE_PER_CPU(unsigned int, nr_bp_flexible);

/* Gather the number of total pinned and un-pinned bp in a cpuset */
struct bp_busy_slots {
	unsigned int pinned;
	unsigned int flexible;
};

/* Serialize accesses to the above constraints */
static DEFINE_MUTEX(nr_bp_mutex);

/*
 * Report the maximum number of pinned breakpoints a task
 * have in this cpu
 */
static unsigned int max_task_bp_pinned(int cpu)
{
	int i;
	unsigned int *tsk_pinned = per_cpu(task_bp_pinned, cpu);

	for (i = HBP_NUM -1; i >= 0; i--) {
		if (tsk_pinned[i] > 0)
			return i + 1;
	}

	return 0;
}

/*
 * Report the number of pinned/un-pinned breakpoints we have in
 * a given cpu (cpu > -1) or in all of them (cpu = -1).
 */
static void fetch_bp_busy_slots(struct bp_busy_slots *slots, int cpu)
{
	if (cpu >= 0) {
		slots->pinned = per_cpu(nr_cpu_bp_pinned, cpu);
		slots->pinned += max_task_bp_pinned(cpu);
		slots->flexible = per_cpu(nr_bp_flexible, cpu);

		return;
	}

	for_each_online_cpu(cpu) {
		unsigned int nr;

		nr = per_cpu(nr_cpu_bp_pinned, cpu);
		nr += max_task_bp_pinned(cpu);

		if (nr > slots->pinned)
			slots->pinned = nr;

		nr = per_cpu(nr_bp_flexible, cpu);

		if (nr > slots->flexible)
			slots->flexible = nr;
	}
}

/*
 * Add a pinned breakpoint for the given task in our constraint table
 */
static void toggle_bp_task_slot(struct task_struct *tsk, int cpu, bool enable)
{
	int count = 0;
	struct perf_event *bp;
	struct perf_event_context *ctx = tsk->perf_event_ctxp;
	unsigned int *tsk_pinned;
	struct list_head *list;
	unsigned long flags;

	if (WARN_ONCE(!ctx, "No perf context for this task"))
		return;

	list = &ctx->event_list;

	spin_lock_irqsave(&ctx->lock, flags);

	/*
	 * The current breakpoint counter is not included in the list
	 * at the open() callback time
	 */
	list_for_each_entry(bp, list, event_entry) {
		if (bp->attr.type == PERF_TYPE_BREAKPOINT)
			count++;
	}

	spin_unlock_irqrestore(&ctx->lock, flags);

	if (WARN_ONCE(count < 0, "No breakpoint counter found in the counter list"))
		return;

	tsk_pinned = per_cpu(task_bp_pinned, cpu);
	if (enable) {
		tsk_pinned[count]++;
		if (count > 0)
			tsk_pinned[count-1]--;
	} else {
		tsk_pinned[count]--;
		if (count > 0)
			tsk_pinned[count-1]++;
	}
}

/*
 * Add/remove the given breakpoint in our constraint table
 */
static void toggle_bp_slot(struct perf_event *bp, bool enable)
{
	int cpu = bp->cpu;
	struct task_struct *tsk = bp->ctx->task;

	/* Pinned counter task profiling */
	if (tsk) {
		if (cpu >= 0) {
			toggle_bp_task_slot(tsk, cpu, enable);
			return;
		}

		for_each_online_cpu(cpu)
			toggle_bp_task_slot(tsk, cpu, enable);
		return;
	}

	/* Pinned counter cpu profiling */
	if (enable)
		per_cpu(nr_cpu_bp_pinned, bp->cpu)++;
	else
		per_cpu(nr_cpu_bp_pinned, bp->cpu)--;
}

/*
 * Contraints to check before allowing this new breakpoint counter:
 *
 *  == Non-pinned counter == (Considered as pinned for now)
 *
 *   - If attached to a single cpu, check:
 *
 *       (per_cpu(nr_bp_flexible, cpu) || (per_cpu(nr_cpu_bp_pinned, cpu)
 *           + max(per_cpu(task_bp_pinned, cpu)))) < HBP_NUM
 *
 *       -> If there are already non-pinned counters in this cpu, it means
 *          there is already a free slot for them.
 *          Otherwise, we check that the maximum number of per task
 *          breakpoints (for this cpu) plus the number of per cpu breakpoint
 *          (for this cpu) doesn't cover every registers.
 *
 *   - If attached to every cpus, check:
 *
 *       (per_cpu(nr_bp_flexible, *) || (max(per_cpu(nr_cpu_bp_pinned, *))
 *           + max(per_cpu(task_bp_pinned, *)))) < HBP_NUM
 *
 *       -> This is roughly the same, except we check the number of per cpu
 *          bp for every cpu and we keep the max one. Same for the per tasks
 *          breakpoints.
 *
 *
 * == Pinned counter ==
 *
 *   - If attached to a single cpu, check:
 *
 *       ((per_cpu(nr_bp_flexible, cpu) > 1) + per_cpu(nr_cpu_bp_pinned, cpu)
 *            + max(per_cpu(task_bp_pinned, cpu))) < HBP_NUM
 *
 *       -> Same checks as before. But now the nr_bp_flexible, if any, must keep
 *          one register at least (or they will never be fed).
 *
 *   - If attached to every cpus, check:
 *
 *       ((per_cpu(nr_bp_flexible, *) > 1) + max(per_cpu(nr_cpu_bp_pinned, *))
 *            + max(per_cpu(task_bp_pinned, *))) < HBP_NUM
 */
int reserve_bp_slot(struct perf_event *bp)
{
	struct bp_busy_slots slots = {0};
	int ret = 0;

	mutex_lock(&nr_bp_mutex);

	fetch_bp_busy_slots(&slots, bp->cpu);

	/* Flexible counters need to keep at least one slot */
	if (slots.pinned + (!!slots.flexible) == HBP_NUM) {
		ret = -ENOSPC;
		goto end;
	}

	toggle_bp_slot(bp, true);

end:
	mutex_unlock(&nr_bp_mutex);

	return ret;
}

void release_bp_slot(struct perf_event *bp)
{
	mutex_lock(&nr_bp_mutex);

	toggle_bp_slot(bp, false);

	mutex_unlock(&nr_bp_mutex);
}


int register_perf_hw_breakpoint(struct perf_event *bp)
{
	int ret;

	ret = reserve_bp_slot(bp);
	if (ret)
		return ret;

	/*
	 * Ptrace breakpoints can be temporary perf events only
	 * meant to reserve a slot. In this case, it is created disabled and
	 * we don't want to check the params right now (as we put a null addr)
	 * But perf tools create events as disabled and we want to check
	 * the params for them.
	 * This is a quick hack that will be removed soon, once we remove
	 * the tmp breakpoints from ptrace
	 */
	if (!bp->attr.disabled || !bp->overflow_handler)
		ret = arch_validate_hwbkpt_settings(bp, bp->ctx->task);

	return ret;
}

/**
 * register_user_hw_breakpoint - register a hardware breakpoint for user space
 * @attr: breakpoint attributes
 * @triggered: callback to trigger when we hit the breakpoint
 * @tsk: pointer to 'task_struct' of the process to which the address belongs
 */
struct perf_event *
register_user_hw_breakpoint(struct perf_event_attr *attr,
			    perf_overflow_handler_t triggered,
			    struct task_struct *tsk)
{
	return perf_event_create_kernel_counter(attr, -1, tsk->pid, triggered);
}
EXPORT_SYMBOL_GPL(register_user_hw_breakpoint);

/**
 * modify_user_hw_breakpoint - modify a user-space hardware breakpoint
 * @bp: the breakpoint structure to modify
 * @attr: new breakpoint attributes
 * @triggered: callback to trigger when we hit the breakpoint
 * @tsk: pointer to 'task_struct' of the process to which the address belongs
 */
struct perf_event *
modify_user_hw_breakpoint(struct perf_event *bp, struct perf_event_attr *attr)
{
	/*
	 * FIXME: do it without unregistering
	 * - We don't want to lose our slot
	 * - If the new bp is incorrect, don't lose the older one
	 */
	unregister_hw_breakpoint(bp);

	return perf_event_create_kernel_counter(attr, -1, bp->ctx->task->pid,
						bp->overflow_handler);
}
EXPORT_SYMBOL_GPL(modify_user_hw_breakpoint);

/**
 * unregister_hw_breakpoint - unregister a user-space hardware breakpoint
 * @bp: the breakpoint structure to unregister
 */
void unregister_hw_breakpoint(struct perf_event *bp)
{
	if (!bp)
		return;
	perf_event_release_kernel(bp);
}
EXPORT_SYMBOL_GPL(unregister_hw_breakpoint);

/**
 * register_wide_hw_breakpoint - register a wide breakpoint in the kernel
 * @attr: breakpoint attributes
 * @triggered: callback to trigger when we hit the breakpoint
 *
 * @return a set of per_cpu pointers to perf events
 */
struct perf_event **
register_wide_hw_breakpoint(struct perf_event_attr *attr,
			    perf_overflow_handler_t triggered)
{
	struct perf_event **cpu_events, **pevent, *bp;
	long err;
	int cpu;

	cpu_events = alloc_percpu(typeof(*cpu_events));
	if (!cpu_events)
		return ERR_PTR(-ENOMEM);

	for_each_possible_cpu(cpu) {
		pevent = per_cpu_ptr(cpu_events, cpu);
		bp = perf_event_create_kernel_counter(attr, cpu, -1, triggered);

		*pevent = bp;

		if (IS_ERR(bp)) {
			err = PTR_ERR(bp);
			goto fail;
		}
	}

	return cpu_events;

fail:
	for_each_possible_cpu(cpu) {
		pevent = per_cpu_ptr(cpu_events, cpu);
		if (IS_ERR(*pevent))
			break;
		unregister_hw_breakpoint(*pevent);
	}
	free_percpu(cpu_events);
	/* return the error if any */
	return ERR_PTR(err);
}
EXPORT_SYMBOL_GPL(register_wide_hw_breakpoint);

/**
 * unregister_wide_hw_breakpoint - unregister a wide breakpoint in the kernel
 * @cpu_events: the per cpu set of events to unregister
 */
void unregister_wide_hw_breakpoint(struct perf_event **cpu_events)
{
	int cpu;
	struct perf_event **pevent;

	for_each_possible_cpu(cpu) {
		pevent = per_cpu_ptr(cpu_events, cpu);
		unregister_hw_breakpoint(*pevent);
	}
	free_percpu(cpu_events);
}
EXPORT_SYMBOL_GPL(unregister_wide_hw_breakpoint);

static struct notifier_block hw_breakpoint_exceptions_nb = {
	.notifier_call = hw_breakpoint_exceptions_notify,
	/* we need to be notified first */
	.priority = 0x7fffffff
};

static int __init init_hw_breakpoint(void)
{
	return register_die_notifier(&hw_breakpoint_exceptions_nb);
}
core_initcall(init_hw_breakpoint);


struct pmu perf_ops_bp = {
	.enable		= arch_install_hw_breakpoint,
	.disable	= arch_uninstall_hw_breakpoint,
	.read		= hw_breakpoint_pmu_read,
	.unthrottle	= hw_breakpoint_pmu_unthrottle
};
