/*
 *  History:
 *  Started: Aug 9 by Lawrence Foard (entropy@world.std.com),
 *           to allow user process control of SCSI devices.
 *  Development Sponsored by Killy Corp. NY NY
 *
 * Original driver (sg.c):
 *        Copyright (C) 1992 Lawrence Foard
 * Version 2 and 3 extensions to driver:
 *        Copyright (C) 1998 - 2014 Douglas Gilbert
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 */

static int sg_version_num = 30536;	/* 2 digits for each component */
#define SG_VERSION_STR "3.5.36"

/*
 *  D. P. Gilbert (dgilbert@interlog.com), notes:
 *      - scsi logging is available via SCSI_LOG_TIMEOUT macros. First
 *        the kernel/module needs to be built with CONFIG_SCSI_LOGGING
 *        (otherwise the macros compile to empty statements).
 *
 */
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/aio.h>
#include <linux/errno.h>
#include <linux/mtio.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h>
#include <linux/idr.h>
#include <linux/seq_file.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/blktrace_api.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/ratelimit.h>

#include "scsi.h"
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_driver.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>

#include "scsi_logging.h"

#ifdef CONFIG_SCSI_PROC_FS
#include <linux/proc_fs.h>
static char *sg_version_date = "20140603";

static int sg_proc_init(void);
static void sg_proc_cleanup(void);
#endif

#define SG_ALLOW_DIO_DEF 0

#define SG_MAX_DEVS 32768

/* SG_MAX_CDB_SIZE should be 260 (spc4r37 section 3.1.30) however the type
 * of sg_io_hdr::cmd_len can only represent 255. All SCSI commands greater
 * than 16 bytes are "variable length" whose length is a multiple of 4
 */
#define SG_MAX_CDB_SIZE 252

/*
 * Suppose you want to calculate the formula muldiv(x,m,d)=int(x * m / d)
 * Then when using 32 bit integers x * m may overflow during the calculation.
 * Replacing muldiv(x) by muldiv(x)=((x % d) * m) / d + int(x / d) * m
 * calculates the same, but prevents the overflow when both m and d
 * are "small" numbers (like HZ and USER_HZ).
 * Of course an overflow is inavoidable if the result of muldiv doesn't fit
 * in 32 bits.
 */
#define MULDIV(X,MUL,DIV) ((((X % DIV) * MUL) / DIV) + ((X / DIV) * MUL))

#define SG_DEFAULT_TIMEOUT MULDIV(SG_DEFAULT_TIMEOUT_USER, HZ, USER_HZ)

int sg_big_buff = SG_DEF_RESERVED_SIZE;
/* N.B. This variable is readable and writeable via
   /proc/scsi/sg/def_reserved_size . Each time sg_open() is called a buffer
   of this size (or less if there is not enough memory) will be reserved
   for use by this file descriptor. [Deprecated usage: this variable is also
   readable via /proc/sys/kernel/sg-big-buff if the sg driver is built into
   the kernel (i.e. it is not a module).] */
static int def_reserved_size = -1;	/* picks up init parameter */
static int sg_allow_dio = SG_ALLOW_DIO_DEF;

static int scatter_elem_sz = SG_SCATTER_SZ;
static int scatter_elem_sz_prev = SG_SCATTER_SZ;

#define SG_SECTOR_SZ 512

static int sg_add_device(struct device *, struct class_interface *);
static void sg_remove_device(struct device *, struct class_interface *);

static DEFINE_IDR(sg_index_idr);
static DEFINE_RWLOCK(sg_index_lock);	/* Also used to lock
							   file descriptor list for device */

static struct class_interface sg_interface = {
	.add_dev        = sg_add_device,
	.remove_dev     = sg_remove_device,
};

typedef struct sg_scatter_hold { /* holding area for scsi scatter gather info */
	unsigned short k_use_sg; /* Count of kernel scatter-gather pieces */
	unsigned sglist_len; /* size of malloc'd scatter-gather list ++ */
	unsigned bufflen;	/* Size of (aggregate) data buffer */
	struct page **pages;
	int page_order;
	char dio_in_use;	/* 0->indirect IO (or mmap), 1->dio */
	unsigned char cmd_opcode; /* first byte of command */
} Sg_scatter_hold;

struct sg_device;		/* forward declarations */
struct sg_fd;

typedef struct sg_request {	/* SG_MAX_QUEUE requests outstanding per file */
	struct sg_request *nextrp;	/* NULL -> tail request (slist) */
	struct sg_fd *parentfp;	/* NULL -> not in use */
	Sg_scatter_hold data;	/* hold buffer, perhaps scatter list */
	sg_io_hdr_t header;	/* scsi command+info, see <scsi/sg.h> */
	unsigned char sense_b[SCSI_SENSE_BUFFERSIZE];
	char res_used;		/* 1 -> using reserve buffer, 0 -> not ... */
	char orphan;		/* 1 -> drop on sight, 0 -> normal */
	char sg_io_owned;	/* 1 -> packet belongs to SG_IO */
	/* done protected by rq_list_lock */
	char done;		/* 0->before bh, 1->before read, 2->read */
	struct request *rq;
	struct bio *bio;
	struct execute_work ew;
} Sg_request;

typedef struct sg_fd {		/* holds the state of a file descriptor */
	struct list_head sfd_siblings;  /* protected by device's sfd_lock */
	struct sg_device *parentdp;	/* owning device */
	wait_queue_head_t read_wait;	/* queue read until command done */
	rwlock_t rq_list_lock;	/* protect access to list in req_arr */
	int timeout;		/* defaults to SG_DEFAULT_TIMEOUT      */
	int timeout_user;	/* defaults to SG_DEFAULT_TIMEOUT_USER */
	Sg_scatter_hold reserve;	/* buffer held for this file descriptor */
	unsigned save_scat_len;	/* original length of trunc. scat. element */
	Sg_request *headrp;	/* head of request slist, NULL->empty */
	struct fasync_struct *async_qp;	/* used by asynchronous notification */
	Sg_request req_arr[SG_MAX_QUEUE];	/* used as singly-linked list */
	char low_dma;		/* as in parent but possibly overridden to 1 */
	char force_packid;	/* 1 -> pack_id input to read(), 0 -> ignored */
	char cmd_q;		/* 1 -> allow command queuing, 0 -> don't */
	unsigned char next_cmd_len; /* 0: automatic, >0: use on next write() */
	char keep_orphan;	/* 0 -> drop orphan (def), 1 -> keep for read() */
	char mmap_called;	/* 0 -> mmap() never called on this fd */
	struct kref f_ref;
	struct execute_work ew;
} Sg_fd;

typedef struct sg_device { /* holds the state of each scsi generic device */
	struct scsi_device *device;
	wait_queue_head_t open_wait;    /* queue open() when O_EXCL present */
	struct mutex open_rel_lock;     /* held when in open() or release() */
	int sg_tablesize;	/* adapter's max scatter-gather table size */
	u32 index;		/* device index number */
	struct list_head sfds;
	rwlock_t sfd_lock;      /* protect access to sfd list */
	atomic_t detaching;     /* 0->device usable, 1->device detaching */
	bool exclude;		/* 1->open(O_EXCL) succeeded and is active */
	int open_cnt;		/* count of opens (perhaps < num(sfds) ) */
	char sgdebug;		/* 0->off, 1->sense, 9->dump dev, 10-> all devs */
	struct gendisk *disk;
	struct cdev * cdev;	/* char_dev [sysfs: /sys/cdev/major/sg<n>] */
	struct kref d_ref;
} Sg_device;

/* tasklet or soft irq callback */
static void sg_rq_end_io(struct request *rq, int uptodate);
static int sg_start_req(Sg_request *srp, unsigned char *cmd);
static int sg_finish_rem_req(Sg_request * srp);
static int sg_build_indirect(Sg_scatter_hold * schp, Sg_fd * sfp, int buff_size);
static ssize_t sg_new_read(Sg_fd * sfp, char __user *buf, size_t count,
			   Sg_request * srp);
static ssize_t sg_new_write(Sg_fd *sfp, struct file *file,
			const char __user *buf, size_t count, int blocking,
			int read_only, int sg_io_owned, Sg_request **o_srp);
static int sg_common_write(Sg_fd * sfp, Sg_request * srp,
			   unsigned char *cmnd, int timeout, int blocking);
static int sg_read_oxfer(Sg_request * srp, char __user *outp, int num_read_xfer);
static void sg_remove_scat(Sg_fd * sfp, Sg_scatter_hold * schp);
static void sg_build_reserve(Sg_fd * sfp, int req_size);
static void sg_link_reserve(Sg_fd * sfp, Sg_request * srp, int size);
static void sg_unlink_reserve(Sg_fd * sfp, Sg_request * srp);
static Sg_fd *sg_add_sfp(Sg_device * sdp);
static void sg_remove_sfp(struct kref *);
static Sg_request *sg_get_rq_mark(Sg_fd * sfp, int pack_id);
static Sg_request *sg_add_request(Sg_fd * sfp);
static int sg_remove_request(Sg_fd * sfp, Sg_request * srp);
static int sg_res_in_use(Sg_fd * sfp);
static Sg_device *sg_get_dev(int dev);
static void sg_device_destroy(struct kref *kref);

#define SZ_SG_HEADER sizeof(struct sg_header)
#define SZ_SG_IO_HDR sizeof(sg_io_hdr_t)
#define SZ_SG_IOVEC sizeof(sg_iovec_t)
#define SZ_SG_REQ_INFO sizeof(sg_req_info_t)

