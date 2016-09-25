/* Copyright (c) 2014-2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/msm_mhi.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/ipc_logging.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/of_device.h>

#define MHI_DEV_NODE_NAME_LEN 13
#define MHI_SOFTWARE_CLIENT_LIMIT 23
#define TRE_TYPICAL_SIZE 0x1000
#define TRE_MAX_SIZE 0xFFFF
#define MHI_UCI_IPC_LOG_PAGES (25)

#define MAX_NR_TRBS_PER_CHAN 10
#define DEVICE_NAME "mhi"
#define MHI_UCI_DRIVER_NAME "mhi_uci"
#define CTRL_MAGIC 0x4C525443

enum UCI_DBG_LEVEL {
	UCI_DBG_VERBOSE = 0x0,
	UCI_DBG_INFO = 0x1,
	UCI_DBG_DBG = 0x2,
	UCI_DBG_WARNING = 0x3,
	UCI_DBG_ERROR = 0x4,
	UCI_DBG_CRITICAL = 0x5,
	UCI_DBG_reserved = 0x80000000
};
enum UCI_DBG_LEVEL mhi_uci_msg_lvl = UCI_DBG_CRITICAL;

#ifdef CONFIG_MSM_MHI_DEBUG
enum UCI_DBG_LEVEL mhi_uci_ipc_log_lvl = UCI_DBG_VERBOSE;
#else
enum UCI_DBG_LEVEL mhi_uci_ipc_log_lvl = UCI_DBG_ERROR;
#endif

struct __packed rs232_ctrl_msg {
	u32 preamble;
	u32 msg_id;
	u32 dest_id;
	u32 size;
	u32 msg;
};

enum MHI_SERIAL_STATE {
	MHI_SERIAL_STATE_DCD = 0x1,
	MHI_SERIAL_STATE_DSR = 0x2,
	MHI_SERIAL_STATE_RI = 0x3,
	MHI_SERIAL_STATE_reserved = 0x80000000,
};

enum MHI_CTRL_LINE_STATE {
	MHI_CTRL_LINE_STATE_DTR = 0x1,
	MHI_CTRL_LINE_STATE_RTS = 0x2,
	MHI_CTRL_LINE_STATE_reserved = 0x80000000,
};

enum MHI_MSG_ID {
	MHI_CTRL_LINE_STATE_ID = 0x10,
	MHI_SERIAL_STATE_ID = 0x11,
	MHI_CTRL_MSG_ID = 0x12,
};

enum MHI_CHAN_DIR {
	MHI_DIR_INVALID = 0x0,
	MHI_DIR_OUT = 0x1,
	MHI_DIR_IN = 0x2,
	MHI_DIR__reserved = 0x80000000
};

struct chan_attr {
	enum MHI_CLIENT_CHANNEL chan_id;
	size_t max_packet_size;
	u32 nr_trbs;
	enum MHI_CHAN_DIR dir;
	u32 uci_ownership;
};

struct uci_client {
	u32 out_chan;
	u32 in_chan;
	u32 out_chan_state;
	u32 in_chan_state;
	struct chan_attr in_attr;
	struct chan_attr out_attr;
	struct mhi_client_handle *out_handle;
	struct mhi_client_handle *in_handle;
	size_t pending_data;
	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;
	atomic_t avail_pkts;
	struct device *dev;
	u8 local_tiocm;
	atomic_t ref_count;
	int mhi_status;
	void *pkt_loc;
	size_t pkt_size;
	void **in_buf_list;
	atomic_t out_pkt_pend_ack;
	atomic_t mhi_disabled;
	struct mhi_uci_ctxt_t *uci_ctxt;
	struct mutex in_chan_lock;
	struct mutex out_chan_lock;
	void *uci_ipc_log;
};

struct mhi_uci_ctxt_t {
	struct list_head node;
	struct platform_dev *pdev;
	struct uci_client client_handles[MHI_SOFTWARE_CLIENT_LIMIT];
	struct mhi_client_info_t client_info;
	dev_t dev_t;
	struct mutex ctrl_mutex;
	struct cdev cdev[MHI_SOFTWARE_CLIENT_LIMIT];
	struct uci_client *ctrl_client;
	atomic_t mhi_disabled;
	atomic_t mhi_enable_notif_wq_active;
};

struct mhi_uci_drv_ctxt {
	struct list_head head;
	struct mutex list_lock;
	struct class *mhi_uci_class;
	void *mhi_uci_ipc_log;
};

#define CHAN_TO_CLIENT(_CHAN_NR) (_CHAN_NR / 2)

#define CTRL_MSG_ID
#define MHI_CTRL_MSG_ID__MASK (0xFFFFFF)
#define MHI_CTRL_MSG_ID__SHIFT (0)
#define MHI_SET_CTRL_MSG_ID(_FIELD, _PKT, _VAL) \
{ \
	u32 new_val = ((_PKT)->msg_id); \
	new_val &= (~((MHI_##_FIELD ## __MASK) << MHI_##_FIELD ## __SHIFT));\
	new_val |= _VAL << MHI_##_FIELD ## __SHIFT; \
	(_PKT)->msg_id = new_val; \
};
#define MHI_GET_CTRL_MSG_ID(_FIELD, _PKT, _VAL) \
{ \
	(_VAL) = ((_PKT)->msg_id); \
	(_VAL) >>= (MHI_##_FIELD ## __SHIFT);\
	(_VAL) &= (MHI_##_FIELD ## __MASK); \
};

#define CTRL_DEST_ID
#define MHI_CTRL_DEST_ID__MASK (0xFFFFFF)
#define MHI_CTRL_DEST_ID__SHIFT (0)
#define MHI_SET_CTRL_DEST_ID(_FIELD, _PKT, _VAL) \
{ \
	u32 new_val = ((_PKT)->dest_id); \
	new_val &= (~((MHI_##_FIELD ## __MASK) << MHI_##_FIELD ## __SHIFT));\
	new_val |= _VAL << MHI_##_FIELD ## __SHIFT; \
	(_PKT)->dest_id = new_val; \
};
#define MHI_GET_CTRL_DEST_ID(_FIELD, _PKT, _VAL) \
{ \
	(_VAL) = ((_PKT)->dest_id); \
	(_VAL) >>= (MHI_##_FIELD ## __SHIFT);\
	(_VAL) &= (MHI_##_FIELD ## __MASK); \
};

#define CTRL_MSG_DTR
#define MHI_CTRL_MSG_DTR__MASK (0xFFFFFFFE)
#define MHI_CTRL_MSG_DTR__SHIFT (0)

#define CTRL_MSG_RTS
#define MHI_CTRL_MSG_RTS__MASK (0xFFFFFFFD)
#define MHI_CTRL_MSG_RTS__SHIFT (1)

#define STATE_MSG_DCD
#define MHI_STATE_MSG_DCD__MASK (0xFFFFFFFE)
#define MHI_STATE_MSG_DCD__SHIFT (0)

#define STATE_MSG_DSR
#define MHI_STATE_MSG_DSR__MASK (0xFFFFFFFD)
#define MHI_STATE_MSG_DSR__SHIFT (1)

#define STATE_MSG_RI
#define MHI_STATE_MSG_RI__MASK (0xFFFFFFFB)
#define MHI_STATE_MSG_RI__SHIFT (3)
#define MHI_SET_CTRL_MSG(_FIELD, _PKT, _VAL) \
{ \
	u32 new_val = (_PKT->msg); \
	new_val &= (~((MHI_##_FIELD ## __MASK)));\
	new_val |= _VAL << MHI_##_FIELD ## __SHIFT; \
	(_PKT)->msg = new_val; \
};
#define MHI_GET_STATE_MSG(_FIELD, _PKT) \
	(((_PKT)->msg & (~(MHI_##_FIELD ## __MASK))) \
		>> MHI_##_FIELD ## __SHIFT)

#define CTRL_MSG_SIZE
#define MHI_CTRL_MSG_SIZE__MASK (0xFFFFFF)
#define MHI_CTRL_MSG_SIZE__SHIFT (0)
#define MHI_SET_CTRL_MSG_SIZE(_FIELD, _PKT, _VAL) \
{ \
	u32 new_val = (_PKT->size); \
	new_val &= (~((MHI_##_FIELD ## __MASK) << MHI_##_FIELD ## __SHIFT));\
	new_val |= _VAL << MHI_##_FIELD ## __SHIFT; \
	(_PKT)->size = new_val; \
};

#define uci_log(uci_ipc_log, _msg_lvl, _msg, ...) do { \
	if (_msg_lvl >= mhi_uci_msg_lvl) { \
		pr_err("[%s] "_msg, __func__, ##__VA_ARGS__); \
	} \
	if (uci_ipc_log && (_msg_lvl >= mhi_uci_ipc_log_lvl)) { \
		ipc_log_string(uci_ipc_log, \
			"[%s] " _msg, __func__, ##__VA_ARGS__); \
	} \
} while (0)

module_param(mhi_uci_msg_lvl , uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mhi_uci_msg_lvl, "uci dbg lvl");

module_param(mhi_uci_ipc_log_lvl, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mhi_uci_ipc_log_lvl, "ipc dbg lvl");

static ssize_t mhi_uci_client_read(struct file *file, char __user *buf,
		size_t count, loff_t *offp);
static ssize_t mhi_uci_client_write(struct file *file,
		const char __user *buf, size_t count, loff_t *offp);
static int mhi_uci_client_open(struct inode *mhi_inode, struct file*);
static int mhi_uci_client_release(struct inode *mhi_inode,
		struct file *file_handle);
static unsigned int mhi_uci_client_poll(struct file *file, poll_table *wait);
static long mhi_uci_ctl_ioctl(struct file *file, unsigned int cmd,
					unsigned long arg);

static struct mhi_uci_drv_ctxt mhi_uci_drv_ctxt;

static int mhi_init_inbound(struct uci_client *client_handle)
{
	int ret_val = 0;
	u32 i = 0;
	struct chan_attr *chan_attributes = &client_handle->in_attr;
	void *data_loc = NULL;
	size_t buf_size = chan_attributes->max_packet_size;

	if (client_handle == NULL) {
		uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
			UCI_DBG_ERROR,
			"Bad Input data, quitting\n");
		return -EINVAL;
	}
	chan_attributes->nr_trbs =
			mhi_get_free_desc(client_handle->in_handle);
	client_handle->in_buf_list =
			kmalloc(sizeof(void *) * chan_attributes->nr_trbs,
			GFP_KERNEL);
	if (!client_handle->in_buf_list)
		return -ENOMEM;

	uci_log(client_handle->uci_ipc_log,
		UCI_DBG_INFO, "Channel %d supports %d desc\n",
		chan_attributes->chan_id,
		chan_attributes->nr_trbs);
	for (i = 0; i < chan_attributes->nr_trbs; ++i) {
		data_loc = kmalloc(buf_size, GFP_KERNEL);
		uci_log(client_handle->uci_ipc_log,
			UCI_DBG_INFO,
			"Allocated buffer %p size %zd\n",
			data_loc,
			buf_size);
		if (data_loc == NULL)
			return -ENOMEM;
		client_handle->in_buf_list[i] = data_loc;
		ret_val = mhi_queue_xfer(client_handle->in_handle,
					  data_loc, buf_size, MHI_EOT);
		if (0 != ret_val) {
			kfree(data_loc);
			uci_log(client_handle->uci_ipc_log,
				UCI_DBG_ERROR,
				"Failed insertion for chan %d, ret %d\n",
				chan_attributes->chan_id,
				ret_val);
			break;
		}
	}
	return ret_val;
}

static int mhi_uci_send_packet(struct mhi_client_handle **client_handle,
		void *buf, u32 size, u32 is_uspace_buf)
{
	u32 nr_avail_trbs = 0;
	u32 i = 0;
	void *data_loc = NULL;
	uintptr_t memcpy_result = 0;
	int data_left_to_insert = 0;
	size_t data_to_insert_now = 0;
	u32 data_inserted_so_far = 0;
	int ret_val = 0;
	enum MHI_FLAGS flags;
	struct uci_client *uci_handle;
	uci_handle = container_of(client_handle, struct uci_client,
					out_handle);

	if (client_handle == NULL || buf == NULL ||
		!size || uci_handle == NULL)
		return -EINVAL;

	nr_avail_trbs = mhi_get_free_desc(*client_handle);

	data_left_to_insert = size;

	if (0 == nr_avail_trbs)
		return 0;

	for (i = 0; i < nr_avail_trbs; ++i) {
		data_to_insert_now = min(data_left_to_insert,
				TRE_MAX_SIZE);
		if (is_uspace_buf) {
			data_loc = kmalloc(data_to_insert_now, GFP_KERNEL);
			if (NULL == data_loc) {
				uci_log(uci_handle->uci_ipc_log,
					UCI_DBG_VERBOSE,
					"Failed to allocate memory 0x%zx\n",
					data_to_insert_now);
				return -ENOMEM;
			}
			memcpy_result = copy_from_user(data_loc,
					buf + data_inserted_so_far,
					data_to_insert_now);

			if (0 != memcpy_result)
				goto error_memcpy;
		} else {
			data_loc = buf;
		}

		flags = MHI_EOT;
		if (data_left_to_insert - data_to_insert_now > 0)
			flags |= MHI_CHAIN | MHI_EOB;
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_VERBOSE,
			"At trb i = %d/%d, chain = %d, eob = %d, addr 0x%p chan %d\n",
			i,
			nr_avail_trbs,
			flags & MHI_CHAIN,
			flags & MHI_EOB,
			data_loc,
			uci_handle->out_chan);
		ret_val = mhi_queue_xfer(*client_handle, data_loc,
					data_to_insert_now, flags);

		if (0 != ret_val) {
			goto error_queue;
		} else {
			data_left_to_insert -= data_to_insert_now;
			data_inserted_so_far += data_to_insert_now;
			atomic_inc(&uci_handle->out_pkt_pend_ack);
		}
		if (0 == data_left_to_insert)
			break;
	}
	return data_inserted_so_far;

error_queue:
error_memcpy:
	kfree(data_loc);
	return data_inserted_so_far;
}

static int mhi_uci_send_status_cmd(struct uci_client *client)
{
	struct rs232_ctrl_msg *rs232_pkt = NULL;
	struct uci_client *uci_ctrl_handle;
	struct mhi_uci_ctxt_t *uci_ctxt = client->uci_ctxt;
	int ret_val = 0;
	size_t pkt_size = sizeof(struct rs232_ctrl_msg);
	u32 amount_sent;

	if (!uci_ctxt->ctrl_client) {
		uci_log(client->uci_ipc_log,
			UCI_DBG_INFO,
			"Control channel is not defined\n");
		return -EIO;
	}

	uci_ctrl_handle = uci_ctxt->ctrl_client;
	mutex_lock(&uci_ctrl_handle->out_chan_lock);

	if (!atomic_read(&uci_ctrl_handle->mhi_disabled) &&
			 !uci_ctrl_handle->out_chan_state) {
		uci_log(uci_ctrl_handle->uci_ipc_log,
			UCI_DBG_INFO,
			"Opening outbound control channel %d for chan:%d\n",
			uci_ctrl_handle->out_chan,
			client->out_chan);
		ret_val = mhi_open_channel(uci_ctrl_handle->out_handle);
		if (0 != ret_val) {
			uci_log(uci_ctrl_handle->uci_ipc_log,
				UCI_DBG_CRITICAL,
				"Could not open chan %d, for sideband ctrl\n",
				uci_ctrl_handle->out_chan);
			ret_val = -EIO;
			goto error_open;
		}
		uci_ctrl_handle->out_chan_state = 1;
	}

	rs232_pkt = kzalloc(sizeof(struct rs232_ctrl_msg), GFP_KERNEL);
	if (rs232_pkt == NULL) {
		ret_val = -ENOMEM;
		goto error_open;
	}
	uci_log(uci_ctrl_handle->uci_ipc_log,
		UCI_DBG_VERBOSE,
		"Received request to send msg for chan %d\n",
		client->out_chan);
	rs232_pkt->preamble = CTRL_MAGIC;
	if (client->local_tiocm & TIOCM_DTR)
		MHI_SET_CTRL_MSG(CTRL_MSG_DTR, rs232_pkt, 1);
	if (client->local_tiocm & TIOCM_RTS)
		MHI_SET_CTRL_MSG(CTRL_MSG_RTS, rs232_pkt, 1);

	MHI_SET_CTRL_MSG_ID(CTRL_MSG_ID, rs232_pkt, MHI_CTRL_LINE_STATE_ID);
	MHI_SET_CTRL_MSG_SIZE(CTRL_MSG_SIZE, rs232_pkt, sizeof(u32));
	MHI_SET_CTRL_DEST_ID(CTRL_DEST_ID, rs232_pkt, client->out_chan);

	amount_sent = mhi_uci_send_packet(&uci_ctrl_handle->out_handle,
						rs232_pkt,
						pkt_size, 0);

	if (pkt_size != amount_sent) {
		uci_log(uci_ctrl_handle->uci_ipc_log,
			UCI_DBG_INFO,
			"Failed to send signal for chan %d, ret : %d\n",
			client->out_chan,
			ret_val);
		goto error;
	}
error_open:
	mutex_unlock(&uci_ctrl_handle->out_chan_lock);
	return ret_val;
error:
	kfree(rs232_pkt);
	mutex_unlock(&uci_ctrl_handle->out_chan_lock);
	return ret_val;
}

static int mhi_uci_tiocm_set(struct uci_client *client_ctxt, u32 set, u32 clear)
{
	u8 status_set = 0;
	u8 status_clear = 0;
	u8 old_status = 0;

	mutex_lock(&client_ctxt->uci_ctxt->ctrl_mutex);

	status_set |= (set & TIOCM_DTR) ? TIOCM_DTR : 0;
	status_clear |= (clear & TIOCM_DTR) ? TIOCM_DTR : 0;
	old_status = client_ctxt->local_tiocm;
	client_ctxt->local_tiocm |= status_set;
	client_ctxt->local_tiocm &= ~status_clear;

	uci_log(client_ctxt->uci_ipc_log,
		UCI_DBG_VERBOSE,
		"Old TIOCM0x%x for chan %d, Current TIOCM 0x%x\n",
		old_status,
		client_ctxt->out_chan,
		client_ctxt->local_tiocm);
	mutex_unlock(&client_ctxt->uci_ctxt->ctrl_mutex);

	if (client_ctxt->local_tiocm != old_status) {
		uci_log(client_ctxt->uci_ipc_log,
			UCI_DBG_VERBOSE,
			"Setting TIOCM to 0x%x for chan %d\n",
			client_ctxt->local_tiocm,
			client_ctxt->out_chan);
		return mhi_uci_send_status_cmd(client_ctxt);
	}
	return 0;
}

static long mhi_uci_ctl_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	int ret_val = 0;
	u32 set_val;
	struct uci_client *uci_handle = NULL;
	uci_handle = file->private_data;

	if (uci_handle == NULL) {
		uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
			UCI_DBG_VERBOSE,
			"Invalid handle for client\n");
		return -ENODEV;
	}
	uci_log(uci_handle->uci_ipc_log,
		UCI_DBG_VERBOSE,
		"Attempting to dtr cmd 0x%x arg 0x%lx for chan %d\n",
		cmd,
		arg,
		uci_handle->out_chan);

	switch (cmd) {
	case TIOCMGET:
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_VERBOSE,
			"Returning 0x%x mask\n",
			uci_handle->local_tiocm);
		ret_val = uci_handle->local_tiocm;
		break;
	case TIOCMSET:
		if (0 != copy_from_user(&set_val, (void *)arg, sizeof(set_val)))
			return -ENOMEM;
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_VERBOSE,
			"Attempting to set cmd 0x%x arg 0x%x for chan %d\n",
			cmd,
			set_val,
			uci_handle->out_chan);
		ret_val = mhi_uci_tiocm_set(uci_handle, set_val, ~set_val);
		break;
	default:
		ret_val = -EINVAL;
		break;
	}
	return ret_val;
}

static unsigned int mhi_uci_client_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	struct uci_client *uci_handle = file->private_data;
	struct mhi_uci_ctxt_t *uci_ctxt;

	if (uci_handle == NULL)
		return -ENODEV;

	uci_ctxt = uci_handle->uci_ctxt;
	poll_wait(file, &uci_handle->read_wq, wait);
	poll_wait(file, &uci_handle->write_wq, wait);
	if (atomic_read(&uci_handle->avail_pkts) > 0) {
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_VERBOSE,
			"Client can read chan %d\n",
			uci_handle->in_chan);
		mask |= POLLIN | POLLRDNORM;
	}
	if (!atomic_read(&uci_ctxt->mhi_disabled) &&
	    (mhi_get_free_desc(uci_handle->out_handle) > 0)) {
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_VERBOSE,
			"Client can write chan %d\n",
			uci_handle->out_chan);
		mask |= POLLOUT | POLLWRNORM;
	}

	uci_log(uci_handle->uci_ipc_log,
		UCI_DBG_VERBOSE,
		"Client attempted to poll chan %d, returning mask 0x%x\n",
		uci_handle->in_chan,
		mask);
	return mask;
}

static int open_client_mhi_channels(struct uci_client *uci_client)
{
	int ret_val = 0;
	int r = 0;

	uci_log(uci_client->uci_ipc_log,
		UCI_DBG_INFO,
		"Starting channels %d %d\n",
		uci_client->out_chan,
		uci_client->in_chan);
	mutex_lock(&uci_client->out_chan_lock);
	mutex_lock(&uci_client->in_chan_lock);
	ret_val = mhi_open_channel(uci_client->out_handle);
	if (ret_val != 0) {
		if (ret_val == -ENOTCONN)
			r = -EAGAIN;
		else
			r = -EIO;
		goto handle_not_rdy_err;
	}
	uci_client->out_chan_state = 1;

	ret_val = mhi_open_channel(uci_client->in_handle);
	if (ret_val != 0) {
		uci_log(uci_client->uci_ipc_log,
			UCI_DBG_ERROR,
			"Failed to open chan %d, ret 0x%x\n",
			uci_client->out_chan,
			ret_val);
		goto handle_in_err;
	}
	uci_log(uci_client->uci_ipc_log,
		UCI_DBG_INFO,
		"Initializing inbound chan %d\n",
		uci_client->in_chan);
	uci_client->in_chan_state = 1;

	ret_val = mhi_init_inbound(uci_client);
	if (0 != ret_val) {
		uci_log(uci_client->uci_ipc_log,
			UCI_DBG_ERROR,
			"Failed to init inbound 0x%x, ret 0x%x\n",
			uci_client->in_chan,
			ret_val);
	}

	mutex_unlock(&uci_client->in_chan_lock);
	mutex_unlock(&uci_client->out_chan_lock);
	return 0;

handle_in_err:
	mhi_close_channel(uci_client->out_handle);
	uci_client->out_chan_state = 0;
handle_not_rdy_err:
	mutex_unlock(&uci_client->in_chan_lock);
	mutex_unlock(&uci_client->out_chan_lock);
	return r;
}

static int mhi_uci_client_open(struct inode *inode,
				struct file *file_handle)
{
	struct uci_client *uci_handle = NULL;
	struct mhi_uci_ctxt_t *uci_ctxt = NULL, *itr;
	int r = 0;
	int client_id = iminor(inode);
	int major = imajor(inode);

	/* Find the uci ctxt from major */
	mutex_lock(&mhi_uci_drv_ctxt.list_lock);
	list_for_each_entry(itr, &mhi_uci_drv_ctxt.head, node) {
		if (MAJOR(itr->dev_t) == major) {
			uci_ctxt = itr;
			break;
		}
	}
	mutex_unlock(&mhi_uci_drv_ctxt.list_lock);
	if (!uci_ctxt || client_id >= MHI_SOFTWARE_CLIENT_LIMIT)
		return -EINVAL;

	uci_handle = &uci_ctxt->client_handles[client_id];
	if (atomic_read(&uci_handle->mhi_disabled)) {
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_INFO,
			"MHI channel still disable for, client %d\n",
			client_id);
		msleep(500);
		return -EAGAIN;
	}

	uci_log(uci_handle->uci_ipc_log,
		UCI_DBG_INFO,
		"Client opened device node 0x%x, ref count 0x%x\n",
		client_id,
		atomic_read(&uci_handle->ref_count));

	if (1 == atomic_add_return(1, &uci_handle->ref_count)) {
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_INFO,
			"Opening channels client %d\n",
			client_id);
		r = open_client_mhi_channels(uci_handle);
		if (r)
			uci_log(uci_handle->uci_ipc_log,
				UCI_DBG_INFO,
				"Failed to open channels ret %d\n",
				r);
	}
	file_handle->private_data = uci_handle;
	return r;
}

