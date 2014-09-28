/*
 * net/sched/sch_api.c	Packet scheduler API.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 * Fixes:
 *
 * Rani Assaf <rani@magic.metawire.com> :980802: JIFFIES and CPU clock sources are repaired.
 * Eduardo J. Blanco <ejbs@netlabs.com.uy> :990222: kmod support
 * Jamal Hadi Salim <hadi@nortelnetworks.com>: 990601: ingress support
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kmod.h>
#include <linux/list.h>
#include <linux/hrtimer.h>
#include <linux/lockdep.h>
#include <linux/slab.h>

#include <net/net_namespace.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <net/pkt_sched.h>

static int qdisc_notify(struct net *net, struct sk_buff *oskb,
			struct nlmsghdr *n, u32 clid,
			struct Qdisc *old, struct Qdisc *new);
static int tclass_notify(struct net *net, struct sk_buff *oskb,
			 struct nlmsghdr *n, struct Qdisc *q,
			 unsigned long cl, int event);

/*

   Short review.
   -------------

   This file consists of two interrelated parts:

   1. queueing disciplines manager frontend.
   2. traffic classes manager frontend.

   Generally, queueing discipline ("qdisc") is a black box,
   which is able to enqueue packets and to dequeue them (when
   device is ready to send something) in order and at times
   determined by algorithm hidden in it.

   qdisc's are divided to two categories:
   - "queues", which have no internal structure visible from outside.
   - "schedulers", which split all the packets to "traffic classes",
     using "packet classifiers" (look at cls_api.c)

   In turn, classes may have child qdiscs (as rule, queues)
   attached to them etc. etc. etc.

   The goal of the routines in this file is to translate
   information supplied by user in the form of handles
   to more intelligible for kernel form, to make some sanity
   checks and part of work, which is common to all qdiscs
   and to provide rtnetlink notifications.

   All real intelligent work is done inside qdisc modules.



   Every discipline has two major routines: enqueue and dequeue.

   ---dequeue

   dequeue usually returns a skb to send. It is allowed to return NULL,
   but it does not mean that queue is empty, it just means that
   discipline does not want to send anything this time.
   Queue is really empty if q->q.qlen == 0.
   For complicated disciplines with multiple queues q->q is not
   real packet queue, but however q->q.qlen must be valid.

   ---enqueue

   enqueue returns 0, if packet was enqueued successfully.
   If packet (this one or another one) was dropped, it returns
   not zero error code.
   NET_XMIT_DROP 	- this packet dropped
     Expected action: do not backoff, but wait until queue will clear.
   NET_XMIT_CN	 	- probably this packet enqueued, but another one dropped.
     Expected action: backoff or ignore
   NET_XMIT_POLICED	- dropped by police.
     Expected action: backoff or error to real-time apps.

   Auxiliary routines:

   ---peek

   like dequeue but without removing a packet from the queue

   ---reset

   returns qdisc to initial state: purge all buffers, clear all
   timers, counters (except for statistics) etc.

   ---init

   initializes newly created qdisc.

   ---destroy

   destroys resources allocated by init and during lifetime of qdisc.

   ---change

   changes qdisc parameters.
 */

/* Protects list of registered TC modules. It is pure SMP lock. */
static DEFINE_RWLOCK(qdisc_mod_lock);


/************************************************
 *	Queueing disciplines manipulation.	*
 ************************************************/


/* The list of all installed queueing disciplines. */

static struct Qdisc_ops *qdisc_base;

/* Register/unregister queueing discipline */

int register_qdisc(struct Qdisc_ops *qops)
{
	struct Qdisc_ops *q, **qp;
	int rc = -EEXIST;

	write_lock(&qdisc_mod_lock);
	for (qp = &qdisc_base; (q = *qp) != NULL; qp = &q->next)
		if (!strcmp(qops->id, q->id))
			goto out;

	if (qops->enqueue == NULL)
		qops->enqueue = noop_qdisc_ops.enqueue;
	if (qops->peek == NULL) {
		if (qops->dequeue == NULL)
			qops->peek = noop_qdisc_ops.peek;
		else
			goto out_einval;
	}
	if (qops->dequeue == NULL)
		qops->dequeue = noop_qdisc_ops.dequeue;

	if (qops->cl_ops) {
		const struct Qdisc_class_ops *cops = qops->cl_ops;

		if (!(cops->get && cops->put && cops->walk && cops->leaf))
			goto out_einval;

		if (cops->tcf_chain && !(cops->bind_tcf && cops->unbind_tcf))
			goto out_einval;
	}

	qops->next = NULL;
	*qp = qops;
	rc = 0;
out:
	write_unlock(&qdisc_mod_lock);
	return rc;

out_einval:
	rc = -EINVAL;
	goto out;
}
EXPORT_SYMBOL(register_qdisc);

int unregister_qdisc(struct Qdisc_ops *qops)
{
	struct Qdisc_ops *q, **qp;
	int err = -ENOENT;

	write_lock(&qdisc_mod_lock);
	for (qp = &qdisc_base; (q = *qp) != NULL; qp = &q->next)
		if (q == qops)
			break;
	if (q) {
		*qp = q->next;
		q->next = NULL;
		err = 0;
	}
	write_unlock(&qdisc_mod_lock);
	return err;
}
EXPORT_SYMBOL(unregister_qdisc);

/* Get default qdisc if not otherwise specified */
void qdisc_get_default(char *name, size_t len)
{
	read_lock(&qdisc_mod_lock);
	strlcpy(name, default_qdisc_ops->id, len);
	read_unlock(&qdisc_mod_lock);
}

static struct Qdisc_ops *qdisc_lookup_default(const char *name)
{
	struct Qdisc_ops *q = NULL;

	for (q = qdisc_base; q; q = q->next) {
		if (!strcmp(name, q->id)) {
			if (!try_module_get(q->owner))
				q = NULL;
			break;
		}
	}

	return q;
}

/* Set new default qdisc to use */
int qdisc_set_default(const char *name)
{
	const struct Qdisc_ops *ops;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	write_lock(&qdisc_mod_lock);
	ops = qdisc_lookup_default(name);
	if (!ops) {
		/* Not found, drop lock and try to load module */
		write_unlock(&qdisc_mod_lock);
		request_module("sch_%s", name);
		write_lock(&qdisc_mod_lock);

		ops = qdisc_lookup_default(name);
	}

	if (ops) {
		/* Set new default */
		module_put(default_qdisc_ops->owner);
		default_qdisc_ops = ops;
	}
	write_unlock(&qdisc_mod_lock);

	return ops ? 0 : -ENOENT;
}

/* We know handle. Find qdisc among all qdisc's attached to device
   (root qdisc, all its children, children of children etc.)
 */

static struct Qdisc *qdisc_match_from_root(struct Qdisc *root, u32 handle)
{
	struct Qdisc *q;

	if (!(root->flags & TCQ_F_BUILTIN) &&
	    root->handle == handle)
		return root;

	list_for_each_entry(q, &root->list, list) {
		if (q->handle == handle)
			return q;
	}
	return NULL;
}

void qdisc_list_add(struct Qdisc *q)
{
	if ((q->parent != TC_H_ROOT) && !(q->flags & TCQ_F_INGRESS)) {
		struct Qdisc *root = qdisc_dev(q)->qdisc;

		WARN_ON_ONCE(root == &noop_qdisc);
		list_add_tail(&q->list, &root->list);
	}
}
EXPORT_SYMBOL(qdisc_list_add);

void qdisc_list_del(struct Qdisc *q)
{
	if ((q->parent != TC_H_ROOT) && !(q->flags & TCQ_F_INGRESS))
		list_del(&q->list);
}
EXPORT_SYMBOL(qdisc_list_del);

struct Qdisc *qdisc_lookup(struct net_device *dev, u32 handle)
{
	struct Qdisc *q;

