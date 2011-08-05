/*
 * Copyright (C) 2005, 2006
 * Avishay Traeger (avishay@gmail.com)
 * Copyright (C) 2008, 2009
 * Boaz Harrosh <bharrosh@panasas.com>
 *
 * Copyrights for code taken from ext2:
 *     Copyright (C) 1992, 1993, 1994, 1995
 *     Remy Card (card@masi.ibp.fr)
 *     Laboratoire MASI - Institut Blaise Pascal
 *     Universite Pierre et Marie Curie (Paris VI)
 *     from
 *     linux/fs/minix/inode.c
 *     Copyright (C) 1991, 1992  Linus Torvalds
 *
 * This file is part of exofs.
 *
 * exofs is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.  Since it is based on ext2, and the only
 * valid version of GPL for the Linux kernel is version 2, the only valid
 * version of GPL for exofs is version 2.
 *
 * exofs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with exofs; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef __EXOFS_H__
#define __EXOFS_H__

#include <linux/fs.h>
#include <linux/time.h>
#include <linux/backing-dev.h>
#include <linux/pnfs_osd_xdr.h>
#include "common.h"

#define EXOFS_ERR(fmt, a...) printk(KERN_ERR "exofs: " fmt, ##a)

#ifdef CONFIG_EXOFS_DEBUG
#define EXOFS_DBGMSG(fmt, a...) \
	printk(KERN_NOTICE "exofs @%s:%d: " fmt, __func__, __LINE__, ##a)
#else
#define EXOFS_DBGMSG(fmt, a...) \
	do { if (0) printk(fmt, ##a); } while (0)
#endif

/* u64 has problems with printk this will cast it to unsigned long long */
#define _LLU(x) (unsigned long long)(x)

struct exofs_comp {
	struct osd_obj_id	obj;
	u8			cred[OSD_CAP_LEN];
};

struct exofs_layout {
	/* Our way of looking at the data_map */
	unsigned stripe_unit;
	unsigned mirrors_p1;

	unsigned group_width;
	u64	 group_depth;
	unsigned group_count;
};

struct exofs_components {
	unsigned	numdevs;		/* Num of devices in array    */
	/* If @single_comp == EC_SINGLE_COMP, @comps points to a single
	 * component. else there are @numdevs components
	 */
	enum EC_COMP_USAGE {
		EC_SINGLE_COMP = 0, EC_MULTPLE_COMPS = 0xffffffff
	} single_comp;
	struct exofs_comp *comps;
	struct osd_dev	**ods;			/* osd_dev array              */
};

/*
 * our extension to the in-memory superblock
 */
struct exofs_sb_info {
	struct backing_dev_info bdi;		/* register our bdi with VFS  */
	struct exofs_sb_stats s_ess;		/* Written often, pre-allocate*/
	int		s_timeout;		/* timeout for OSD operations */
	uint64_t	s_nextid;		/* highest object ID used     */
	uint32_t	s_numfiles;		/* number of files on fs      */
	spinlock_t	s_next_gen_lock;	/* spinlock for gen # update  */
	u32		s_next_generation;	/* next gen # to use          */
	atomic_t	s_curr_pending;		/* number of pending commands */

	struct pnfs_osd_data_map data_map;	/* Default raid to use
						 * FIXME: Needed ?
						 */
	struct exofs_layout	layout;		/* Default files layout       */
	struct exofs_comp one_comp;		/* id & cred of partition id=0*/
	struct exofs_components comps;		/* comps for the partition    */
	struct osd_dev	*_min_one_dev[1];	/* Place holder for one dev   */
};

/*
 * our extension to the in-memory inode
 */
struct exofs_i_info {
	struct inode   vfs_inode;          /* normal in-memory inode          */
	wait_queue_head_t i_wq;            /* wait queue for inode            */
	unsigned long  i_flags;            /* various atomic flags            */
	uint32_t       i_data[EXOFS_IDATA];/*short symlink names and device #s*/
	uint32_t       i_dir_start_lookup; /* which page to start lookup      */
	uint64_t       i_commit_size;      /* the object's written length     */
	struct exofs_comp one_comp;	   /* same component for all devices  */
	struct exofs_components comps;	   /* inode view of the device table  */
};

static inline osd_id exofs_oi_objno(struct exofs_i_info *oi)
{
	return oi->vfs_inode.i_ino + EXOFS_OBJ_OFF;
}

struct exofs_io_state;
typedef void (*exofs_io_done_fn)(struct exofs_io_state *ios, void *private);

struct exofs_io_state {
	struct kref		kref;

	void			*private;
	exofs_io_done_fn	done;

	struct exofs_layout	*layout;
	struct exofs_components	*comps;

	/* Global read/write IO*/
	loff_t			offset;
	unsigned long		length;
	void			*kern_buff;

	struct page		**pages;
	unsigned		nr_pages;
	unsigned		pgbase;
	unsigned		pages_consumed;

	/* Attributes */
	unsigned		in_attr_len;
	struct osd_attr 	*in_attr;
	unsigned		out_attr_len;
	struct osd_attr 	*out_attr;

	bool			reading;

	/* Variable array of size numdevs */
	unsigned numdevs;
	struct exofs_per_dev_state {
		struct osd_request *or;
		struct bio *bio;
		loff_t offset;
		unsigned length;
		unsigned dev;
	} per_dev[];
};

static inline unsigned exofs_io_state_size(unsigned numdevs)
{
	return sizeof(struct exofs_io_state) +
		sizeof(struct exofs_per_dev_state) * numdevs;
}

/*
 * our inode flags
 */