static int mhi_uci_client_release(struct inode *mhi_inode,
		struct file *file_handle)
{
	struct uci_client *uci_handle = file_handle->private_data;
	u32 nr_in_bufs = 0;
	int in_chan = 0;
	int i = 0;
	u32 buf_size = 0;

	if (uci_handle == NULL)
		return -EINVAL;

	in_chan = uci_handle->in_attr.chan_id;
	nr_in_bufs = uci_handle->in_attr.nr_trbs;
	buf_size = uci_handle->in_attr.max_packet_size;

	if (atomic_sub_return(1, &uci_handle->ref_count) == 0) {
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_ERROR,
			"Last client left, closing channel 0x%x\n",
			in_chan);

		if (atomic_read(&uci_handle->out_pkt_pend_ack))
			uci_log(uci_handle->uci_ipc_log,
				UCI_DBG_CRITICAL,
				"Still waiting on %d acks!, chan %d\n",
				atomic_read(&uci_handle->out_pkt_pend_ack),
				uci_handle->out_attr.chan_id);

		mhi_close_channel(uci_handle->out_handle);
		mhi_close_channel(uci_handle->in_handle);
		uci_handle->out_chan_state = 0;
		uci_handle->in_chan_state = 0;
		atomic_set(&uci_handle->out_pkt_pend_ack, 0);
		for (i = 0; i < nr_in_bufs; ++i) {
			kfree((void *)uci_handle->in_buf_list[i]);
		}
		kfree(uci_handle->in_buf_list);
		atomic_set(&uci_handle->avail_pkts, 0);
	} else {
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_ERROR,
			"Client close chan %d, ref count 0x%x\n",
			iminor(mhi_inode),
			atomic_read(&uci_handle->ref_count));
	}
	return 0;
}

