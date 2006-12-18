/*
 * linux/kernel/workqueue.c
 *
 * Generic mechanism for defining kernel helper threads for running
 * arbitrary tasks in process context.
 *
 * Started by Ingo Molnar, Copyright (C) 2002
 *
 * Derived from the taskqueue/keventd code by:
 *
 *   David Woodhouse <dwmw2@infradead.org>
 *   Andrew Morton <andrewm@uow.edu.au>
 *   Kai Petzke <wpp@marie.physik.tu-berlin.de>
 *   Theodore Ts'o <tytso@mit.edu>
 *
 * Made to use alloc_percpu by Christoph Lameter <clameter@sgi.com>.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/kthread.h>
#include <linux/hardirq.h>
#include <linux/mempolicy.h>
#include <linux/freezer.h>
#include <linux/kallsyms.h>
#include <linux/debug_locks.h>

/*
 * The per-CPU workqueue (if single thread, we always use the first
 * possible cpu).
 *
 * The sequence counters are for flush_scheduled_work().  It wants to wait
 * until all currently-scheduled works are completed, but it doesn't
 * want to be livelocked by new, incoming ones.  So it waits until
 * remove_sequence is >= the insert_sequence which pertained when
 * flush_scheduled_work() was called.
 */
struct cpu_workqueue_struct {

	spinlock_t lock;

	long remove_sequence;	/* Least-recently added (next to run) */
	long insert_sequence;	/* Next to add */

	struct list_head worklist;
	wait_queue_head_t more_work;
	wait_queue_head_t work_done;

	struct workqueue_struct *wq;
	struct task_struct *thread;

	int run_depth;		/* Detect run_workqueue() recursion depth */

	int freezeable;		/* Freeze the thread during suspend */
} ____cacheline_aligned;

/*
 * The externally visible workqueue abstraction is an array of
 * per-CPU workqueues:
 */
struct workqueue_struct {
	struct cpu_workqueue_struct *cpu_wq;
	const char *name;
	struct list_head list; 	/* Empty if single thread */
};

/* All the per-cpu workqueues on the system, for hotplug cpu to add/remove
   threads to each one as cpus come/go. */
static DEFINE_MUTEX(workqueue_mutex);
static LIST_HEAD(workqueues);

static int singlethread_cpu;

/* If it's single threaded, it isn't in the list of workqueues. */
static inline int is_single_threaded(struct workqueue_struct *wq)
{
	return list_empty(&wq->list);
}

/*
 * Set the workqueue on which a work item is to be run
 * - Must *only* be called if the pending flag is set
 */
static inline void set_wq_data(struct work_struct *work, void *wq)
{
	unsigned long new;

	BUG_ON(!work_pending(work));

	new = (unsigned long) wq | (1UL << WORK_STRUCT_PENDING);
	new |= WORK_STRUCT_FLAG_MASK & *work_data_bits(work);
	atomic_long_set(&work->data, new);
}

static inline void *get_wq_data(struct work_struct *work)
{
	return (void *) (atomic_long_read(&work->data) & WORK_STRUCT_WQ_DATA_MASK);
}

static int __run_work(struct cpu_workqueue_struct *cwq, struct work_struct *work)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&cwq->lock, flags);
	/*
	 * We need to re-validate the work info after we've gotten
	 * the cpu_workqueue lock. We can run the work now iff:
	 *
	 *  - the wq_data still matches the cpu_workqueue_struct
	 *  - AND the work is still marked pending
	 *  - AND the work is still on a list (which will be this
	 *    workqueue_struct list)
	 *
	 * All these conditions are important, because we
	 * need to protect against the work being run right
	 * now on another CPU (all but the last one might be
	 * true if it's currently running and has not been
	 * released yet, for example).
	 */
	if (get_wq_data(work) == cwq
	    && work_pending(work)
	    && !list_empty(&work->entry)) {
		work_func_t f = work->func;
		list_del_init(&work->entry);
		spin_unlock_irqrestore(&cwq->lock, flags);

		if (!test_bit(WORK_STRUCT_NOAUTOREL, work_data_bits(work)))
			work_release(work);
		f(work);

		spin_lock_irqsave(&cwq->lock, flags);
		cwq->remove_sequence++;
		wake_up(&cwq->work_done);
		ret = 1;
	}
	spin_unlock_irqrestore(&cwq->lock, flags);
	return ret;
}

