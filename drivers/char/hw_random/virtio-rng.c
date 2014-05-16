/*
 * Randomness driver for virtio
 *  Copyright (C) 2007, 2008 Rusty Russell IBM Corporation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <linux/err.h>
#include <linux/hw_random.h>
#include <linux/scatterlist.h>
#include <linux/spinlock.h>
#include <linux/virtio.h>
#include <linux/virtio_rng.h>
#include <linux/module.h>


struct virtrng_info {
	struct virtio_device *vdev;
	struct hwrng hwrng;
	struct virtqueue *vq;
	unsigned int data_avail;
	struct completion have_data;
	bool busy;
};

static void random_recv_done(struct virtqueue *vq)
{
	struct virtrng_info *vi = vq->vdev->priv;

	/* We can get spurious callbacks, e.g. shared IRQs + virtio_pci. */
	if (!virtqueue_get_buf(vi->vq, &vi->data_avail))
		return;

	complete(&vi->have_data);
}

/* The host will fill any buffer we give it with sweet, sweet randomness. */
static void register_buffer(struct virtrng_info *vi, u8 *buf, size_t size)
{
	struct scatterlist sg;

	sg_init_one(&sg, buf, size);

	/* There should always be room for one buffer. */
	virtqueue_add_inbuf(vi->vq, &sg, 1, buf, GFP_KERNEL);

	virtqueue_kick(vi->vq);
}

static int virtio_read(struct hwrng *rng, void *buf, size_t size, bool wait)
{
	int ret;
	struct virtrng_info *vi = (struct virtrng_info *)rng->priv;

	if (!vi->busy) {
		vi->busy = true;
		init_completion(&vi->have_data);
		register_buffer(vi, buf, size);
	}

	if (!wait)
		return 0;

	ret = wait_for_completion_killable(&vi->have_data);
	if (ret < 0)
		return ret;

	vi->busy = false;

	return vi->data_avail;
}

static void virtio_cleanup(struct hwrng *rng)
{
	struct virtrng_info *vi = (struct virtrng_info *)rng->priv;

	if (vi->busy)
		wait_for_completion(&vi->have_data);
}

static int probe_common(struct virtio_device *vdev)
{
	int err, i;
	struct virtrng_info *vi = NULL;

	vi = kzalloc(sizeof(struct virtrng_info), GFP_KERNEL);
	vi->hwrng.name = kmalloc(40, GFP_KERNEL);
	init_completion(&vi->have_data);

	vi->hwrng.read = virtio_read;
	vi->hwrng.cleanup = virtio_cleanup;
	vi->hwrng.priv = (unsigned long)vi;
	vdev->priv = vi;

	/* We expect a single virtqueue. */
	vi->vq = virtio_find_single_vq(vdev, random_recv_done, "input");
	if (IS_ERR(vi->vq)) {
		err = PTR_ERR(vi->vq);
		kfree(vi->hwrng.name);
		vi->vq = NULL;
		kfree(vi);
		vi = NULL;
		return err;
	}

	i = 0;
	do {
		sprintf(vi->hwrng.name, "virtio_rng.%d", i++);
		err = hwrng_register(&vi->hwrng);
	} while (err == -EEXIST);

	if (err) {
		vdev->config->del_vqs(vdev);
		kfree(vi->hwrng.name);
		vi->vq = NULL;
		kfree(vi);
		vi = NULL;
		return err;
	}

	return 0;
}

static void remove_common(struct virtio_device *vdev)
{
	struct virtrng_info *vi = vdev->priv;
	vdev->config->reset(vdev);
	vi->busy = false;
	hwrng_unregister(&vi->hwrng);
	vdev->config->del_vqs(vdev);
	kfree(vi->hwrng.name);
	vi->vq = NULL;
	kfree(vi);
	vi = NULL;
}

static int virtrng_probe(struct virtio_device *vdev)
{
	return probe_common(vdev);
}

static void virtrng_remove(struct virtio_device *vdev)
{
	remove_common(vdev);
}

#ifdef CONFIG_PM_SLEEP
static int virtrng_freeze(struct virtio_device *vdev)
{
	remove_common(vdev);
	return 0;
}

static int virtrng_restore(struct virtio_device *vdev)
{
	return probe_common(vdev);
}
#endif

static struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_RNG, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static struct virtio_driver virtio_rng_driver = {
	.driver.name =	KBUILD_MODNAME,
	.driver.owner =	THIS_MODULE,
	.id_table =	id_table,
	.probe =	virtrng_probe,
	.remove =	virtrng_remove,
#ifdef CONFIG_PM_SLEEP
	.freeze =	virtrng_freeze,
	.restore =	virtrng_restore,
#endif
};

module_virtio_driver(virtio_rng_driver);
MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("Virtio random number driver");
MODULE_LICENSE("GPL");