static ssize_t mhi_uci_client_read(struct file *file, char __user *buf,
		size_t uspace_buf_size, loff_t *bytes_pending)
{
	struct uci_client *uci_handle = NULL;
	struct mhi_client_handle *client_handle = NULL;
	int ret_val = 0;
	size_t buf_size = 0;
	struct mutex *mutex;
	u32 chan = 0;
	ssize_t bytes_copied = 0;
	u32 addr_offset = 0;
	struct mhi_result result;

	if (file == NULL || buf == NULL ||
	    uspace_buf_size == 0 || file->private_data == NULL)
		return -EINVAL;

	uci_handle = file->private_data;
	client_handle = uci_handle->in_handle;
	mutex = &uci_handle->in_chan_lock;
	chan = uci_handle->in_chan;
	mutex_lock(mutex);
	buf_size = uci_handle->in_attr.max_packet_size;
	result.buf_addr = NULL;

	uci_log(uci_handle->uci_ipc_log,
		UCI_DBG_VERBOSE,
		"Client attempted read on chan %d\n",
		chan);
	do {
		if (!uci_handle->pkt_loc &&
		    !atomic_read(&uci_handle->uci_ctxt->mhi_disabled)) {
			ret_val = mhi_poll_inbound(client_handle, &result);
			if (ret_val) {
				uci_log(uci_handle->uci_ipc_log,
					UCI_DBG_ERROR,
					"Failed to poll inbound ret %d avail pkt %d\n",
					ret_val,
					atomic_read(&uci_handle->avail_pkts));
			}
			if (result.buf_addr)
				uci_handle->pkt_loc = result.buf_addr;
			else
				uci_handle->pkt_loc = 0;
			uci_handle->pkt_size = result.bytes_xferd;
			*bytes_pending = uci_handle->pkt_size;
			uci_log(uci_handle->uci_ipc_log,
				UCI_DBG_VERBOSE,
				"Got pkt size 0x%zx at addr 0x%lx, chan %d\n",
				uci_handle->pkt_size,
				(uintptr_t)result.buf_addr,
				chan);
		}
		if ((*bytes_pending == 0 || uci_handle->pkt_loc == 0) &&
				(atomic_read(&uci_handle->avail_pkts) <= 0)) {
			/* If nothing was copied yet, wait for data */
			uci_log(uci_handle->uci_ipc_log,
				UCI_DBG_VERBOSE,
				"No data avail_pkts %d, chan %d\n",
				atomic_read(&uci_handle->avail_pkts),
				chan);
			ret_val = wait_event_interruptible(
				uci_handle->read_wq,
				(atomic_read(&uci_handle->avail_pkts) > 0));
			if (ret_val == -ERESTARTSYS) {
				uci_log(uci_handle->uci_ipc_log,
					UCI_DBG_ERROR,
					"Exit signal caught\n");
				goto error;
			}
		/* A pending reset exists */
		} else if ((atomic_read(&uci_handle->avail_pkts) > 0) &&
			    0 == uci_handle->pkt_size &&
			    0 == uci_handle->pkt_loc &&
			    uci_handle->mhi_status == -ENETRESET) {
			uci_log(uci_handle->uci_ipc_log,
				UCI_DBG_ERROR,
				"Detected pending reset, reporting chan %d\n",
				chan);
			atomic_dec(&uci_handle->avail_pkts);
			uci_handle->mhi_status = 0;
			mutex_unlock(mutex);
			return -ENETRESET;
			/* A valid packet was returned from MHI */
		} else if (atomic_read(&uci_handle->avail_pkts) &&
			   uci_handle->pkt_size != 0 &&
			   uci_handle->pkt_loc != 0) {
			uci_log(uci_handle->uci_ipc_log,
				UCI_DBG_VERBOSE,
				"Got packet: avail pkts %d phy_adr 0x%p, chan %d\n",
				atomic_read(&uci_handle->avail_pkts),
				result.buf_addr,
				chan);
			break;
			/*
			 * MHI did not return a valid packet, but we have one
			 * which we did not finish returning to user
			 */
		} else {
			uci_log(uci_handle->uci_ipc_log,
				UCI_DBG_CRITICAL,
				"chan %d err: avail pkts %d mhi_stat%d\n",
				chan,
				atomic_read(&uci_handle->avail_pkts),
				uci_handle->mhi_status);
			return -EIO;
		}
	} while (!uci_handle->pkt_loc);

	if (uspace_buf_size >= *bytes_pending) {
		addr_offset = uci_handle->pkt_size - *bytes_pending;
		if (0 != copy_to_user(buf,
				     uci_handle->pkt_loc + addr_offset,
				     *bytes_pending)) {
			ret_val = -EIO;
			goto error;
		}

		bytes_copied = *bytes_pending;
		*bytes_pending = 0;
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_VERBOSE,
			"Copied 0x%zx of 0x%x, chan %d\n",
			bytes_copied,
			(u32)*bytes_pending,
			chan);
	} else {
		addr_offset = uci_handle->pkt_size - *bytes_pending;
		if (copy_to_user(buf,
				     (void *)(uintptr_t)uci_handle->pkt_loc +
				     addr_offset,
				     uspace_buf_size) != 0) {
			ret_val = -EIO;
			goto error;
		}
		bytes_copied = uspace_buf_size;
		*bytes_pending -= uspace_buf_size;
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_VERBOSE,
			"Copied 0x%zx of 0x%x,chan %d\n",
			bytes_copied,
			(u32)*bytes_pending,
			chan);
	}
	/* We finished with this buffer, map it back */
	if (*bytes_pending == 0) {
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_VERBOSE,
			"Pkt loc %p ,chan %d\n",
			uci_handle->pkt_loc,
			chan);
		memset(uci_handle->pkt_loc, 0, buf_size);
		atomic_dec(&uci_handle->avail_pkts);
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_VERBOSE,
			"Decremented avail pkts avail 0x%x\n",
			atomic_read(&uci_handle->avail_pkts));
		ret_val = mhi_queue_xfer(client_handle, uci_handle->pkt_loc,
					 buf_size, MHI_EOT);
		if (0 != ret_val) {
			uci_log(uci_handle->uci_ipc_log,
				UCI_DBG_ERROR,
				"Failed to recycle element\n");
			ret_val = -EIO;
			goto error;
		}
		uci_handle->pkt_loc = 0;
	}
	uci_log(uci_handle->uci_ipc_log,
		UCI_DBG_VERBOSE,
		"Returning 0x%zx bytes, 0x%x bytes left\n",
		bytes_copied,
		(u32)*bytes_pending);
	mutex_unlock(mutex);
	return bytes_copied;