#define sg_printk(prefix, sdp, fmt, a...) \
	sdev_printk(prefix, (sdp)->device, "[%s] " fmt, \
		    (sdp)->disk->disk_name, ##a)

static int sg_allow_access(struct file *filp, unsigned char *cmd)
{
	struct sg_fd *sfp = filp->private_data;

	if (sfp->parentdp->device->type == TYPE_SCANNER)
		return 0;

	return blk_verify_command(cmd, filp->f_mode & FMODE_WRITE);
}

static int
open_wait(Sg_device *sdp, int flags)
{
	int retval = 0;

	if (flags & O_EXCL) {
		while (sdp->open_cnt > 0) {
			mutex_unlock(&sdp->open_rel_lock);
			retval = wait_event_interruptible(sdp->open_wait,
					(atomic_read(&sdp->detaching) ||
					 !sdp->open_cnt));
			mutex_lock(&sdp->open_rel_lock);

			if (retval) /* -ERESTARTSYS */
				return retval;
			if (atomic_read(&sdp->detaching))
				return -ENODEV;
		}
	} else {
		while (sdp->exclude) {
			mutex_unlock(&sdp->open_rel_lock);
			retval = wait_event_interruptible(sdp->open_wait,
					(atomic_read(&sdp->detaching) ||
					 !sdp->exclude));
			mutex_lock(&sdp->open_rel_lock);

			if (retval) /* -ERESTARTSYS */
				return retval;
			if (atomic_read(&sdp->detaching))
				return -ENODEV;
		}
	}

	return retval;
}

/* Returns 0 on success, else a negated errno value */
static int
sg_open(struct inode *inode, struct file *filp)
{
	int dev = iminor(inode);
	int flags = filp->f_flags;
	struct request_queue *q;
	Sg_device *sdp;
	Sg_fd *sfp;
	int retval;

	nonseekable_open(inode, filp);
	if ((flags & O_EXCL) && (O_RDONLY == (flags & O_ACCMODE)))
		return -EPERM; /* Can't lock it with read only access */
	sdp = sg_get_dev(dev);
	if (IS_ERR(sdp))
		return PTR_ERR(sdp);

	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_open: flags=0x%x\n", flags));

	/* This driver's module count bumped by fops_get in <linux/fs.h> */
	/* Prevent the device driver from vanishing while we sleep */
	retval = scsi_device_get(sdp->device);
	if (retval)
		goto sg_put;

	retval = scsi_autopm_get_device(sdp->device);
	if (retval)
		goto sdp_put;

	/* scsi_block_when_processing_errors() may block so bypass
	 * check if O_NONBLOCK. Permits SCSI commands to be issued
	 * during error recovery. Tread carefully. */
	if (!((flags & O_NONBLOCK) ||
	      scsi_block_when_processing_errors(sdp->device))) {
		retval = -ENXIO;
		/* we are in error recovery for this device */
		goto error_out;
	}

	mutex_lock(&sdp->open_rel_lock);
	if (flags & O_NONBLOCK) {
		if (flags & O_EXCL) {
			if (sdp->open_cnt > 0) {
				retval = -EBUSY;
				goto error_mutex_locked;
			}
		} else {
			if (sdp->exclude) {
				retval = -EBUSY;
				goto error_mutex_locked;
			}
		}
	} else {
		retval = open_wait(sdp, flags);
		if (retval) /* -ERESTARTSYS or -ENODEV */
			goto error_mutex_locked;
	}

	/* N.B. at this point we are holding the open_rel_lock */
	if (flags & O_EXCL)
		sdp->exclude = true;

	if (sdp->open_cnt < 1) {  /* no existing opens */
		sdp->sgdebug = 0;
		q = sdp->device->request_queue;
		sdp->sg_tablesize = queue_max_segments(q);
	}
	sfp = sg_add_sfp(sdp);
	if (IS_ERR(sfp)) {
		retval = PTR_ERR(sfp);
		goto out_undo;
	}

	filp->private_data = sfp;
	sdp->open_cnt++;
	mutex_unlock(&sdp->open_rel_lock);

	retval = 0;
sg_put:
	kref_put(&sdp->d_ref, sg_device_destroy);
	return retval;

out_undo:
	if (flags & O_EXCL) {
		sdp->exclude = false;   /* undo if error */
		wake_up_interruptible(&sdp->open_wait);
	}
error_mutex_locked:
	mutex_unlock(&sdp->open_rel_lock);
error_out:
	scsi_autopm_put_device(sdp->device);
sdp_put:
	scsi_device_put(sdp->device);
	goto sg_put;
}

/* Release resources associated with a successful sg_open()
 * Returns 0 on success, else a negated errno value */
static int
sg_release(struct inode *inode, struct file *filp)
{
	Sg_device *sdp;
	Sg_fd *sfp;

	if ((!(sfp = (Sg_fd *) filp->private_data)) || (!(sdp = sfp->parentdp)))
		return -ENXIO;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp, "sg_release\n"));

	mutex_lock(&sdp->open_rel_lock);
	scsi_autopm_put_device(sdp->device);
	kref_put(&sfp->f_ref, sg_remove_sfp);
	sdp->open_cnt--;

	/* possibly many open()s waiting on exlude clearing, start many;
	 * only open(O_EXCL)s wait on 0==open_cnt so only start one */
	if (sdp->exclude) {
		sdp->exclude = false;
		wake_up_interruptible_all(&sdp->open_wait);
	} else if (0 == sdp->open_cnt) {
		wake_up_interruptible(&sdp->open_wait);
	}
	mutex_unlock(&sdp->open_rel_lock);
	return 0;
}

static ssize_t
sg_read(struct file *filp, char __user *buf, size_t count, loff_t * ppos)
{
	Sg_device *sdp;
	Sg_fd *sfp;
	Sg_request *srp;
	int req_pack_id = -1;
	sg_io_hdr_t *hp;
	struct sg_header *old_hdr = NULL;
	int retval = 0;

	if ((!(sfp = (Sg_fd *) filp->private_data)) || (!(sdp = sfp->parentdp)))
		return -ENXIO;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_read: count=%d\n", (int) count));

	if (!access_ok(VERIFY_WRITE, buf, count))
		return -EFAULT;
	if (sfp->force_packid && (count >= SZ_SG_HEADER)) {
		old_hdr = kmalloc(SZ_SG_HEADER, GFP_KERNEL);
		if (!old_hdr)
			return -ENOMEM;
		if (__copy_from_user(old_hdr, buf, SZ_SG_HEADER)) {
			retval = -EFAULT;
			goto free_old_hdr;
		}
		if (old_hdr->reply_len < 0) {
			if (count >= SZ_SG_IO_HDR) {
				sg_io_hdr_t *new_hdr;
				new_hdr = kmalloc(SZ_SG_IO_HDR, GFP_KERNEL);
				if (!new_hdr) {
					retval = -ENOMEM;
					goto free_old_hdr;
				}
				retval =__copy_from_user
				    (new_hdr, buf, SZ_SG_IO_HDR);
				req_pack_id = new_hdr->pack_id;
				kfree(new_hdr);
				if (retval) {
					retval = -EFAULT;
					goto free_old_hdr;
				}
			}
		} else
			req_pack_id = old_hdr->pack_id;
	}
	srp = sg_get_rq_mark(sfp, req_pack_id);
	if (!srp) {		/* now wait on packet to arrive */
		if (atomic_read(&sdp->detaching)) {
			retval = -ENODEV;
			goto free_old_hdr;
		}
		if (filp->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto free_old_hdr;
		}
		retval = wait_event_interruptible(sfp->read_wait,
			(atomic_read(&sdp->detaching) ||
			(srp = sg_get_rq_mark(sfp, req_pack_id))));
		if (atomic_read(&sdp->detaching)) {
			retval = -ENODEV;
			goto free_old_hdr;
		}
		if (retval) {
			/* -ERESTARTSYS as signal hit process */
			goto free_old_hdr;
		}
	}
	if (srp->header.interface_id != '\0') {
		retval = sg_new_read(sfp, buf, count, srp);
		goto free_old_hdr;
	}

	hp = &srp->header;
	if (old_hdr == NULL) {
		old_hdr = kmalloc(SZ_SG_HEADER, GFP_KERNEL);
		if (! old_hdr) {
			retval = -ENOMEM;
			goto free_old_hdr;
		}
	}
	memset(old_hdr, 0, SZ_SG_HEADER);
	old_hdr->reply_len = (int) hp->timeout;
	old_hdr->pack_len = old_hdr->reply_len; /* old, strange behaviour */
	old_hdr->pack_id = hp->pack_id;
	old_hdr->twelve_byte =
	    ((srp->data.cmd_opcode >= 0xc0) && (12 == hp->cmd_len)) ? 1 : 0;
	old_hdr->target_status = hp->masked_status;
	old_hdr->host_status = hp->host_status;
	old_hdr->driver_status = hp->driver_status;
	if ((CHECK_CONDITION & hp->masked_status) ||
	    (DRIVER_SENSE & hp->driver_status))
		memcpy(old_hdr->sense_buffer, srp->sense_b,
		       sizeof (old_hdr->sense_buffer));
	switch (hp->host_status) {
	/* This setup of 'result' is for backward compatibility and is best
	   ignored by the user who should use target, host + driver status */
	case DID_OK:
	case DID_PASSTHROUGH:
	case DID_SOFT_ERROR:
		old_hdr->result = 0;
		break;
	case DID_NO_CONNECT:
	case DID_BUS_BUSY:
	case DID_TIME_OUT:
		old_hdr->result = EBUSY;
		break;
	case DID_BAD_TARGET:
	case DID_ABORT:
	case DID_PARITY:
	case DID_RESET:
	case DID_BAD_INTR:
		old_hdr->result = EIO;
		break;
	case DID_ERROR:
		old_hdr->result = (srp->sense_b[0] == 0 && 
				  hp->masked_status == GOOD) ? 0 : EIO;
		break;
	default:
		old_hdr->result = EIO;
		break;
	}

	/* Now copy the result back to the user buffer.  */
	if (count >= SZ_SG_HEADER) {
		if (__copy_to_user(buf, old_hdr, SZ_SG_HEADER)) {
			retval = -EFAULT;
			goto free_old_hdr;
		}
		buf += SZ_SG_HEADER;
		if (count > old_hdr->reply_len)
			count = old_hdr->reply_len;
		if (count > SZ_SG_HEADER) {
			if (sg_read_oxfer(srp, buf, count - SZ_SG_HEADER)) {
				retval = -EFAULT;
				goto free_old_hdr;
			}
		}
	} else
		count = (old_hdr->result == 0) ? 0 : -EIO;
	sg_finish_rem_req(srp);
	retval = count;
free_old_hdr:
	kfree(old_hdr);
	return retval;
}

static ssize_t
sg_new_read(Sg_fd * sfp, char __user *buf, size_t count, Sg_request * srp)
{
	sg_io_hdr_t *hp = &srp->header;
	int err = 0;
	int len;

	if (count < SZ_SG_IO_HDR) {
		err = -EINVAL;
		goto err_out;
	}
	hp->sb_len_wr = 0;
	if ((hp->mx_sb_len > 0) && hp->sbp) {
		if ((CHECK_CONDITION & hp->masked_status) ||
		    (DRIVER_SENSE & hp->driver_status)) {
			int sb_len = SCSI_SENSE_BUFFERSIZE;
			sb_len = (hp->mx_sb_len > sb_len) ? sb_len : hp->mx_sb_len;
			len = 8 + (int) srp->sense_b[7];	/* Additional sense length field */
			len = (len > sb_len) ? sb_len : len;
			if (copy_to_user(hp->sbp, srp->sense_b, len)) {
				err = -EFAULT;
				goto err_out;
			}
			hp->sb_len_wr = len;
		}
	}
	if (hp->masked_status || hp->host_status || hp->driver_status)
		hp->info |= SG_INFO_CHECK;
	if (copy_to_user(buf, hp, SZ_SG_IO_HDR)) {
		err = -EFAULT;
		goto err_out;
	}
err_out:
	err = sg_finish_rem_req(srp);
	return (0 == err) ? count : err;
}

static ssize_t
sg_write(struct file *filp, const char __user *buf, size_t count, loff_t * ppos)
{
	int mxsize, cmd_size, k;
	int input_size, blocking;
	unsigned char opcode;
	Sg_device *sdp;
	Sg_fd *sfp;
	Sg_request *srp;
	struct sg_header old_hdr;
	sg_io_hdr_t *hp;
	unsigned char cmnd[SG_MAX_CDB_SIZE];

	if ((!(sfp = (Sg_fd *) filp->private_data)) || (!(sdp = sfp->parentdp)))
		return -ENXIO;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_write: count=%d\n", (int) count));
	if (atomic_read(&sdp->detaching))
		return -ENODEV;
	if (!((filp->f_flags & O_NONBLOCK) ||
	      scsi_block_when_processing_errors(sdp->device)))
		return -ENXIO;

	if (!access_ok(VERIFY_READ, buf, count))
		return -EFAULT;	/* protects following copy_from_user()s + get_user()s */
	if (count < SZ_SG_HEADER)
		return -EIO;
	if (__copy_from_user(&old_hdr, buf, SZ_SG_HEADER))
		return -EFAULT;
	blocking = !(filp->f_flags & O_NONBLOCK);
	if (old_hdr.reply_len < 0)
		return sg_new_write(sfp, filp, buf, count,
				    blocking, 0, 0, NULL);
	if (count < (SZ_SG_HEADER + 6))
		return -EIO;	/* The minimum scsi command length is 6 bytes. */

	if (!(srp = sg_add_request(sfp))) {
		SCSI_LOG_TIMEOUT(1, sg_printk(KERN_INFO, sdp,
					      "sg_write: queue full\n"));
		return -EDOM;
	}
	buf += SZ_SG_HEADER;
	__get_user(opcode, buf);
	if (sfp->next_cmd_len > 0) {
		cmd_size = sfp->next_cmd_len;
		sfp->next_cmd_len = 0;	/* reset so only this write() effected */
	} else {
		cmd_size = COMMAND_SIZE(opcode);	/* based on SCSI command group */
		if ((opcode >= 0xc0) && old_hdr.twelve_byte)
			cmd_size = 12;
	}
	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sdp,
		"sg_write:   scsi opcode=0x%02x, cmd_size=%d\n", (int) opcode, cmd_size));
