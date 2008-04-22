/*
 * camera image capture (abstract) bus driver header
 *
 * Copyright (C) 2006, Sascha Hauer, Pengutronix
 * Copyright (C) 2008, Guennadi Liakhovetski <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SOC_CAMERA_H
#define SOC_CAMERA_H

#include <linux/videodev2.h>
#include <media/videobuf-dma-sg.h>

struct soc_camera_device {
	struct list_head list;
	struct device dev;
	struct device *control;
	unsigned short width;		/* Current window */
	unsigned short height;		/* sizes */
	unsigned short x_min;		/* Camera capabilities */
	unsigned short y_min;
	unsigned short x_current;	/* Current window location */
	unsigned short y_current;
	unsigned short width_min;
	unsigned short width_max;
	unsigned short height_min;
	unsigned short height_max;
	unsigned short y_skip_top;	/* Lines to skip at the top */
	unsigned short gain;
	unsigned short exposure;
	unsigned char iface;		/* Host number */
	unsigned char devnum;		/* Device number per host */
	unsigned char cached_datawidth;	/* See comment in .c */
	struct soc_camera_ops *ops;
	struct video_device *vdev;
	const struct soc_camera_data_format *current_fmt;
	int (*probe)(struct soc_camera_device *icd);
	void (*remove)(struct soc_camera_device *icd);
	struct module *owner;
};

struct soc_camera_file {
	struct soc_camera_device *icd;
	struct videobuf_queue vb_vidq;
};

struct soc_camera_host {
	struct list_head list;
	struct device dev;
	unsigned char nr;				/* Host number */
	size_t msize;
	struct videobuf_queue_ops *vbq_ops;
	struct module *owner;
	void *priv;
	char *drv_name;
	int (*add)(struct soc_camera_device *);
	void (*remove)(struct soc_camera_device *);
	int (*set_capture_format)(struct soc_camera_device *, __u32,
				  struct v4l2_rect *);
	int (*try_fmt_cap)(struct soc_camera_host *, struct v4l2_format *);
	int (*reqbufs)(struct soc_camera_file *, struct v4l2_requestbuffers *);
	int (*querycap)(struct soc_camera_host *, struct v4l2_capability *);
	unsigned int (*poll)(struct file *, poll_table *);
};

struct soc_camera_link {
	/* Camera bus id, used to match a camera and a bus */
	int bus_id;
	/* GPIO number to switch between 8 and 10 bit modes */
	unsigned int gpio;
};

static inline struct soc_camera_device *to_soc_camera_dev(struct device *dev)
{
	return container_of(dev, struct soc_camera_device, dev);
}

static inline struct soc_camera_host *to_soc_camera_host(struct device *dev)
{
	return container_of(dev, struct soc_camera_host, dev);
}

extern int soc_camera_host_register(struct soc_camera_host *ici,
				     struct module *owner);
extern void soc_camera_host_unregister(struct soc_camera_host *ici);
extern int soc_camera_device_register(struct soc_camera_device *icd);
extern void soc_camera_device_unregister(struct soc_camera_device *icd);

extern int soc_camera_video_start(struct soc_camera_device *icd);
extern void soc_camera_video_stop(struct soc_camera_device *icd);

struct soc_camera_data_format {
	char *name;
	unsigned int depth;
	__u32 fourcc;
	enum v4l2_colorspace colorspace;
};

struct soc_camera_ops {
	struct module *owner;
	int (*init)(struct soc_camera_device *);
	int (*release)(struct soc_camera_device *);
	int (*start_capture)(struct soc_camera_device *);
	int (*stop_capture)(struct soc_camera_device *);
	int (*set_capture_format)(struct soc_camera_device *, __u32,
				  struct v4l2_rect *, unsigned int);
	int (*try_fmt_cap)(struct soc_camera_device *, struct v4l2_format *);
	int (*get_chip_id)(struct soc_camera_device *,
			   struct v4l2_chip_ident *);
#ifdef CONFIG_VIDEO_ADV_DEBUG
	int (*get_register)(struct soc_camera_device *, struct v4l2_register *);
	int (*set_register)(struct soc_camera_device *, struct v4l2_register *);
#endif
	const struct soc_camera_data_format *formats;
	int num_formats;
	int (*get_control)(struct soc_camera_device *, struct v4l2_control *);
	int (*set_control)(struct soc_camera_device *, struct v4l2_control *);
	const struct v4l2_queryctrl *controls;
	int num_controls;
	unsigned int(*get_datawidth)(struct soc_camera_device *icd);
};

static inline struct v4l2_queryctrl const *soc_camera_find_qctrl(
	struct soc_camera_ops *ops, int id)
{
	int i;

	for (i = 0; i < ops->num_controls; i++)
		if (ops->controls[i].id == id)
			return &ops->controls[i];

	return NULL;
}

#define IS_MASTER		(1<<0)
#define IS_HSYNC_ACTIVE_HIGH	(1<<1)
#define IS_VSYNC_ACTIVE_HIGH	(1<<2)
#define IS_DATAWIDTH_8		(1<<3)
#define IS_DATAWIDTH_9		(1<<4)
#define IS_DATAWIDTH_10		(1<<5)
#define IS_PCLK_SAMPLE_RISING	(1<<6)

#endif