error:
	mutex_unlock(mutex);
	uci_log(uci_handle->uci_ipc_log,
		UCI_DBG_ERROR,
		"Returning %d\n",
		ret_val);
	return ret_val;
}

static ssize_t mhi_uci_client_write(struct file *file,
		const char __user *buf,
		size_t count, loff_t *offp)
{
	struct uci_client *uci_handle = NULL;
	int ret_val = 0;
	u32 chan = 0xFFFFFFFF;

	if (file == NULL || buf == NULL ||
			!count || file->private_data == NULL)
		return -EINVAL;
	else
		uci_handle = file->private_data;

	chan = uci_handle->out_chan;
	mutex_lock(&uci_handle->out_chan_lock);

	while (ret_val == 0) {
		ret_val = mhi_uci_send_packet(&uci_handle->out_handle,
				(void *)buf, count, 1);
		if (!ret_val) {
			uci_log(uci_handle->uci_ipc_log,
				UCI_DBG_VERBOSE,
				"No descriptors available, did we poll, chan %d?\n",
				chan);
			mutex_unlock(&uci_handle->out_chan_lock);
			ret_val =
				wait_event_interruptible(
						uci_handle->write_wq,
			mhi_get_free_desc(uci_handle->out_handle) > 0);
			mutex_lock(&uci_handle->out_chan_lock);
			if (-ERESTARTSYS == ret_val) {
				goto sys_interrupt;
				uci_log(uci_handle->uci_ipc_log,
					UCI_DBG_WARNING,
					"Waitqueue cancelled by system\n");
			}
		}
	}
sys_interrupt:
	mutex_unlock(&uci_handle->out_chan_lock);
	return ret_val;
}