	q = qdisc_match_from_root(dev->qdisc, handle);
	if (q)
		goto out;

	if (dev_ingress_queue(dev))
		q = qdisc_match_from_root(
			dev_ingress_queue(dev)->qdisc_sleeping,
			handle);
out:
	return q;
}

static struct Qdisc *qdisc_leaf(struct Qdisc *p, u32 classid)
{
	unsigned long cl;
	struct Qdisc *leaf;
	const struct Qdisc_class_ops *cops = p->ops->cl_ops;

	if (cops == NULL)
		return NULL;
	cl = cops->get(p, classid);

	if (cl == 0)
		return NULL;
	leaf = cops->leaf(p, cl);
	cops->put(p, cl);
	return leaf;
}

/* Find queueing discipline by name */

static struct Qdisc_ops *qdisc_lookup_ops(struct nlattr *kind)
{
	struct Qdisc_ops *q = NULL;

	if (kind) {
		read_lock(&qdisc_mod_lock);
		for (q = qdisc_base; q; q = q->next) {
			if (nla_strcmp(kind, q->id) == 0) {
				if (!try_module_get(q->owner))
					q = NULL;
				break;
			}
		}
		read_unlock(&qdisc_mod_lock);
	}
	return q;
}

/* The linklayer setting were not transferred from iproute2, in older
 * versions, and the rate tables lookup systems have been dropped in
 * the kernel. To keep backward compatible with older iproute2 tc
 * utils, we detect the linklayer setting by detecting if the rate
 * table were modified.
 *
 * For linklayer ATM table entries, the rate table will be aligned to
 * 48 bytes, thus some table entries will contain the same value.  The
 * mpu (min packet unit) is also encoded into the old rate table, thus
 * starting from the mpu, we find low and high table entries for
 * mapping this cell.  If these entries contain the same value, when
 * the rate tables have been modified for linklayer ATM.
 *
 * This is done by rounding mpu to the nearest 48 bytes cell/entry,
 * and then roundup to the next cell, calc the table entry one below,
 * and compare.
 */
static __u8 __detect_linklayer(struct tc_ratespec *r, __u32 *rtab)
{
	int low       = roundup(r->mpu, 48);
	int high      = roundup(low+1, 48);
	int cell_low  = low >> r->cell_log;
	int cell_high = (high >> r->cell_log) - 1;

	/* rtab is too inaccurate at rates > 100Mbit/s */
	if ((r->rate > (100000000/8)) || (rtab[0] == 0)) {
		pr_debug("TC linklayer: Giving up ATM detection\n");
		return TC_LINKLAYER_ETHERNET;
	}

	if ((cell_high > cell_low) && (cell_high < 256)
	    && (rtab[cell_low] == rtab[cell_high])) {
		pr_debug("TC linklayer: Detected ATM, low(%d)=high(%d)=%u\n",
			 cell_low, cell_high, rtab[cell_high]);
		return TC_LINKLAYER_ATM;
	}
	return TC_LINKLAYER_ETHERNET;
}

static struct qdisc_rate_table *qdisc_rtab_list;

struct qdisc_rate_table *qdisc_get_rtab(struct tc_ratespec *r, struct nlattr *tab)
{
	struct qdisc_rate_table *rtab;

	if (tab == NULL || r->rate == 0 || r->cell_log == 0 ||
	    nla_len(tab) != TC_RTAB_SIZE)
		return NULL;

	for (rtab = qdisc_rtab_list; rtab; rtab = rtab->next) {
		if (!memcmp(&rtab->rate, r, sizeof(struct tc_ratespec)) &&
		    !memcmp(&rtab->data, nla_data(tab), 1024)) {
			rtab->refcnt++;
			return rtab;
		}
	}

	rtab = kmalloc(sizeof(*rtab), GFP_KERNEL);
	if (rtab) {
		rtab->rate = *r;
		rtab->refcnt = 1;
		memcpy(rtab->data, nla_data(tab), 1024);
		if (r->linklayer == TC_LINKLAYER_UNAWARE)
			r->linklayer = __detect_linklayer(r, rtab->data);
		rtab->next = qdisc_rtab_list;
		qdisc_rtab_list = rtab;
	}
	return rtab;
}
EXPORT_SYMBOL(qdisc_get_rtab);

void qdisc_put_rtab(struct qdisc_rate_table *tab)
{
	struct qdisc_rate_table *rtab, **rtabp;

	if (!tab || --tab->refcnt)
		return;

	for (rtabp = &qdisc_rtab_list;
	     (rtab = *rtabp) != NULL;
	     rtabp = &rtab->next) {
		if (rtab == tab) {
			*rtabp = rtab->next;
			kfree(rtab);
			return;
		}
	}
}
EXPORT_SYMBOL(qdisc_put_rtab);

static LIST_HEAD(qdisc_stab_list);
static DEFINE_SPINLOCK(qdisc_stab_lock);

static const struct nla_policy stab_policy[TCA_STAB_MAX + 1] = {
	[TCA_STAB_BASE]	= { .len = sizeof(struct tc_sizespec) },
	[TCA_STAB_DATA] = { .type = NLA_BINARY },
};

static struct qdisc_size_table *qdisc_get_stab(struct nlattr *opt)
{
	struct nlattr *tb[TCA_STAB_MAX + 1];
	struct qdisc_size_table *stab;
	struct tc_sizespec *s;
	unsigned int tsize = 0;
	u16 *tab = NULL;
	int err;

	err = nla_parse_nested(tb, TCA_STAB_MAX, opt, stab_policy);
	if (err < 0)
		return ERR_PTR(err);
	if (!tb[TCA_STAB_BASE])
		return ERR_PTR(-EINVAL);

	s = nla_data(tb[TCA_STAB_BASE]);

	if (s->tsize > 0) {
		if (!tb[TCA_STAB_DATA])
			return ERR_PTR(-EINVAL);
		tab = nla_data(tb[TCA_STAB_DATA]);
		tsize = nla_len(tb[TCA_STAB_DATA]) / sizeof(u16);
	}

	if (tsize != s->tsize || (!tab && tsize > 0))
		return ERR_PTR(-EINVAL);

	spin_lock(&qdisc_stab_lock);

	list_for_each_entry(stab, &qdisc_stab_list, list) {
		if (memcmp(&stab->szopts, s, sizeof(*s)))
			continue;
		if (tsize > 0 && memcmp(stab->data, tab, tsize * sizeof(u16)))
			continue;
		stab->refcnt++;
		spin_unlock(&qdisc_stab_lock);
		return stab;
	}

	spin_unlock(&qdisc_stab_lock);

	stab = kmalloc(sizeof(*stab) + tsize * sizeof(u16), GFP_KERNEL);
	if (!stab)
		return ERR_PTR(-ENOMEM);

	stab->refcnt = 1;
	stab->szopts = *s;
	if (tsize > 0)
		memcpy(stab->data, tab, tsize * sizeof(u16));

	spin_lock(&qdisc_stab_lock);
	list_add_tail(&stab->list, &qdisc_stab_list);
	spin_unlock(&qdisc_stab_lock);

	return stab;
}

static void stab_kfree_rcu(struct rcu_head *head)
{
	kfree(container_of(head, struct qdisc_size_table, rcu));
}

void qdisc_put_stab(struct qdisc_size_table *tab)
{
	if (!tab)
		return;

	spin_lock(&qdisc_stab_lock);

	if (--tab->refcnt == 0) {
		list_del(&tab->list);
		call_rcu_bh(&tab->rcu, stab_kfree_rcu);
	}

	spin_unlock(&qdisc_stab_lock);
}
EXPORT_SYMBOL(qdisc_put_stab);

