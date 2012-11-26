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
#include <ddi.h>
#include <async.h>
#include <as.h>
#include <bd_srv.h>
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

/** GXE disk hardware registers */
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
} gxe_bd_hw_t;

/** GXE block device soft state */
typedef struct {
	/** Block device service structure */
	bd_srvs_t bds;
	int disk_id;
} gxe_bd_t;

static const size_t block_size = 512;

static uintptr_t dev_physical = 0x13000000;
static gxe_bd_hw_t *dev;

static service_id_t service_id[MAX_DISKS];

static fibril_mutex_t dev_lock[MAX_DISKS];

static gxe_bd_t gxe_bd[MAX_DISKS];

static int gxe_bd_init(void);
static void gxe_bd_connection(ipc_callid_t iid, ipc_call_t *icall, void *);
static int gxe_bd_read_block(int disk_id, uint64_t ba, void *buf);
static int gxe_bd_write_block(int disk_id, uint64_t ba, const void *buf);

static int gxe_bd_open(bd_srvs_t *, bd_srv_t *);
static int gxe_bd_close(bd_srv_t *);
static int gxe_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *, size_t);
static int gxe_bd_write_blocks(bd_srv_t *, aoff64_t, size_t, const void *, size_t);
static int gxe_bd_get_block_size(bd_srv_t *, size_t *);
static int gxe_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static bd_ops_t gxe_bd_ops = {
	.open = gxe_bd_open,
	.close = gxe_bd_close,
	.read_blocks = gxe_bd_read_blocks,
	.write_blocks = gxe_bd_write_blocks,
	.get_block_size = gxe_bd_get_block_size,
	.get_num_blocks = gxe_bd_get_num_blocks
};

static gxe_bd_t *bd_srv_gxe(bd_srv_t *bd)
{
	return (gxe_bd_t *) bd->srvs->sarg;
}

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
	rc = pio_enable((void *) dev_physical, sizeof(gxe_bd_hw_t), &vaddr);
	if (rc != EOK) {
		printf("%s: Could not initialize device I/O space.\n", NAME);
		return rc;
	}
	
	dev = vaddr;
	
	for (unsigned int i = 0; i < MAX_DISKS; i++) {
		char name[16];
		
		bd_srvs_init(&gxe_bd[i].bds);
		gxe_bd[i].bds.ops = &gxe_bd_ops;
		gxe_bd[i].bds.sarg = (void *)&gxe_bd[i];
		
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
	service_id_t dsid;
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

	bd_conn(iid, icall, &gxe_bd[disk_id].bds);
}

/** Open device. */
static int gxe_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	return EOK;
}

/** Close device. */
static int gxe_bd_close(bd_srv_t *bd)
{
	return EOK;
}

/** Read multiple blocks from the device. */
static int gxe_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size)
{
	int disk_id = bd_srv_gxe(bd)->disk_id;
	int rc;

	if (size < cnt * block_size)
		return EINVAL;

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
static int gxe_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	int disk_id = bd_srv_gxe(bd)->disk_id;
	int rc;

	if (size < cnt * block_size)
		return EINVAL;

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

/** Get device block size. */
static int gxe_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	*rsize = block_size;
	return EOK;
}

/** Get number of blocks on device. */
static int gxe_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	return ENOTSUP;
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