static int uci_init_client_attributes(struct mhi_uci_ctxt_t *uci_ctxt,
				      struct device_node *of_node)
{
	int num_rows, ret_val = 0;
	int i, dir;
	u32 ctrl_chan = -1;
	u32 *chan_info, *itr;
	const char *prop_name = "qcom,mhi-uci-channels";

	ret_val = of_property_read_u32(of_node, "qcom,mhi-uci-ctrlchan",
				       &ctrl_chan);
	if (ret_val) {
		uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
			UCI_DBG_INFO,
			"Could not find property 'qcom,mhi-uci-ctrlchan'\n");
	}

	num_rows = of_property_count_elems_of_size(of_node, prop_name,
						   sizeof(u32) * 4);
	/* At least one pair of channels should exist */
	if (num_rows < 1) {
		uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
			UCI_DBG_CRITICAL,
			"Missing or invalid property 'qcom,mhi-uci-channels'\n");
		return -ENODEV;
	}

	if (num_rows > MHI_SOFTWARE_CLIENT_LIMIT)
		num_rows = MHI_SOFTWARE_CLIENT_LIMIT;

	chan_info = kmalloc_array(num_rows, 4 * sizeof(*chan_info), GFP_KERNEL);
	if (!chan_info)
		return -ENOMEM;

	ret_val = of_property_read_u32_array(of_node, prop_name, chan_info,
					     num_rows * 4);
	if (ret_val)
		goto error_dts;

	for (i = 0, itr = chan_info; i < num_rows; i++) {
		struct uci_client *client = &uci_ctxt->client_handles[i];
		struct chan_attr *chan_attrib;

		for (dir = 0; dir < 2; dir++) {
			chan_attrib = (dir) ?
				&client->in_attr : &client->out_attr;
			chan_attrib->uci_ownership = 1;
			chan_attrib->chan_id = *itr++;
			chan_attrib->max_packet_size = *itr++;
			if (dir == 0)
				chan_attrib->dir = MHI_DIR_OUT;
			else
				chan_attrib->dir = MHI_DIR_IN;

			if (chan_attrib->chan_id == ctrl_chan)
				uci_ctxt->ctrl_client = client;
		}
	}