static int qdisc_dump_stab(struct sk_buff *skb, struct qdisc_size_table *stab)
{
	struct nlattr *nest;

	nest = nla_nest_start(skb, TCA_STAB);
	if (nest == NULL)
		goto nla_put_failure;
	if (nla_put(skb, TCA_STAB_BASE, sizeof(stab->szopts), &stab->szopts))
		goto nla_put_failure;
	nla_nest_end(skb, nest);

	return skb->len;

nla_put_failure:
	return -1;
}

void __qdisc_calculate_pkt_len(struct sk_buff *skb, const struct qdisc_size_table *stab)
{
	int pkt_len, slot;

	pkt_len = skb->len + stab->szopts.overhead;
	if (unlikely(!stab->szopts.tsize))
		goto out;

	slot = pkt_len + stab->szopts.cell_align;
	if (unlikely(slot < 0))
		slot = 0;

	slot >>= stab->szopts.cell_log;
	if (likely(slot < stab->szopts.tsize))
		pkt_len = stab->data[slot];
	else
		pkt_len = stab->data[stab->szopts.tsize - 1] *
				(slot / stab->szopts.tsize) +
				stab->data[slot % stab->szopts.tsize];

	pkt_len <<= stab->szopts.size_log;
out:
	if (unlikely(pkt_len < 1))
		pkt_len = 1;
	qdisc_skb_cb(skb)->pkt_len = pkt_len;
}
EXPORT_SYMBOL(__qdisc_calculate_pkt_len);

void qdisc_warn_nonwc(const char *txt, struct Qdisc *qdisc)
{
	if (!(qdisc->flags & TCQ_F_WARN_NONWC)) {
		pr_warn("%s: %s qdisc %X: is non-work-conserving?\n",
			txt, qdisc->ops->id, qdisc->handle >> 16);
		qdisc->flags |= TCQ_F_WARN_NONWC;
	}
}
EXPORT_SYMBOL(qdisc_warn_nonwc);

static enum hrtimer_restart qdisc_watchdog(struct hrtimer *timer)
{
	struct qdisc_watchdog *wd = container_of(timer, struct qdisc_watchdog,
						 timer);

	qdisc_unthrottled(wd->qdisc);
	__netif_schedule(qdisc_root(wd->qdisc));

	return HRTIMER_NORESTART;
}

void qdisc_watchdog_init(struct qdisc_watchdog *wd, struct Qdisc *qdisc)
{
	hrtimer_init(&wd->timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS_PINNED);
	wd->timer.function = qdisc_watchdog;
	wd->qdisc = qdisc;
}
EXPORT_SYMBOL(qdisc_watchdog_init);

void qdisc_watchdog_schedule_ns(struct qdisc_watchdog *wd, u64 expires)
{
	if (test_bit(__QDISC_STATE_DEACTIVATED,
		     &qdisc_root_sleeping(wd->qdisc)->state))
		return;

	qdisc_throttled(wd->qdisc);

	hrtimer_start(&wd->timer,
		      ns_to_ktime(expires),
		      HRTIMER_MODE_ABS_PINNED);
}
EXPORT_SYMBOL(qdisc_watchdog_schedule_ns);

void qdisc_watchdog_cancel(struct qdisc_watchdog *wd)
{
	hrtimer_cancel(&wd->timer);
	qdisc_unthrottled(wd->qdisc);
}
EXPORT_SYMBOL(qdisc_watchdog_cancel);

static struct hlist_head *qdisc_class_hash_alloc(unsigned int n)
{
	unsigned int size = n * sizeof(struct hlist_head), i;
	struct hlist_head *h;

	if (size <= PAGE_SIZE)
		h = kmalloc(size, GFP_KERNEL);
	else
		h = (struct hlist_head *)
			__get_free_pages(GFP_KERNEL, get_order(size));

	if (h != NULL) {
		for (i = 0; i < n; i++)
			INIT_HLIST_HEAD(&h[i]);
	}
	return h;
}

static void qdisc_class_hash_free(struct hlist_head *h, unsigned int n)
{
	unsigned int size = n * sizeof(struct hlist_head);

	if (size <= PAGE_SIZE)
		kfree(h);
	else
		free_pages((unsigned long)h, get_order(size));
}

void qdisc_class_hash_grow(struct Qdisc *sch, struct Qdisc_class_hash *clhash)
{
	struct Qdisc_class_common *cl;
	struct hlist_node *next;
	struct hlist_head *nhash, *ohash;
	unsigned int nsize, nmask, osize;
	unsigned int i, h;

	/* Rehash when load factor exceeds 0.75 */
	if (clhash->hashelems * 4 <= clhash->hashsize * 3)
		return;
	nsize = clhash->hashsize * 2;
	nmask = nsize - 1;
	nhash = qdisc_class_hash_alloc(nsize);
	if (nhash == NULL)
		return;

	ohash = clhash->hash;
	osize = clhash->hashsize;

	sch_tree_lock(sch);
	for (i = 0; i < osize; i++) {
		hlist_for_each_entry_safe(cl, next, &ohash[i], hnode) {
			h = qdisc_class_hash(cl->classid, nmask);
			hlist_add_head(&cl->hnode, &nhash[h]);
		}
	}
	clhash->hash     = nhash;
	clhash->hashsize = nsize;
	clhash->hashmask = nmask;
	sch_tree_unlock(sch);

	qdisc_class_hash_free(ohash, osize);
}
EXPORT_SYMBOL(qdisc_class_hash_grow);

int qdisc_class_hash_init(struct Qdisc_class_hash *clhash)
{
	unsigned int size = 4;

	clhash->hash = qdisc_class_hash_alloc(size);
	if (clhash->hash == NULL)
		return -ENOMEM;
	clhash->hashsize  = size;
	clhash->hashmask  = size - 1;
	clhash->hashelems = 0;
	return 0;
}
EXPORT_SYMBOL(qdisc_class_hash_init);

void qdisc_class_hash_destroy(struct Qdisc_class_hash *clhash)
{
	qdisc_class_hash_free(clhash->hash, clhash->hashsize);
}
EXPORT_SYMBOL(qdisc_class_hash_destroy);

void qdisc_class_hash_insert(struct Qdisc_class_hash *clhash,
			     struct Qdisc_class_common *cl)
{
	unsigned int h;

	INIT_HLIST_NODE(&cl->hnode);
	h = qdisc_class_hash(cl->classid, clhash->hashmask);
	hlist_add_head(&cl->hnode, &clhash->hash[h]);
	clhash->hashelems++;
}
EXPORT_SYMBOL(qdisc_class_hash_insert);

void qdisc_class_hash_remove(struct Qdisc_class_hash *clhash,
			     struct Qdisc_class_common *cl)
{
	hlist_del(&cl->hnode);
	clhash->hashelems--;
}
EXPORT_SYMBOL(qdisc_class_hash_remove);

/* Allocate an unique handle from space managed by kernel
 * Possible range is [8000-FFFF]:0000 (0x8000 values)
 */
static u32 qdisc_alloc_handle(struct net_device *dev)
{
	int i = 0x8000;
	static u32 autohandle = TC_H_MAKE(0x80000000U, 0);

	do {
		autohandle += TC_H_MAKE(0x10000U, 0);
		if (autohandle == TC_H_MAKE(TC_H_ROOT, 0))
			autohandle = TC_H_MAKE(0x80000000U, 0);
		if (!qdisc_lookup(dev, autohandle))
			return autohandle;
		cond_resched();
	} while	(--i > 0);

	return 0;
}

