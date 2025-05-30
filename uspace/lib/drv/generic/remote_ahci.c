/*
 * Copyright (c) 2025 Jiri Svoboda
 * Copyright (c) 2012 Petr Jerman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#include <as.h>
#include <async.h>
#include <devman.h>
#include <errno.h>
#include <stdio.h>
#include <macros.h>
#include <str.h>
#include "ahci_iface.h"
#include "ddf/driver.h"

typedef enum {
	IPC_M_AHCI_GET_SATA_DEVICE_NAME,
	IPC_M_AHCI_GET_NUM_BLOCKS,
	IPC_M_AHCI_GET_BLOCK_SIZE,
	IPC_M_AHCI_READ_BLOCKS,
	IPC_M_AHCI_WRITE_BLOCKS
} ahci_iface_funcs_t;

#define MAX_NAME_LENGTH  1024

#define LO(ptr) \
	((uint32_t) (((uint64_t) ((uintptr_t) (ptr))) & 0xffffffff))

#define HI(ptr) \
	((uint32_t) (((uint64_t) ((uintptr_t) (ptr))) >> 32))

async_sess_t *ahci_get_sess(devman_handle_t funh, char **name)
{
	// FIXME: Use a better way than substring match

	*name = NULL;

	char devn[MAX_NAME_LENGTH];
	errno_t rc = devman_fun_get_name(funh, devn, MAX_NAME_LENGTH);
	if (rc != EOK)
		return NULL;

	size_t devn_size = str_size(devn);

	if ((devn_size > 5) && (str_lcmp(devn, "ahci_", 5) == 0)) {
		async_sess_t *sess = devman_device_connect(funh, IPC_FLAG_BLOCKING);

		if (sess) {
			*name = str_dup(devn);
			return sess;
		}
	}

	return NULL;
}

errno_t ahci_get_sata_device_name(async_sess_t *sess, size_t sata_dev_name_length,
    char *sata_dev_name)
{
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return EINVAL;

	aid_t req = async_send_2(exch, DEV_IFACE_ID(AHCI_DEV_IFACE),
	    IPC_M_AHCI_GET_SATA_DEVICE_NAME, sata_dev_name_length, NULL);

	async_data_read_start(exch, sata_dev_name, sata_dev_name_length);

	errno_t rc;
	async_wait_for(req, &rc);

	return rc;
}

errno_t ahci_get_num_blocks(async_sess_t *sess, uint64_t *blocks)
{
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return EINVAL;

	sysarg_t blocks_hi;
	sysarg_t blocks_lo;
	errno_t rc = async_req_1_2(exch, DEV_IFACE_ID(AHCI_DEV_IFACE),
	    IPC_M_AHCI_GET_NUM_BLOCKS, &blocks_hi, &blocks_lo);

	async_exchange_end(exch);

	if (rc == EOK) {
		*blocks = (((uint64_t) blocks_hi) << 32) |
		    (((uint64_t) blocks_lo) & 0xffffffff);
	}

	return rc;
}

errno_t ahci_get_block_size(async_sess_t *sess, size_t *blocks_size)
{
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return EINVAL;

	sysarg_t bs;
	errno_t rc = async_req_1_1(exch, DEV_IFACE_ID(AHCI_DEV_IFACE),
	    IPC_M_AHCI_GET_BLOCK_SIZE, &bs);

	async_exchange_end(exch);

	if (rc == EOK)
		*blocks_size = (size_t) bs;

	return rc;
}

errno_t ahci_read_blocks(async_sess_t *sess, uint64_t blocknum, size_t count,
    void *buf)
{
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return EINVAL;

	aid_t req;
	req = async_send_4(exch, DEV_IFACE_ID(AHCI_DEV_IFACE),
	    IPC_M_AHCI_READ_BLOCKS, HI(blocknum),  LO(blocknum), count, NULL);

	errno_t rc = async_data_read_start(exch, buf, count * 512);
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	return retval;
}

errno_t ahci_write_blocks(async_sess_t *sess, uint64_t blocknum, size_t count,
    void *buf)
{
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return EINVAL;

	aid_t req = async_send_4(exch, DEV_IFACE_ID(AHCI_DEV_IFACE),
	    IPC_M_AHCI_WRITE_BLOCKS, HI(blocknum),  LO(blocknum), count, NULL);

	errno_t rc = async_data_write_start(exch, buf, count * 512);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	return retval;
}

static void remote_ahci_get_sata_device_name(ddf_fun_t *, void *, ipc_call_t *);
static void remote_ahci_get_num_blocks(ddf_fun_t *, void *, ipc_call_t *);
static void remote_ahci_get_block_size(ddf_fun_t *, void *, ipc_call_t *);
static void remote_ahci_read_blocks(ddf_fun_t *, void *, ipc_call_t *);
static void remote_ahci_write_blocks(ddf_fun_t *, void *, ipc_call_t *);

/** Remote AHCI interface operations. */
static const remote_iface_func_ptr_t remote_ahci_iface_ops [] = {
	[IPC_M_AHCI_GET_SATA_DEVICE_NAME] = remote_ahci_get_sata_device_name,
	[IPC_M_AHCI_GET_NUM_BLOCKS] = remote_ahci_get_num_blocks,
	[IPC_M_AHCI_GET_BLOCK_SIZE] = remote_ahci_get_block_size,
	[IPC_M_AHCI_READ_BLOCKS] = remote_ahci_read_blocks,
	[IPC_M_AHCI_WRITE_BLOCKS] = remote_ahci_write_blocks
};