/* Determine buffer size.  */
	input_size = count - cmd_size;
	mxsize = (input_size > old_hdr.reply_len) ? input_size : old_hdr.reply_len;
	mxsize -= SZ_SG_HEADER;
	input_size -= SZ_SG_HEADER;
	if (input_size < 0) {
		sg_remove_request(sfp, srp);
		return -EIO;	/* User did not pass enough bytes for this command. */
	}
	hp = &srp->header;
	hp->interface_id = '\0';	/* indicator of old interface tunnelled */
	hp->cmd_len = (unsigned char) cmd_size;
	hp->iovec_count = 0;
	hp->mx_sb_len = 0;
	if (input_size > 0)
		hp->dxfer_direction = (old_hdr.reply_len > SZ_SG_HEADER) ?
		    SG_DXFER_TO_FROM_DEV : SG_DXFER_TO_DEV;
	else
		hp->dxfer_direction = (mxsize > 0) ? SG_DXFER_FROM_DEV : SG_DXFER_NONE;
	hp->dxfer_len = mxsize;
	if (hp->dxfer_direction == SG_DXFER_TO_DEV)
		hp->dxferp = (char __user *)buf + cmd_size;
	else
		hp->dxferp = NULL;
	hp->sbp = NULL;
	hp->timeout = old_hdr.reply_len;	/* structure abuse ... */
	hp->flags = input_size;	/* structure abuse ... */
	hp->pack_id = old_hdr.pack_id;
	hp->usr_ptr = NULL;
	if (__copy_from_user(cmnd, buf, cmd_size))
		return -EFAULT;
	/*
	 * SG_DXFER_TO_FROM_DEV is functionally equivalent to SG_DXFER_FROM_DEV,
	 * but is is possible that the app intended SG_DXFER_TO_DEV, because there
	 * is a non-zero input_size, so emit a warning.
	 */
	if (hp->dxfer_direction == SG_DXFER_TO_FROM_DEV) {
		static char cmd[TASK_COMM_LEN];
		if (strcmp(current->comm, cmd)) {
			printk_ratelimited(KERN_WARNING
					   "sg_write: data in/out %d/%d bytes "
					   "for SCSI command 0x%x-- guessing "
					   "data in;\n   program %s not setting "
					   "count and/or reply_len properly\n",
					   old_hdr.reply_len - (int)SZ_SG_HEADER,
					   input_size, (unsigned int) cmnd[0],
					   current->comm);
			strcpy(cmd, current->comm);
		}
	}
	k = sg_common_write(sfp, srp, cmnd, sfp->timeout, blocking);
	return (k < 0) ? k : count;
}

static ssize_t
sg_new_write(Sg_fd *sfp, struct file *file, const char __user *buf,
		 size_t count, int blocking, int read_only, int sg_io_owned,
		 Sg_request **o_srp)
{
	int k;
	Sg_request *srp;
	sg_io_hdr_t *hp;
	unsigned char cmnd[SG_MAX_CDB_SIZE];
	int timeout;
	unsigned long ul_timeout;

	if (count < SZ_SG_IO_HDR)
		return -EINVAL;
	if (!access_ok(VERIFY_READ, buf, count))
		return -EFAULT; /* protects following copy_from_user()s + get_user()s */

	sfp->cmd_q = 1;	/* when sg_io_hdr seen, set command queuing on */
	if (!(srp = sg_add_request(sfp))) {
		SCSI_LOG_TIMEOUT(1, sg_printk(KERN_INFO, sfp->parentdp,
					      "sg_new_write: queue full\n"));
		return -EDOM;
	}
	srp->sg_io_owned = sg_io_owned;
	hp = &srp->header;
	if (__copy_from_user(hp, buf, SZ_SG_IO_HDR)) {
		sg_remove_request(sfp, srp);
		return -EFAULT;
	}
	if (hp->interface_id != 'S') {
		sg_remove_request(sfp, srp);
		return -ENOSYS;
	}
	if (hp->flags & SG_FLAG_MMAP_IO) {
		if (hp->dxfer_len > sfp->reserve.bufflen) {
			sg_remove_request(sfp, srp);
			return -ENOMEM;	/* MMAP_IO size must fit in reserve buffer */
		}
		if (hp->flags & SG_FLAG_DIRECT_IO) {
			sg_remove_request(sfp, srp);
			return -EINVAL;	/* either MMAP_IO or DIRECT_IO (not both) */
		}
		if (sg_res_in_use(sfp)) {
			sg_remove_request(sfp, srp);
			return -EBUSY;	/* reserve buffer already being used */
		}
	}
	ul_timeout = msecs_to_jiffies(srp->header.timeout);
	timeout = (ul_timeout < INT_MAX) ? ul_timeout : INT_MAX;
	if ((!hp->cmdp) || (hp->cmd_len < 6) || (hp->cmd_len > sizeof (cmnd))) {
		sg_remove_request(sfp, srp);
		return -EMSGSIZE;
	}
	if (!access_ok(VERIFY_READ, hp->cmdp, hp->cmd_len)) {
		sg_remove_request(sfp, srp);
		return -EFAULT;	/* protects following copy_from_user()s + get_user()s */
	}
	if (__copy_from_user(cmnd, hp->cmdp, hp->cmd_len)) {
		sg_remove_request(sfp, srp);
		return -EFAULT;
	}
	if (read_only && sg_allow_access(file, cmnd)) {
		sg_remove_request(sfp, srp);
		return -EPERM;
	}
	k = sg_common_write(sfp, srp, cmnd, timeout, blocking);
	if (k < 0)
		return k;
	if (o_srp)
		*o_srp = srp;
	return count;
}

static int
sg_common_write(Sg_fd * sfp, Sg_request * srp,
		unsigned char *cmnd, int timeout, int blocking)
{
	int k, data_dir, at_head;
	Sg_device *sdp = sfp->parentdp;
	sg_io_hdr_t *hp = &srp->header;

	srp->data.cmd_opcode = cmnd[0];	/* hold opcode of command */
	hp->status = 0;
	hp->masked_status = 0;
	hp->msg_status = 0;
	hp->info = 0;
	hp->host_status = 0;
	hp->driver_status = 0;
	hp->resid = 0;
	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
			"sg_common_write:  scsi opcode=0x%02x, cmd_size=%d\n",
			(int) cmnd[0], (int) hp->cmd_len));

	k = sg_start_req(srp, cmnd);
	if (k) {
		SCSI_LOG_TIMEOUT(1, sg_printk(KERN_INFO, sfp->parentdp,
			"sg_common_write: start_req err=%d\n", k));
		sg_finish_rem_req(srp);
		return k;	/* probably out of space --> ENOMEM */
	}
	if (atomic_read(&sdp->detaching)) {
		if (srp->bio)
			blk_end_request_all(srp->rq, -EIO);
		sg_finish_rem_req(srp);
		return -ENODEV;
	}

	switch (hp->dxfer_direction) {
	case SG_DXFER_TO_FROM_DEV:
	case SG_DXFER_FROM_DEV:
		data_dir = DMA_FROM_DEVICE;
		break;
	case SG_DXFER_TO_DEV:
		data_dir = DMA_TO_DEVICE;
		break;
	case SG_DXFER_UNKNOWN:
		data_dir = DMA_BIDIRECTIONAL;
		break;
	default:
		data_dir = DMA_NONE;
		break;
	}
	hp->duration = jiffies_to_msecs(jiffies);
	if (hp->interface_id != '\0' &&	/* v3 (or later) interface */
	    (SG_FLAG_Q_AT_TAIL & hp->flags))
		at_head = 0;
	else
		at_head = 1;

	srp->rq->timeout = timeout;
	kref_get(&sfp->f_ref); /* sg_rq_end_io() does kref_put(). */
	blk_execute_rq_nowait(sdp->device->request_queue, sdp->disk,
			      srp->rq, at_head, sg_rq_end_io);
	return 0;
}

static int srp_done(Sg_fd *sfp, Sg_request *srp)
{
	unsigned long flags;
	int ret;

	read_lock_irqsave(&sfp->rq_list_lock, flags);
	ret = srp->done;
	read_unlock_irqrestore(&sfp->rq_list_lock, flags);
	return ret;
}

static int max_sectors_bytes(struct request_queue *q)
{
	unsigned int max_sectors = queue_max_sectors(q);

	max_sectors = min_t(unsigned int, max_sectors, INT_MAX >> 9);

	return max_sectors << 9;
}

