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
 * @brief ATA disk driver
 *
 * This driver currently works only with CHS addressing and uses PIO.
 * Currently based on the (now obsolete) ANSI X3.221-1994 (ATA-1) standard.
 * At this point only reading is possible, not writing.
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
#include <bool.h>

#include "ata_bd.h"

#define NAME "ata_bd"

static const size_t block_size = 512;
static size_t comm_size;

static uintptr_t cmd_physical = 0x1f0;
static uintptr_t ctl_physical = 0x170;
static ata_cmd_t *cmd;
static ata_ctl_t *ctl;

static dev_handle_t dev_handle[MAX_DISKS];

static atomic_t dev_futex = FUTEX_INITIALIZER;

static disk_t disk[2];

static int ata_bd_init(void);
static void ata_bd_connection(ipc_callid_t iid, ipc_call_t *icall);
static int ata_bd_rdwr(int disk_id, ipcarg_t method, off_t offset, off_t size,
    void *buf);
static int ata_bd_read_block(int disk_id, uint64_t blk_idx, size_t blk_cnt,
    void *buf);
static int drive_identify(int drive_id, disk_t *d);

int main(int argc, char **argv)
{
	uint8_t status;
	char name[16];
	int i, rc;
	int n_disks;

	printf(NAME ": ATA disk driver\n");

	printf("cmd_physical = 0x%x\n", cmd_physical);
	printf("ctl_physical = 0x%x\n", ctl_physical);

	if (ata_bd_init() != EOK)
		return -1;

	/* Put drives to reset, disable interrupts. */
	printf("Reset drives...\n");
	pio_write_8(&ctl->device_control, DCR_SRST);
/*	printf("wait for busy\n");
	do {
		status = pio_read_8(&cmd->status);
	} while ((status & SR_BSY) == 0);
*/
	async_usleep(100);
	pio_write_8(&ctl->device_control, 0);

	do {
		status = pio_read_8(&cmd->status);
	} while ((status & SR_BSY) != 0);
	printf("Done\n");

	printf("Status = 0x%x\n", pio_read_8(&cmd->status));

	(void) drive_identify(0, &disk[0]);
	(void) drive_identify(1, &disk[1]);

	n_disks = 0;

	for (i = 0; i < MAX_DISKS; i++) {
		/* Skip unattached drives. */
		if (disk[i].present == false)
			continue;

		snprintf(name, 16, "disk%d", i);
		rc = devmap_device_register(name, &dev_handle[i]);
		if (rc != EOK) {
			devmap_hangup_phone(DEVMAP_DRIVER);
			printf(NAME ": Unable to register device %s.\n",
				name);
			return rc;
		}
		++n_disks;
	}

	if (n_disks == 0) {
		printf("No disks detected.\n");
		return -1;
	}

	printf(NAME ": Accepting connections\n");
	async_manager();

	/* Not reached */
	return 0;
}

static int drive_identify(int disk_id, disk_t *d)
{
	uint16_t data;
	uint8_t status;
	int i;

	printf("Identify drive %d\n", disk_id);
	pio_write_8(&cmd->drive_head, ((disk_id != 0) ? DHR_DRV : 0));
	async_usleep(100);
	pio_write_8(&cmd->command, 0xEC);

	status = pio_read_8(&cmd->status);
	printf("Status = 0x%x\n", status);

	d->present = false;

	/*
	 * Detect if drive is present. This is Qemu only! Need to
	 * do the right thing to work with real drives.
	 */
	if ((status & SR_DRDY) == 0) {
		printf("None attached.\n");
		return ENOENT;
	}

	for (i = 0; i < 256; i++) {
		do {
			status = pio_read_8(&cmd->status);
		} while ((status & SR_DRDY) == 0);

		data = pio_read_16(&cmd->data_port);

		switch (i) {
		case 1: d->cylinders = data; break;
		case 3: d->heads = data; break;
		case 6: d->sectors = data; break;
		}
	}

	printf("\n\nStatus = 0x%x\n", pio_read_8(&cmd->status));

	d->blocks = d->cylinders * d->heads * d->sectors;

	printf("Geometry: %u cylinders, %u heads, %u sectors\n",
		d->cylinders, d->heads, d->sectors);

	d->present = true;

	return EOK;
}