error_dts:
	kfree(chan_info);
	return ret_val;
}

static int process_mhi_disabled_notif_sync(struct uci_client *uci_handle)
{
	uci_log(uci_handle->uci_ipc_log,
		UCI_DBG_INFO,
		"Entered.\n");
	if (uci_handle->mhi_status != -ENETRESET) {
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_CRITICAL,
			"Setting reset for chan %d\n",
		uci_handle->out_chan);
		uci_handle->pkt_size = 0;
		uci_handle->pkt_loc = NULL;
		uci_handle->mhi_status = -ENETRESET;
		atomic_set(&uci_handle->avail_pkts, 1);
		mhi_close_channel(uci_handle->out_handle);
		mhi_close_channel(uci_handle->in_handle);
		uci_handle->out_chan_state = 0;
		uci_handle->in_chan_state = 0;
		wake_up(&uci_handle->read_wq);
	} else {
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_CRITICAL,
			"Chan %d state already reset\n",
			uci_handle->out_chan);
	}
	uci_log(uci_handle->uci_ipc_log, UCI_DBG_INFO, "Exited\n");
	return 0;
}

static void process_rs232_state(struct uci_client *ctrl_client,
				struct mhi_result *result)
{
	struct rs232_ctrl_msg *rs232_pkt;
	struct uci_client *client = NULL;
	struct mhi_uci_ctxt_t *uci_ctxt = ctrl_client->uci_ctxt;
	u32 msg_id;
	int ret_val, i;
	u32 chan;

	mutex_lock(&uci_ctxt->ctrl_mutex);
	if (result->transaction_status != 0) {
		uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
			UCI_DBG_ERROR,
			"Non successful transfer code 0x%x\n",
			result->transaction_status);
		goto error_bad_xfer;
	}
	if (result->bytes_xferd != sizeof(struct rs232_ctrl_msg)) {
		uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
			UCI_DBG_ERROR,
			"Buffer is of wrong size is: 0x%zx: expected 0x%zx\n",
			result->bytes_xferd,
			sizeof(struct rs232_ctrl_msg));
		goto error_size;
	}
	rs232_pkt = result->buf_addr;
	MHI_GET_CTRL_DEST_ID(CTRL_DEST_ID, rs232_pkt, chan);
	for (i = 0; i < MHI_SOFTWARE_CLIENT_LIMIT; i++)
		if (chan == uci_ctxt->client_handles[i].out_chan ||
		    chan == uci_ctxt->client_handles[i].in_chan) {
			client = &uci_ctxt->client_handles[i];
			break;
		}

	/* No valid channel found */
	if (!client)
		goto error_bad_xfer;

	MHI_GET_CTRL_MSG_ID(CTRL_MSG_ID, rs232_pkt, msg_id);
	client->local_tiocm = 0;
	if (MHI_SERIAL_STATE_ID == msg_id) {
		client->local_tiocm |=
			MHI_GET_STATE_MSG(STATE_MSG_DCD, rs232_pkt) ?
			TIOCM_CD : 0;
		client->local_tiocm |=
			MHI_GET_STATE_MSG(STATE_MSG_DSR, rs232_pkt) ?
			TIOCM_DSR : 0;
		client->local_tiocm |=
			MHI_GET_STATE_MSG(STATE_MSG_RI, rs232_pkt) ?
			TIOCM_RI : 0;
	}