static long
sg_ioctl(struct file *filp, unsigned int cmd_in, unsigned long arg)
{
	void __user *p = (void __user *)arg;
	int __user *ip = p;
	int result, val, read_only;
	Sg_device *sdp;
	Sg_fd *sfp;
	Sg_request *srp;
	unsigned long iflags;

	if ((!(sfp = (Sg_fd *) filp->private_data)) || (!(sdp = sfp->parentdp)))
		return -ENXIO;

	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				   "sg_ioctl: cmd=0x%x\n", (int) cmd_in));
	read_only = (O_RDWR != (filp->f_flags & O_ACCMODE));

	switch (cmd_in) {
	case SG_IO:
		if (atomic_read(&sdp->detaching))
			return -ENODEV;
		if (!scsi_block_when_processing_errors(sdp->device))
			return -ENXIO;
		if (!access_ok(VERIFY_WRITE, p, SZ_SG_IO_HDR))
			return -EFAULT;
		result = sg_new_write(sfp, filp, p, SZ_SG_IO_HDR,
				 1, read_only, 1, &srp);
		if (result < 0)
			return result;
		result = wait_event_interruptible(sfp->read_wait,
			(srp_done(sfp, srp) || atomic_read(&sdp->detaching)));
		if (atomic_read(&sdp->detaching))
			return -ENODEV;
		write_lock_irq(&sfp->rq_list_lock);
		if (srp->done) {
			srp->done = 2;
			write_unlock_irq(&sfp->rq_list_lock);
			result = sg_new_read(sfp, p, SZ_SG_IO_HDR, srp);
			return (result < 0) ? result : 0;
		}
		srp->orphan = 1;
		write_unlock_irq(&sfp->rq_list_lock);
		return result;	/* -ERESTARTSYS because signal hit process */
	case SG_SET_TIMEOUT:
		result = get_user(val, ip);
		if (result)
			return result;
		if (val < 0)
			return -EIO;
		if (val >= MULDIV (INT_MAX, USER_HZ, HZ))
		    val = MULDIV (INT_MAX, USER_HZ, HZ);
		sfp->timeout_user = val;
		sfp->timeout = MULDIV (val, HZ, USER_HZ);

		return 0;
	case SG_GET_TIMEOUT:	/* N.B. User receives timeout as return value */
				/* strange ..., for backward compatibility */
		return sfp->timeout_user;
	case SG_SET_FORCE_LOW_DMA:
		result = get_user(val, ip);
		if (result)
			return result;
		if (val) {
			sfp->low_dma = 1;
			if ((0 == sfp->low_dma) && (0 == sg_res_in_use(sfp))) {
				val = (int) sfp->reserve.bufflen;
				sg_remove_scat(sfp, &sfp->reserve);
				sg_build_reserve(sfp, val);
			}
		} else {
			if (atomic_read(&sdp->detaching))
				return -ENODEV;
			sfp->low_dma = sdp->device->host->unchecked_isa_dma;
		}
		return 0;
	case SG_GET_LOW_DMA:
		return put_user((int) sfp->low_dma, ip);
	case SG_GET_SCSI_ID:
		if (!access_ok(VERIFY_WRITE, p, sizeof (sg_scsi_id_t)))
			return -EFAULT;
		else {
			sg_scsi_id_t __user *sg_idp = p;

			if (atomic_read(&sdp->detaching))
				return -ENODEV;
			__put_user((int) sdp->device->host->host_no,
				   &sg_idp->host_no);
			__put_user((int) sdp->device->channel,
				   &sg_idp->channel);
			__put_user((int) sdp->device->id, &sg_idp->scsi_id);
			__put_user((int) sdp->device->lun, &sg_idp->lun);
			__put_user((int) sdp->device->type, &sg_idp->scsi_type);
			__put_user((short) sdp->device->host->cmd_per_lun,
				   &sg_idp->h_cmd_per_lun);
			__put_user((short) sdp->device->queue_depth,
				   &sg_idp->d_queue_depth);
			__put_user(0, &sg_idp->unused[0]);
			__put_user(0, &sg_idp->unused[1]);
			return 0;
		}
	case SG_SET_FORCE_PACK_ID:
		result = get_user(val, ip);
		if (result)
			return result;
		sfp->force_packid = val ? 1 : 0;
		return 0;
	case SG_GET_PACK_ID:
		if (!access_ok(VERIFY_WRITE, ip, sizeof (int)))
			return -EFAULT;
		read_lock_irqsave(&sfp->rq_list_lock, iflags);
		for (srp = sfp->headrp; srp; srp = srp->nextrp) {
			if ((1 == srp->done) && (!srp->sg_io_owned)) {
				read_unlock_irqrestore(&sfp->rq_list_lock,
						       iflags);
				__put_user(srp->header.pack_id, ip);
				return 0;
			}
		}
		read_unlock_irqrestore(&sfp->rq_list_lock, iflags);
		__put_user(-1, ip);
		return 0;
	case SG_GET_NUM_WAITING:
		read_lock_irqsave(&sfp->rq_list_lock, iflags);
		for (val = 0, srp = sfp->headrp; srp; srp = srp->nextrp) {
			if ((1 == srp->done) && (!srp->sg_io_owned))
				++val;
		}
		read_unlock_irqrestore(&sfp->rq_list_lock, iflags);
		return put_user(val, ip);
	case SG_GET_SG_TABLESIZE:
		return put_user(sdp->sg_tablesize, ip);
	case SG_SET_RESERVED_SIZE:
		result = get_user(val, ip);
		if (result)
			return result;
                if (val < 0)
                        return -EINVAL;
		val = min_t(int, val,
			    max_sectors_bytes(sdp->device->request_queue));
		if (val != sfp->reserve.bufflen) {
			if (sg_res_in_use(sfp) || sfp->mmap_called)
				return -EBUSY;
			sg_remove_scat(sfp, &sfp->reserve);
			sg_build_reserve(sfp, val);
		}
		return 0;
	case SG_GET_RESERVED_SIZE:
		val = min_t(int, sfp->reserve.bufflen,
			    max_sectors_bytes(sdp->device->request_queue));
		return put_user(val, ip);
	case SG_SET_COMMAND_Q:
		result = get_user(val, ip);
		if (result)
			return result;
		sfp->cmd_q = val ? 1 : 0;
		return 0;
	case SG_GET_COMMAND_Q:
		return put_user((int) sfp->cmd_q, ip);
	case SG_SET_KEEP_ORPHAN:
		result = get_user(val, ip);
		if (result)
			return result;
		sfp->keep_orphan = val;
		return 0;
	case SG_GET_KEEP_ORPHAN:
		return put_user((int) sfp->keep_orphan, ip);
	case SG_NEXT_CMD_LEN:
		result = get_user(val, ip);
		if (result)
			return result;
		sfp->next_cmd_len = (val > 0) ? val : 0;
		return 0;
	case SG_GET_VERSION_NUM:
		return put_user(sg_version_num, ip);
	case SG_GET_ACCESS_COUNT:
		/* faked - we don't have a real access count anymore */
		val = (sdp->device ? 1 : 0);
		return put_user(val, ip);
	case SG_GET_REQUEST_TABLE:
		if (!access_ok(VERIFY_WRITE, p, SZ_SG_REQ_INFO * SG_MAX_QUEUE))
			return -EFAULT;
		else {
			sg_req_info_t *rinfo;
			unsigned int ms;

			rinfo = kmalloc(SZ_SG_REQ_INFO * SG_MAX_QUEUE,
								GFP_KERNEL);
			if (!rinfo)
				return -ENOMEM;
			read_lock_irqsave(&sfp->rq_list_lock, iflags);
			for (srp = sfp->headrp, val = 0; val < SG_MAX_QUEUE;
			     ++val, srp = srp ? srp->nextrp : srp) {
				memset(&rinfo[val], 0, SZ_SG_REQ_INFO);
				if (srp) {
					rinfo[val].req_state = srp->done + 1;
					rinfo[val].problem =
					    srp->header.masked_status & 
					    srp->header.host_status & 
					    srp->header.driver_status;
					if (srp->done)
						rinfo[val].duration =
							srp->header.duration;
					else {
						ms = jiffies_to_msecs(jiffies);
						rinfo[val].duration =
						    (ms > srp->header.duration) ?
						    (ms - srp->header.duration) : 0;
					}
					rinfo[val].orphan = srp->orphan;
					rinfo[val].sg_io_owned =
							srp->sg_io_owned;
					rinfo[val].pack_id =
							srp->header.pack_id;
					rinfo[val].usr_ptr =
							srp->header.usr_ptr;
				}
			}
			read_unlock_irqrestore(&sfp->rq_list_lock, iflags);
			result = __copy_to_user(p, rinfo, 
						SZ_SG_REQ_INFO * SG_MAX_QUEUE);
			result = result ? -EFAULT : 0;
			kfree(rinfo);
			return result;
		}
	case SG_EMULATED_HOST:
		if (atomic_read(&sdp->detaching))
			return -ENODEV;
		return put_user(sdp->device->host->hostt->emulated, ip);
	case SG_SCSI_RESET:
		if (atomic_read(&sdp->detaching))
			return -ENODEV;
		if (filp->f_flags & O_NONBLOCK) {
			if (scsi_host_in_recovery(sdp->device->host))
				return -EBUSY;
		} else if (!scsi_block_when_processing_errors(sdp->device))
			return -EBUSY;
		result = get_user(val, ip);
		if (result)
			return result;
		if (SG_SCSI_RESET_NOTHING == val)
			return 0;
		switch (val) {
		case SG_SCSI_RESET_DEVICE:
			val = SCSI_TRY_RESET_DEVICE;
			break;
		case SG_SCSI_RESET_TARGET:
			val = SCSI_TRY_RESET_TARGET;
			break;
		case SG_SCSI_RESET_BUS:
			val = SCSI_TRY_RESET_BUS;
			break;
		case SG_SCSI_RESET_HOST:
			val = SCSI_TRY_RESET_HOST;
			break;
		default:
			return -EINVAL;
		}
		if (!capable(CAP_SYS_ADMIN) || !capable(CAP_SYS_RAWIO))
			return -EACCES;
		return (scsi_reset_provider(sdp->device, val) ==
			SUCCESS) ? 0 : -EIO;
	case SCSI_IOCTL_SEND_COMMAND:
		if (atomic_read(&sdp->detaching))
			return -ENODEV;
		if (read_only) {
			unsigned char opcode = WRITE_6;
			Scsi_Ioctl_Command __user *siocp = p;

			if (copy_from_user(&opcode, siocp->data, 1))
				return -EFAULT;
			if (sg_allow_access(filp, &opcode))
				return -EPERM;
		}
		return sg_scsi_ioctl(sdp->device->request_queue, NULL, filp->f_mode, p);
	case SG_SET_DEBUG:
		result = get_user(val, ip);
		if (result)
			return result;
		sdp->sgdebug = (char) val;
		return 0;
	case SCSI_IOCTL_GET_IDLUN:
	case SCSI_IOCTL_GET_BUS_NUMBER:
	case SCSI_IOCTL_PROBE_HOST:
	case SG_GET_TRANSFORM:
		if (atomic_read(&sdp->detaching))
			return -ENODEV;
		return scsi_ioctl(sdp->device, cmd_in, p);
	case BLKSECTGET:
		return put_user(max_sectors_bytes(sdp->device->request_queue),
				ip);
	case BLKTRACESETUP:
		return blk_trace_setup(sdp->device->request_queue,
				       sdp->disk->disk_name,
				       MKDEV(SCSI_GENERIC_MAJOR, sdp->index),
				       NULL,
				       (char *)arg);
	case BLKTRACESTART:
		return blk_trace_startstop(sdp->device->request_queue, 1);
	case BLKTRACESTOP:
		return blk_trace_startstop(sdp->device->request_queue, 0);
	case BLKTRACETEARDOWN:
		return blk_trace_remove(sdp->device->request_queue);
	default:
		if (read_only)
			return -EPERM;	/* don't know so take safe approach */
		return scsi_ioctl(sdp->device, cmd_in, p);
	}
}

#ifdef CONFIG_COMPAT
static long sg_compat_ioctl(struct file *filp, unsigned int cmd_in, unsigned long arg)
{
	Sg_device *sdp;
	Sg_fd *sfp;
	struct scsi_device *sdev;

	if ((!(sfp = (Sg_fd *) filp->private_data)) || (!(sdp = sfp->parentdp)))
		return -ENXIO;

	sdev = sdp->device;
	if (sdev->host->hostt->compat_ioctl) { 
		int ret;

		ret = sdev->host->hostt->compat_ioctl(sdev, cmd_in, (void __user *)arg);

		return ret;
	}
	
	return -ENOIOCTLCMD;
}
#endif

static unsigned int
sg_poll(struct file *filp, poll_table * wait)
{
	unsigned int res = 0;
	Sg_device *sdp;
	Sg_fd *sfp;
	Sg_request *srp;
	int count = 0;
	unsigned long iflags;

	sfp = filp->private_data;
	if (!sfp)
		return POLLERR;
	sdp = sfp->parentdp;
	if (!sdp)
		return POLLERR;
	poll_wait(filp, &sfp->read_wait, wait);
	read_lock_irqsave(&sfp->rq_list_lock, iflags);
	for (srp = sfp->headrp; srp; srp = srp->nextrp) {
		/* if any read waiting, flag it */
		if ((0 == res) && (1 == srp->done) && (!srp->sg_io_owned))
			res = POLLIN | POLLRDNORM;
		++count;
	}
	read_unlock_irqrestore(&sfp->rq_list_lock, iflags);

	if (atomic_read(&sdp->detaching))
		res |= POLLHUP;
	else if (!sfp->cmd_q) {
		if (0 == count)
			res |= POLLOUT | POLLWRNORM;
	} else if (count < SG_MAX_QUEUE)
		res |= POLLOUT | POLLWRNORM;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_poll: res=0x%x\n", (int) res));
	return res;
}

static int
sg_fasync(int fd, struct file *filp, int mode)
{
	Sg_device *sdp;
	Sg_fd *sfp;

	if ((!(sfp = (Sg_fd *) filp->private_data)) || (!(sdp = sfp->parentdp)))
		return -ENXIO;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_fasync: mode=%d\n", mode));

	return fasync_helper(fd, filp, mode, &sfp->async_qp);
}

static int
sg_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	Sg_fd *sfp;
	unsigned long offset, len, sa;
	Sg_scatter_hold *rsv_schp;
	int k, length;

	if ((NULL == vma) || (!(sfp = (Sg_fd *) vma->vm_private_data)))
		return VM_FAULT_SIGBUS;
	rsv_schp = &sfp->reserve;
	offset = vmf->pgoff << PAGE_SHIFT;
	if (offset >= rsv_schp->bufflen)
		return VM_FAULT_SIGBUS;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sfp->parentdp,
				      "sg_vma_fault: offset=%lu, scatg=%d\n",
				      offset, rsv_schp->k_use_sg));
	sa = vma->vm_start;
	length = 1 << (PAGE_SHIFT + rsv_schp->page_order);
	for (k = 0; k < rsv_schp->k_use_sg && sa < vma->vm_end; k++) {
		len = vma->vm_end - sa;
		len = (len < length) ? len : length;
		if (offset < len) {
			struct page *page = nth_page(rsv_schp->pages[k],
						     offset >> PAGE_SHIFT);
			get_page(page);	/* increment page count */
			vmf->page = page;
			return 0; /* success */
		}
		sa += len;
		offset -= len;
	}

	return VM_FAULT_SIGBUS;
}

static const struct vm_operations_struct sg_mmap_vm_ops = {
	.fault = sg_vma_fault,
};

static int
sg_mmap(struct file *filp, struct vm_area_struct *vma)
{
	Sg_fd *sfp;
	unsigned long req_sz, len, sa;
	Sg_scatter_hold *rsv_schp;
	int k, length;

	if ((!filp) || (!vma) || (!(sfp = (Sg_fd *) filp->private_data)))
		return -ENXIO;
	req_sz = vma->vm_end - vma->vm_start;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sfp->parentdp,
				      "sg_mmap starting, vm_start=%p, len=%d\n",
				      (void *) vma->vm_start, (int) req_sz));
	if (vma->vm_pgoff)
		return -EINVAL;	/* want no offset */
	rsv_schp = &sfp->reserve;
	if (req_sz > rsv_schp->bufflen)
		return -ENOMEM;	/* cannot map more than reserved buffer */

	sa = vma->vm_start;
	length = 1 << (PAGE_SHIFT + rsv_schp->page_order);
	for (k = 0; k < rsv_schp->k_use_sg && sa < vma->vm_end; k++) {
		len = vma->vm_end - sa;
		len = (len < length) ? len : length;
		sa += len;
	}

	sfp->mmap_called = 1;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = sfp;
	vma->vm_ops = &sg_mmap_vm_ops;
	return 0;
}

