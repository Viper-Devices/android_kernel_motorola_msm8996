/*
 * kernel/freezer.c - Function to freeze a process
 *
 * Originally from kernel/power/process.c
 */

#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/export.h>
#include <linux/syscalls.h>
#include <linux/freezer.h>
#include <linux/kthread.h>

/* protects freezing and frozen transitions */
static DEFINE_SPINLOCK(freezer_lock);

/* Refrigerator is place where frozen processes are stored :-). */
bool __refrigerator(bool check_kthr_stop)
{
	/* Hmm, should we be allowed to suspend when there are realtime
	   processes around? */
	bool was_frozen = false;
	long save;

	spin_lock_irq(&freezer_lock);
	if (!freezing(current)) {
		spin_unlock_irq(&freezer_lock);
		return was_frozen;
	}
	if (!(current->flags & PF_NOFREEZE))
		current->flags |= PF_FROZEN;
	clear_freeze_flag(current);
	spin_unlock_irq(&freezer_lock);

	save = current->state;
	pr_debug("%s entered refrigerator\n", current->comm);

	spin_lock_irq(&current->sighand->siglock);
	recalc_sigpending(); /* We sent fake signal, clean it up */
	spin_unlock_irq(&current->sighand->siglock);

	/* prevent accounting of that task to load */
	current->flags |= PF_FREEZING;

	for (;;) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (!frozen(current) ||
		    (check_kthr_stop && kthread_should_stop()))
			break;
		was_frozen = true;
		schedule();
	}

	/* Remove the accounting blocker */
	current->flags &= ~PF_FREEZING;

	pr_debug("%s left refrigerator\n", current->comm);

	/*
	 * Restore saved task state before returning.  The mb'd version
	 * needs to be used; otherwise, it might silently break
	 * synchronization which depends on ordered task state change.
	 */
	set_current_state(save);

	return was_frozen;
}
EXPORT_SYMBOL(__refrigerator);

static void fake_signal_wake_up(struct task_struct *p)
{
	unsigned long flags;

	spin_lock_irqsave(&p->sighand->siglock, flags);
	signal_wake_up(p, 0);
	spin_unlock_irqrestore(&p->sighand->siglock, flags);
}

/**
 *	freeze_task - send a freeze request to given task
 *	@p: task to send the request to
 *	@sig_only: if set, the request will only be sent if the task has the
 *		PF_FREEZER_NOSIG flag unset
 *	Return value: 'false', if @sig_only is set and the task has
 *		PF_FREEZER_NOSIG set or the task is frozen, 'true', otherwise
 *
 *	The freeze request is sent by setting the tasks's TIF_FREEZE flag and
 *	either sending a fake signal to it or waking it up, depending on whether
 *	or not it has PF_FREEZER_NOSIG set.  If @sig_only is set and the task
 *	has PF_FREEZER_NOSIG set (ie. it is a typical kernel thread), its
 *	TIF_FREEZE flag will not be set.
 */
bool freeze_task(struct task_struct *p, bool sig_only)
{
	unsigned long flags;
	bool ret = false;

	spin_lock_irqsave(&freezer_lock, flags);

	if (sig_only && !should_send_signal(p))
		goto out_unlock;

	if (frozen(p))
		goto out_unlock;

	set_freeze_flag(p);

	if (should_send_signal(p)) {
		fake_signal_wake_up(p);
		/*
		 * fake_signal_wake_up() goes through p's scheduler
		 * lock and guarantees that TASK_STOPPED/TRACED ->
		 * TASK_RUNNING transition can't race with task state
		 * testing in try_to_freeze_tasks().
		 */
	} else {
		wake_up_state(p, TASK_INTERRUPTIBLE);
	}
	ret = true;
out_unlock:
	spin_unlock_irqrestore(&freezer_lock, flags);
	return ret;
}

void cancel_freezing(struct task_struct *p)
{
	unsigned long flags;

	spin_lock_irqsave(&freezer_lock, flags);
	if (freezing(p)) {
		pr_debug("  clean up: %s\n", p->comm);
		clear_freeze_flag(p);
		spin_lock(&p->sighand->siglock);
		recalc_sigpending_and_wake(p);
		spin_unlock(&p->sighand->siglock);
	}
	spin_unlock_irqrestore(&freezer_lock, flags);
}

/*
 * Wake up a frozen task
 *
 * task_lock() is needed to prevent the race with refrigerator() which may
 * occur if the freezing of tasks fails.  Namely, without the lock, if the
 * freezing of tasks failed, thaw_tasks() might have run before a task in
 * refrigerator() could call frozen_process(), in which case the task would be
 * frozen and no one would thaw it.
 */
void __thaw_task(struct task_struct *p)
{
	unsigned long flags;

	spin_lock_irqsave(&freezer_lock, flags);
	if (frozen(p)) {
		p->flags &= ~PF_FROZEN;
		wake_up_process(p);
	} else {
		clear_freeze_flag(p);
	}
	spin_unlock_irqrestore(&freezer_lock, flags);
}