void qdisc_tree_decrease_qlen(struct Qdisc *sch, unsigned int n)
{
	const struct Qdisc_class_ops *cops;
	unsigned long cl;
	u32 parentid;
	int drops;

	if (n == 0)
		return;
	drops = max_t(int, n, 0);
	while ((parentid = sch->parent)) {
		if (TC_H_MAJ(parentid) == TC_H_MAJ(TC_H_INGRESS))
			return;

		sch = qdisc_lookup(qdisc_dev(sch), TC_H_MAJ(parentid));
		if (sch == NULL) {
			WARN_ON(parentid != TC_H_ROOT);
			return;
		}
		cops = sch->ops->cl_ops;
		if (cops->qlen_notify) {
			cl = cops->get(sch, parentid);
			cops->qlen_notify(sch, cl);
			cops->put(sch, cl);
		}
		sch->q.qlen -= n;
		__qdisc_qstats_drop(sch, drops);
	}
}
EXPORT_SYMBOL(qdisc_tree_decrease_qlen);

static void notify_and_destroy(struct net *net, struct sk_buff *skb,
			       struct nlmsghdr *n, u32 clid,
			       struct Qdisc *old, struct Qdisc *new)
{
	if (new || old)
		qdisc_notify(net, skb, n, clid, old, new);

	if (old)
		qdisc_destroy(old);
}

/* Graft qdisc "new" to class "classid" of qdisc "parent" or
 * to device "dev".
 *
 * When appropriate send a netlink notification using 'skb'
 * and "n".
 *
 * On success, destroy old qdisc.
 */

static int qdisc_graft(struct net_device *dev, struct Qdisc *parent,
		       struct sk_buff *skb, struct nlmsghdr *n, u32 classid,
		       struct Qdisc *new, struct Qdisc *old)
{
	struct Qdisc *q = old;
	struct net *net = dev_net(dev);
	int err = 0;

	if (parent == NULL) {
		unsigned int i, num_q, ingress;

		ingress = 0;
		num_q = dev->num_tx_queues;
		if ((q && q->flags & TCQ_F_INGRESS) ||
		    (new && new->flags & TCQ_F_INGRESS)) {
			num_q = 1;
			ingress = 1;
			if (!dev_ingress_queue(dev))
				return -ENOENT;
		}

		if (dev->flags & IFF_UP)
			dev_deactivate(dev);

		if (new && new->ops->attach) {
			new->ops->attach(new);
			num_q = 0;
		}

		for (i = 0; i < num_q; i++) {
			struct netdev_queue *dev_queue = dev_ingress_queue(dev);

			if (!ingress)
				dev_queue = netdev_get_tx_queue(dev, i);

			old = dev_graft_qdisc(dev_queue, new);
			if (new && i > 0)
				atomic_inc(&new->refcnt);

			if (!ingress)
				qdisc_destroy(old);
		}

		if (!ingress) {
			notify_and_destroy(net, skb, n, classid,
					   dev->qdisc, new);
			if (new && !new->ops->attach)
				atomic_inc(&new->refcnt);
			dev->qdisc = new ? : &noop_qdisc;
		} else {
			notify_and_destroy(net, skb, n, classid, old, new);
		}

		if (dev->flags & IFF_UP)
			dev_activate(dev);
	} else {
		const struct Qdisc_class_ops *cops = parent->ops->cl_ops;

		err = -EOPNOTSUPP;
		if (cops && cops->graft) {
			unsigned long cl = cops->get(parent, classid);
			if (cl) {
				err = cops->graft(parent, cl, new, &old);
				cops->put(parent, cl);
			} else
				err = -ENOENT;
		}
		if (!err)
			notify_and_destroy(net, skb, n, classid, old, new);
	}
	return err;
}

/* lockdep annotation is needed for ingress; egress gets it only for name */
static struct lock_class_key qdisc_tx_lock;
static struct lock_class_key qdisc_rx_lock;

/*
   Allocate and initialize new qdisc.

   Parameters are passed via opt.
 */

static struct Qdisc *
qdisc_create(struct net_device *dev, struct netdev_queue *dev_queue,
	     struct Qdisc *p, u32 parent, u32 handle,
	     struct nlattr **tca, int *errp)
{
	int err;
	struct nlattr *kind = tca[TCA_KIND];
	struct Qdisc *sch;
	struct Qdisc_ops *ops;
	struct qdisc_size_table *stab;

	ops = qdisc_lookup_ops(kind);
#ifdef CONFIG_MODULES
	if (ops == NULL && kind != NULL) {
		char name[IFNAMSIZ];
		if (nla_strlcpy(name, kind, IFNAMSIZ) < IFNAMSIZ) {
			/* We dropped the RTNL semaphore in order to
			 * perform the module load.  So, even if we
			 * succeeded in loading the module we have to
			 * tell the caller to replay the request.  We
			 * indicate this using -EAGAIN.
			 * We replay the request because the device may
			 * go away in the mean time.
			 */
			rtnl_unlock();
			request_module("sch_%s", name);
			rtnl_lock();
			ops = qdisc_lookup_ops(kind);
			if (ops != NULL) {
				/* We will try again qdisc_lookup_ops,
				 * so don't keep a reference.
				 */
				module_put(ops->owner);
				err = -EAGAIN;
				goto err_out;
			}
		}
	}
#endif

	err = -ENOENT;
	if (ops == NULL)
		goto err_out;

	sch = qdisc_alloc(dev_queue, ops);
	if (IS_ERR(sch)) {
		err = PTR_ERR(sch);
		goto err_out2;
	}

	sch->parent = parent;

	if (handle == TC_H_INGRESS) {
		sch->flags |= TCQ_F_INGRESS;
		handle = TC_H_MAKE(TC_H_INGRESS, 0);
		lockdep_set_class(qdisc_lock(sch), &qdisc_rx_lock);
	} else {
		if (handle == 0) {
			handle = qdisc_alloc_handle(dev);
			err = -ENOMEM;
			if (handle == 0)
				goto err_out3;
		}
		lockdep_set_class(qdisc_lock(sch), &qdisc_tx_lock);
		if (!netif_is_multiqueue(dev))
			sch->flags |= TCQ_F_ONETXQUEUE;
	}

	sch->handle = handle;

	if (!ops->init || (err = ops->init(sch, tca[TCA_OPTIONS])) == 0) {
		if (qdisc_is_percpu_stats(sch)) {
			sch->cpu_bstats =
				alloc_percpu(struct gnet_stats_basic_cpu);
			if (!sch->cpu_bstats)
				goto err_out4;

			sch->cpu_qstats = alloc_percpu(struct gnet_stats_queue);
			if (!sch->cpu_qstats)
				goto err_out4;
		}

		if (tca[TCA_STAB]) {
			stab = qdisc_get_stab(tca[TCA_STAB]);
			if (IS_ERR(stab)) {
				err = PTR_ERR(stab);
				goto err_out4;
			}
			rcu_assign_pointer(sch->stab, stab);
		}
		if (tca[TCA_RATE]) {
			spinlock_t *root_lock;

			err = -EOPNOTSUPP;
			if (sch->flags & TCQ_F_MQROOT)
				goto err_out4;

			if ((sch->parent != TC_H_ROOT) &&
			    !(sch->flags & TCQ_F_INGRESS) &&
			    (!p || !(p->flags & TCQ_F_MQROOT)))
				root_lock = qdisc_root_sleeping_lock(sch);
			else
				root_lock = qdisc_lock(sch);

			err = gen_new_estimator(&sch->bstats,
						sch->cpu_bstats,
						&sch->rate_est,
						root_lock,
						tca[TCA_RATE]);
			if (err)
				goto err_out4;
		}

		qdisc_list_add(sch);

		return sch;
	}
err_out3:
	dev_put(dev);
	kfree((char *) sch - sch->padded);
err_out2:
	module_put(ops->owner);
err_out:
	*errp = err;
	return NULL;

err_out4:
	free_percpu(sch->cpu_bstats);
	free_percpu(sch->cpu_qstats);
	/*
	 * Any broken qdiscs that would require a ops->reset() here?
	 * The qdisc was never in action so it shouldn't be necessary.
	 */
	qdisc_put_stab(rtnl_dereference(sch->stab));
	if (ops->destroy)
		ops->destroy(sch);
	goto err_out3;
}