static void
sg_rq_end_io_usercontext(struct work_struct *work)
{
	struct sg_request *srp = container_of(work, struct sg_request, ew.work);
	struct sg_fd *sfp = srp->parentfp;

	sg_finish_rem_req(srp);
	kref_put(&sfp->f_ref, sg_remove_sfp);
}

/*
 * This function is a "bottom half" handler that is called by the mid
 * level when a command is completed (or has failed).
 */
static void
sg_rq_end_io(struct request *rq, int uptodate)
{
	struct sg_request *srp = rq->end_io_data;
	Sg_device *sdp;
	Sg_fd *sfp;
	unsigned long iflags;
	unsigned int ms;
	char *sense;
	int result, resid, done = 1;

	if (WARN_ON(srp->done != 0))
		return;

	sfp = srp->parentfp;
	if (WARN_ON(sfp == NULL))
		return;

	sdp = sfp->parentdp;
	if (unlikely(atomic_read(&sdp->detaching)))
		pr_info("%s: device detaching\n", __func__);

	sense = rq->sense;
	result = rq->errors;
	resid = rq->resid_len;

	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sdp,
				      "sg_cmd_done: pack_id=%d, res=0x%x\n",
				      srp->header.pack_id, result));
	srp->header.resid = resid;
	ms = jiffies_to_msecs(jiffies);
	srp->header.duration = (ms > srp->header.duration) ?
				(ms - srp->header.duration) : 0;
	if (0 != result) {
		struct scsi_sense_hdr sshdr;

		srp->header.status = 0xff & result;
		srp->header.masked_status = status_byte(result);
		srp->header.msg_status = msg_byte(result);
		srp->header.host_status = host_byte(result);
		srp->header.driver_status = driver_byte(result);
		if ((sdp->sgdebug > 0) &&
		    ((CHECK_CONDITION == srp->header.masked_status) ||
		     (COMMAND_TERMINATED == srp->header.masked_status)))
			__scsi_print_sense(__func__, sense,
					   SCSI_SENSE_BUFFERSIZE);

		/* Following if statement is a patch supplied by Eric Youngdale */
		if (driver_byte(result) != 0
		    && scsi_normalize_sense(sense, SCSI_SENSE_BUFFERSIZE, &sshdr)
		    && !scsi_sense_is_deferred(&sshdr)
		    && sshdr.sense_key == UNIT_ATTENTION
		    && sdp->device->removable) {
			/* Detected possible disc change. Set the bit - this */
			/* may be used if there are filesystems using this device */
			sdp->device->changed = 1;
		}
	}
	/* Rely on write phase to clean out srp status values, so no "else" */

	/*
	 * Free the request as soon as it is complete so that its resources
	 * can be reused without waiting for userspace to read() the
	 * result.  But keep the associated bio (if any) around until
	 * blk_rq_unmap_user() can be called from user context.
	 */
	srp->rq = NULL;
	if (rq->cmd != rq->__cmd)
		kfree(rq->cmd);
	__blk_put_request(rq->q, rq);

	write_lock_irqsave(&sfp->rq_list_lock, iflags);
	if (unlikely(srp->orphan)) {
		if (sfp->keep_orphan)
			srp->sg_io_owned = 0;
		else
			done = 0;
	}
	srp->done = done;
	write_unlock_irqrestore(&sfp->rq_list_lock, iflags);

	if (likely(done)) {
		/* Now wake up any sg_read() that is waiting for this
		 * packet.
		 */
		wake_up_interruptible(&sfp->read_wait);
		kill_fasync(&sfp->async_qp, SIGPOLL, POLL_IN);
		kref_put(&sfp->f_ref, sg_remove_sfp);
	} else {
		INIT_WORK(&srp->ew.work, sg_rq_end_io_usercontext);
		schedule_work(&srp->ew.work);
	}
}

static const struct file_operations sg_fops = {
	.owner = THIS_MODULE,
	.read = sg_read,
	.write = sg_write,
	.poll = sg_poll,
	.unlocked_ioctl = sg_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sg_compat_ioctl,
#endif
	.open = sg_open,
	.mmap = sg_mmap,
	.release = sg_release,
	.fasync = sg_fasync,
	.llseek = no_llseek,
};

static struct class *sg_sysfs_class;

static int sg_sysfs_valid = 0;

static Sg_device *
sg_alloc(struct gendisk *disk, struct scsi_device *scsidp)
{
	struct request_queue *q = scsidp->request_queue;
	Sg_device *sdp;
	unsigned long iflags;
	int error;
	u32 k;

	sdp = kzalloc(sizeof(Sg_device), GFP_KERNEL);
	if (!sdp) {
		sdev_printk(KERN_WARNING, scsidp, "%s: kmalloc Sg_device "
			    "failure\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	idr_preload(GFP_KERNEL);
	write_lock_irqsave(&sg_index_lock, iflags);

	error = idr_alloc(&sg_index_idr, sdp, 0, SG_MAX_DEVS, GFP_NOWAIT);
	if (error < 0) {
		if (error == -ENOSPC) {
			sdev_printk(KERN_WARNING, scsidp,
				    "Unable to attach sg device type=%d, minor number exceeds %d\n",
				    scsidp->type, SG_MAX_DEVS - 1);
			error = -ENODEV;
		} else {
			sdev_printk(KERN_WARNING, scsidp, "%s: idr "
				    "allocation Sg_device failure: %d\n",
				    __func__, error);
		}
		goto out_unlock;
	}
	k = error;

	SCSI_LOG_TIMEOUT(3, sdev_printk(KERN_INFO, scsidp,
					"sg_alloc: dev=%d \n", k));
	sprintf(disk->disk_name, "sg%d", k);
	disk->first_minor = k;
	sdp->disk = disk;
	sdp->device = scsidp;
	mutex_init(&sdp->open_rel_lock);
	INIT_LIST_HEAD(&sdp->sfds);
	init_waitqueue_head(&sdp->open_wait);
	atomic_set(&sdp->detaching, 0);
	rwlock_init(&sdp->sfd_lock);
	sdp->sg_tablesize = queue_max_segments(q);
	sdp->index = k;
	kref_init(&sdp->d_ref);
	error = 0;

out_unlock:
	write_unlock_irqrestore(&sg_index_lock, iflags);
	idr_preload_end();

	if (error) {
		kfree(sdp);
		return ERR_PTR(error);
	}
	return sdp;
}

static int
sg_add_device(struct device *cl_dev, struct class_interface *cl_intf)
{
	struct scsi_device *scsidp = to_scsi_device(cl_dev->parent);
	struct gendisk *disk;
	Sg_device *sdp = NULL;
	struct cdev * cdev = NULL;
	int error;
	unsigned long iflags;

	disk = alloc_disk(1);
	if (!disk) {
		pr_warn("%s: alloc_disk failed\n", __func__);
		return -ENOMEM;
	}
	disk->major = SCSI_GENERIC_MAJOR;

	error = -ENOMEM;
	cdev = cdev_alloc();
	if (!cdev) {
		pr_warn("%s: cdev_alloc failed\n", __func__);
		goto out;
	}
	cdev->owner = THIS_MODULE;
	cdev->ops = &sg_fops;

	sdp = sg_alloc(disk, scsidp);
	if (IS_ERR(sdp)) {
		pr_warn("%s: sg_alloc failed\n", __func__);
		error = PTR_ERR(sdp);
		goto out;
	}

	error = cdev_add(cdev, MKDEV(SCSI_GENERIC_MAJOR, sdp->index), 1);
	if (error)
		goto cdev_add_err;

	sdp->cdev = cdev;
	if (sg_sysfs_valid) {
		struct device *sg_class_member;

		sg_class_member = device_create(sg_sysfs_class, cl_dev->parent,
						MKDEV(SCSI_GENERIC_MAJOR,
						      sdp->index),
						sdp, "%s", disk->disk_name);
		if (IS_ERR(sg_class_member)) {
			pr_err("%s: device_create failed\n", __func__);
			error = PTR_ERR(sg_class_member);
			goto cdev_add_err;
		}
		error = sysfs_create_link(&scsidp->sdev_gendev.kobj,
					  &sg_class_member->kobj, "generic");
		if (error)
			pr_err("%s: unable to make symlink 'generic' back "
			       "to sg%d\n", __func__, sdp->index);
	} else
		pr_warn("%s: sg_sys Invalid\n", __func__);

	sdev_printk(KERN_NOTICE, scsidp, "Attached scsi generic sg%d "
		    "type %d\n", sdp->index, scsidp->type);

	dev_set_drvdata(cl_dev, sdp);

	return 0;

cdev_add_err:
	write_lock_irqsave(&sg_index_lock, iflags);
	idr_remove(&sg_index_idr, sdp->index);
	write_unlock_irqrestore(&sg_index_lock, iflags);
	kfree(sdp);

out:
	put_disk(disk);
	if (cdev)
		cdev_del(cdev);
	return error;
}

static void
sg_device_destroy(struct kref *kref)
{
	struct sg_device *sdp = container_of(kref, struct sg_device, d_ref);
	unsigned long flags;

	/* CAUTION!  Note that the device can still be found via idr_find()
	 * even though the refcount is 0.  Therefore, do idr_remove() BEFORE
	 * any other cleanup.
	 */

	write_lock_irqsave(&sg_index_lock, flags);
	idr_remove(&sg_index_idr, sdp->index);
	write_unlock_irqrestore(&sg_index_lock, flags);

	SCSI_LOG_TIMEOUT(3,
		sg_printk(KERN_INFO, sdp, "sg_device_destroy\n"));

	put_disk(sdp->disk);
	kfree(sdp);
}

static void
sg_remove_device(struct device *cl_dev, struct class_interface *cl_intf)
{
	struct scsi_device *scsidp = to_scsi_device(cl_dev->parent);
	Sg_device *sdp = dev_get_drvdata(cl_dev);
	unsigned long iflags;
	Sg_fd *sfp;
	int val;

	if (!sdp)
		return;
	/* want sdp->detaching non-zero as soon as possible */
	val = atomic_inc_return(&sdp->detaching);
	if (val > 1)
		return; /* only want to do following once per device */

	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "%s\n", __func__));

	read_lock_irqsave(&sdp->sfd_lock, iflags);
	list_for_each_entry(sfp, &sdp->sfds, sfd_siblings) {
		wake_up_interruptible_all(&sfp->read_wait);
		kill_fasync(&sfp->async_qp, SIGPOLL, POLL_HUP);
	}
	wake_up_interruptible_all(&sdp->open_wait);
	read_unlock_irqrestore(&sdp->sfd_lock, iflags);

	sysfs_remove_link(&scsidp->sdev_gendev.kobj, "generic");
	device_destroy(sg_sysfs_class, MKDEV(SCSI_GENERIC_MAJOR, sdp->index));
	cdev_del(sdp->cdev);
	sdp->cdev = NULL;

	kref_put(&sdp->d_ref, sg_device_destroy);
}

module_param_named(scatter_elem_sz, scatter_elem_sz, int, S_IRUGO | S_IWUSR);
module_param_named(def_reserved_size, def_reserved_size, int,
		   S_IRUGO | S_IWUSR);
module_param_named(allow_dio, sg_allow_dio, int, S_IRUGO | S_IWUSR);