#define OBJ_2BCREATED	0	/* object will be created soon*/
#define OBJ_CREATED	1	/* object has been created on the osd*/

static inline int obj_2bcreated(struct exofs_i_info *oi)
{
	return test_bit(OBJ_2BCREATED, &oi->i_flags);
}

static inline void set_obj_2bcreated(struct exofs_i_info *oi)
{
	set_bit(OBJ_2BCREATED, &oi->i_flags);
}

static inline int obj_created(struct exofs_i_info *oi)
{
	return test_bit(OBJ_CREATED, &oi->i_flags);
}

static inline void set_obj_created(struct exofs_i_info *oi)
{
	set_bit(OBJ_CREATED, &oi->i_flags);
}

int __exofs_wait_obj_created(struct exofs_i_info *oi);
static inline int wait_obj_created(struct exofs_i_info *oi)
{
	if (likely(obj_created(oi)))
		return 0;

	return __exofs_wait_obj_created(oi);
}

/*
 * get to our inode from the vfs inode
 */
static inline struct exofs_i_info *exofs_i(struct inode *inode)
{
	return container_of(inode, struct exofs_i_info, vfs_inode);
}

/*
 * Maximum count of links to a file
 */
#define EXOFS_LINK_MAX           32000

/*************************
 * function declarations *
 *************************/

/* ios.c */
int exofs_get_rw_state(struct exofs_layout *layout,
		       struct exofs_components *comps,
		       bool is_reading, u64 offset, u64 length,
		       struct exofs_io_state **ios);
int exofs_get_io_state(struct exofs_layout *layout,
		       struct exofs_components *comps,
		       struct exofs_io_state **ios);
void exofs_put_io_state(struct exofs_io_state *ios);

int exofs_check_io(struct exofs_io_state *ios, u64 *resid);

int exofs_sbi_create(struct exofs_io_state *ios);
int exofs_sbi_remove(struct exofs_io_state *ios);
int exofs_sbi_write(struct exofs_io_state *ios);
int exofs_sbi_read(struct exofs_io_state *ios);
int exofs_truncate(struct exofs_layout *layout, struct exofs_components *comps,
		 u64 size);

int extract_attr_from_ios(struct exofs_io_state *ios, struct osd_attr *attr);
extern const struct osd_attr g_attr_logical_length;

/* inode.c               */
unsigned exofs_max_io_pages(struct exofs_layout *layout,
			    unsigned expected_pages);
int exofs_setattr(struct dentry *, struct iattr *);
int exofs_write_begin(struct file *file, struct address_space *mapping,
		loff_t pos, unsigned len, unsigned flags,
		struct page **pagep, void **fsdata);
extern struct inode *exofs_iget(struct super_block *, unsigned long);
struct inode *exofs_new_inode(struct inode *, int);
extern int exofs_write_inode(struct inode *, struct writeback_control *wbc);
extern void exofs_evict_inode(struct inode *);

/* dir.c:                */
int exofs_add_link(struct dentry *, struct inode *);
ino_t exofs_inode_by_name(struct inode *, struct dentry *);
int exofs_delete_entry(struct exofs_dir_entry *, struct page *);
int exofs_make_empty(struct inode *, struct inode *);
struct exofs_dir_entry *exofs_find_entry(struct inode *, struct dentry *,
					 struct page **);
int exofs_empty_dir(struct inode *);
struct exofs_dir_entry *exofs_dotdot(struct inode *, struct page **);
ino_t exofs_parent_ino(struct dentry *child);
int exofs_set_link(struct inode *, struct exofs_dir_entry *, struct page *,
		    struct inode *);

/* super.c               */
void exofs_make_credential(u8 cred_a[OSD_CAP_LEN],
			   const struct osd_obj_id *obj);
int exofs_sbi_write_stats(struct exofs_sb_info *sbi);

/*********************
 * operation vectors *
 *********************/
/* dir.c:            */
extern const struct file_operations exofs_dir_operations;

/* file.c            */
extern const struct inode_operations exofs_file_inode_operations;
extern const struct file_operations exofs_file_operations;

/* inode.c           */
extern const struct address_space_operations exofs_aops;

/* namei.c           */
extern const struct inode_operations exofs_dir_inode_operations;
extern const struct inode_operations exofs_special_inode_operations;

/* symlink.c         */
extern const struct inode_operations exofs_symlink_inode_operations;
extern const struct inode_operations exofs_fast_symlink_inode_operations;

/* exofs_init_comps will initialize an exofs_components device array
 * pointing to a single exofs_comp struct, and a round-robin view
 * of the device table.
 * The first device of each inode is the [inode->ino % num_devices]
 * and the rest of the devices sequentially following where the
 * first device is after the last device.
 * It is assumed that the global device array at @sbi is twice
 * bigger and that the device table repeats twice.
 * See: exofs_read_lookup_dev_table()
 */
static inline void exofs_init_comps(struct exofs_components *comps,
				    struct exofs_comp *one_comp,
				    struct exofs_sb_info *sbi, osd_id oid)
{
	unsigned dev_mod = (unsigned)oid, first_dev;

	one_comp->obj.partition = sbi->one_comp.obj.partition;
	one_comp->obj.id = oid;
	exofs_make_credential(one_comp->cred, &one_comp->obj);

	comps->numdevs = sbi->comps.numdevs;
	comps->single_comp = EC_SINGLE_COMP;
	comps->comps = one_comp;

	/* Round robin device view of the table */
	first_dev = (dev_mod * sbi->layout.mirrors_p1) % sbi->comps.numdevs;
	comps->ods = sbi->comps.ods + first_dev;
}

#endif