static int qdisc_change(struct Qdisc *sch, struct nlattr **tca)
{
	struct qdisc_size_table *ostab, *stab = NULL;
	int err = 0;

	if (tca[TCA_OPTIONS]) {
		if (sch->ops->change == NULL)
			return -EINVAL;
		err = sch->ops->change(sch, tca[TCA_OPTIONS]);
		if (err)
			return err;
	}

	if (tca[TCA_STAB]) {
		stab = qdisc_get_stab(tca[TCA_STAB]);
		if (IS_ERR(stab))
			return PTR_ERR(stab);
	}

	ostab = rtnl_dereference(sch->stab);
	rcu_assign_pointer(sch->stab, stab);
	qdisc_put_stab(ostab);

	if (tca[TCA_RATE]) {
		/* NB: ignores errors from replace_estimator
		   because change can't be undone. */
		if (sch->flags & TCQ_F_MQROOT)
			goto out;
		gen_replace_estimator(&sch->bstats,
				      sch->cpu_bstats,
				      &sch->rate_est,
				      qdisc_root_sleeping_lock(sch),
				      tca[TCA_RATE]);
	}
out:
	return 0;
}

struct check_loop_arg {
	struct qdisc_walker	w;
	struct Qdisc		*p;
	int			depth;
};

static int check_loop_fn(struct Qdisc *q, unsigned long cl, struct qdisc_walker *w);

static int check_loop(struct Qdisc *q, struct Qdisc *p, int depth)
{
	struct check_loop_arg	arg;

	if (q->ops->cl_ops == NULL)
		return 0;

	arg.w.stop = arg.w.skip = arg.w.count = 0;
	arg.w.fn = check_loop_fn;
	arg.depth = depth;
	arg.p = p;
	q->ops->cl_ops->walk(q, &arg.w);
	return arg.w.stop ? -ELOOP : 0;
}

static int
check_loop_fn(struct Qdisc *q, unsigned long cl, struct qdisc_walker *w)
{
	struct Qdisc *leaf;
	const struct Qdisc_class_ops *cops = q->ops->cl_ops;
	struct check_loop_arg *arg = (struct check_loop_arg *)w;

	leaf = cops->leaf(q, cl);
	if (leaf) {
		if (leaf == arg->p || arg->depth > 7)
			return -ELOOP;
		return check_loop(leaf, arg->p, arg->depth + 1);
	}
	return 0;
}

/*
 * Delete/get qdisc.
 */

static int tc_get_qdisc(struct sk_buff *skb, struct nlmsghdr *n)
{
	struct net *net = sock_net(skb->sk);
	struct tcmsg *tcm = nlmsg_data(n);
	struct nlattr *tca[TCA_MAX + 1];
	struct net_device *dev;
	u32 clid;
	struct Qdisc *q = NULL;
	struct Qdisc *p = NULL;
	int err;

	if ((n->nlmsg_type != RTM_GETQDISC) &&
	    !netlink_ns_capable(skb, net->user_ns, CAP_NET_ADMIN))
		return -EPERM;

	err = nlmsg_parse(n, sizeof(*tcm), tca, TCA_MAX, NULL);
	if (err < 0)
		return err;

	dev = __dev_get_by_index(net, tcm->tcm_ifindex);
	if (!dev)
		return -ENODEV;

	clid = tcm->tcm_parent;
	if (clid) {
		if (clid != TC_H_ROOT) {
			if (TC_H_MAJ(clid) != TC_H_MAJ(TC_H_INGRESS)) {
				p = qdisc_lookup(dev, TC_H_MAJ(clid));
				if (!p)
					return -ENOENT;
				q = qdisc_leaf(p, clid);
			} else if (dev_ingress_queue(dev)) {
				q = dev_ingress_queue(dev)->qdisc_sleeping;
			}
		} else {
			q = dev->qdisc;
		}
		if (!q)
			return -ENOENT;

		if (tcm->tcm_handle && q->handle != tcm->tcm_handle)
			return -EINVAL;
	} else {
		q = qdisc_lookup(dev, tcm->tcm_handle);
		if (!q)
			return -ENOENT;
	}

	if (tca[TCA_KIND] && nla_strcmp(tca[TCA_KIND], q->ops->id))
		return -EINVAL;

	if (n->nlmsg_type == RTM_DELQDISC) {
		if (!clid)
			return -EINVAL;
		if (q->handle == 0)
			return -ENOENT;
		err = qdisc_graft(dev, p, skb, n, clid, NULL, q);
		if (err != 0)
			return err;
	} else {
		qdisc_notify(net, skb, n, clid, NULL, q);
	}
	return 0;
}

/*
 * Create/change qdisc.
 */

static int tc_modify_qdisc(struct sk_buff *skb, struct nlmsghdr *n)
{
	struct net *net = sock_net(skb->sk);
	struct tcmsg *tcm;
	struct nlattr *tca[TCA_MAX + 1];
	struct net_device *dev;
	u32 clid;
	struct Qdisc *q, *p;
	int err;

	if (!netlink_ns_capable(skb, net->user_ns, CAP_NET_ADMIN))
		return -EPERM;

replay:
	/* Reinit, just in case something touches this. */
	err = nlmsg_parse(n, sizeof(*tcm), tca, TCA_MAX, NULL);
	if (err < 0)
		return err;

	tcm = nlmsg_data(n);
	clid = tcm->tcm_parent;
	q = p = NULL;

	dev = __dev_get_by_index(net, tcm->tcm_ifindex);
	if (!dev)
		return -ENODEV;


	if (clid) {
		if (clid != TC_H_ROOT) {
			if (clid != TC_H_INGRESS) {
				p = qdisc_lookup(dev, TC_H_MAJ(clid));
				if (!p)
					return -ENOENT;
				q = qdisc_leaf(p, clid);
			} else if (dev_ingress_queue_create(dev)) {
				q = dev_ingress_queue(dev)->qdisc_sleeping;
			}
		} else {
			q = dev->qdisc;
		}

		/* It may be default qdisc, ignore it */
		if (q && q->handle == 0)
			q = NULL;

		if (!q || !tcm->tcm_handle || q->handle != tcm->tcm_handle) {
			if (tcm->tcm_handle) {
				if (q && !(n->nlmsg_flags & NLM_F_REPLACE))
					return -EEXIST;
				if (TC_H_MIN(tcm->tcm_handle))
					return -EINVAL;
				q = qdisc_lookup(dev, tcm->tcm_handle);
				if (!q)
					goto create_n_graft;
				if (n->nlmsg_flags & NLM_F_EXCL)
					return -EEXIST;
				if (tca[TCA_KIND] && nla_strcmp(tca[TCA_KIND], q->ops->id))
					return -EINVAL;
				if (q == p ||
				    (p && check_loop(q, p, 0)))
					return -ELOOP;
				atomic_inc(&q->refcnt);
				goto graft;
			} else {
				if (!q)
					goto create_n_graft;

				/* This magic test requires explanation.
				 *
				 *   We know, that some child q is already
				 *   attached to this parent and have choice:
				 *   either to change it or to create/graft new one.
				 *
				 *   1. We are allowed to create/graft only
				 *   if CREATE and REPLACE flags are set.
				 *
				 *   2. If EXCL is set, requestor wanted to say,
				 *   that qdisc tcm_handle is not expected
				 *   to exist, so that we choose create/graft too.
				 *
				 *   3. The last case is when no flags are set.
				 *   Alas, it is sort of hole in API, we
				 *   cannot decide what to do unambiguously.
				 *   For now we select create/graft, if
				 *   user gave KIND, which does not match existing.
				 */
				if ((n->nlmsg_flags & NLM_F_CREATE) &&
				    (n->nlmsg_flags & NLM_F_REPLACE) &&
				    ((n->nlmsg_flags & NLM_F_EXCL) ||
				     (tca[TCA_KIND] &&
				      nla_strcmp(tca[TCA_KIND], q->ops->id))))
					goto create_n_graft;
			}
		}
	} else {
		if (!tcm->tcm_handle)
			return -EINVAL;
		q = qdisc_lookup(dev, tcm->tcm_handle);
	}

	/* Change qdisc parameters */
	if (q == NULL)
		return -ENOENT;
	if (n->nlmsg_flags & NLM_F_EXCL)
		return -EEXIST;
	if (tca[TCA_KIND] && nla_strcmp(tca[TCA_KIND], q->ops->id))
		return -EINVAL;
	err = qdisc_change(q, tca);
	if (err == 0)
		qdisc_notify(net, skb, n, clid, NULL, q);
	return err;

create_n_graft:
	if (!(n->nlmsg_flags & NLM_F_CREATE))
		return -ENOENT;
	if (clid == TC_H_INGRESS) {
		if (dev_ingress_queue(dev))
			q = qdisc_create(dev, dev_ingress_queue(dev), p,
					 tcm->tcm_parent, tcm->tcm_parent,
					 tca, &err);
		else
			err = -ENOENT;
	} else {
		struct netdev_queue *dev_queue;

		if (p && p->ops->cl_ops && p->ops->cl_ops->select_queue)
			dev_queue = p->ops->cl_ops->select_queue(p, tcm);
		else if (p)
			dev_queue = p->dev_queue;
		else
			dev_queue = netdev_get_tx_queue(dev, 0);

		q = qdisc_create(dev, dev_queue, p,
				 tcm->tcm_parent, tcm->tcm_handle,
				 tca, &err);
	}
	if (q == NULL) {
		if (err == -EAGAIN)
			goto replay;
		return err;
	}

graft:
	err = qdisc_graft(dev, p, skb, n, clid, q, NULL);
	if (err) {
		if (q)
			qdisc_destroy(q);
		return err;
	}

	return 0;
}