error_bad_xfer:
error_size:
	memset(rs232_pkt, 0, sizeof(struct rs232_ctrl_msg));
	ret_val = mhi_queue_xfer(ctrl_client->in_handle,
				 result->buf_addr,
				 result->bytes_xferd,
				 result->flags);
	if (0 != ret_val) {
		uci_log(ctrl_client->uci_ipc_log,
			UCI_DBG_ERROR,
			"Failed to recycle ctrl msg buffer\n");
	}
	mutex_unlock(&uci_ctxt->ctrl_mutex);
}

static void parse_inbound_ack(struct uci_client *uci_handle,
			struct mhi_result *result)
{
	atomic_inc(&uci_handle->avail_pkts);
	uci_log(uci_handle->uci_ipc_log,
		UCI_DBG_VERBOSE,
		"Received cb on chan %d, avail pkts: 0x%x\n",
		uci_handle->in_chan,
		atomic_read(&uci_handle->avail_pkts));
	wake_up(&uci_handle->read_wq);
	if (uci_handle == uci_handle->uci_ctxt->ctrl_client)
		process_rs232_state(uci_handle, result);
}

static void parse_outbound_ack(struct uci_client *uci_handle,
			struct mhi_result *result)
{
	kfree(result->buf_addr);
	uci_log(uci_handle->uci_ipc_log,
		UCI_DBG_VERBOSE,
		"Received ack on chan %d, pending acks: 0x%x\n",
		uci_handle->out_chan,
		atomic_read(&uci_handle->out_pkt_pend_ack));
	atomic_dec(&uci_handle->out_pkt_pend_ack);
	if (mhi_get_free_desc(uci_handle->out_handle))
		wake_up(&uci_handle->write_wq);
}

static void uci_xfer_cb(struct mhi_cb_info *cb_info)
{
	struct uci_client *uci_handle = NULL;
	struct mhi_result *result;

	if (!cb_info || !cb_info->result) {
		uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
			UCI_DBG_CRITICAL,
			"Bad CB info from MHI\n");
		return;
	}

	uci_handle = cb_info->result->user_data;
	switch (cb_info->cb_reason) {
	case MHI_CB_MHI_ENABLED:
		atomic_set(&uci_handle->mhi_disabled, 0);
		break;
	case MHI_CB_MHI_DISABLED:
		atomic_set(&uci_handle->mhi_disabled, 1);
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_INFO,
			"MHI disabled CB received\n");
		process_mhi_disabled_notif_sync(uci_handle);
		break;
	case MHI_CB_XFER:
		if (!cb_info->result) {
			uci_log(uci_handle->uci_ipc_log,
				UCI_DBG_CRITICAL,
				"Failed to obtain mhi result from CB\n");
				return;
		}
		result = cb_info->result;
		if (cb_info->chan % 2)
			parse_inbound_ack(uci_handle, result);
		else
			parse_outbound_ack(uci_handle, result);
		break;
	default:
		uci_log(uci_handle->uci_ipc_log,
			UCI_DBG_VERBOSE,
			"Cannot handle cb reason 0x%x\n",
			cb_info->cb_reason);
	}
}

static int mhi_register_client(struct uci_client *mhi_client)
{
	int ret_val = 0;

	uci_log(mhi_client->uci_ipc_log,
		UCI_DBG_INFO,
		"Setting up workqueues\n");
	init_waitqueue_head(&mhi_client->read_wq);
	init_waitqueue_head(&mhi_client->write_wq);
	mhi_client->out_chan = mhi_client->out_attr.chan_id;
	mhi_client->in_chan = mhi_client->in_attr.chan_id;

	mutex_init(&mhi_client->in_chan_lock);
	mutex_init(&mhi_client->out_chan_lock);
	atomic_set(&mhi_client->mhi_disabled, 1);

	uci_log(mhi_client->uci_ipc_log,
		UCI_DBG_INFO,
		"Registering chan %d\n",
		mhi_client->out_chan);
	ret_val = mhi_register_channel(&mhi_client->out_handle,
			mhi_client->out_chan,
			0,
			&mhi_client->uci_ctxt->client_info,
			mhi_client);
	if (0 != ret_val)
		uci_log(mhi_client->uci_ipc_log,
			UCI_DBG_ERROR,
			"Failed to init outbound chan 0x%x, ret 0x%x\n",
			mhi_client->out_chan,
			ret_val);

	uci_log(mhi_client->uci_ipc_log,
		UCI_DBG_INFO,
		"Registering chan %d\n",
		mhi_client->in_chan);
	ret_val = mhi_register_channel(&mhi_client->in_handle,
			mhi_client->in_chan,
			0,
			&mhi_client->uci_ctxt->client_info,
			mhi_client);
	if (0 != ret_val)
		uci_log(mhi_client->uci_ipc_log,
			UCI_DBG_ERROR,
			"Failed to init inbound chan 0x%x, ret 0x%x\n",
			mhi_client->in_chan,
			ret_val);
	return 0;
}

static const struct file_operations mhi_uci_client_fops = {
	.read = mhi_uci_client_read,
	.write = mhi_uci_client_write,
	.open = mhi_uci_client_open,
	.release = mhi_uci_client_release,
	.poll = mhi_uci_client_poll,
	.unlocked_ioctl = mhi_uci_ctl_ioctl,
};