MODULE_AUTHOR("Douglas Gilbert");
MODULE_DESCRIPTION("SCSI generic (sg) driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(SG_VERSION_STR);
MODULE_ALIAS_CHARDEV_MAJOR(SCSI_GENERIC_MAJOR);

MODULE_PARM_DESC(scatter_elem_sz, "scatter gather element "
                "size (default: max(SG_SCATTER_SZ, PAGE_SIZE))");
MODULE_PARM_DESC(def_reserved_size, "size of buffer reserved for each fd");
MODULE_PARM_DESC(allow_dio, "allow direct I/O (default: 0 (disallow))");

static int __init
init_sg(void)
{
	int rc;

	if (scatter_elem_sz < PAGE_SIZE) {
		scatter_elem_sz = PAGE_SIZE;
		scatter_elem_sz_prev = scatter_elem_sz;
	}
	if (def_reserved_size >= 0)
		sg_big_buff = def_reserved_size;
	else
		def_reserved_size = sg_big_buff;

	rc = register_chrdev_region(MKDEV(SCSI_GENERIC_MAJOR, 0), 
				    SG_MAX_DEVS, "sg");
	if (rc)
		return rc;
        sg_sysfs_class = class_create(THIS_MODULE, "scsi_generic");
        if ( IS_ERR(sg_sysfs_class) ) {
		rc = PTR_ERR(sg_sysfs_class);
		goto err_out;
        }
	sg_sysfs_valid = 1;
	rc = scsi_register_interface(&sg_interface);
	if (0 == rc) {
#ifdef CONFIG_SCSI_PROC_FS
		sg_proc_init();
#endif				/* CONFIG_SCSI_PROC_FS */
		return 0;
	}
	class_destroy(sg_sysfs_class);
err_out:
	unregister_chrdev_region(MKDEV(SCSI_GENERIC_MAJOR, 0), SG_MAX_DEVS);
	return rc;
}

static void __exit
exit_sg(void)
{
#ifdef CONFIG_SCSI_PROC_FS
	sg_proc_cleanup();
#endif				/* CONFIG_SCSI_PROC_FS */
	scsi_unregister_interface(&sg_interface);
	class_destroy(sg_sysfs_class);
	sg_sysfs_valid = 0;
	unregister_chrdev_region(MKDEV(SCSI_GENERIC_MAJOR, 0),
				 SG_MAX_DEVS);
	idr_destroy(&sg_index_idr);
}

static int
sg_start_req(Sg_request *srp, unsigned char *cmd)
{
	int res;
	struct request *rq;
	Sg_fd *sfp = srp->parentfp;
	sg_io_hdr_t *hp = &srp->header;
	int dxfer_len = (int) hp->dxfer_len;
	int dxfer_dir = hp->dxfer_direction;
	unsigned int iov_count = hp->iovec_count;
	Sg_scatter_hold *req_schp = &srp->data;
	Sg_scatter_hold *rsv_schp = &sfp->reserve;
	struct request_queue *q = sfp->parentdp->device->request_queue;
	struct rq_map_data *md, map_data;
	int rw = hp->dxfer_direction == SG_DXFER_TO_DEV ? WRITE : READ;
	unsigned char *long_cmdp = NULL;

	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
				      "sg_start_req: dxfer_len=%d\n",
				      dxfer_len));

	if (hp->cmd_len > BLK_MAX_CDB) {
		long_cmdp = kzalloc(hp->cmd_len, GFP_KERNEL);
		if (!long_cmdp)
			return -ENOMEM;
	}

	/*
	 * NOTE
	 *
	 * With scsi-mq enabled, there are a fixed number of preallocated
	 * requests equal in number to shost->can_queue.  If all of the
	 * preallocated requests are already in use, then using GFP_ATOMIC with
	 * blk_get_request() will return -EWOULDBLOCK, whereas using GFP_KERNEL
	 * will cause blk_get_request() to sleep until an active command
	 * completes, freeing up a request.  Neither option is ideal, but
	 * GFP_KERNEL is the better choice to prevent userspace from getting an
	 * unexpected EWOULDBLOCK.
	 *
	 * With scsi-mq disabled, blk_get_request() with GFP_KERNEL usually
	 * does not sleep except under memory pressure.
	 */
	rq = blk_get_request(q, rw, GFP_KERNEL);
	if (IS_ERR(rq)) {
		kfree(long_cmdp);
		return PTR_ERR(rq);
	}

	blk_rq_set_block_pc(rq);

	if (hp->cmd_len > BLK_MAX_CDB)
		rq->cmd = long_cmdp;
	memcpy(rq->cmd, cmd, hp->cmd_len);
	rq->cmd_len = hp->cmd_len;

	srp->rq = rq;
	rq->end_io_data = srp;
	rq->sense = srp->sense_b;
	rq->retries = SG_DEFAULT_RETRIES;

	if ((dxfer_len <= 0) || (dxfer_dir == SG_DXFER_NONE))
		return 0;

	if (sg_allow_dio && hp->flags & SG_FLAG_DIRECT_IO &&
	    dxfer_dir != SG_DXFER_UNKNOWN && !iov_count &&
	    !sfp->parentdp->device->host->unchecked_isa_dma &&
	    blk_rq_aligned(q, (unsigned long)hp->dxferp, dxfer_len))
		md = NULL;
	else
		md = &map_data;

	if (md) {
		if (!sg_res_in_use(sfp) && dxfer_len <= rsv_schp->bufflen)
			sg_link_reserve(sfp, srp, dxfer_len);
		else {
			res = sg_build_indirect(req_schp, sfp, dxfer_len);
			if (res)
				return res;
		}

		md->pages = req_schp->pages;
		md->page_order = req_schp->page_order;
		md->nr_entries = req_schp->k_use_sg;
		md->offset = 0;
		md->null_mapped = hp->dxferp ? 0 : 1;
		if (dxfer_dir == SG_DXFER_TO_FROM_DEV)
			md->from_user = 1;
		else
			md->from_user = 0;
	}

	if (iov_count) {
		int len, size = sizeof(struct sg_iovec) * iov_count;
		struct iovec *iov;

		iov = memdup_user(hp->dxferp, size);
		if (IS_ERR(iov))
			return PTR_ERR(iov);

		len = iov_length(iov, iov_count);
		if (hp->dxfer_len < len) {
			iov_count = iov_shorten(iov, iov_count, hp->dxfer_len);
			len = hp->dxfer_len;
		}

		res = blk_rq_map_user_iov(q, rq, md, (struct sg_iovec *)iov,
					  iov_count,
					  len, GFP_ATOMIC);
		kfree(iov);
	} else
		res = blk_rq_map_user(q, rq, md, hp->dxferp,
				      hp->dxfer_len, GFP_ATOMIC);

	if (!res) {
		srp->bio = rq->bio;

		if (!md) {
			req_schp->dio_in_use = 1;
			hp->info |= SG_INFO_DIRECT_IO;
		}
	}
	return res;
}

static int
sg_finish_rem_req(Sg_request *srp)
{
	int ret = 0;

	Sg_fd *sfp = srp->parentfp;
	Sg_scatter_hold *req_schp = &srp->data;

	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
				      "sg_finish_rem_req: res_used=%d\n",
				      (int) srp->res_used));
	if (srp->bio)
		ret = blk_rq_unmap_user(srp->bio);

	if (srp->rq) {
		if (srp->rq->cmd != srp->rq->__cmd)
			kfree(srp->rq->cmd);
		blk_put_request(srp->rq);
	}

	if (srp->res_used)
		sg_unlink_reserve(sfp, srp);
	else
		sg_remove_scat(sfp, req_schp);

	sg_remove_request(sfp, srp);

	return ret;
}

static int
sg_build_sgat(Sg_scatter_hold * schp, const Sg_fd * sfp, int tablesize)
{
	int sg_bufflen = tablesize * sizeof(struct page *);
	gfp_t gfp_flags = GFP_ATOMIC | __GFP_NOWARN;

	schp->pages = kzalloc(sg_bufflen, gfp_flags);
	if (!schp->pages)
		return -ENOMEM;
	schp->sglist_len = sg_bufflen;
	return tablesize;	/* number of scat_gath elements allocated */
}

static int
sg_build_indirect(Sg_scatter_hold * schp, Sg_fd * sfp, int buff_size)
{
	int ret_sz = 0, i, k, rem_sz, num, mx_sc_elems;
	int sg_tablesize = sfp->parentdp->sg_tablesize;
	int blk_size = buff_size, order;
	gfp_t gfp_mask = GFP_ATOMIC | __GFP_COMP | __GFP_NOWARN;

	if (blk_size < 0)
		return -EFAULT;
	if (0 == blk_size)
		++blk_size;	/* don't know why */
	/* round request up to next highest SG_SECTOR_SZ byte boundary */
	blk_size = ALIGN(blk_size, SG_SECTOR_SZ);
	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
		"sg_build_indirect: buff_size=%d, blk_size=%d\n",
		buff_size, blk_size));

	/* N.B. ret_sz carried into this block ... */
	mx_sc_elems = sg_build_sgat(schp, sfp, sg_tablesize);
	if (mx_sc_elems < 0)
		return mx_sc_elems;	/* most likely -ENOMEM */

	num = scatter_elem_sz;
	if (unlikely(num != scatter_elem_sz_prev)) {
		if (num < PAGE_SIZE) {
			scatter_elem_sz = PAGE_SIZE;
			scatter_elem_sz_prev = PAGE_SIZE;
		} else
			scatter_elem_sz_prev = num;
	}

	if (sfp->low_dma)
		gfp_mask |= GFP_DMA;

	if (!capable(CAP_SYS_ADMIN) || !capable(CAP_SYS_RAWIO))
		gfp_mask |= __GFP_ZERO;

	order = get_order(num);
retry:
	ret_sz = 1 << (PAGE_SHIFT + order);

	for (k = 0, rem_sz = blk_size; rem_sz > 0 && k < mx_sc_elems;
	     k++, rem_sz -= ret_sz) {

		num = (rem_sz > scatter_elem_sz_prev) ?
			scatter_elem_sz_prev : rem_sz;

		schp->pages[k] = alloc_pages(gfp_mask, order);
		if (!schp->pages[k])
			goto out;

		if (num == scatter_elem_sz_prev) {
			if (unlikely(ret_sz > scatter_elem_sz_prev)) {
				scatter_elem_sz = ret_sz;
				scatter_elem_sz_prev = ret_sz;
			}
		}

		SCSI_LOG_TIMEOUT(5, sg_printk(KERN_INFO, sfp->parentdp,
				 "sg_build_indirect: k=%d, num=%d, ret_sz=%d\n",
				 k, num, ret_sz));
	}		/* end of for loop */

	schp->page_order = order;
	schp->k_use_sg = k;
	SCSI_LOG_TIMEOUT(5, sg_printk(KERN_INFO, sfp->parentdp,
			 "sg_build_indirect: k_use_sg=%d, rem_sz=%d\n",
			 k, rem_sz));

	schp->bufflen = blk_size;
	if (rem_sz > 0)	/* must have failed */
		return -ENOMEM;
	return 0;
out:
	for (i = 0; i < k; i++)
		__free_pages(schp->pages[i], order);

	if (--order >= 0)
		goto retry;

	return -ENOMEM;
}

static void
sg_remove_scat(Sg_fd * sfp, Sg_scatter_hold * schp)
{
	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
			 "sg_remove_scat: k_use_sg=%d\n", schp->k_use_sg));
	if (schp->pages && schp->sglist_len > 0) {
		if (!schp->dio_in_use) {
			int k;

			for (k = 0; k < schp->k_use_sg && schp->pages[k]; k++) {
				SCSI_LOG_TIMEOUT(5,
					sg_printk(KERN_INFO, sfp->parentdp,
					"sg_remove_scat: k=%d, pg=0x%p\n",
					k, schp->pages[k]));
				__free_pages(schp->pages[k], schp->page_order);
			}

			kfree(schp->pages);
		}
	}
	memset(schp, 0, sizeof (*schp));
}

static int
sg_read_oxfer(Sg_request * srp, char __user *outp, int num_read_xfer)
{
	Sg_scatter_hold *schp = &srp->data;
	int k, num;

	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, srp->parentfp->parentdp,
			 "sg_read_oxfer: num_read_xfer=%d\n",
			 num_read_xfer));
	if ((!outp) || (num_read_xfer <= 0))
		return 0;

	num = 1 << (PAGE_SHIFT + schp->page_order);
	for (k = 0; k < schp->k_use_sg && schp->pages[k]; k++) {
		if (num > num_read_xfer) {
			if (__copy_to_user(outp, page_address(schp->pages[k]),
					   num_read_xfer))
				return -EFAULT;
			break;
		} else {
			if (__copy_to_user(outp, page_address(schp->pages[k]),
					   num))
				return -EFAULT;
			num_read_xfer -= num;
			if (num_read_xfer <= 0)
				break;
			outp += num;
		}
	}

	return 0;
}