/** Remote AHCI interface structure.
 */
const remote_iface_t remote_ahci_iface = {
	.method_count = ARRAY_SIZE(remote_ahci_iface_ops),
	.methods = remote_ahci_iface_ops
};

void remote_ahci_get_sata_device_name(ddf_fun_t *fun, void *iface,
    ipc_call_t *call)
{
	const ahci_iface_t *ahci_iface = (ahci_iface_t *) iface;

	if (ahci_iface->get_sata_device_name == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	const size_t sata_dev_name_length =
	    (size_t) DEV_IPC_GET_ARG1(*call);

	char *sata_dev_name = malloc(sata_dev_name_length);
	if (sata_dev_name == NULL) {
		async_answer_0(call, ENOMEM);
		return;
	}

	const errno_t ret = ahci_iface->get_sata_device_name(fun,
	    sata_dev_name_length, sata_dev_name);

	ipc_call_t data;
	size_t real_size;
	if ((async_data_read_receive(&data, &real_size)) &&
	    (real_size == sata_dev_name_length))
		async_data_read_finalize(&data, sata_dev_name,
		    sata_dev_name_length);

	free(sata_dev_name);
	async_answer_0(call, ret);
}

static void remote_ahci_get_num_blocks(ddf_fun_t *fun, void *iface,
    ipc_call_t *call)
{
	const ahci_iface_t *ahci_iface = (ahci_iface_t *) iface;

	if (ahci_iface->get_num_blocks == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	uint64_t blocks;
	const errno_t ret = ahci_iface->get_num_blocks(fun, &blocks);

	if (ret != EOK)
		async_answer_0(call, ret);
	else
		async_answer_2(call, EOK, HI(blocks), LO(blocks));
}

static void remote_ahci_get_block_size(ddf_fun_t *fun, void *iface,
    ipc_call_t *call)
{
	const ahci_iface_t *ahci_iface = (ahci_iface_t *) iface;

	if (ahci_iface->get_block_size == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	size_t blocks;
	const errno_t ret = ahci_iface->get_block_size(fun, &blocks);

	if (ret != EOK)
		async_answer_0(call, ret);
	else
		async_answer_1(call, EOK, blocks);
}

void remote_ahci_read_blocks(ddf_fun_t *fun, void *iface, ipc_call_t *call)
{
	const ahci_iface_t *ahci_iface = (ahci_iface_t *) iface;

	if (ahci_iface->read_blocks == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	ipc_call_t rcall;
	size_t size;
	if (!async_data_read_receive(&rcall, &size)) {
		async_answer_0(&rcall, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	void *buf = malloc(size);
	if (buf == NULL) {
		async_answer_0(&rcall, ENOMEM);
		async_answer_0(call, ENOMEM);
		return;
	}

	uint64_t blocknum =
	    (((uint64_t) (DEV_IPC_GET_ARG1(*call))) << 32) |
	    (((uint64_t) (DEV_IPC_GET_ARG2(*call))) & 0xffffffff);
	size_t cnt = (size_t) DEV_IPC_GET_ARG3(*call);

	errno_t rc = ahci_iface->read_blocks(fun, blocknum, cnt, buf);
	if (rc != EOK) {
		async_answer_0(&rcall, ENOMEM);
		async_answer_0(call, ENOMEM);
		free(buf);
		return;
	}

	async_data_read_finalize(&rcall, buf, size);

	free(buf);
	async_answer_0(call, EOK);
}

void remote_ahci_write_blocks(ddf_fun_t *fun, void *iface, ipc_call_t *call)
{
	const ahci_iface_t *ahci_iface = (ahci_iface_t *) iface;

	if (ahci_iface->write_blocks == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	void *data;
	size_t dsize;
	errno_t rc = async_data_write_accept(&data, false, 0, 0, 0, &dsize);
	if (rc != EOK) {
		async_answer_0(call, rc);
		return;
	}

	uint64_t blocknum =
	    (((uint64_t)(DEV_IPC_GET_ARG1(*call))) << 32) |
	    (((uint64_t)(DEV_IPC_GET_ARG2(*call))) & 0xffffffff);
	size_t cnt = (size_t) DEV_IPC_GET_ARG3(*call);

	if (dsize != cnt * 512) {
		async_answer_0(call, EINVAL);
		return;
	}

	rc = ahci_iface->write_blocks(fun, blocknum, cnt, data);
	free(data);
	async_answer_0(call, rc);
}

/**
 * @}
 */