static int tc_fill_qdisc(struct sk_buff *skb, struct Qdisc *q, u32 clid,
			 u32 portid, u32 seq, u16 flags, int event)
{
	struct gnet_stats_basic_cpu __percpu *cpu_bstats = NULL;
	struct gnet_stats_queue __percpu *cpu_qstats = NULL;
	struct tcmsg *tcm;
	struct nlmsghdr  *nlh;
	unsigned char *b = skb_tail_pointer(skb);
	struct gnet_dump d;
	struct qdisc_size_table *stab;
	__u32 qlen;

	cond_resched();
	nlh = nlmsg_put(skb, portid, seq, event, sizeof(*tcm), flags);
	if (!nlh)
		goto out_nlmsg_trim;
	tcm = nlmsg_data(nlh);
	tcm->tcm_family = AF_UNSPEC;
	tcm->tcm__pad1 = 0;
	tcm->tcm__pad2 = 0;
	tcm->tcm_ifindex = qdisc_dev(q)->ifindex;
	tcm->tcm_parent = clid;
	tcm->tcm_handle = q->handle;
	tcm->tcm_info = atomic_read(&q->refcnt);
	if (nla_put_string(skb, TCA_KIND, q->ops->id))
		goto nla_put_failure;
	if (q->ops->dump && q->ops->dump(q, skb) < 0)
		goto nla_put_failure;
	qlen = q->q.qlen;

	stab = rtnl_dereference(q->stab);
	if (stab && qdisc_dump_stab(skb, stab) < 0)
		goto nla_put_failure;

	if (gnet_stats_start_copy_compat(skb, TCA_STATS2, TCA_STATS, TCA_XSTATS,
					 qdisc_root_sleeping_lock(q), &d) < 0)
		goto nla_put_failure;

	if (q->ops->dump_stats && q->ops->dump_stats(q, &d) < 0)
		goto nla_put_failure;

	if (qdisc_is_percpu_stats(q)) {
		cpu_bstats = q->cpu_bstats;
		cpu_qstats = q->cpu_qstats;
	}

	if (gnet_stats_copy_basic(&d, cpu_bstats, &q->bstats) < 0 ||
	    gnet_stats_copy_rate_est(&d, &q->bstats, &q->rate_est) < 0 ||
	    gnet_stats_copy_queue(&d, cpu_qstats, &q->qstats, qlen) < 0)
		goto nla_put_failure;

	if (gnet_stats_finish_copy(&d) < 0)
		goto nla_put_failure;

	nlh->nlmsg_len = skb_tail_pointer(skb) - b;
	return skb->len;

out_nlmsg_trim:
nla_put_failure:
	nlmsg_trim(skb, b);
	return -1;
}

static bool tc_qdisc_dump_ignore(struct Qdisc *q)
{
	return (q->flags & TCQ_F_BUILTIN) ? true : false;
}

static int qdisc_notify(struct net *net, struct sk_buff *oskb,
			struct nlmsghdr *n, u32 clid,
			struct Qdisc *old, struct Qdisc *new)
{
	struct sk_buff *skb;
	u32 portid = oskb ? NETLINK_CB(oskb).portid : 0;

	skb = alloc_skb(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!skb)
		return -ENOBUFS;

	if (old && !tc_qdisc_dump_ignore(old)) {
		if (tc_fill_qdisc(skb, old, clid, portid, n->nlmsg_seq,
				  0, RTM_DELQDISC) < 0)
			goto err_out;
	}
	if (new && !tc_qdisc_dump_ignore(new)) {
		if (tc_fill_qdisc(skb, new, clid, portid, n->nlmsg_seq,
				  old ? NLM_F_REPLACE : 0, RTM_NEWQDISC) < 0)
			goto err_out;
	}

	if (skb->len)
		return rtnetlink_send(skb, net, portid, RTNLGRP_TC,
				      n->nlmsg_flags & NLM_F_ECHO);

err_out:
	kfree_skb(skb);
	return -EINVAL;
}

static int tc_dump_qdisc_root(struct Qdisc *root, struct sk_buff *skb,
			      struct netlink_callback *cb,
			      int *q_idx_p, int s_q_idx)
{
	int ret = 0, q_idx = *q_idx_p;
	struct Qdisc *q;

	if (!root)
		return 0;

	q = root;
	if (q_idx < s_q_idx) {
		q_idx++;
	} else {
		if (!tc_qdisc_dump_ignore(q) &&
		    tc_fill_qdisc(skb, q, q->parent, NETLINK_CB(cb->skb).portid,
				  cb->nlh->nlmsg_seq, NLM_F_MULTI, RTM_NEWQDISC) <= 0)
			goto done;
		q_idx++;
	}
	list_for_each_entry(q, &root->list, list) {
		if (q_idx < s_q_idx) {
			q_idx++;
			continue;
		}
		if (!tc_qdisc_dump_ignore(q) &&
		    tc_fill_qdisc(skb, q, q->parent, NETLINK_CB(cb->skb).portid,
				  cb->nlh->nlmsg_seq, NLM_F_MULTI, RTM_NEWQDISC) <= 0)
			goto done;
		q_idx++;
	}

out:
	*q_idx_p = q_idx;
	return ret;
done:
	ret = -1;
	goto out;
}