/**
 * run_scheduled_work - run scheduled work synchronously
 * @work: work to run
 *
 * This checks if the work was pending, and runs it
 * synchronously if so. It returns a boolean to indicate
 * whether it had any scheduled work to run or not.
 *
 * NOTE! This _only_ works for normal work_structs. You
 * CANNOT use this for delayed work, because the wq data
 * for delayed work will not point properly to the per-
 * CPU workqueue struct, but will change!
 */
int fastcall run_scheduled_work(struct work_struct *work)
{
	for (;;) {
		struct cpu_workqueue_struct *cwq;

		if (!work_pending(work))
			return 0;
		if (list_empty(&work->entry))
			return 0;
		/* NOTE! This depends intimately on __queue_work! */
		cwq = get_wq_data(work);
		if (!cwq)
			return 0;
		if (__run_work(cwq, work))
			return 1;
	}
}
EXPORT_SYMBOL(run_scheduled_work);

/* Preempt must be disabled. */
static void __queue_work(struct cpu_workqueue_struct *cwq,
			 struct work_struct *work)
{
	unsigned long flags;

	spin_lock_irqsave(&cwq->lock, flags);
	set_wq_data(work, cwq);
	list_add_tail(&work->entry, &cwq->worklist);
	cwq->insert_sequence++;
	wake_up(&cwq->more_work);
	spin_unlock_irqrestore(&cwq->lock, flags);
}

/**
 * queue_work - queue work on a workqueue
 * @wq: workqueue to use
 * @work: work to queue
 *
 * Returns 0 if @work was already on a queue, non-zero otherwise.
 *
 * We queue the work to the CPU it was submitted, but there is no
 * guarantee that it will be processed by that CPU.
 */
int fastcall queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
	int ret = 0, cpu = get_cpu();

	if (!test_and_set_bit(WORK_STRUCT_PENDING, work_data_bits(work))) {
		if (unlikely(is_single_threaded(wq)))
			cpu = singlethread_cpu;
		BUG_ON(!list_empty(&work->entry));
		__queue_work(per_cpu_ptr(wq->cpu_wq, cpu), work);
		ret = 1;
	}
	put_cpu();
	return ret;
}
EXPORT_SYMBOL_GPL(queue_work);

static void delayed_work_timer_fn(unsigned long __data)
{
	struct delayed_work *dwork = (struct delayed_work *)__data;
	struct workqueue_struct *wq = get_wq_data(&dwork->work);
	int cpu = smp_processor_id();

	if (unlikely(is_single_threaded(wq)))
		cpu = singlethread_cpu;

	__queue_work(per_cpu_ptr(wq->cpu_wq, cpu), &dwork->work);
}

/**
 * queue_delayed_work - queue work on a workqueue after delay
 * @wq: workqueue to use
 * @work: delayable work to queue
 * @delay: number of jiffies to wait before queueing
 *
 * Returns 0 if @work was already on a queue, non-zero otherwise.
 */
int fastcall queue_delayed_work(struct workqueue_struct *wq,
			struct delayed_work *dwork, unsigned long delay)
{
	int ret = 0;
	struct timer_list *timer = &dwork->timer;
	struct work_struct *work = &dwork->work;

	if (delay == 0)
		return queue_work(wq, work);

