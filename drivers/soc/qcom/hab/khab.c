/* Copyright (c) 2016-2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include "hab.h"

int32_t habmm_socket_open(int32_t *handle, uint32_t mm_ip_id,
		uint32_t timeout, uint32_t flags)
{
	return hab_vchan_open(hab_driver.kctx, mm_ip_id, handle, flags);
}
EXPORT_SYMBOL(habmm_socket_open);

int32_t habmm_socket_close(int32_t handle)
{
	hab_vchan_close(hab_driver.kctx, handle);
	return 0;
}
EXPORT_SYMBOL(habmm_socket_close);

int32_t habmm_socket_send(int32_t handle, void *src_buff,
		uint32_t size_bytes, uint32_t flags)
{
	struct hab_send param = {0};

	param.vcid = handle;
	param.data = (uint64_t)(uintptr_t)src_buff;
	param.sizebytes = size_bytes;
	param.flags = flags;

	return hab_vchan_send(hab_driver.kctx, handle,
			size_bytes, src_buff, flags);
}
EXPORT_SYMBOL(habmm_socket_send);

int32_t habmm_socket_recv(int32_t handle, void *dst_buff, uint32_t *size_bytes,
		uint32_t timeout, uint32_t flags)
{
	int ret = 0;
	struct hab_message *msg;

	if (!size_bytes || !dst_buff)
		return -EINVAL;

	msg = hab_vchan_recv(hab_driver.kctx, handle, flags);

	if (IS_ERR(msg)) {
		*size_bytes = 0;
		return PTR_ERR(msg);
	}

	if (*size_bytes < msg->sizebytes) {
		*size_bytes = 0;
		ret = -EINVAL;
	} else {
		memcpy(dst_buff, msg->data, msg->sizebytes);
		*size_bytes = msg->sizebytes;
	}

	hab_msg_free(msg);
	return ret;
}
EXPORT_SYMBOL(habmm_socket_recv);

int32_t habmm_export(int32_t handle, void *buff_to_share, uint32_t size_bytes,
		 uint32_t *export_id, uint32_t flags)
{
	int ret;
	struct hab_export param = {0};

	if (!export_id)
		return -EINVAL;

	param.vcid = handle;
	param.buffer = (uint64_t)(uintptr_t)buff_to_share;
	param.sizebytes = size_bytes;

	ret = hab_mem_export(hab_driver.kctx, &param, 1);

	*export_id = param.exportid;
	return ret;
}
EXPORT_SYMBOL(habmm_export);

int32_t habmm_unexport(int32_t handle, uint32_t export_id, uint32_t flags)
{
	struct hab_unexport param = {0};

	param.vcid = handle;
	param.exportid = export_id;

	return hab_mem_unexport(hab_driver.kctx, &param, 1);
}
EXPORT_SYMBOL(habmm_unexport);

int32_t habmm_import(int32_t handle, void **buff_shared, uint32_t size_bytes,
		uint32_t export_id, uint32_t flags)
{
	int ret;
	struct hab_import param = {0};

	if (!buff_shared)
		return -EINVAL;

	param.vcid = handle;
	param.sizebytes = size_bytes;
	param.exportid = export_id;
	param.flags = flags;

	ret = hab_mem_import(hab_driver.kctx, &param, 1);
	if (!IS_ERR(ret))
		*buff_shared = (void *)(uintptr_t)param.kva;

	return ret;
}
EXPORT_SYMBOL(habmm_import);

int32_t habmm_unimport(int32_t handle,
		uint32_t export_id,
		void *buff_shared,
		uint32_t flags)
{
	struct hab_unimport param = {0};

	param.vcid = handle;
	param.exportid = export_id;
	param.kva = (uint64_t)(uintptr_t)buff_shared;

	return hab_mem_unimport(hab_driver.kctx, &param, 1);
}
EXPORT_SYMBOL(habmm_unimport);
