/*
 * kernel/lockdep_proc.c
 *
 * Runtime locking correctness validator
 *
 * Started by Ingo Molnar:
 *
 *  Copyright (C) 2006 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *
 * Code for /proc/lockdep and /proc/lockdep_stats:
 *
 */
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/debug_locks.h>
#include <linux/vmalloc.h>
#include <linux/sort.h>
#include <asm/uaccess.h>
#include <asm/div64.h>

#include "lockdep_internals.h"

static void *l_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct lock_class *class = v;

	(*pos)++;

	if (class->lock_entry.next != &all_lock_classes)
		class = list_entry(class->lock_entry.next, struct lock_class,
				  lock_entry);
	else
		class = NULL;
	m->private = class;

	return class;
}

static void *l_start(struct seq_file *m, loff_t *pos)
{
	struct lock_class *class = m->private;

	if (&class->lock_entry == all_lock_classes.next)
		seq_printf(m, "all lock classes:\n");

	return class;
}

static void l_stop(struct seq_file *m, void *v)
{
}

static unsigned long count_forward_deps(struct lock_class *class)
{
	struct lock_list *entry;
	unsigned long ret = 1;

	/*
	 * Recurse this class's dependency list:
	 */
	list_for_each_entry(entry, &class->locks_after, entry)
		ret += count_forward_deps(entry->class);

	return ret;
}

static unsigned long count_backward_deps(struct lock_class *class)
{
	struct lock_list *entry;
	unsigned long ret = 1;

	/*
	 * Recurse this class's dependency list:
	 */
	list_for_each_entry(entry, &class->locks_before, entry)
		ret += count_backward_deps(entry->class);

	return ret;
}

static void print_name(struct seq_file *m, struct lock_class *class)
{
	char str[128];
	const char *name = class->name;

	if (!name) {
		name = __get_key_name(class->key, str);
		seq_printf(m, "%s", name);
	} else{
		seq_printf(m, "%s", name);
		if (class->name_version > 1)
			seq_printf(m, "#%d", class->name_version);
		if (class->subclass)
			seq_printf(m, "/%d", class->subclass);
	}
}

static int l_show(struct seq_file *m, void *v)
{
	unsigned long nr_forward_deps, nr_backward_deps;
	struct lock_class *class = m->private;
	struct lock_list *entry;
	char c1, c2, c3, c4;

	seq_printf(m, "%p", class->key);
#ifdef CONFIG_DEBUG_LOCKDEP
	seq_printf(m, " OPS:%8ld", class->ops);
#endif
	nr_forward_deps = count_forward_deps(class);
	seq_printf(m, " FD:%5ld", nr_forward_deps);

	nr_backward_deps = count_backward_deps(class);
	seq_printf(m, " BD:%5ld", nr_backward_deps);

	get_usage_chars(class, &c1, &c2, &c3, &c4);
	seq_printf(m, " %c%c%c%c", c1, c2, c3, c4);

	seq_printf(m, ": ");
	print_name(m, class);
	seq_puts(m, "\n");

	list_for_each_entry(entry, &class->locks_after, entry) {
		if (entry->distance == 1) {
			seq_printf(m, " -> [%p] ", entry->class);
			print_name(m, entry->class);
			seq_puts(m, "\n");
		}
	}
	seq_puts(m, "\n");

	return 0;
}

static const struct seq_operations lockdep_ops = {
	.start	= l_start,
	.next	= l_next,
	.stop	= l_stop,
	.show	= l_show,
};

static int lockdep_open(struct inode *inode, struct file *file)
{
	int res = seq_open(file, &lockdep_ops);
	if (!res) {
		struct seq_file *m = file->private_data;

		if (!list_empty(&all_lock_classes))
			m->private = list_entry(all_lock_classes.next,
					struct lock_class, lock_entry);
		else
			m->private = NULL;
	}
	return res;
}