	if (!test_and_set_bit(WORK_STRUCT_PENDING, work_data_bits(work))) {
		BUG_ON(timer_pending(timer));
		BUG_ON(!list_empty(&work->entry));

		/* This stores wq for the moment, for the timer_fn */
		set_wq_data(work, wq);
		timer->expires = jiffies + delay;
		timer->data = (unsigned long)dwork;
		timer->function = delayed_work_timer_fn;
		add_timer(timer);
		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL_GPL(queue_delayed_work);

/**
 * queue_delayed_work_on - queue work on specific CPU after delay
 * @cpu: CPU number to execute work on
 * @wq: workqueue to use
 * @work: work to queue
 * @delay: number of jiffies to wait before queueing
 *
 * Returns 0 if @work was already on a queue, non-zero otherwise.
 */
int queue_delayed_work_on(int cpu, struct workqueue_struct *wq,
			struct delayed_work *dwork, unsigned long delay)
{
	int ret = 0;
	struct timer_list *timer = &dwork->timer;
	struct work_struct *work = &dwork->work;

	if (!test_and_set_bit(WORK_STRUCT_PENDING, work_data_bits(work))) {
		BUG_ON(timer_pending(timer));
		BUG_ON(!list_empty(&work->entry));

		/* This stores wq for the moment, for the timer_fn */
		set_wq_data(work, wq);
		timer->expires = jiffies + delay;
		timer->data = (unsigned long)dwork;
		timer->function = delayed_work_timer_fn;
		add_timer_on(timer, cpu);
		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL_GPL(queue_delayed_work_on);

static void run_workqueue(struct cpu_workqueue_struct *cwq)
{
	unsigned long flags;

	/*
	 * Keep taking off work from the queue until
	 * done.
	 */
	spin_lock_irqsave(&cwq->lock, flags);
	cwq->run_depth++;
	if (cwq->run_depth > 3) {
		/* morton gets to eat his hat */
		printk("%s: recursion depth exceeded: %d\n",
			__FUNCTION__, cwq->run_depth);
		dump_stack();
	}
	while (!list_empty(&cwq->worklist)) {
		struct work_struct *work = list_entry(cwq->worklist.next,
						struct work_struct, entry);
		work_func_t f = work->func;

		list_del_init(cwq->worklist.next);
		spin_unlock_irqrestore(&cwq->lock, flags);

		BUG_ON(get_wq_data(work) != cwq);
		if (!test_bit(WORK_STRUCT_NOAUTOREL, work_data_bits(work)))
			work_release(work);
		f(work);

		if (unlikely(in_atomic() || lockdep_depth(current) > 0)) {
			printk(KERN_ERR "BUG: workqueue leaked lock or atomic: "
					"%s/0x%08x/%d\n",
					current->comm, preempt_count(),
				       	current->pid);
			printk(KERN_ERR "    last function: ");
			print_symbol("%s\n", (unsigned long)f);
			debug_show_held_locks(current);
			dump_stack();
		}

		spin_lock_irqsave(&cwq->lock, flags);
		cwq->remove_sequence++;
		wake_up(&cwq->work_done);
	}
	cwq->run_depth--;
	spin_unlock_irqrestore(&cwq->lock, flags);
}

static int worker_thread(void *__cwq)
{
	struct cpu_workqueue_struct *cwq = __cwq;
	DECLARE_WAITQUEUE(wait, current);
	struct k_sigaction sa;
	sigset_t blocked;

	if (!cwq->freezeable)
		current->flags |= PF_NOFREEZE;

	set_user_nice(current, -5);

	/* Block and flush all signals */
	sigfillset(&blocked);
	sigprocmask(SIG_BLOCK, &blocked, NULL);
	flush_signals(current);

	/*
	 * We inherited MPOL_INTERLEAVE from the booting kernel.
	 * Set MPOL_DEFAULT to insure node local allocations.
	 */
	numa_default_policy();

	/* SIG_IGN makes children autoreap: see do_notify_parent(). */
	sa.sa.sa_handler = SIG_IGN;
	sa.sa.sa_flags = 0;
	siginitset(&sa.sa.sa_mask, sigmask(SIGCHLD));
	do_sigaction(SIGCHLD, &sa, (struct k_sigaction *)0);

	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		if (cwq->freezeable)
			try_to_freeze();

		add_wait_queue(&cwq->more_work, &wait);
		if (list_empty(&cwq->worklist))
			schedule();
		else
			__set_current_state(TASK_RUNNING);
		remove_wait_queue(&cwq->more_work, &wait);

		if (!list_empty(&cwq->worklist))
			run_workqueue(cwq);
		set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);
	return 0;
}

static void flush_cpu_workqueue(struct cpu_workqueue_struct *cwq)
{
	if (cwq->thread == current) {
		/*
		 * Probably keventd trying to flush its own queue. So simply run
		 * it by hand rather than deadlocking.
		 */
		run_workqueue(cwq);
	} else {
		DEFINE_WAIT(wait);
		long sequence_needed;

		spin_lock_irq(&cwq->lock);
		sequence_needed = cwq->insert_sequence;

		while (sequence_needed - cwq->remove_sequence > 0) {
			prepare_to_wait(&cwq->work_done, &wait,
					TASK_UNINTERRUPTIBLE);
			spin_unlock_irq(&cwq->lock);
			schedule();
			spin_lock_irq(&cwq->lock);
		}
		finish_wait(&cwq->work_done, &wait);
		spin_unlock_irq(&cwq->lock);
	}
}

/**
 * flush_workqueue - ensure that any scheduled work has run to completion.
 * @wq: workqueue to flush
 *
 * Forces execution of the workqueue and blocks until its completion.
 * This is typically used in driver shutdown handlers.
 *
 * This function will sample each workqueue's current insert_sequence number and
 * will sleep until the head sequence is greater than or equal to that.  This
 * means that we sleep until all works which were queued on entry have been
 * handled, but we are not livelocked by new incoming ones.
 *
 * This function used to run the workqueues itself.  Now we just wait for the
 * helper threads to do it.
 */
void fastcall flush_workqueue(struct workqueue_struct *wq)
{
	might_sleep();

	if (is_single_threaded(wq)) {
		/* Always use first cpu's area. */
		flush_cpu_workqueue(per_cpu_ptr(wq->cpu_wq, singlethread_cpu));
	} else {
		int cpu;

		mutex_lock(&workqueue_mutex);
		for_each_online_cpu(cpu)
			flush_cpu_workqueue(per_cpu_ptr(wq->cpu_wq, cpu));
		mutex_unlock(&workqueue_mutex);
	}
}
EXPORT_SYMBOL_GPL(flush_workqueue);

static struct task_struct *create_workqueue_thread(struct workqueue_struct *wq,
						   int cpu, int freezeable)
{
	struct cpu_workqueue_struct *cwq = per_cpu_ptr(wq->cpu_wq, cpu);
	struct task_struct *p;

	spin_lock_init(&cwq->lock);
	cwq->wq = wq;
	cwq->thread = NULL;
	cwq->insert_sequence = 0;
	cwq->remove_sequence = 0;
	cwq->freezeable = freezeable;
	INIT_LIST_HEAD(&cwq->worklist);
	init_waitqueue_head(&cwq->more_work);
	init_waitqueue_head(&cwq->work_done);

	if (is_single_threaded(wq))
		p = kthread_create(worker_thread, cwq, "%s", wq->name);
	else
		p = kthread_create(worker_thread, cwq, "%s/%d", wq->name, cpu);
	if (IS_ERR(p))
		return NULL;
	cwq->thread = p;
	return p;
}

struct workqueue_struct *__create_workqueue(const char *name,
					    int singlethread, int freezeable)
{
	int cpu, destroy = 0;
	struct workqueue_struct *wq;
	struct task_struct *p;

	wq = kzalloc(sizeof(*wq), GFP_KERNEL);
	if (!wq)
		return NULL;

	wq->cpu_wq = alloc_percpu(struct cpu_workqueue_struct);
	if (!wq->cpu_wq) {
		kfree(wq);
		return NULL;
	}

	wq->name = name;
	mutex_lock(&workqueue_mutex);
	if (singlethread) {
		INIT_LIST_HEAD(&wq->list);
		p = create_workqueue_thread(wq, singlethread_cpu, freezeable);
		if (!p)
			destroy = 1;
		else
			wake_up_process(p);
	} else {
		list_add(&wq->list, &workqueues);
		for_each_online_cpu(cpu) {
			p = create_workqueue_thread(wq, cpu, freezeable);
			if (p) {
				kthread_bind(p, cpu);
				wake_up_process(p);
			} else
				destroy = 1;
		}
	}
	mutex_unlock(&workqueue_mutex);

	/*
	 * Was there any error during startup? If yes then clean up:
	 */
	if (destroy) {
		destroy_workqueue(wq);
		wq = NULL;
	}
	return wq;
}
EXPORT_SYMBOL_GPL(__create_workqueue);

static void cleanup_workqueue_thread(struct workqueue_struct *wq, int cpu)
{
	struct cpu_workqueue_struct *cwq;
	unsigned long flags;
	struct task_struct *p;

	cwq = per_cpu_ptr(wq->cpu_wq, cpu);
	spin_lock_irqsave(&cwq->lock, flags);
	p = cwq->thread;
	cwq->thread = NULL;
	spin_unlock_irqrestore(&cwq->lock, flags);
	if (p)
		kthread_stop(p);
}

/**
 * destroy_workqueue - safely terminate a workqueue
 * @wq: target workqueue
 *
 * Safely destroy a workqueue. All work currently pending will be done first.
 */
void destroy_workqueue(struct workqueue_struct *wq)
{
	int cpu;

	flush_workqueue(wq);

	/* We don't need the distraction of CPUs appearing and vanishing. */
	mutex_lock(&workqueue_mutex);
	if (is_single_threaded(wq))
		cleanup_workqueue_thread(wq, singlethread_cpu);
	else {
		for_each_online_cpu(cpu)
			cleanup_workqueue_thread(wq, cpu);
		list_del(&wq->list);
	}
	mutex_unlock(&workqueue_mutex);
	free_percpu(wq->cpu_wq);
	kfree(wq);
}
EXPORT_SYMBOL_GPL(destroy_workqueue);

static struct workqueue_struct *keventd_wq;

/**
 * schedule_work - put work task in global workqueue
 * @work: job to be done
 *
 * This puts a job in the kernel-global workqueue.
 */
int fastcall schedule_work(struct work_struct *work)
{
	return queue_work(keventd_wq, work);
}
EXPORT_SYMBOL(schedule_work);

/**
 * schedule_delayed_work - put work task in global workqueue after delay
 * @dwork: job to be done
 * @delay: number of jiffies to wait or 0 for immediate execution
 *
 * After waiting for a given time this puts a job in the kernel-global
 * workqueue.
 */
int fastcall schedule_delayed_work(struct delayed_work *dwork, unsigned long delay)
{
	return queue_delayed_work(keventd_wq, dwork, delay);
}
EXPORT_SYMBOL(schedule_delayed_work);

/**
 * schedule_delayed_work_on - queue work in global workqueue on CPU after delay
 * @cpu: cpu to use
 * @dwork: job to be done
 * @delay: number of jiffies to wait
 *
 * After waiting for a given time this puts a job in the kernel-global
 * workqueue on the specified CPU.
 */
int schedule_delayed_work_on(int cpu,
			struct delayed_work *dwork, unsigned long delay)
{
	return queue_delayed_work_on(cpu, keventd_wq, dwork, delay);
}
EXPORT_SYMBOL(schedule_delayed_work_on);

/**
 * schedule_on_each_cpu - call a function on each online CPU from keventd
 * @func: the function to call
 *
 * Returns zero on success.
 * Returns -ve errno on failure.
 *
 * Appears to be racy against CPU hotplug.
 *
 * schedule_on_each_cpu() is very slow.
 */
int schedule_on_each_cpu(work_func_t func)
{
	int cpu;
	struct work_struct *works;

	works = alloc_percpu(struct work_struct);
	if (!works)
		return -ENOMEM;

	mutex_lock(&workqueue_mutex);
	for_each_online_cpu(cpu) {
		struct work_struct *work = per_cpu_ptr(works, cpu);

		INIT_WORK(work, func);
		set_bit(WORK_STRUCT_PENDING, work_data_bits(work));
		__queue_work(per_cpu_ptr(keventd_wq->cpu_wq, cpu), work);
	}
	mutex_unlock(&workqueue_mutex);
	flush_workqueue(keventd_wq);
	free_percpu(works);
	return 0;
}

void flush_scheduled_work(void)
{
	flush_workqueue(keventd_wq);
}
EXPORT_SYMBOL(flush_scheduled_work);

/**
 * cancel_rearming_delayed_workqueue - reliably kill off a delayed
 *			work whose handler rearms the delayed work.
 * @wq:   the controlling workqueue structure
 * @dwork: the delayed work struct
 */
void cancel_rearming_delayed_workqueue(struct workqueue_struct *wq,
				       struct delayed_work *dwork)
{
	while (!cancel_delayed_work(dwork))
		flush_workqueue(wq);
}
EXPORT_SYMBOL(cancel_rearming_delayed_workqueue);

/**
 * cancel_rearming_delayed_work - reliably kill off a delayed keventd
 *			work whose handler rearms the delayed work.
 * @dwork: the delayed work struct
 */
void cancel_rearming_delayed_work(struct delayed_work *dwork)
{
	cancel_rearming_delayed_workqueue(keventd_wq, dwork);
}
EXPORT_SYMBOL(cancel_rearming_delayed_work);

/**
 * execute_in_process_context - reliably execute the routine with user context
 * @fn:		the function to execute
 * @ew:		guaranteed storage for the execute work structure (must
 *		be available when the work executes)
 *
 * Executes the function immediately if process context is available,
 * otherwise schedules the function for delayed execution.
 *
 * Returns:	0 - function was executed
 *		1 - function was scheduled for execution
 */
int execute_in_process_context(work_func_t fn, struct execute_work *ew)
{
	if (!in_interrupt()) {
		fn(&ew->work);
		return 0;
	}

	INIT_WORK(&ew->work, fn);
	schedule_work(&ew->work);

	return 1;
}
EXPORT_SYMBOL_GPL(execute_in_process_context);

int keventd_up(void)
{
	return keventd_wq != NULL;
}

int current_is_keventd(void)
{
	struct cpu_workqueue_struct *cwq;
	int cpu = smp_processor_id();	/* preempt-safe: keventd is per-cpu */
	int ret = 0;

	BUG_ON(!keventd_wq);

	cwq = per_cpu_ptr(keventd_wq->cpu_wq, cpu);
	if (current == cwq->thread)
		ret = 1;

	return ret;

}

/* Take the work from this (downed) CPU. */
static void take_over_work(struct workqueue_struct *wq, unsigned int cpu)
{
	struct cpu_workqueue_struct *cwq = per_cpu_ptr(wq->cpu_wq, cpu);
	struct list_head list;
	struct work_struct *work;

	spin_lock_irq(&cwq->lock);
	list_replace_init(&cwq->worklist, &list);

	while (!list_empty(&list)) {
		printk("Taking work for %s\n", wq->name);
		work = list_entry(list.next,struct work_struct,entry);
		list_del(&work->entry);
		__queue_work(per_cpu_ptr(wq->cpu_wq, smp_processor_id()), work);
	}
	spin_unlock_irq(&cwq->lock);
}

/* We're holding the cpucontrol mutex here */
static int __devinit workqueue_cpu_callback(struct notifier_block *nfb,
				  unsigned long action,
				  void *hcpu)
{
	unsigned int hotcpu = (unsigned long)hcpu;
	struct workqueue_struct *wq;

	switch (action) {
	case CPU_UP_PREPARE:
		mutex_lock(&workqueue_mutex);
		/* Create a new workqueue thread for it. */
		list_for_each_entry(wq, &workqueues, list) {
			if (!create_workqueue_thread(wq, hotcpu, 0)) {
				printk("workqueue for %i failed\n", hotcpu);
				return NOTIFY_BAD;
			}
		}
		break;

	case CPU_ONLINE:
		/* Kick off worker threads. */
		list_for_each_entry(wq, &workqueues, list) {
			struct cpu_workqueue_struct *cwq;

			cwq = per_cpu_ptr(wq->cpu_wq, hotcpu);
			kthread_bind(cwq->thread, hotcpu);
			wake_up_process(cwq->thread);
		}
		mutex_unlock(&workqueue_mutex);
		break;

	case CPU_UP_CANCELED:
		list_for_each_entry(wq, &workqueues, list) {
			if (!per_cpu_ptr(wq->cpu_wq, hotcpu)->thread)
				continue;
			/* Unbind so it can run. */
			kthread_bind(per_cpu_ptr(wq->cpu_wq, hotcpu)->thread,
				     any_online_cpu(cpu_online_map));
			cleanup_workqueue_thread(wq, hotcpu);
		}
		mutex_unlock(&workqueue_mutex);
		break;

	case CPU_DOWN_PREPARE:
		mutex_lock(&workqueue_mutex);
		break;

	case CPU_DOWN_FAILED:
		mutex_unlock(&workqueue_mutex);
		break;

	case CPU_DEAD:
		list_for_each_entry(wq, &workqueues, list)
			cleanup_workqueue_thread(wq, hotcpu);
		list_for_each_entry(wq, &workqueues, list)
			take_over_work(wq, hotcpu);
		mutex_unlock(&workqueue_mutex);
		break;
	}

	return NOTIFY_OK;
}

void init_workqueues(void)
{
	singlethread_cpu = first_cpu(cpu_possible_map);
	hotcpu_notifier(workqueue_cpu_callback, 0);
	keventd_wq = create_workqueue("events");
	BUG_ON(!keventd_wq);
}

