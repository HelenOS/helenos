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
#include <ipc/ipc.h>
#include <ipc/bd.h>
#include <async.h>
#include <as.h>
#include <futex.h>
#include <devmap.h>
#include <sys/types.h>
#include <errno.h>

#define NAME "gxe_bd"

enum {
	CTL_READ_START	= 0,
	CTL_WRITE_START = 1,
};

enum {
	STATUS_FAILURE	= 0
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

/*	FIXME: Need to fix pio_enable() to support >= page_size areas.

	uint32_t pad4[3];
	uint32_t pad5[0x3fc0];

	uint8_t buffer[512];*/
} gxe_bd_t;

typedef struct {
	uint8_t buffer[512];
} gxe_buf_t;

static size_t maxblock_size = 512;
static uintptr_t dev_physical = 0x13000000;
static gxe_bd_t *dev;
static gxe_buf_t *devbuf;

static uint32_t disk_id = 0;

static atomic_t dev_futex = FUTEX_INITIALIZER;

static int gxe_bd_init(void);
static void gxe_bd_connection(ipc_callid_t iid, ipc_call_t *icall);
static int gxe_bd_read_block(uint64_t offset, size_t block_size, void *buf);
static int gxe_bd_write_block(uint64_t offset, size_t block_size,
    const void *buf);

int main(int argc, char **argv)
{
	printf(NAME ": GXemul disk driver\n");

	if (gxe_bd_init() != EOK)
		return -1;

	printf(NAME ": Accepting connections\n");
	async_manager();

	/* Not reached */
	return 0;
}

static int gxe_bd_init(void)
{
	dev_handle_t dev_handle;
	void *vaddr;
	int rc;

	rc = devmap_driver_register(NAME, gxe_bd_connection);
	if (rc < 0) {
		printf(NAME ": Unable to register driver.\n");
		return rc;
	}

	rc = pio_enable((void *) dev_physical, sizeof(gxe_bd_t), &vaddr);
	if (rc != EOK) {
		printf(NAME ": Could not initialize device I/O space.\n");
		return rc;
	}

	dev = vaddr;

	rc = pio_enable((void *) dev_physical + 0x4000, sizeof(gxe_buf_t), &vaddr);
	if (rc != EOK) {
		printf(NAME ": Could not initialize device I/O space.\n");
		return rc;
	}

	devbuf = vaddr;

	rc = devmap_device_register("disk0", &dev_handle);
	if (rc != EOK) {
		devmap_hangup_phone(DEVMAP_DRIVER);
		printf(NAME ": Unable to register device.\n");
		return rc;
	}

	return EOK;
}

static void gxe_bd_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	void *fs_va = NULL;
	ipc_callid_t callid;
	ipc_call_t call;
	int flags;
	int retval;
	off_t offset;
	size_t block_size;

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	ipc_answer_0(iid, EOK);

	if (!ipc_share_out_receive(&callid, &maxblock_size, &flags)) {
		ipc_answer_0(callid, EHANGUP);
		return;
	}
	maxblock_size = 512;

	fs_va = as_get_mappable_page(maxblock_size);
	if (fs_va == NULL) {
		ipc_answer_0(callid, EHANGUP);
		return;
	}

	(void) ipc_share_out_finalize(callid, fs_va);

	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			/* The other side has hung up. */
			ipc_answer_0(callid, EOK);
			return;
		case BD_READ_BLOCK:
			offset = IPC_GET_ARG1(call);
			block_size = IPC_GET_ARG2(call);
			retval = gxe_bd_read_block(offset, block_size, fs_va);
			break;
		case BD_WRITE_BLOCK:
			offset = IPC_GET_ARG1(call);
			block_size = IPC_GET_ARG2(call);
			retval = gxe_bd_write_block(offset, block_size, fs_va);
			break;
		default:
			retval = EINVAL;
			break;
		}
		ipc_answer_0(callid, retval);
	}
}

static int gxe_bd_read_block(uint64_t offset, size_t block_size, void *buf)
{
	uint32_t status;
	size_t i;
	uint32_t w;

	if (block_size != maxblock_size) {
		printf("Failed: bs = %d, mbs = %d\n", block_size,
		    maxblock_size);
		return EINVAL;
	}

	futex_down(&dev_futex);
	pio_write_32(&dev->offset_lo, (uint32_t) offset);
	pio_write_32(&dev->offset_hi, offset >> 32);
	pio_write_32(&dev->disk_id, disk_id);
	pio_write_32(&dev->control, CTL_READ_START);

	status = pio_read_32(&dev->status);
	if (status == STATUS_FAILURE) {
		return EIO;
	}

	for (i = 0; i < maxblock_size; i++) {
		((uint8_t *) buf)[i] = w =
		    pio_read_8(&devbuf->buffer[i]);
	}

	futex_up(&dev_futex);
	return EOK;
}

static int gxe_bd_write_block(uint64_t offset, size_t block_size,
    const void *buf)
{
	uint32_t status;
	size_t i;
	uint32_t w;

	if (block_size != maxblock_size) {
		printf("Failed: bs = %d, mbs = %d\n", block_size,
		    maxblock_size);
		return EINVAL;
	}

	for (i = 0; i < maxblock_size; i++) {
		pio_write_8(&devbuf->buffer[i], ((const uint8_t *) buf)[i]);
	}

	futex_down(&dev_futex);
	pio_write_32(&dev->offset_lo, (uint32_t) offset);
	pio_write_32(&dev->offset_hi, offset >> 32);
	pio_write_32(&dev->disk_id, disk_id);
	pio_write_32(&dev->control, CTL_WRITE_START);

	status = pio_read_32(&dev->status);
	if (status == STATUS_FAILURE) {
		return EIO;
	}

	futex_up(&dev_futex);
	return EOK;
}

/**
 * @}
 */