static const struct file_operations proc_lockdep_operations = {
	.open		= lockdep_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static void lockdep_stats_debug_show(struct seq_file *m)
{
#ifdef CONFIG_DEBUG_LOCKDEP
	unsigned int hi1 = debug_atomic_read(&hardirqs_on_events),
		     hi2 = debug_atomic_read(&hardirqs_off_events),
		     hr1 = debug_atomic_read(&redundant_hardirqs_on),
		     hr2 = debug_atomic_read(&redundant_hardirqs_off),
		     si1 = debug_atomic_read(&softirqs_on_events),
		     si2 = debug_atomic_read(&softirqs_off_events),
		     sr1 = debug_atomic_read(&redundant_softirqs_on),
		     sr2 = debug_atomic_read(&redundant_softirqs_off);

	seq_printf(m, " chain lookup misses:           %11u\n",
		debug_atomic_read(&chain_lookup_misses));
	seq_printf(m, " chain lookup hits:             %11u\n",
		debug_atomic_read(&chain_lookup_hits));
	seq_printf(m, " cyclic checks:                 %11u\n",
		debug_atomic_read(&nr_cyclic_checks));
	seq_printf(m, " cyclic-check recursions:       %11u\n",
		debug_atomic_read(&nr_cyclic_check_recursions));
	seq_printf(m, " find-mask forwards checks:     %11u\n",
		debug_atomic_read(&nr_find_usage_forwards_checks));
	seq_printf(m, " find-mask forwards recursions: %11u\n",
		debug_atomic_read(&nr_find_usage_forwards_recursions));
	seq_printf(m, " find-mask backwards checks:    %11u\n",
		debug_atomic_read(&nr_find_usage_backwards_checks));
	seq_printf(m, " find-mask backwards recursions:%11u\n",
		debug_atomic_read(&nr_find_usage_backwards_recursions));

	seq_printf(m, " hardirq on events:             %11u\n", hi1);
	seq_printf(m, " hardirq off events:            %11u\n", hi2);
	seq_printf(m, " redundant hardirq ons:         %11u\n", hr1);
	seq_printf(m, " redundant hardirq offs:        %11u\n", hr2);
	seq_printf(m, " softirq on events:             %11u\n", si1);
	seq_printf(m, " softirq off events:            %11u\n", si2);
	seq_printf(m, " redundant softirq ons:         %11u\n", sr1);
	seq_printf(m, " redundant softirq offs:        %11u\n", sr2);
#endif
}

static int lockdep_stats_show(struct seq_file *m, void *v)
{
	struct lock_class *class;
	unsigned long nr_unused = 0, nr_uncategorized = 0,
		      nr_irq_safe = 0, nr_irq_unsafe = 0,
		      nr_softirq_safe = 0, nr_softirq_unsafe = 0,
		      nr_hardirq_safe = 0, nr_hardirq_unsafe = 0,
		      nr_irq_read_safe = 0, nr_irq_read_unsafe = 0,
		      nr_softirq_read_safe = 0, nr_softirq_read_unsafe = 0,
		      nr_hardirq_read_safe = 0, nr_hardirq_read_unsafe = 0,
		      sum_forward_deps = 0, factor = 0;

	list_for_each_entry(class, &all_lock_classes, lock_entry) {

		if (class->usage_mask == 0)
			nr_unused++;
		if (class->usage_mask == LOCKF_USED)
			nr_uncategorized++;
		if (class->usage_mask & LOCKF_USED_IN_IRQ)
			nr_irq_safe++;
		if (class->usage_mask & LOCKF_ENABLED_IRQS)
			nr_irq_unsafe++;
		if (class->usage_mask & LOCKF_USED_IN_SOFTIRQ)
			nr_softirq_safe++;
		if (class->usage_mask & LOCKF_ENABLED_SOFTIRQS)
			nr_softirq_unsafe++;
		if (class->usage_mask & LOCKF_USED_IN_HARDIRQ)
			nr_hardirq_safe++;
		if (class->usage_mask & LOCKF_ENABLED_HARDIRQS)
			nr_hardirq_unsafe++;
		if (class->usage_mask & LOCKF_USED_IN_IRQ_READ)
			nr_irq_read_safe++;
		if (class->usage_mask & LOCKF_ENABLED_IRQS_READ)
			nr_irq_read_unsafe++;
		if (class->usage_mask & LOCKF_USED_IN_SOFTIRQ_READ)
			nr_softirq_read_safe++;
		if (class->usage_mask & LOCKF_ENABLED_SOFTIRQS_READ)
			nr_softirq_read_unsafe++;
		if (class->usage_mask & LOCKF_USED_IN_HARDIRQ_READ)
			nr_hardirq_read_safe++;
		if (class->usage_mask & LOCKF_ENABLED_HARDIRQS_READ)
			nr_hardirq_read_unsafe++;

		sum_forward_deps += count_forward_deps(class);
	}
#ifdef CONFIG_DEBUG_LOCKDEP
	DEBUG_LOCKS_WARN_ON(debug_atomic_read(&nr_unused_locks) != nr_unused);
#endif
	seq_printf(m, " lock-classes:                  %11lu [max: %lu]\n",
			nr_lock_classes, MAX_LOCKDEP_KEYS);
	seq_printf(m, " direct dependencies:           %11lu [max: %lu]\n",
			nr_list_entries, MAX_LOCKDEP_ENTRIES);
	seq_printf(m, " indirect dependencies:         %11lu\n",
			sum_forward_deps);

	/*
	 * Total number of dependencies:
	 *
	 * All irq-safe locks may nest inside irq-unsafe locks,
	 * plus all the other known dependencies:
	 */
	seq_printf(m, " all direct dependencies:       %11lu\n",
			nr_irq_unsafe * nr_irq_safe +
			nr_hardirq_unsafe * nr_hardirq_safe +
			nr_list_entries);

	/*
	 * Estimated factor between direct and indirect
	 * dependencies:
	 */
	if (nr_list_entries)
		factor = sum_forward_deps / nr_list_entries;

#ifdef CONFIG_PROVE_LOCKING
	seq_printf(m, " dependency chains:             %11lu [max: %lu]\n",
			nr_lock_chains, MAX_LOCKDEP_CHAINS);
#endif

#ifdef CONFIG_TRACE_IRQFLAGS
	seq_printf(m, " in-hardirq chains:             %11u\n",
			nr_hardirq_chains);
	seq_printf(m, " in-softirq chains:             %11u\n",
			nr_softirq_chains);
#endif
	seq_printf(m, " in-process chains:             %11u\n",
			nr_process_chains);
	seq_printf(m, " stack-trace entries:           %11lu [max: %lu]\n",
			nr_stack_trace_entries, MAX_STACK_TRACE_ENTRIES);
	seq_printf(m, " combined max dependencies:     %11u\n",
			(nr_hardirq_chains + 1) *
			(nr_softirq_chains + 1) *
			(nr_process_chains + 1)
	);
	seq_printf(m, " hardirq-safe locks:            %11lu\n",
			nr_hardirq_safe);
	seq_printf(m, " hardirq-unsafe locks:          %11lu\n",
			nr_hardirq_unsafe);
	seq_printf(m, " softirq-safe locks:            %11lu\n",
			nr_softirq_safe);
	seq_printf(m, " softirq-unsafe locks:          %11lu\n",
			nr_softirq_unsafe);
	seq_printf(m, " irq-safe locks:                %11lu\n",
			nr_irq_safe);
	seq_printf(m, " irq-unsafe locks:              %11lu\n",
			nr_irq_unsafe);

	seq_printf(m, " hardirq-read-safe locks:       %11lu\n",
			nr_hardirq_read_safe);
	seq_printf(m, " hardirq-read-unsafe locks:     %11lu\n",
			nr_hardirq_read_unsafe);
	seq_printf(m, " softirq-read-safe locks:       %11lu\n",
			nr_softirq_read_safe);
	seq_printf(m, " softirq-read-unsafe locks:     %11lu\n",
			nr_softirq_read_unsafe);
	seq_printf(m, " irq-read-safe locks:           %11lu\n",
			nr_irq_read_safe);
	seq_printf(m, " irq-read-unsafe locks:         %11lu\n",
			nr_irq_read_unsafe);

	seq_printf(m, " uncategorized locks:           %11lu\n",
			nr_uncategorized);
	seq_printf(m, " unused locks:                  %11lu\n",
			nr_unused);
	seq_printf(m, " max locking depth:             %11u\n",
			max_lockdep_depth);
	seq_printf(m, " max recursion depth:           %11u\n",
			max_recursion_depth);
	lockdep_stats_debug_show(m);
	seq_printf(m, " debug_locks:                   %11u\n",
			debug_locks);

	return 0;
}

static int lockdep_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, lockdep_stats_show, NULL);
}

