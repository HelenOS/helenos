/*
 * SPDX-FileCopyrightText: 2012 Petr Jerman
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

	async_share_out_start(exch, buf, AS_AREA_READ | AS_AREA_WRITE);

	async_exchange_end(exch);

	errno_t rc;
	async_wait_for(req, &rc);

	return rc;
}

errno_t ahci_write_blocks(async_sess_t *sess, uint64_t blocknum, size_t count,
    void *buf)
{
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return EINVAL;

	aid_t req = async_send_4(exch, DEV_IFACE_ID(AHCI_DEV_IFACE),
	    IPC_M_AHCI_WRITE_BLOCKS, HI(blocknum),  LO(blocknum), count, NULL);

	async_share_out_start(exch, buf, AS_AREA_READ | AS_AREA_WRITE);

	async_exchange_end(exch);

	errno_t rc;
	async_wait_for(req, &rc);

	return rc;
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

	ipc_call_t data;
	size_t maxblock_size;
	unsigned int flags;
	async_share_out_receive(&data, &maxblock_size, &flags);

	void *buf;
	async_share_out_finalize(&data, &buf);

	const uint64_t blocknum =
	    (((uint64_t) (DEV_IPC_GET_ARG1(*call))) << 32) |
	    (((uint64_t) (DEV_IPC_GET_ARG2(*call))) & 0xffffffff);
	const size_t cnt = (size_t) DEV_IPC_GET_ARG3(*call);

	const errno_t ret = ahci_iface->read_blocks(fun, blocknum, cnt, buf);

	async_answer_0(call, ret);
}

void remote_ahci_write_blocks(ddf_fun_t *fun, void *iface, ipc_call_t *call)
{
	const ahci_iface_t *ahci_iface = (ahci_iface_t *) iface;

	if (ahci_iface->read_blocks == NULL) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	ipc_call_t data;
	size_t maxblock_size;
	unsigned int flags;
	async_share_out_receive(&data, &maxblock_size, &flags);

	void *buf;
	async_share_out_finalize(&data, &buf);

	const uint64_t blocknum =
	    (((uint64_t)(DEV_IPC_GET_ARG1(*call))) << 32) |
	    (((uint64_t)(DEV_IPC_GET_ARG2(*call))) & 0xffffffff);
	const size_t cnt = (size_t) DEV_IPC_GET_ARG3(*call);

	const errno_t ret = ahci_iface->write_blocks(fun, blocknum, cnt, buf);

	async_answer_0(call, ret);
}

/**
 * @}
 */
