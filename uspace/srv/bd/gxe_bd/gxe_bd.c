/*
 * Copyright (c) 2009 Jiri Svoboda
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

/** @addtogroup bd
 * @{
 */

/**
 * @file
 * @brief GXemul disk driver
 */

#include <stdio.h>
#include <libarch/ddi.h>
#include <ddi.h>
#include <ipc/bd.h>
#include <async.h>
#include <as.h>
#include <fibril_synch.h>
#include <loc.h>
#include <sys/types.h>
#include <errno.h>
#include <macros.h>
#include <task.h>

#define NAME       "gxe_bd"
#define NAMESPACE  "bd"

enum {
	CTL_READ_START	= 0,
	CTL_WRITE_START = 1,
};

enum {
	STATUS_FAILURE	= 0
};

enum {
	MAX_DISKS	= 2
};

typedef struct {
	uint32_t offset_lo;
	uint32_t pad0;
	uint32_t offset_hi;
	uint32_t pad1;

	uint32_t disk_id;
	uint32_t pad2[3];

	uint32_t control;
	uint32_t pad3[3];

	uint32_t status;

	uint32_t pad4[3];
	uint8_t pad5[0x3fc0];

	uint8_t buffer[512];
} gxe_bd_t;


static const size_t block_size = 512;
static size_t comm_size;

static uintptr_t dev_physical = 0x13000000;
static gxe_bd_t *dev;

static service_id_t service_id[MAX_DISKS];

static fibril_mutex_t dev_lock[MAX_DISKS];

static int gxe_bd_init(void);
static void gxe_bd_connection(ipc_callid_t iid, ipc_call_t *icall, void *);
static int gxe_bd_read_blocks(int disk_id, uint64_t ba, unsigned cnt,
    void *buf);
static int gxe_bd_write_blocks(int disk_id, uint64_t ba, unsigned cnt,
    const void *buf);
static int gxe_bd_read_block(int disk_id, uint64_t ba, void *buf);
static int gxe_bd_write_block(int disk_id, uint64_t ba, const void *buf);

int main(int argc, char **argv)
{
	printf(NAME ": GXemul disk driver\n");

	if (gxe_bd_init() != EOK)
		return -1;

	printf(NAME ": Accepting connections\n");
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

static int gxe_bd_init(void)
{
	async_set_client_connection(gxe_bd_connection);
	int rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf("%s: Unable to register driver.\n", NAME);
		return rc;
	}
	
	void *vaddr;
	rc = pio_enable((void *) dev_physical, sizeof(gxe_bd_t), &vaddr);
	if (rc != EOK) {
		printf("%s: Could not initialize device I/O space.\n", NAME);
		return rc;
	}
	
	dev = vaddr;
	
	for (unsigned int i = 0; i < MAX_DISKS; i++) {
		char name[16];
		
		snprintf(name, 16, "%s/disk%u", NAMESPACE, i);
		rc = loc_service_register(name, &service_id[i]);
		if (rc != EOK) {
			printf("%s: Unable to register device %s.\n", NAME,
			    name);
			return rc;
		}
		
		fibril_mutex_initialize(&dev_lock[i]);
	}
	
	return EOK;
}