static const struct file_operations proc_lockdep_stats_operations = {
	.open		= lockdep_stats_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

#ifdef CONFIG_LOCK_STAT

struct lock_stat_data {
	struct lock_class *class;
	struct lock_class_stats stats;
};

struct lock_stat_seq {
	struct lock_stat_data *iter;
	struct lock_stat_data *iter_end;
	struct lock_stat_data stats[MAX_LOCKDEP_KEYS];
};

/*
 * sort on absolute number of contentions
 */
static int lock_stat_cmp(const void *l, const void *r)
{
	const struct lock_stat_data *dl = l, *dr = r;
	unsigned long nl, nr;

	nl = dl->stats.read_waittime.nr + dl->stats.write_waittime.nr;
	nr = dr->stats.read_waittime.nr + dr->stats.write_waittime.nr;

	return nr - nl;
}

static void seq_line(struct seq_file *m, char c, int offset, int length)
{
	int i;

	for (i = 0; i < offset; i++)
		seq_puts(m, " ");
	for (i = 0; i < length; i++)
		seq_printf(m, "%c", c);
	seq_puts(m, "\n");
}

static void snprint_time(char *buf, size_t bufsiz, s64 nr)
{
	unsigned long rem;

	rem = do_div(nr, 1000); /* XXX: do_div_signed */
	snprintf(buf, bufsiz, "%lld.%02d", (long long)nr, ((int)rem+5)/10);
}

static void seq_time(struct seq_file *m, s64 time)
{
	char num[15];

	snprint_time(num, sizeof(num), time);
	seq_printf(m, " %14s", num);
}

static void seq_lock_time(struct seq_file *m, struct lock_time *lt)
{
	seq_printf(m, "%14lu", lt->nr);
	seq_time(m, lt->min);
	seq_time(m, lt->max);
	seq_time(m, lt->total);
}

static void seq_stats(struct seq_file *m, struct lock_stat_data *data)
{
	char name[39];
	struct lock_class *class;
	struct lock_class_stats *stats;
	int i, namelen;

	class = data->class;
	stats = &data->stats;

	snprintf(name, 38, "%s", class->name);
	namelen = strlen(name);

	if (stats->write_holdtime.nr) {
		if (stats->read_holdtime.nr)
			seq_printf(m, "%38s-W:", name);
		else
			seq_printf(m, "%40s:", name);

		seq_lock_time(m, &stats->write_waittime);
		seq_puts(m, " ");
		seq_lock_time(m, &stats->write_holdtime);
		seq_puts(m, "\n");
	}

	if (stats->read_holdtime.nr) {
		seq_printf(m, "%38s-R:", name);
		seq_lock_time(m, &stats->read_waittime);
		seq_puts(m, " ");
		seq_lock_time(m, &stats->read_holdtime);
		seq_puts(m, "\n");
	}

	if (stats->read_waittime.nr + stats->write_waittime.nr == 0)
		return;

	if (stats->read_holdtime.nr)
		namelen += 2;

	for (i = 0; i < ARRAY_SIZE(class->contention_point); i++) {
		char sym[KSYM_SYMBOL_LEN];
		char ip[32];

		if (class->contention_point[i] == 0)
			break;

		if (!i)
			seq_line(m, '-', 40-namelen, namelen);

		sprint_symbol(sym, class->contention_point[i]);
		snprintf(ip, sizeof(ip), "[<%p>]",
				(void *)class->contention_point[i]);
		seq_printf(m, "%40s %14lu %29s %s\n", name,
				stats->contention_point[i],
				ip, sym);
	}
	if (i) {
		seq_puts(m, "\n");
		seq_line(m, '.', 0, 40 + 1 + 8 * (14 + 1));
		seq_puts(m, "\n");
	}
}

static void seq_header(struct seq_file *m)
{
	seq_printf(m, "lock_stat version 0.1\n");
	seq_line(m, '-', 0, 40 + 1 + 8 * (14 + 1));
	seq_printf(m, "%40s %14s %14s %14s %14s %14s %14s %14s %14s\n",
			"class name",
			"contentions",
			"waittime-min",
			"waittime-max",
			"waittime-total",
			"acquisitions",
			"holdtime-min",
			"holdtime-max",
			"holdtime-total");
	seq_line(m, '-', 0, 40 + 1 + 8 * (14 + 1));
	seq_printf(m, "\n");
}

static void *ls_start(struct seq_file *m, loff_t *pos)
{
	struct lock_stat_seq *data = m->private;

	if (data->iter == data->stats)
		seq_header(m);

	return data->iter;
}

static void *ls_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct lock_stat_seq *data = m->private;

	(*pos)++;

	data->iter = v;
	data->iter++;
	if (data->iter == data->iter_end)
		data->iter = NULL;

	return data->iter;
}