static int mhi_uci_probe(struct platform_device *pdev)
{
	struct mhi_uci_ctxt_t *uci_ctxt;
	int ret_val;
	int i;
	char node_name[16];

	uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
		UCI_DBG_INFO,
		"Entered with pdev:%p\n",
		pdev);

	if (pdev->dev.of_node == NULL)
		return -ENODEV;

	pdev->id = of_alias_get_id(pdev->dev.of_node, "mhi_uci");
	if (pdev->id < 0)
		return -ENODEV;

	uci_ctxt = devm_kzalloc(&pdev->dev,
				sizeof(*uci_ctxt),
				GFP_KERNEL);
	if (!uci_ctxt)
		return -ENOMEM;

	uci_ctxt->client_info.mhi_client_cb = uci_xfer_cb;
	mutex_init(&uci_ctxt->ctrl_mutex);

	uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
		UCI_DBG_INFO,
		"Setting up channel attributes\n");
	ret_val = uci_init_client_attributes(uci_ctxt,
					     pdev->dev.of_node);
	if (ret_val) {
		uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
			UCI_DBG_ERROR,
			"Failed to init client attributes\n");
		return -EIO;
	}

	uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
		UCI_DBG_INFO,
		"Registering for MHI events\n");
	for (i = 0; i < MHI_SOFTWARE_CLIENT_LIMIT; ++i) {
		struct uci_client *uci_client = &uci_ctxt->client_handles[i];

		uci_client->uci_ctxt = uci_ctxt;
		if (uci_client->in_attr.uci_ownership) {
			ret_val = mhi_register_client(uci_client);
			if (ret_val) {
				uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
					UCI_DBG_CRITICAL,
					"Failed to reg client %d ret %d\n",
					ret_val,
					i);

				return -EIO;
			}
			snprintf(node_name, sizeof(node_name), "mhi-uci%d",
				 uci_client->out_attr.chan_id);
			uci_client->uci_ipc_log = ipc_log_context_create
				(MHI_UCI_IPC_LOG_PAGES,
				 node_name,
				 0);
		}
	}

	uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
		UCI_DBG_INFO,
		"Allocating char devices\n");
	ret_val = alloc_chrdev_region(&uci_ctxt->dev_t,
				      0,
				      MHI_SOFTWARE_CLIENT_LIMIT,
				      DEVICE_NAME);
	if (IS_ERR_VALUE(ret_val)) {
		uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
			UCI_DBG_ERROR,
			"Failed to alloc char devs, ret 0x%x\n", ret_val);
		goto failed_char_alloc;
	}

	uci_log(mhi_uci_drv_ctxt.mhi_uci_ipc_log,
		UCI_DBG_INFO,
		"Setting up device nodes. for dev_t: 0x%x major:0x%x\n",
		uci_ctxt->dev_t,
		MAJOR(uci_ctxt->dev_t));
	for (i = 0; i < MHI_SOFTWARE_CLIENT_LIMIT; ++i) {
		struct uci_client *uci_client = &uci_ctxt->client_handles[i];

		if (uci_client->in_attr.uci_ownership) {
			cdev_init(&uci_ctxt->cdev[i], &mhi_uci_client_fops);
			uci_ctxt->cdev[i].owner = THIS_MODULE;
			ret_val = cdev_add(&uci_ctxt->cdev[i],
					   uci_ctxt->dev_t + i , 1);
			if (IS_ERR_VALUE(ret_val)) {
				uci_log(uci_client->uci_ipc_log,
					UCI_DBG_ERROR,
					"Failed to add cdev %d, ret 0x%x\n",
					i, ret_val);
				goto failed_char_add;
			}
			uci_client->dev =
				device_create(mhi_uci_drv_ctxt.mhi_uci_class,
					      NULL,
					      uci_ctxt->dev_t + i,
					      NULL,
					      DEVICE_NAME "_pipe_%d",
					      uci_client->out_chan);
			if (IS_ERR(uci_client->dev)) {
				uci_log(uci_client->uci_ipc_log,
					UCI_DBG_ERROR,
					"Failed to add cdev %d\n", i);
				cdev_del(&uci_ctxt->cdev[i]);
				ret_val = -EIO;
				goto failed_device_create;
			}
		}
	}
	platform_set_drvdata(pdev, uci_ctxt);
	mutex_lock(&mhi_uci_drv_ctxt.list_lock);
	list_add_tail(&uci_ctxt->node, &mhi_uci_drv_ctxt.head);
	mutex_unlock(&mhi_uci_drv_ctxt.list_lock);
	return 0;

failed_char_add:
failed_device_create:
	while (--i >= 0) {
		cdev_del(&uci_ctxt->cdev[i]);
		device_destroy(mhi_uci_drv_ctxt.mhi_uci_class,
			       MKDEV(MAJOR(uci_ctxt->dev_t), i));
	};

	unregister_chrdev_region(MAJOR(uci_ctxt->dev_t),
				 MHI_SOFTWARE_CLIENT_LIMIT);
failed_char_alloc:

	return ret_val;
};

static int mhi_uci_remove(struct platform_device *pdev)
{
	struct mhi_uci_ctxt_t *uci_ctxt = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < MHI_SOFTWARE_CLIENT_LIMIT; ++i) {
		struct uci_client *uci_client = &uci_ctxt->client_handles[i];

		uci_client->uci_ctxt = uci_ctxt;
		if (uci_client->in_attr.uci_ownership) {
			mhi_deregister_channel(uci_client->out_handle);
			mhi_deregister_channel(uci_client->in_handle);
			cdev_del(&uci_ctxt->cdev[i]);
			device_destroy(mhi_uci_drv_ctxt.mhi_uci_class,
				       MKDEV(MAJOR(uci_ctxt->dev_t), i));
		}
	}

	unregister_chrdev_region(MAJOR(uci_ctxt->dev_t),
				 MHI_SOFTWARE_CLIENT_LIMIT);

	mutex_lock(&mhi_uci_drv_ctxt.list_lock);
	list_del(&uci_ctxt->node);
	mutex_unlock(&mhi_uci_drv_ctxt.list_lock);
	return 0;
};

static struct of_device_id mhi_uci_match_table[] = {
	{.compatible = "qcom,mhi-uci"},
	{},
};

static struct platform_driver mhi_uci_driver = {
	.probe = mhi_uci_probe,
	.remove = mhi_uci_remove,
	.driver = {
		.name = MHI_UCI_DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = mhi_uci_match_table,
	},
};

static int mhi_uci_init(void)
{
	mhi_uci_drv_ctxt.mhi_uci_ipc_log =
		ipc_log_context_create(MHI_UCI_IPC_LOG_PAGES,
				       "mhi-uci",
				       0);
	if (mhi_uci_drv_ctxt.mhi_uci_ipc_log == NULL) {
		uci_log(NULL,
			UCI_DBG_WARNING,
			"Failed to create IPC logging context");
	}

	mhi_uci_drv_ctxt.mhi_uci_class =
		class_create(THIS_MODULE, DEVICE_NAME);

	if (IS_ERR(mhi_uci_drv_ctxt.mhi_uci_class))
		return -ENODEV;

	mutex_init(&mhi_uci_drv_ctxt.list_lock);
	INIT_LIST_HEAD(&mhi_uci_drv_ctxt.head);

	return platform_driver_register(&mhi_uci_driver);
}

static void __exit mhi_uci_exit(void)
{
	class_destroy(mhi_uci_drv_ctxt.mhi_uci_class);
	platform_driver_unregister(&mhi_uci_driver);
}

module_exit(mhi_uci_exit);
module_init(mhi_uci_init);
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("MHI_UCI");
MODULE_DESCRIPTION("MHI UCI Driver");
