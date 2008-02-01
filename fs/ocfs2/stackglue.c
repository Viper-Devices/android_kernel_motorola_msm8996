/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * stackglue.c
 *
 * Code which implements an OCFS2 specific interface to underlying
 * cluster stacks.
 *
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kmod.h>

#include "stackglue.h"

static struct ocfs2_locking_protocol *lproto;
static DEFINE_SPINLOCK(ocfs2_stack_lock);
static LIST_HEAD(ocfs2_stack_list);

/*
 * The stack currently in use.  If not null, active_stack->sp_count > 0,
 * the module is pinned, and the locking protocol cannot be changed.
 */
static struct ocfs2_stack_plugin *active_stack;

static struct ocfs2_stack_plugin *ocfs2_stack_lookup(const char *name)
{
	struct ocfs2_stack_plugin *p;

	assert_spin_locked(&ocfs2_stack_lock);

	list_for_each_entry(p, &ocfs2_stack_list, sp_list) {
		if (!strcmp(p->sp_name, name))
			return p;
	}

	return NULL;
}

static int ocfs2_stack_driver_request(const char *name)
{
	int rc;
	struct ocfs2_stack_plugin *p;

	spin_lock(&ocfs2_stack_lock);

	if (active_stack) {
		/*
		 * If the active stack isn't the one we want, it cannot
		 * be selected right now.
		 */
		if (!strcmp(active_stack->sp_name, name))
			rc = 0;
		else
			rc = -EBUSY;
		goto out;
	}

	p = ocfs2_stack_lookup(name);
	if (!p || !try_module_get(p->sp_owner)) {
		rc = -ENOENT;
		goto out;
	}

	/* Ok, the stack is pinned */
	p->sp_count++;
	active_stack = p;

	rc = 0;

out:
	spin_unlock(&ocfs2_stack_lock);
	return rc;
}

/*
 * This function looks up the appropriate stack and makes it active.  If
 * there is no stack, it tries to load it.  It will fail if the stack still
 * cannot be found.  It will also fail if a different stack is in use.
 */
static int ocfs2_stack_driver_get(const char *name)
{
	int rc;

	rc = ocfs2_stack_driver_request(name);
	if (rc == -ENOENT) {
		request_module("ocfs2_stack_%s", name);
		rc = ocfs2_stack_driver_request(name);
	}

	if (rc == -ENOENT) {
		printk(KERN_ERR
		       "ocfs2: Cluster stack driver \"%s\" cannot be found\n",
		       name);
	} else if (rc == -EBUSY) {
		printk(KERN_ERR
		       "ocfs2: A different cluster stack driver is in use\n");
	}

	return rc;
}

static void ocfs2_stack_driver_put(void)
{
	spin_lock(&ocfs2_stack_lock);
	BUG_ON(active_stack == NULL);
	BUG_ON(active_stack->sp_count == 0);

	active_stack->sp_count--;
	if (!active_stack->sp_count) {
		module_put(active_stack->sp_owner);
		active_stack = NULL;
	}
	spin_unlock(&ocfs2_stack_lock);
}

int ocfs2_stack_glue_register(struct ocfs2_stack_plugin *plugin)
{
	int rc;

	spin_lock(&ocfs2_stack_lock);
	if (!ocfs2_stack_lookup(plugin->sp_name)) {
		plugin->sp_count = 0;
		plugin->sp_proto = lproto;
		list_add(&plugin->sp_list, &ocfs2_stack_list);
		printk(KERN_INFO "ocfs2: Registered cluster interface %s\n",
		       plugin->sp_name);
		rc = 0;
	} else {
		printk(KERN_ERR "ocfs2: Stack \"%s\" already registered\n",
		       plugin->sp_name);
		rc = -EEXIST;
	}
	spin_unlock(&ocfs2_stack_lock);

	return rc;
}
EXPORT_SYMBOL_GPL(ocfs2_stack_glue_register);

void ocfs2_stack_glue_unregister(struct ocfs2_stack_plugin *plugin)
{
	struct ocfs2_stack_plugin *p;

	spin_lock(&ocfs2_stack_lock);
	p = ocfs2_stack_lookup(plugin->sp_name);
	if (p) {
		BUG_ON(p != plugin);
		BUG_ON(plugin == active_stack);
		BUG_ON(plugin->sp_count != 0);
		list_del_init(&plugin->sp_list);
		printk(KERN_INFO "ocfs2: Unregistered cluster interface %s\n",
		       plugin->sp_name);
	} else {
		printk(KERN_ERR "Stack \"%s\" is not registered\n",
		       plugin->sp_name);
	}
	spin_unlock(&ocfs2_stack_lock);
}
EXPORT_SYMBOL_GPL(ocfs2_stack_glue_unregister);

void ocfs2_stack_glue_set_locking_protocol(struct ocfs2_locking_protocol *proto)
{
	struct ocfs2_stack_plugin *p;

	BUG_ON(proto == NULL);

	spin_lock(&ocfs2_stack_lock);
	BUG_ON(active_stack != NULL);

	lproto = proto;
	list_for_each_entry(p, &ocfs2_stack_list, sp_list) {
		p->sp_proto = lproto;
	}

	spin_unlock(&ocfs2_stack_lock);
}
EXPORT_SYMBOL_GPL(ocfs2_stack_glue_set_locking_protocol);