static void
sg_build_reserve(Sg_fd * sfp, int req_size)
{
	Sg_scatter_hold *schp = &sfp->reserve;

	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
			 "sg_build_reserve: req_size=%d\n", req_size));
	do {
		if (req_size < PAGE_SIZE)
			req_size = PAGE_SIZE;
		if (0 == sg_build_indirect(schp, sfp, req_size))
			return;
		else
			sg_remove_scat(sfp, schp);
		req_size >>= 1;	/* divide by 2 */
	} while (req_size > (PAGE_SIZE / 2));
}

static void
sg_link_reserve(Sg_fd * sfp, Sg_request * srp, int size)
{
	Sg_scatter_hold *req_schp = &srp->data;
	Sg_scatter_hold *rsv_schp = &sfp->reserve;
	int k, num, rem;

	srp->res_used = 1;
	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sfp->parentdp,
			 "sg_link_reserve: size=%d\n", size));
	rem = size;

	num = 1 << (PAGE_SHIFT + rsv_schp->page_order);
	for (k = 0; k < rsv_schp->k_use_sg; k++) {
		if (rem <= num) {
			req_schp->k_use_sg = k + 1;
			req_schp->sglist_len = rsv_schp->sglist_len;
			req_schp->pages = rsv_schp->pages;

			req_schp->bufflen = size;
			req_schp->page_order = rsv_schp->page_order;
			break;
		} else
			rem -= num;
	}

	if (k >= rsv_schp->k_use_sg)
		SCSI_LOG_TIMEOUT(1, sg_printk(KERN_INFO, sfp->parentdp,
				 "sg_link_reserve: BAD size\n"));
}

static void
sg_unlink_reserve(Sg_fd * sfp, Sg_request * srp)
{
	Sg_scatter_hold *req_schp = &srp->data;

	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, srp->parentfp->parentdp,
				      "sg_unlink_reserve: req->k_use_sg=%d\n",
				      (int) req_schp->k_use_sg));
	req_schp->k_use_sg = 0;
	req_schp->bufflen = 0;
	req_schp->pages = NULL;
	req_schp->page_order = 0;
	req_schp->sglist_len = 0;
	sfp->save_scat_len = 0;
	srp->res_used = 0;
}

static Sg_request *
sg_get_rq_mark(Sg_fd * sfp, int pack_id)
{
	Sg_request *resp;
	unsigned long iflags;

	write_lock_irqsave(&sfp->rq_list_lock, iflags);
	for (resp = sfp->headrp; resp; resp = resp->nextrp) {
		/* look for requests that are ready + not SG_IO owned */
		if ((1 == resp->done) && (!resp->sg_io_owned) &&
		    ((-1 == pack_id) || (resp->header.pack_id == pack_id))) {
			resp->done = 2;	/* guard against other readers */
			break;
		}
	}
	write_unlock_irqrestore(&sfp->rq_list_lock, iflags);
	return resp;
}

/* always adds to end of list */
static Sg_request *
sg_add_request(Sg_fd * sfp)
{
	int k;
	unsigned long iflags;
	Sg_request *resp;
	Sg_request *rp = sfp->req_arr;

	write_lock_irqsave(&sfp->rq_list_lock, iflags);
	resp = sfp->headrp;
	if (!resp) {
		memset(rp, 0, sizeof (Sg_request));
		rp->parentfp = sfp;
		resp = rp;
		sfp->headrp = resp;
	} else {
		if (0 == sfp->cmd_q)
			resp = NULL;	/* command queuing disallowed */
		else {
			for (k = 0; k < SG_MAX_QUEUE; ++k, ++rp) {
				if (!rp->parentfp)
					break;
			}
			if (k < SG_MAX_QUEUE) {
				memset(rp, 0, sizeof (Sg_request));
				rp->parentfp = sfp;
				while (resp->nextrp)
					resp = resp->nextrp;
				resp->nextrp = rp;
				resp = rp;
			} else
				resp = NULL;
		}
	}
	if (resp) {
		resp->nextrp = NULL;
		resp->header.duration = jiffies_to_msecs(jiffies);
	}
	write_unlock_irqrestore(&sfp->rq_list_lock, iflags);
	return resp;
}

/* Return of 1 for found; 0 for not found */
static int
sg_remove_request(Sg_fd * sfp, Sg_request * srp)
{
	Sg_request *prev_rp;
	Sg_request *rp;
	unsigned long iflags;
	int res = 0;

	if ((!sfp) || (!srp) || (!sfp->headrp))
		return res;
	write_lock_irqsave(&sfp->rq_list_lock, iflags);
	prev_rp = sfp->headrp;
	if (srp == prev_rp) {
		sfp->headrp = prev_rp->nextrp;
		prev_rp->parentfp = NULL;
		res = 1;
	} else {
		while ((rp = prev_rp->nextrp)) {
			if (srp == rp) {
				prev_rp->nextrp = rp->nextrp;
				rp->parentfp = NULL;
				res = 1;
				break;
			}
			prev_rp = rp;
		}
	}
	write_unlock_irqrestore(&sfp->rq_list_lock, iflags);
	return res;
}

static Sg_fd *
sg_add_sfp(Sg_device * sdp)
{
	Sg_fd *sfp;
	unsigned long iflags;
	int bufflen;

	sfp = kzalloc(sizeof(*sfp), GFP_ATOMIC | __GFP_NOWARN);
	if (!sfp)
		return ERR_PTR(-ENOMEM);

	init_waitqueue_head(&sfp->read_wait);
	rwlock_init(&sfp->rq_list_lock);

	kref_init(&sfp->f_ref);
	sfp->timeout = SG_DEFAULT_TIMEOUT;
	sfp->timeout_user = SG_DEFAULT_TIMEOUT_USER;
	sfp->force_packid = SG_DEF_FORCE_PACK_ID;
	sfp->low_dma = (SG_DEF_FORCE_LOW_DMA == 0) ?
	    sdp->device->host->unchecked_isa_dma : 1;
	sfp->cmd_q = SG_DEF_COMMAND_Q;
	sfp->keep_orphan = SG_DEF_KEEP_ORPHAN;
	sfp->parentdp = sdp;
	write_lock_irqsave(&sdp->sfd_lock, iflags);
	if (atomic_read(&sdp->detaching)) {
		write_unlock_irqrestore(&sdp->sfd_lock, iflags);
		return ERR_PTR(-ENODEV);
	}
	list_add_tail(&sfp->sfd_siblings, &sdp->sfds);
	write_unlock_irqrestore(&sdp->sfd_lock, iflags);
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_add_sfp: sfp=0x%p\n", sfp));
	if (unlikely(sg_big_buff != def_reserved_size))
		sg_big_buff = def_reserved_size;

	bufflen = min_t(int, sg_big_buff,
			max_sectors_bytes(sdp->device->request_queue));
	sg_build_reserve(sfp, bufflen);
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_add_sfp: bufflen=%d, k_use_sg=%d\n",
				      sfp->reserve.bufflen,
				      sfp->reserve.k_use_sg));

	kref_get(&sdp->d_ref);
	__module_get(THIS_MODULE);
	return sfp;
}

static void
sg_remove_sfp_usercontext(struct work_struct *work)
{
	struct sg_fd *sfp = container_of(work, struct sg_fd, ew.work);
	struct sg_device *sdp = sfp->parentdp;

	/* Cleanup any responses which were never read(). */
	while (sfp->headrp)
		sg_finish_rem_req(sfp->headrp);

	if (sfp->reserve.bufflen > 0) {
		SCSI_LOG_TIMEOUT(6, sg_printk(KERN_INFO, sdp,
				"sg_remove_sfp:    bufflen=%d, k_use_sg=%d\n",
				(int) sfp->reserve.bufflen,
				(int) sfp->reserve.k_use_sg));
		sg_remove_scat(sfp, &sfp->reserve);
	}

	SCSI_LOG_TIMEOUT(6, sg_printk(KERN_INFO, sdp,
			"sg_remove_sfp: sfp=0x%p\n", sfp));
	kfree(sfp);

	scsi_device_put(sdp->device);
	kref_put(&sdp->d_ref, sg_device_destroy);
	module_put(THIS_MODULE);
}

static void
sg_remove_sfp(struct kref *kref)
{
	struct sg_fd *sfp = container_of(kref, struct sg_fd, f_ref);
	struct sg_device *sdp = sfp->parentdp;
	unsigned long iflags;

	write_lock_irqsave(&sdp->sfd_lock, iflags);
	list_del(&sfp->sfd_siblings);
	write_unlock_irqrestore(&sdp->sfd_lock, iflags);

	INIT_WORK(&sfp->ew.work, sg_remove_sfp_usercontext);
	schedule_work(&sfp->ew.work);
}

static int
sg_res_in_use(Sg_fd * sfp)
{
	const Sg_request *srp;
	unsigned long iflags;

	read_lock_irqsave(&sfp->rq_list_lock, iflags);
	for (srp = sfp->headrp; srp; srp = srp->nextrp)
		if (srp->res_used)
			break;
	read_unlock_irqrestore(&sfp->rq_list_lock, iflags);
	return srp ? 1 : 0;
}

#ifdef CONFIG_SCSI_PROC_FS
static int
sg_idr_max_id(int id, void *p, void *data)
{
	int *k = data;

	if (*k < id)
		*k = id;

	return 0;
}

static int
sg_last_dev(void)
{
	int k = -1;
	unsigned long iflags;

	read_lock_irqsave(&sg_index_lock, iflags);
	idr_for_each(&sg_index_idr, sg_idr_max_id, &k);
	read_unlock_irqrestore(&sg_index_lock, iflags);
	return k + 1;		/* origin 1 */
}
#endif

/* must be called with sg_index_lock held */
static Sg_device *sg_lookup_dev(int dev)
{
	return idr_find(&sg_index_idr, dev);
}

static Sg_device *
sg_get_dev(int dev)
{
	struct sg_device *sdp;
	unsigned long flags;

	read_lock_irqsave(&sg_index_lock, flags);
	sdp = sg_lookup_dev(dev);
	if (!sdp)
		sdp = ERR_PTR(-ENXIO);
	else if (atomic_read(&sdp->detaching)) {
		/* If sdp->detaching, then the refcount may already be 0, in
		 * which case it would be a bug to do kref_get().
		 */
		sdp = ERR_PTR(-ENODEV);
	} else
		kref_get(&sdp->d_ref);
	read_unlock_irqrestore(&sg_index_lock, flags);

	return sdp;
}

#ifdef CONFIG_SCSI_PROC_FS

static struct proc_dir_entry *sg_proc_sgp = NULL;

static char sg_proc_sg_dirname[] = "scsi/sg";

static int sg_proc_seq_show_int(struct seq_file *s, void *v);

static int sg_proc_single_open_adio(struct inode *inode, struct file *file);
static ssize_t sg_proc_write_adio(struct file *filp, const char __user *buffer,
			          size_t count, loff_t *off);
static const struct file_operations adio_fops = {
	.owner = THIS_MODULE,
	.open = sg_proc_single_open_adio,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = sg_proc_write_adio,
	.release = single_release,
};

static int sg_proc_single_open_dressz(struct inode *inode, struct file *file);
static ssize_t sg_proc_write_dressz(struct file *filp, 
		const char __user *buffer, size_t count, loff_t *off);
static const struct file_operations dressz_fops = {
	.owner = THIS_MODULE,
	.open = sg_proc_single_open_dressz,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = sg_proc_write_dressz,
	.release = single_release,
};