static void ls_stop(struct seq_file *m, void *v)
{
}

static int ls_show(struct seq_file *m, void *v)
{
	struct lock_stat_seq *data = m->private;

	seq_stats(m, data->iter);
	return 0;
}

static struct seq_operations lockstat_ops = {
	.start	= ls_start,
	.next	= ls_next,
	.stop	= ls_stop,
	.show	= ls_show,
};

static int lock_stat_open(struct inode *inode, struct file *file)
{
	int res;
	struct lock_class *class;
	struct lock_stat_seq *data = vmalloc(sizeof(struct lock_stat_seq));

	if (!data)
		return -ENOMEM;

	res = seq_open(file, &lockstat_ops);
	if (!res) {
		struct lock_stat_data *iter = data->stats;
		struct seq_file *m = file->private_data;

		data->iter = iter;
		list_for_each_entry(class, &all_lock_classes, lock_entry) {
			iter->class = class;
			iter->stats = lock_stats(class);
			iter++;
		}
		data->iter_end = iter;

		sort(data->stats, data->iter_end - data->iter,
				sizeof(struct lock_stat_data),
				lock_stat_cmp, NULL);

		m->private = data;
	} else
		vfree(data);

	return res;
}

static ssize_t lock_stat_write(struct file *file, const char __user *buf,
			       size_t count, loff_t *ppos)
{
	struct lock_class *class;
	char c;

	if (count) {
		if (get_user(c, buf))
			return -EFAULT;

		if (c != '0')
			return count;

		list_for_each_entry(class, &all_lock_classes, lock_entry)
			clear_lock_stats(class);
	}
	return count;
}

static int lock_stat_release(struct inode *inode, struct file *file)
{
	struct seq_file *seq = file->private_data;

	vfree(seq->private);
	seq->private = NULL;
	return seq_release(inode, file);
}

static const struct file_operations proc_lock_stat_operations = {
	.open		= lock_stat_open,
	.write		= lock_stat_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= lock_stat_release,
};
#endif /* CONFIG_LOCK_STAT */

static int __init lockdep_proc_init(void)
{
	struct proc_dir_entry *entry;

	entry = create_proc_entry("lockdep", S_IRUSR, NULL);
	if (entry)
		entry->proc_fops = &proc_lockdep_operations;

	entry = create_proc_entry("lockdep_stats", S_IRUSR, NULL);
	if (entry)
		entry->proc_fops = &proc_lockdep_stats_operations;

#ifdef CONFIG_LOCK_STAT
	entry = create_proc_entry("lock_stat", S_IRUSR, NULL);
	if (entry)
		entry->proc_fops = &proc_lock_stat_operations;
#endif

	return 0;
}

__initcall(lockdep_proc_init);

