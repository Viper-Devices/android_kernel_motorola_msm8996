/*
 * NetLabel Unlabeled Support
 *
 * This file defines functions for dealing with unlabeled packets for the
 * NetLabel system.  The NetLabel system manages static and dynamic label
 * mappings for network protocols such as CIPSO and RIPSO.
 *
 * Author: Paul Moore <paul.moore@hp.com>
 *
 */

/*
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2006
 *
 * This program is free software;  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;  if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/types.h>
#include <linux/rcupdate.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/socket.h>
#include <linux/string.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <net/genetlink.h>

#include <net/netlabel.h>
#include <asm/bug.h>

#include "netlabel_user.h"
#include "netlabel_domainhash.h"
#include "netlabel_unlabeled.h"

/* Accept unlabeled packets flag */
static atomic_t netlabel_unlabel_accept_flg = ATOMIC_INIT(0);

/* NetLabel Generic NETLINK CIPSOv4 family */
static struct genl_family netlbl_unlabel_gnl_family = {
	.id = GENL_ID_GENERATE,
	.hdrsize = 0,
	.name = NETLBL_NLTYPE_UNLABELED_NAME,
	.version = NETLBL_PROTO_VERSION,
	.maxattr = NLBL_UNLABEL_A_MAX,
};

/* NetLabel Netlink attribute policy */
static struct nla_policy netlbl_unlabel_genl_policy[NLBL_UNLABEL_A_MAX + 1] = {
	[NLBL_UNLABEL_A_ACPTFLG] = { .type = NLA_U8 },
};

/*
 * Helper Functions
 */

/**
 * netlbl_unlabel_acceptflg_set - Set the unlabeled accept flag
 * @value: desired value
 * @audit_secid: the LSM secid to use in the audit message
 *
 * Description:
 * Set the value of the unlabeled accept flag to @value.
 *
 */
static void netlbl_unlabel_acceptflg_set(u8 value, u32 audit_secid)
{
	atomic_set(&netlabel_unlabel_accept_flg, value);
	netlbl_audit_nomsg((value ?
			    AUDIT_MAC_UNLBL_ACCEPT : AUDIT_MAC_UNLBL_DENY),
			   audit_secid);
}

/*
 * NetLabel Command Handlers
 */

/**
 * netlbl_unlabel_accept - Handle an ACCEPT message
 * @skb: the NETLINK buffer
 * @info: the Generic NETLINK info block
 *
 * Description:
 * Process a user generated ACCEPT message and set the accept flag accordingly.
 * Returns zero on success, negative values on failure.
 *
 */
static int netlbl_unlabel_accept(struct sk_buff *skb, struct genl_info *info)
{
	u8 value;

	if (info->attrs[NLBL_UNLABEL_A_ACPTFLG]) {
		value = nla_get_u8(info->attrs[NLBL_UNLABEL_A_ACPTFLG]);
		if (value == 1 || value == 0) {
			netlbl_unlabel_acceptflg_set(value,
						     NETLINK_CB(skb).sid);
			return 0;
		}
	}

	return -EINVAL;
}

/**
 * netlbl_unlabel_list - Handle a LIST message
 * @skb: the NETLINK buffer
 * @info: the Generic NETLINK info block
 *
 * Description:
 * Process a user generated LIST message and respond with the current status.
 * Returns zero on success, negative values on failure.
 *
 */
static int netlbl_unlabel_list(struct sk_buff *skb, struct genl_info *info)
{
	int ret_val = -EINVAL;
	struct sk_buff *ans_skb;
	void *data;

	ans_skb = nlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (ans_skb == NULL)
		goto list_failure;
	data = netlbl_netlink_hdr_put(ans_skb,
				      info->snd_pid,
				      info->snd_seq,
				      netlbl_unlabel_gnl_family.id,
				      0,
				      NLBL_UNLABEL_C_LIST);
	if (data == NULL) {
		ret_val = -ENOMEM;
		goto list_failure;
	}

	ret_val = nla_put_u8(ans_skb,
			     NLBL_UNLABEL_A_ACPTFLG,
			     atomic_read(&netlabel_unlabel_accept_flg));
	if (ret_val != 0)
		goto list_failure;

	genlmsg_end(ans_skb, data);

	ret_val = genlmsg_unicast(ans_skb, info->snd_pid);
	if (ret_val != 0)
		goto list_failure;
	return 0;

list_failure:
	kfree(ans_skb);
	return ret_val;
}


/*
 * NetLabel Generic NETLINK Command Definitions
 */

static struct genl_ops netlbl_unlabel_genl_c_accept = {
	.cmd = NLBL_UNLABEL_C_ACCEPT,
	.flags = GENL_ADMIN_PERM,
	.policy = netlbl_unlabel_genl_policy,
	.doit = netlbl_unlabel_accept,
	.dumpit = NULL,
};

static struct genl_ops netlbl_unlabel_genl_c_list = {
	.cmd = NLBL_UNLABEL_C_LIST,
	.flags = 0,
	.policy = netlbl_unlabel_genl_policy,
	.doit = netlbl_unlabel_list,
	.dumpit = NULL,
};


/*
 * NetLabel Generic NETLINK Protocol Functions
 */

/**
 * netlbl_unlabel_genl_init - Register the Unlabeled NetLabel component
 *
 * Description:
 * Register the unlabeled packet NetLabel component with the Generic NETLINK
 * mechanism.  Returns zero on success, negative values on failure.
 *
 */
int netlbl_unlabel_genl_init(void)
{
	int ret_val;

	ret_val = genl_register_family(&netlbl_unlabel_gnl_family);
	if (ret_val != 0)
		return ret_val;

	ret_val = genl_register_ops(&netlbl_unlabel_gnl_family,
				    &netlbl_unlabel_genl_c_accept);
	if (ret_val != 0)
		return ret_val;

	ret_val = genl_register_ops(&netlbl_unlabel_gnl_family,
				    &netlbl_unlabel_genl_c_list);
	if (ret_val != 0)
		return ret_val;

	return 0;
}

/*
 * NetLabel KAPI Hooks
 */

/**
 * netlbl_unlabel_getattr - Get the security attributes for an unlabled packet
 * @secattr: the security attributes
 *
 * Description:
 * Determine the security attributes, if any, for an unlabled packet and return
 * them in @secattr.  Returns zero on success and negative values on failure.
 *
 */
int netlbl_unlabel_getattr(struct netlbl_lsm_secattr *secattr)
{
	if (atomic_read(&netlabel_unlabel_accept_flg) == 1)
		return netlbl_secattr_init(secattr);

	return -ENOMSG;
}

/**
 * netlbl_unlabel_defconf - Set the default config to allow unlabeled packets
 *
 * Description:
 * Set the default NetLabel configuration to allow incoming unlabeled packets
 * and to send unlabeled network traffic by default.
 *
 */
int netlbl_unlabel_defconf(void)
{
	int ret_val;
	struct netlbl_dom_map *entry;
	u32 secid;

	security_task_getsecid(current, &secid);

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (entry == NULL)
		return -ENOMEM;
	entry->type = NETLBL_NLTYPE_UNLABELED;
	ret_val = netlbl_domhsh_add_default(entry, secid);
	if (ret_val != 0)
		return ret_val;

	netlbl_unlabel_acceptflg_set(1, secid);

	return 0;
}