static int ata_bd_init(void)
{
	void *vaddr;
	int rc;

	rc = devmap_driver_register(NAME, ata_bd_connection);
	if (rc < 0) {
		printf(NAME ": Unable to register driver.\n");
		return rc;
	}

	rc = pio_enable((void *) cmd_physical, sizeof(ata_cmd_t), &vaddr);
	if (rc != EOK) {
		printf(NAME ": Could not initialize device I/O space.\n");
		return rc;
	}

	cmd = vaddr;

	rc = pio_enable((void *) ctl_physical, sizeof(ata_ctl_t), &vaddr);
	if (rc != EOK) {
		printf(NAME ": Could not initialize device I/O space.\n");
		return rc;
	}

	ctl = vaddr;


	return EOK;
}

static void ata_bd_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	void *fs_va = NULL;
	ipc_callid_t callid;
	ipc_call_t call;
	ipcarg_t method;
	dev_handle_t dh;
	int flags;
	int retval;
	off_t idx;
	off_t size;
	int disk_id, i;

	/* Get the device handle. */
	dh = IPC_GET_ARG1(*icall);

	/* Determine which disk device is the client connecting to. */
	disk_id = -1;
	for (i = 0; i < MAX_DISKS; i++)
		if (dev_handle[i] == dh)
			disk_id = i;

	if (disk_id < 0 || disk[disk_id].present == false) {
		ipc_answer_0(iid, EINVAL);
		return;
	}

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	ipc_answer_0(iid, EOK);

	if (!ipc_share_out_receive(&callid, &comm_size, &flags)) {
		ipc_answer_0(callid, EHANGUP);
		return;
	}

	fs_va = as_get_mappable_page(comm_size);
	if (fs_va == NULL) {
		ipc_answer_0(callid, EHANGUP);
		return;
	}

	(void) ipc_share_out_finalize(callid, fs_va);

	while (1) {
		callid = async_get_call(&call);
		method = IPC_GET_METHOD(call);
		switch (method) {
		case IPC_M_PHONE_HUNGUP:
			/* The other side has hung up. */
			ipc_answer_0(callid, EOK);
			return;
		case BD_READ_BLOCK:
		case BD_WRITE_BLOCK:
			idx = IPC_GET_ARG1(call);
			size = IPC_GET_ARG2(call);
			if (size > comm_size) {
				retval = EINVAL;
				break;
			}
			retval = ata_bd_rdwr(disk_id, method, idx,
			    size, fs_va);
			break;
		default:
			retval = EINVAL;
			break;
		}
		ipc_answer_0(callid, retval);
	}
}

static int ata_bd_rdwr(int disk_id, ipcarg_t method, off_t blk_idx, off_t size,
    void *buf)
{
	int rc;
	off_t now;

	while (size > 0) {
		now = size < block_size ? size : (off_t) block_size;
		if (now != block_size)
			return EINVAL;

		if (method == BD_READ_BLOCK)
			rc = ata_bd_read_block(disk_id, blk_idx, 1, buf);
		else
			rc = ENOTSUP;

		if (rc != EOK)
			return rc;

		buf += block_size;
		blk_idx++;

		if (size > block_size)
			size -= block_size;
		else
			size = 0;
	}

	return EOK;
}


static int ata_bd_read_block(int disk_id, uint64_t blk_idx, size_t blk_cnt,
    void *buf)
{
	size_t i;
	uint16_t data;
	uint8_t status;
	uint64_t c, h, s;
	uint64_t idx;
	uint8_t drv_head;
	disk_t *d;

	d = &disk[disk_id];

	/* Check device bounds. */
	if (blk_idx >= d->blocks)
		return EINVAL;

	/* Compute CHS. */
	c = blk_idx / (d->heads * d->sectors);
	idx = blk_idx % (d->heads * d->sectors);

	h = idx / d->sectors;
	s = 1 + (idx % d->sectors);

	/* New value for Drive/Head register */
	drv_head =
	    ((disk_id != 0) ? DHR_DRV : 0) |
	    (h & 0x0f);

	futex_down(&dev_futex);

	/* Program a Read Sectors operation. */

	pio_write_8(&cmd->drive_head, drv_head);
	pio_write_8(&cmd->sector_count, 1);
	pio_write_8(&cmd->sector_number, s);
	pio_write_8(&cmd->cylinder_low, c & 0xff);
	pio_write_8(&cmd->cylinder_high, c >> 16);
	pio_write_8(&cmd->command, 0x20);

	/* Read data from the disk buffer. */

	for (i = 0; i < 256; i++) {
		do {
			status = pio_read_8(&cmd->status);
		} while ((status & SR_DRDY) == 0);

		data = pio_read_16(&cmd->data_port);
		((uint16_t *) buf)[i] = data;
	}

	futex_up(&dev_futex);
	return EOK;
}


/**
 * @}
 */