int ocfs2_dlm_lock(struct ocfs2_cluster_connection *conn,
		   int mode,
		   union ocfs2_dlm_lksb *lksb,
		   u32 flags,
		   void *name,
		   unsigned int namelen,
		   void *astarg)
{
	BUG_ON(lproto == NULL);

	return active_stack->sp_ops->dlm_lock(conn, mode, lksb, flags,
					      name, namelen, astarg);
}
EXPORT_SYMBOL_GPL(ocfs2_dlm_lock);

int ocfs2_dlm_unlock(struct ocfs2_cluster_connection *conn,
		     union ocfs2_dlm_lksb *lksb,
		     u32 flags,
		     void *astarg)
{
	BUG_ON(lproto == NULL);

	return active_stack->sp_ops->dlm_unlock(conn, lksb, flags, astarg);
}
EXPORT_SYMBOL_GPL(ocfs2_dlm_unlock);

int ocfs2_dlm_lock_status(union ocfs2_dlm_lksb *lksb)
{
	return active_stack->sp_ops->lock_status(lksb);
}
EXPORT_SYMBOL_GPL(ocfs2_dlm_lock_status);

/*
 * Why don't we cast to ocfs2_meta_lvb?  The "clean" answer is that we
 * don't cast at the glue level.  The real answer is that the header
 * ordering is nigh impossible.
 */
void *ocfs2_dlm_lvb(union ocfs2_dlm_lksb *lksb)
{
	return active_stack->sp_ops->lock_lvb(lksb);
}
EXPORT_SYMBOL_GPL(ocfs2_dlm_lvb);

void ocfs2_dlm_dump_lksb(union ocfs2_dlm_lksb *lksb)
{
	active_stack->sp_ops->dump_lksb(lksb);
}
EXPORT_SYMBOL_GPL(ocfs2_dlm_dump_lksb);

int ocfs2_cluster_connect(const char *group,
			  int grouplen,
			  void (*recovery_handler)(int node_num,
						   void *recovery_data),
			  void *recovery_data,
			  struct ocfs2_cluster_connection **conn)
{
	int rc = 0;
	struct ocfs2_cluster_connection *new_conn;

	BUG_ON(group == NULL);
	BUG_ON(conn == NULL);
	BUG_ON(recovery_handler == NULL);

	if (grouplen > GROUP_NAME_MAX) {
		rc = -EINVAL;
		goto out;
	}

	new_conn = kzalloc(sizeof(struct ocfs2_cluster_connection),
			   GFP_KERNEL);
	if (!new_conn) {
		rc = -ENOMEM;
		goto out;
	}

	memcpy(new_conn->cc_name, group, grouplen);
	new_conn->cc_namelen = grouplen;
	new_conn->cc_recovery_handler = recovery_handler;
	new_conn->cc_recovery_data = recovery_data;

	/* Start the new connection at our maximum compatibility level */
	new_conn->cc_version = lproto->lp_max_version;

	/* This will pin the stack driver if successful */
	rc = ocfs2_stack_driver_get("o2cb");
	if (rc)
		goto out_free;

	rc = active_stack->sp_ops->connect(new_conn);
	if (rc) {
		ocfs2_stack_driver_put();
		goto out_free;
	}

	*conn = new_conn;

out_free:
	if (rc)
		kfree(new_conn);

out:
	return rc;
}
EXPORT_SYMBOL_GPL(ocfs2_cluster_connect);

/* If hangup_pending is 0, the stack driver will be dropped */
int ocfs2_cluster_disconnect(struct ocfs2_cluster_connection *conn,
			     int hangup_pending)
{
	int ret;

	BUG_ON(conn == NULL);

	ret = active_stack->sp_ops->disconnect(conn, hangup_pending);

	/* XXX Should we free it anyway? */
	if (!ret) {
		kfree(conn);
		if (!hangup_pending)
			ocfs2_stack_driver_put();
	}

	return ret;
}
EXPORT_SYMBOL_GPL(ocfs2_cluster_disconnect);

void ocfs2_cluster_hangup(const char *group, int grouplen)
{
	BUG_ON(group == NULL);
	BUG_ON(group[grouplen] != '\0');

	active_stack->sp_ops->hangup(group, grouplen);

	/* cluster_disconnect() was called with hangup_pending==1 */
	ocfs2_stack_driver_put();
}
EXPORT_SYMBOL_GPL(ocfs2_cluster_hangup);

int ocfs2_cluster_this_node(unsigned int *node)
{
	return active_stack->sp_ops->this_node(node);
}
EXPORT_SYMBOL_GPL(ocfs2_cluster_this_node);


static int __init ocfs2_stack_glue_init(void)
{
	return 0;
}

static void __exit ocfs2_stack_glue_exit(void)
{
	lproto = NULL;
}

MODULE_AUTHOR("Oracle");
MODULE_DESCRIPTION("ocfs2 cluter stack glue layer");
MODULE_LICENSE("GPL");
module_init(ocfs2_stack_glue_init);
module_exit(ocfs2_stack_glue_exit);