static int tc_dump_qdisc(struct sk_buff *skb, struct netlink_callback *cb)
{
	struct net *net = sock_net(skb->sk);
	int idx, q_idx;
	int s_idx, s_q_idx;
	struct net_device *dev;

	s_idx = cb->args[0];
	s_q_idx = q_idx = cb->args[1];

	idx = 0;
	ASSERT_RTNL();
	for_each_netdev(net, dev) {
		struct netdev_queue *dev_queue;

		if (idx < s_idx)
			goto cont;
		if (idx > s_idx)
			s_q_idx = 0;
		q_idx = 0;

		if (tc_dump_qdisc_root(dev->qdisc, skb, cb, &q_idx, s_q_idx) < 0)
			goto done;

		dev_queue = dev_ingress_queue(dev);
		if (dev_queue &&
		    tc_dump_qdisc_root(dev_queue->qdisc_sleeping, skb, cb,
				       &q_idx, s_q_idx) < 0)
			goto done;

cont:
		idx++;
	}

done:
	cb->args[0] = idx;
	cb->args[1] = q_idx;

	return skb->len;
}



/************************************************
 *	Traffic classes manipulation.		*
 ************************************************/



static int tc_ctl_tclass(struct sk_buff *skb, struct nlmsghdr *n)
{
	struct net *net = sock_net(skb->sk);
	struct tcmsg *tcm = nlmsg_data(n);
	struct nlattr *tca[TCA_MAX + 1];
	struct net_device *dev;
	struct Qdisc *q = NULL;
	const struct Qdisc_class_ops *cops;
	unsigned long cl = 0;
	unsigned long new_cl;
	u32 portid;
	u32 clid;
	u32 qid;
	int err;

	if ((n->nlmsg_type != RTM_GETTCLASS) &&
	    !netlink_ns_capable(skb, net->user_ns, CAP_NET_ADMIN))
		return -EPERM;

	err = nlmsg_parse(n, sizeof(*tcm), tca, TCA_MAX, NULL);
	if (err < 0)
		return err;

	dev = __dev_get_by_index(net, tcm->tcm_ifindex);
	if (!dev)
		return -ENODEV;

	/*
	   parent == TC_H_UNSPEC - unspecified parent.
	   parent == TC_H_ROOT   - class is root, which has no parent.
	   parent == X:0	 - parent is root class.
	   parent == X:Y	 - parent is a node in hierarchy.
	   parent == 0:Y	 - parent is X:Y, where X:0 is qdisc.

	   handle == 0:0	 - generate handle from kernel pool.
	   handle == 0:Y	 - class is X:Y, where X:0 is qdisc.
	   handle == X:Y	 - clear.
	   handle == X:0	 - root class.
	 */

	/* Step 1. Determine qdisc handle X:0 */

	portid = tcm->tcm_parent;
	clid = tcm->tcm_handle;
	qid = TC_H_MAJ(clid);

	if (portid != TC_H_ROOT) {
		u32 qid1 = TC_H_MAJ(portid);

		if (qid && qid1) {
			/* If both majors are known, they must be identical. */
			if (qid != qid1)
				return -EINVAL;
		} else if (qid1) {
			qid = qid1;
		} else if (qid == 0)
			qid = dev->qdisc->handle;

		/* Now qid is genuine qdisc handle consistent
		 * both with parent and child.
		 *
		 * TC_H_MAJ(portid) still may be unspecified, complete it now.
		 */
		if (portid)
			portid = TC_H_MAKE(qid, portid);
	} else {
		if (qid == 0)
			qid = dev->qdisc->handle;
	}

	/* OK. Locate qdisc */
	q = qdisc_lookup(dev, qid);
	if (!q)
		return -ENOENT;

	/* An check that it supports classes */
	cops = q->ops->cl_ops;
	if (cops == NULL)
		return -EINVAL;

	/* Now try to get class */
	if (clid == 0) {
		if (portid == TC_H_ROOT)
			clid = qid;
	} else
		clid = TC_H_MAKE(qid, clid);

	if (clid)
		cl = cops->get(q, clid);

	if (cl == 0) {
		err = -ENOENT;
		if (n->nlmsg_type != RTM_NEWTCLASS ||
		    !(n->nlmsg_flags & NLM_F_CREATE))
			goto out;
	} else {
		switch (n->nlmsg_type) {
		case RTM_NEWTCLASS:
			err = -EEXIST;
			if (n->nlmsg_flags & NLM_F_EXCL)
				goto out;
			break;
		case RTM_DELTCLASS:
			err = -EOPNOTSUPP;
			if (cops->delete)
				err = cops->delete(q, cl);
			if (err == 0)
				tclass_notify(net, skb, n, q, cl, RTM_DELTCLASS);
			goto out;
		case RTM_GETTCLASS:
			err = tclass_notify(net, skb, n, q, cl, RTM_NEWTCLASS);
			goto out;
		default:
			err = -EINVAL;
			goto out;
		}
	}

	new_cl = cl;
	err = -EOPNOTSUPP;
	if (cops->change)
		err = cops->change(q, clid, portid, tca, &new_cl);
	if (err == 0)
		tclass_notify(net, skb, n, q, new_cl, RTM_NEWTCLASS);

out:
	if (cl)
		cops->put(q, cl);

	return err;
}


static int tc_fill_tclass(struct sk_buff *skb, struct Qdisc *q,
			  unsigned long cl,
			  u32 portid, u32 seq, u16 flags, int event)
{
	struct tcmsg *tcm;
	struct nlmsghdr  *nlh;
	unsigned char *b = skb_tail_pointer(skb);
	struct gnet_dump d;
	const struct Qdisc_class_ops *cl_ops = q->ops->cl_ops;

	cond_resched();
	nlh = nlmsg_put(skb, portid, seq, event, sizeof(*tcm), flags);
	if (!nlh)
		goto out_nlmsg_trim;
	tcm = nlmsg_data(nlh);
	tcm->tcm_family = AF_UNSPEC;
	tcm->tcm__pad1 = 0;
	tcm->tcm__pad2 = 0;
	tcm->tcm_ifindex = qdisc_dev(q)->ifindex;
	tcm->tcm_parent = q->handle;
	tcm->tcm_handle = q->handle;
	tcm->tcm_info = 0;
	if (nla_put_string(skb, TCA_KIND, q->ops->id))
		goto nla_put_failure;
	if (cl_ops->dump && cl_ops->dump(q, cl, skb, tcm) < 0)
		goto nla_put_failure;

	if (gnet_stats_start_copy_compat(skb, TCA_STATS2, TCA_STATS, TCA_XSTATS,
					 qdisc_root_sleeping_lock(q), &d) < 0)
		goto nla_put_failure;

	if (cl_ops->dump_stats && cl_ops->dump_stats(q, cl, &d) < 0)
		goto nla_put_failure;

	if (gnet_stats_finish_copy(&d) < 0)
		goto nla_put_failure;

	nlh->nlmsg_len = skb_tail_pointer(skb) - b;
	return skb->len;

out_nlmsg_trim:
nla_put_failure:
	nlmsg_trim(skb, b);
	return -1;
}

static int tclass_notify(struct net *net, struct sk_buff *oskb,
			 struct nlmsghdr *n, struct Qdisc *q,
			 unsigned long cl, int event)
{
	struct sk_buff *skb;
	u32 portid = oskb ? NETLINK_CB(oskb).portid : 0;

	skb = alloc_skb(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!skb)
		return -ENOBUFS;

	if (tc_fill_tclass(skb, q, cl, portid, n->nlmsg_seq, 0, event) < 0) {
		kfree_skb(skb);
		return -EINVAL;
	}

	return rtnetlink_send(skb, net, portid, RTNLGRP_TC,
			      n->nlmsg_flags & NLM_F_ECHO);
}

struct qdisc_dump_args {
	struct qdisc_walker	w;
	struct sk_buff		*skb;
	struct netlink_callback	*cb;
};

static int qdisc_class_dump(struct Qdisc *q, unsigned long cl, struct qdisc_walker *arg)
{
	struct qdisc_dump_args *a = (struct qdisc_dump_args *)arg;

	return tc_fill_tclass(a->skb, q, cl, NETLINK_CB(a->cb->skb).portid,
			      a->cb->nlh->nlmsg_seq, NLM_F_MULTI, RTM_NEWTCLASS);
}