static void gxe_bd_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	void *fs_va = NULL;
	ipc_callid_t callid;
	ipc_call_t call;
	sysarg_t method;
	service_id_t dsid;
	unsigned int flags;
	int retval;
	uint64_t ba;
	unsigned cnt;
	int disk_id, i;

	/* Get the device handle. */
	dsid = IPC_GET_ARG1(*icall);

	/* Determine which disk device is the client connecting to. */
	disk_id = -1;
	for (i = 0; i < MAX_DISKS; i++)
		if (service_id[i] == dsid)
			disk_id = i;

	if (disk_id < 0) {
		async_answer_0(iid, EINVAL);
		return;
	}

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	async_answer_0(iid, EOK);

	if (!async_share_out_receive(&callid, &comm_size, &flags)) {
		async_answer_0(callid, EHANGUP);
		return;
	}

	if (comm_size < block_size) {
		async_answer_0(callid, EHANGUP);
		return;
	}

	(void) async_share_out_finalize(callid, &fs_va);
	if (fs_va == AS_MAP_FAILED) {
		async_answer_0(callid, EHANGUP);
		return;
	}

	while (true) {
		callid = async_get_call(&call);
		method = IPC_GET_IMETHOD(call);
		
		if (!method) {
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		}
		
		switch (method) {
		case BD_READ_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			if (cnt * block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = gxe_bd_read_blocks(disk_id, ba, cnt, fs_va);
			break;
		case BD_WRITE_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			if (cnt * block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = gxe_bd_write_blocks(disk_id, ba, cnt, fs_va);
			break;
		case BD_GET_BLOCK_SIZE:
			async_answer_1(callid, EOK, block_size);
			continue;
		case BD_GET_NUM_BLOCKS:
			retval = ENOTSUP;
			break;
		default:
			retval = EINVAL;
			break;
		}
		async_answer_0(callid, retval);
	}
}

/** Read multiple blocks from the device. */
static int gxe_bd_read_blocks(int disk_id, uint64_t ba, unsigned cnt,
    void *buf) {

	int rc;

	while (cnt > 0) {
		rc = gxe_bd_read_block(disk_id, ba, buf);
		if (rc != EOK)
			return rc;

		++ba;
		--cnt;
		buf += block_size;
	}

	return EOK;
}

/** Write multiple blocks to the device. */
static int gxe_bd_write_blocks(int disk_id, uint64_t ba, unsigned cnt,
    const void *buf) {

	int rc;

	while (cnt > 0) {
		rc = gxe_bd_write_block(disk_id, ba, buf);
		if (rc != EOK)
			return rc;

		++ba;
		--cnt;
		buf += block_size;
	}

	return EOK;
}

/** Read a block from the device. */
static int gxe_bd_read_block(int disk_id, uint64_t ba, void *buf)
{
	uint32_t status;
	uint64_t byte_addr;
	size_t i;
	uint32_t w;

	byte_addr = ba * block_size;

	fibril_mutex_lock(&dev_lock[disk_id]);
	pio_write_32(&dev->offset_lo, (uint32_t) byte_addr);
	pio_write_32(&dev->offset_hi, byte_addr >> 32);
	pio_write_32(&dev->disk_id, disk_id);
	pio_write_32(&dev->control, CTL_READ_START);

	status = pio_read_32(&dev->status);
	if (status == STATUS_FAILURE) {
		fibril_mutex_unlock(&dev_lock[disk_id]);
		return EIO;
	}

	for (i = 0; i < block_size; i++) {
		((uint8_t *) buf)[i] = w = pio_read_8(&dev->buffer[i]);
	}

	fibril_mutex_unlock(&dev_lock[disk_id]);
	return EOK;
}

/** Write a block to the device. */
static int gxe_bd_write_block(int disk_id, uint64_t ba, const void *buf)
{
	uint32_t status;
	uint64_t byte_addr;
	size_t i;

	byte_addr = ba * block_size;

	fibril_mutex_lock(&dev_lock[disk_id]);

	for (i = 0; i < block_size; i++) {
		pio_write_8(&dev->buffer[i], ((const uint8_t *) buf)[i]);
	}

	pio_write_32(&dev->offset_lo, (uint32_t) byte_addr);
	pio_write_32(&dev->offset_hi, byte_addr >> 32);
	pio_write_32(&dev->disk_id, disk_id);
	pio_write_32(&dev->control, CTL_WRITE_START);

	status = pio_read_32(&dev->status);
	if (status == STATUS_FAILURE) {
		fibril_mutex_unlock(&dev_lock[disk_id]);
		return EIO;
	}

	fibril_mutex_unlock(&dev_lock[disk_id]);
	return EOK;
}

/**
 * @}
 */