static int sg_proc_seq_show_version(struct seq_file *s, void *v);
static int sg_proc_single_open_version(struct inode *inode, struct file *file);
static const struct file_operations version_fops = {
	.owner = THIS_MODULE,
	.open = sg_proc_single_open_version,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sg_proc_seq_show_devhdr(struct seq_file *s, void *v);
static int sg_proc_single_open_devhdr(struct inode *inode, struct file *file);
static const struct file_operations devhdr_fops = {
	.owner = THIS_MODULE,
	.open = sg_proc_single_open_devhdr,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sg_proc_seq_show_dev(struct seq_file *s, void *v);
static int sg_proc_open_dev(struct inode *inode, struct file *file);
static void * dev_seq_start(struct seq_file *s, loff_t *pos);
static void * dev_seq_next(struct seq_file *s, void *v, loff_t *pos);
static void dev_seq_stop(struct seq_file *s, void *v);
static const struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.open = sg_proc_open_dev,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
static const struct seq_operations dev_seq_ops = {
	.start = dev_seq_start,
	.next  = dev_seq_next,
	.stop  = dev_seq_stop,
	.show  = sg_proc_seq_show_dev,
};

static int sg_proc_seq_show_devstrs(struct seq_file *s, void *v);
static int sg_proc_open_devstrs(struct inode *inode, struct file *file);
static const struct file_operations devstrs_fops = {
	.owner = THIS_MODULE,
	.open = sg_proc_open_devstrs,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
static const struct seq_operations devstrs_seq_ops = {
	.start = dev_seq_start,
	.next  = dev_seq_next,
	.stop  = dev_seq_stop,
	.show  = sg_proc_seq_show_devstrs,
};

static int sg_proc_seq_show_debug(struct seq_file *s, void *v);
static int sg_proc_open_debug(struct inode *inode, struct file *file);
static const struct file_operations debug_fops = {
	.owner = THIS_MODULE,
	.open = sg_proc_open_debug,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
static const struct seq_operations debug_seq_ops = {
	.start = dev_seq_start,
	.next  = dev_seq_next,
	.stop  = dev_seq_stop,
	.show  = sg_proc_seq_show_debug,
};


struct sg_proc_leaf {
	const char * name;
	const struct file_operations * fops;
};

static const struct sg_proc_leaf sg_proc_leaf_arr[] = {
	{"allow_dio", &adio_fops},
	{"debug", &debug_fops},
	{"def_reserved_size", &dressz_fops},
	{"device_hdr", &devhdr_fops},
	{"devices", &dev_fops},
	{"device_strs", &devstrs_fops},
	{"version", &version_fops}
};

static int
sg_proc_init(void)
{
	int num_leaves = ARRAY_SIZE(sg_proc_leaf_arr);
	int k;

	sg_proc_sgp = proc_mkdir(sg_proc_sg_dirname, NULL);
	if (!sg_proc_sgp)
		return 1;
	for (k = 0; k < num_leaves; ++k) {
		const struct sg_proc_leaf *leaf = &sg_proc_leaf_arr[k];
		umode_t mask = leaf->fops->write ? S_IRUGO | S_IWUSR : S_IRUGO;
		proc_create(leaf->name, mask, sg_proc_sgp, leaf->fops);
	}
	return 0;
}

static void
sg_proc_cleanup(void)
{
	int k;
	int num_leaves = ARRAY_SIZE(sg_proc_leaf_arr);

	if (!sg_proc_sgp)
		return;
	for (k = 0; k < num_leaves; ++k)
		remove_proc_entry(sg_proc_leaf_arr[k].name, sg_proc_sgp);
	remove_proc_entry(sg_proc_sg_dirname, NULL);
}


static int sg_proc_seq_show_int(struct seq_file *s, void *v)
{
	seq_printf(s, "%d\n", *((int *)s->private));
	return 0;
}

static int sg_proc_single_open_adio(struct inode *inode, struct file *file)
{
	return single_open(file, sg_proc_seq_show_int, &sg_allow_dio);
}

static ssize_t 
sg_proc_write_adio(struct file *filp, const char __user *buffer,
		   size_t count, loff_t *off)
{
	int err;
	unsigned long num;

	if (!capable(CAP_SYS_ADMIN) || !capable(CAP_SYS_RAWIO))
		return -EACCES;
	err = kstrtoul_from_user(buffer, count, 0, &num);
	if (err)
		return err;
	sg_allow_dio = num ? 1 : 0;
	return count;
}

static int sg_proc_single_open_dressz(struct inode *inode, struct file *file)
{
	return single_open(file, sg_proc_seq_show_int, &sg_big_buff);
}

static ssize_t 
sg_proc_write_dressz(struct file *filp, const char __user *buffer,
		     size_t count, loff_t *off)
{
	int err;
	unsigned long k = ULONG_MAX;

	if (!capable(CAP_SYS_ADMIN) || !capable(CAP_SYS_RAWIO))
		return -EACCES;

	err = kstrtoul_from_user(buffer, count, 0, &k);
	if (err)
		return err;
	if (k <= 1048576) {	/* limit "big buff" to 1 MB */
		sg_big_buff = k;
		return count;
	}
	return -ERANGE;
}

static int sg_proc_seq_show_version(struct seq_file *s, void *v)
{
	seq_printf(s, "%d\t%s [%s]\n", sg_version_num, SG_VERSION_STR,
		   sg_version_date);
	return 0;
}

static int sg_proc_single_open_version(struct inode *inode, struct file *file)
{
	return single_open(file, sg_proc_seq_show_version, NULL);
}

static int sg_proc_seq_show_devhdr(struct seq_file *s, void *v)
{
	seq_puts(s, "host\tchan\tid\tlun\ttype\topens\tqdepth\tbusy\tonline\n");
	return 0;
}

static int sg_proc_single_open_devhdr(struct inode *inode, struct file *file)
{
	return single_open(file, sg_proc_seq_show_devhdr, NULL);
}

struct sg_proc_deviter {
	loff_t	index;
	size_t	max;
};

static void * dev_seq_start(struct seq_file *s, loff_t *pos)
{
	struct sg_proc_deviter * it = kmalloc(sizeof(*it), GFP_KERNEL);

	s->private = it;
	if (! it)
		return NULL;

	it->index = *pos;
	it->max = sg_last_dev();
	if (it->index >= it->max)
		return NULL;
	return it;
}

static void * dev_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct sg_proc_deviter * it = s->private;

	*pos = ++it->index;
	return (it->index < it->max) ? it : NULL;
}

static void dev_seq_stop(struct seq_file *s, void *v)
{
	kfree(s->private);
}

static int sg_proc_open_dev(struct inode *inode, struct file *file)
{
        return seq_open(file, &dev_seq_ops);
}

static int sg_proc_seq_show_dev(struct seq_file *s, void *v)
{
	struct sg_proc_deviter * it = (struct sg_proc_deviter *) v;
	Sg_device *sdp;
	struct scsi_device *scsidp;
	unsigned long iflags;

	read_lock_irqsave(&sg_index_lock, iflags);
	sdp = it ? sg_lookup_dev(it->index) : NULL;
	if ((NULL == sdp) || (NULL == sdp->device) ||
	    (atomic_read(&sdp->detaching)))
		seq_puts(s, "-1\t-1\t-1\t-1\t-1\t-1\t-1\t-1\t-1\n");
	else {
		scsidp = sdp->device;
		seq_printf(s, "%d\t%d\t%d\t%llu\t%d\t%d\t%d\t%d\t%d\n",
			      scsidp->host->host_no, scsidp->channel,
			      scsidp->id, scsidp->lun, (int) scsidp->type,
			      1,
			      (int) scsidp->queue_depth,
			      (int) atomic_read(&scsidp->device_busy),
			      (int) scsi_device_online(scsidp));
	}
	read_unlock_irqrestore(&sg_index_lock, iflags);
	return 0;
}

static int sg_proc_open_devstrs(struct inode *inode, struct file *file)
{
        return seq_open(file, &devstrs_seq_ops);
}

static int sg_proc_seq_show_devstrs(struct seq_file *s, void *v)
{
	struct sg_proc_deviter * it = (struct sg_proc_deviter *) v;
	Sg_device *sdp;
	struct scsi_device *scsidp;
	unsigned long iflags;

	read_lock_irqsave(&sg_index_lock, iflags);
	sdp = it ? sg_lookup_dev(it->index) : NULL;
	scsidp = sdp ? sdp->device : NULL;
	if (sdp && scsidp && (!atomic_read(&sdp->detaching)))
		seq_printf(s, "%8.8s\t%16.16s\t%4.4s\n",
			   scsidp->vendor, scsidp->model, scsidp->rev);
	else
		seq_puts(s, "<no active device>\n");
	read_unlock_irqrestore(&sg_index_lock, iflags);
	return 0;
}

/* must be called while holding sg_index_lock */
static void sg_proc_debug_helper(struct seq_file *s, Sg_device * sdp)
{
	int k, m, new_interface, blen, usg;
	Sg_request *srp;
	Sg_fd *fp;
	const sg_io_hdr_t *hp;
	const char * cp;
	unsigned int ms;

	k = 0;
	list_for_each_entry(fp, &sdp->sfds, sfd_siblings) {
		k++;
		read_lock(&fp->rq_list_lock); /* irqs already disabled */
		seq_printf(s, "   FD(%d): timeout=%dms bufflen=%d "
			   "(res)sgat=%d low_dma=%d\n", k,
			   jiffies_to_msecs(fp->timeout),
			   fp->reserve.bufflen,
			   (int) fp->reserve.k_use_sg,
			   (int) fp->low_dma);
		seq_printf(s, "   cmd_q=%d f_packid=%d k_orphan=%d closed=0\n",
			   (int) fp->cmd_q, (int) fp->force_packid,
			   (int) fp->keep_orphan);
		for (m = 0, srp = fp->headrp;
				srp != NULL;
				++m, srp = srp->nextrp) {
			hp = &srp->header;
			new_interface = (hp->interface_id == '\0') ? 0 : 1;
			if (srp->res_used) {
				if (new_interface && 
				    (SG_FLAG_MMAP_IO & hp->flags))
					cp = "     mmap>> ";
				else
					cp = "     rb>> ";
			} else {
				if (SG_INFO_DIRECT_IO_MASK & hp->info)
					cp = "     dio>> ";
				else
					cp = "     ";
			}
			seq_puts(s, cp);
			blen = srp->data.bufflen;
			usg = srp->data.k_use_sg;
			seq_puts(s, srp->done ?
				 ((1 == srp->done) ?  "rcv:" : "fin:")
				  : "act:");
			seq_printf(s, " id=%d blen=%d",
				   srp->header.pack_id, blen);
			if (srp->done)
				seq_printf(s, " dur=%d", hp->duration);
			else {
				ms = jiffies_to_msecs(jiffies);
				seq_printf(s, " t_o/elap=%d/%d",
					(new_interface ? hp->timeout :
						  jiffies_to_msecs(fp->timeout)),
					(ms > hp->duration ? ms - hp->duration : 0));
			}
			seq_printf(s, "ms sgat=%d op=0x%02x\n", usg,
				   (int) srp->data.cmd_opcode);
		}
		if (0 == m)
			seq_puts(s, "     No requests active\n");
		read_unlock(&fp->rq_list_lock);
	}
}

static int sg_proc_open_debug(struct inode *inode, struct file *file)
{
        return seq_open(file, &debug_seq_ops);
}

static int sg_proc_seq_show_debug(struct seq_file *s, void *v)
{
	struct sg_proc_deviter * it = (struct sg_proc_deviter *) v;
	Sg_device *sdp;
	unsigned long iflags;

	if (it && (0 == it->index))
		seq_printf(s, "max_active_device=%d  def_reserved_size=%d\n",
			   (int)it->max, sg_big_buff);

	read_lock_irqsave(&sg_index_lock, iflags);
	sdp = it ? sg_lookup_dev(it->index) : NULL;
	if (NULL == sdp)
		goto skip;
	read_lock(&sdp->sfd_lock);
	if (!list_empty(&sdp->sfds)) {
		seq_printf(s, " >>> device=%s ", sdp->disk->disk_name);
		if (atomic_read(&sdp->detaching))
			seq_puts(s, "detaching pending close ");
		else if (sdp->device) {
			struct scsi_device *scsidp = sdp->device;

			seq_printf(s, "%d:%d:%d:%llu   em=%d",
				   scsidp->host->host_no,
				   scsidp->channel, scsidp->id,
				   scsidp->lun,
				   scsidp->host->hostt->emulated);
		}
		seq_printf(s, " sg_tablesize=%d excl=%d open_cnt=%d\n",
			   sdp->sg_tablesize, sdp->exclude, sdp->open_cnt);
		sg_proc_debug_helper(s, sdp);
	}
	read_unlock(&sdp->sfd_lock);
skip:
	read_unlock_irqrestore(&sg_index_lock, iflags);
	return 0;
}

#endif				/* CONFIG_SCSI_PROC_FS */

module_init(init_sg);
module_exit(exit_sg);