static int tc_dump_tclass_qdisc(struct Qdisc *q, struct sk_buff *skb,
				struct tcmsg *tcm, struct netlink_callback *cb,
				int *t_p, int s_t)
{
	struct qdisc_dump_args arg;

	if (tc_qdisc_dump_ignore(q) ||
	    *t_p < s_t || !q->ops->cl_ops ||
	    (tcm->tcm_parent &&
	     TC_H_MAJ(tcm->tcm_parent) != q->handle)) {
		(*t_p)++;
		return 0;
	}
	if (*t_p > s_t)
		memset(&cb->args[1], 0, sizeof(cb->args)-sizeof(cb->args[0]));
	arg.w.fn = qdisc_class_dump;
	arg.skb = skb;
	arg.cb = cb;
	arg.w.stop  = 0;
	arg.w.skip = cb->args[1];
	arg.w.count = 0;
	q->ops->cl_ops->walk(q, &arg.w);
	cb->args[1] = arg.w.count;
	if (arg.w.stop)
		return -1;
	(*t_p)++;
	return 0;
}

static int tc_dump_tclass_root(struct Qdisc *root, struct sk_buff *skb,
			       struct tcmsg *tcm, struct netlink_callback *cb,
			       int *t_p, int s_t)
{
	struct Qdisc *q;

	if (!root)
		return 0;

	if (tc_dump_tclass_qdisc(root, skb, tcm, cb, t_p, s_t) < 0)
		return -1;

	list_for_each_entry(q, &root->list, list) {
		if (tc_dump_tclass_qdisc(q, skb, tcm, cb, t_p, s_t) < 0)
			return -1;
	}

	return 0;
}

static int tc_dump_tclass(struct sk_buff *skb, struct netlink_callback *cb)
{
	struct tcmsg *tcm = nlmsg_data(cb->nlh);
	struct net *net = sock_net(skb->sk);
	struct netdev_queue *dev_queue;
	struct net_device *dev;
	int t, s_t;

	if (nlmsg_len(cb->nlh) < sizeof(*tcm))
		return 0;
	dev = dev_get_by_index(net, tcm->tcm_ifindex);
	if (!dev)
		return 0;

	s_t = cb->args[0];
	t = 0;

	if (tc_dump_tclass_root(dev->qdisc, skb, tcm, cb, &t, s_t) < 0)
		goto done;

	dev_queue = dev_ingress_queue(dev);
	if (dev_queue &&
	    tc_dump_tclass_root(dev_queue->qdisc_sleeping, skb, tcm, cb,
				&t, s_t) < 0)
		goto done;

done:
	cb->args[0] = t;

	dev_put(dev);
	return skb->len;
}

/* Main classifier routine: scans classifier chain attached
 * to this qdisc, (optionally) tests for protocol and asks
 * specific classifiers.
 */
int tc_classify_compat(struct sk_buff *skb, const struct tcf_proto *tp,
		       struct tcf_result *res)
{
	__be16 protocol = skb->protocol;
	int err;

	for (; tp; tp = rcu_dereference_bh(tp->next)) {
		if (tp->protocol != protocol &&
		    tp->protocol != htons(ETH_P_ALL))
			continue;
		err = tp->classify(skb, tp, res);

		if (err >= 0) {
#ifdef CONFIG_NET_CLS_ACT
			if (err != TC_ACT_RECLASSIFY && skb->tc_verd)
				skb->tc_verd = SET_TC_VERD(skb->tc_verd, 0);
#endif
			return err;
		}
	}
	return -1;
}
EXPORT_SYMBOL(tc_classify_compat);

int tc_classify(struct sk_buff *skb, const struct tcf_proto *tp,
		struct tcf_result *res)
{
	int err = 0;
#ifdef CONFIG_NET_CLS_ACT
	const struct tcf_proto *otp = tp;
reclassify:
#endif

	err = tc_classify_compat(skb, tp, res);
#ifdef CONFIG_NET_CLS_ACT
	if (err == TC_ACT_RECLASSIFY) {
		u32 verd = G_TC_VERD(skb->tc_verd);
		tp = otp;

		if (verd++ >= MAX_REC_LOOP) {
			net_notice_ratelimited("%s: packet reclassify loop rule prio %u protocol %02x\n",
					       tp->q->ops->id,
					       tp->prio & 0xffff,
					       ntohs(tp->protocol));
			return TC_ACT_SHOT;
		}
		skb->tc_verd = SET_TC_VERD(skb->tc_verd, verd);
		goto reclassify;
	}
#endif
	return err;
}
EXPORT_SYMBOL(tc_classify);

void tcf_destroy(struct tcf_proto *tp)
{
	tp->ops->destroy(tp);
	module_put(tp->ops->owner);
	kfree_rcu(tp, rcu);
}

void tcf_destroy_chain(struct tcf_proto __rcu **fl)
{
	struct tcf_proto *tp;

	while ((tp = rtnl_dereference(*fl)) != NULL) {
		RCU_INIT_POINTER(*fl, tp->next);
		tcf_destroy(tp);
	}
}
EXPORT_SYMBOL(tcf_destroy_chain);

#ifdef CONFIG_PROC_FS
static int psched_show(struct seq_file *seq, void *v)
{
	struct timespec ts;

	hrtimer_get_res(CLOCK_MONOTONIC, &ts);
	seq_printf(seq, "%08x %08x %08x %08x\n",
		   (u32)NSEC_PER_USEC, (u32)PSCHED_TICKS2NS(1),
		   1000000,
		   (u32)NSEC_PER_SEC/(u32)ktime_to_ns(timespec_to_ktime(ts)));

	return 0;
}

static int psched_open(struct inode *inode, struct file *file)
{
	return single_open(file, psched_show, NULL);
}

static const struct file_operations psched_fops = {
	.owner = THIS_MODULE,
	.open = psched_open,
	.read  = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __net_init psched_net_init(struct net *net)
{
	struct proc_dir_entry *e;

	e = proc_create("psched", 0, net->proc_net, &psched_fops);
	if (e == NULL)
		return -ENOMEM;

	return 0;
}

static void __net_exit psched_net_exit(struct net *net)
{
	remove_proc_entry("psched", net->proc_net);
}
#else
static int __net_init psched_net_init(struct net *net)
{
	return 0;
}

static void __net_exit psched_net_exit(struct net *net)
{
}
#endif

static struct pernet_operations psched_net_ops = {
	.init = psched_net_init,
	.exit = psched_net_exit,
};

static int __init pktsched_init(void)
{
	int err;

	err = register_pernet_subsys(&psched_net_ops);
	if (err) {
		pr_err("pktsched_init: "
		       "cannot initialize per netns operations\n");
		return err;
	}

	register_qdisc(&pfifo_fast_ops);
	register_qdisc(&pfifo_qdisc_ops);
	register_qdisc(&bfifo_qdisc_ops);
	register_qdisc(&pfifo_head_drop_qdisc_ops);
	register_qdisc(&mq_qdisc_ops);

	rtnl_register(PF_UNSPEC, RTM_NEWQDISC, tc_modify_qdisc, NULL, NULL);
	rtnl_register(PF_UNSPEC, RTM_DELQDISC, tc_get_qdisc, NULL, NULL);
	rtnl_register(PF_UNSPEC, RTM_GETQDISC, tc_get_qdisc, tc_dump_qdisc, NULL);
	rtnl_register(PF_UNSPEC, RTM_NEWTCLASS, tc_ctl_tclass, NULL, NULL);
	rtnl_register(PF_UNSPEC, RTM_DELTCLASS, tc_ctl_tclass, NULL, NULL);
	rtnl_register(PF_UNSPEC, RTM_GETTCLASS, tc_ctl_tclass, tc_dump_tclass, NULL);

	return 0;
}

subsys_initcall(pktsched_init);
