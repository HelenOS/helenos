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
 * Currently based on the (now obsolete) ATA-1, ATA-2 standards.
 *
 * The driver services a single controller which can have up to two disks
 * attached.
 */

#include <stdio.h>
#include <libarch/ddi.h>
#include <ddi.h>
#include <ipc/ipc.h>
#include <ipc/bd.h>
#include <async.h>
#include <as.h>
#include <fibril_sync.h>
#include <string.h>
#include <devmap.h>
#include <sys/types.h>
#include <errno.h>
#include <bool.h>
#include <task.h>

#include "ata_bd.h"

#define NAME "ata_bd"

/** Physical block size. Should be always 512. */
static const size_t block_size = 512;

/** Size of the communication area. */
static size_t comm_size;

/** I/O base address of the command registers. */
static uintptr_t cmd_physical = 0x1f0;
/** I/O base address of the control registers. */
static uintptr_t ctl_physical = 0x170;

static ata_cmd_t *cmd;
static ata_ctl_t *ctl;

/** Per-disk state. */
static disk_t disk[MAX_DISKS];

static int ata_bd_init(void);
static void ata_bd_connection(ipc_callid_t iid, ipc_call_t *icall);
static int ata_bd_rdwr(int disk_id, ipcarg_t method, off_t offset, size_t size,
    void *buf);
static int ata_bd_read_block(int disk_id, uint64_t blk_idx, size_t blk_cnt,
    void *buf);
static int ata_bd_write_block(int disk_id, uint64_t blk_idx, size_t blk_cnt,
    const void *buf);
static int disk_init(disk_t *d, int disk_id);
static int drive_identify(int drive_id, void *buf);
static void disk_print_summary(disk_t *d);
static int wait_status(unsigned set, unsigned n_reset, uint8_t *pstatus,
    unsigned timeout);

int main(int argc, char **argv)
{
	char name[16];
	int i, rc;
	int n_disks;

	printf(NAME ": ATA disk driver\n");

	printf("I/O address 0x%x\n", cmd_physical);

	if (ata_bd_init() != EOK)
		return -1;

	for (i = 0; i < MAX_DISKS; i++) {
		printf("Identify drive %d... ", i);
		fflush(stdout);

		rc = disk_init(&disk[i], i);

		if (rc == EOK) {
			disk_print_summary(&disk[i]);
		} else {
			printf("Not found.\n");
		}
	}

	n_disks = 0;

	for (i = 0; i < MAX_DISKS; i++) {
		/* Skip unattached drives. */
		if (disk[i].present == false)
			continue;

		snprintf(name, 16, "disk%d", i);
		rc = devmap_device_register(name, &disk[i].dev_handle);
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
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

/** Print one-line device summary. */
static void disk_print_summary(disk_t *d)
{
	printf("%s: ", d->model);

	if (d->amode == am_chs) {
		printf("CHS %u cylinders, %u heads, %u sectors",
		    disk->geom.cylinders, disk->geom.heads, disk->geom.sectors);
	} else {
		printf("LBA-28");
	}

	printf(" %llu blocks.\n", d->blocks);
}

/** Register driver and enable device I/O. */
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

/** Block device connection handler */
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
	size_t size;
	int disk_id, i;

	/* Get the device handle. */
	dh = IPC_GET_ARG1(*icall);

	/* Determine which disk device is the client connecting to. */
	disk_id = -1;
	for (i = 0; i < MAX_DISKS; i++)
		if (disk[i].dev_handle == dh)
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

/** Initialize a disk.
 *
 * Probes for a disk, determines its parameters and initializes
 * the disk structure.
 */
static int disk_init(disk_t *d, int disk_id)
{
	identify_data_t idata;
	uint8_t model[40];
	uint16_t w;
	uint8_t c;
	size_t pos, len;
	int rc;
	int i;

	rc = drive_identify(disk_id, &idata);
	if (rc != EOK) {
		d->present = false;
		return rc;
	}

	if ((idata.caps & cap_lba) == 0) {
		/* Device only supports CHS addressing. */
		d->amode = am_chs;

		d->geom.cylinders = idata.cylinders;
		d->geom.heads = idata.heads;
		d->geom.sectors = idata.sectors;

		d->blocks = d->geom.cylinders * d->geom.heads * d->geom.sectors;
	} else {
		/* Device supports LBA-28. */
		d->amode = am_lba28;

		d->geom.cylinders = 0;
		d->geom.heads = 0;
		d->geom.sectors = 0;

		d->blocks =
		    (uint32_t) idata.total_lba_sec0 | 
		    ((uint32_t) idata.total_lba_sec1 << 16);
	}

	/*
	 * Convert model name to string representation.
	 */
	for (i = 0; i < 20; i++) {
		w = idata.model_name[i];
		model[2 * i] = w >> 8;
		model[2 * i + 1] = w & 0x00ff;
	}

	len = 40;
	while (len > 0 && model[len - 1] == 0x20)
		--len;

	pos = 0;
	for (i = 0; i < len; ++i) {
		c = model[i];
		if (c >= 0x80) c = '?';

		chr_encode(c, d->model, &pos, 40);
	}
	d->model[pos] = '\0';

	d->present = true;
	fibril_mutex_initialize(&d->lock);

	return EOK;
}

/** Transfer a logical block from/to the device.
 *
 * @param disk_id	Device index (0 or 1)
 * @param method	@c BD_READ_BLOCK or @c BD_WRITE_BLOCK
 * @param blk_idx	Index of the first block.
 * @param size		Size of the logical block.
 * @param buf		Data buffer.
 *
 * @return EOK on success, EIO on error.
 */
static int ata_bd_rdwr(int disk_id, ipcarg_t method, off_t blk_idx, size_t size,
    void *buf)
{
	int rc;
	size_t now;

	while (size > 0) {
		now = size < block_size ? size : block_size;
		if (now != block_size)
			return EINVAL;

		if (method == BD_READ_BLOCK)
			rc = ata_bd_read_block(disk_id, blk_idx, 1, buf);
		else
			rc = ata_bd_write_block(disk_id, blk_idx, 1, buf);

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

/** Issue IDENTIFY command.
 *
 * Reads @c identify data into the provided buffer. This is used to detect
 * whether an ATA device is present and if so, to determine its parameters.
 *
 * @param disk_id	Device ID, 0 or 1.
 * @param buf		Pointer to a 512-byte buffer.
 */
static int drive_identify(int disk_id, void *buf)
{
	uint16_t data;
	uint8_t status;
	uint8_t drv_head;
	size_t i;

	drv_head = ((disk_id != 0) ? DHR_DRV : 0);

	if (wait_status(0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return EIO;

	pio_write_8(&cmd->drive_head, drv_head);

	/*
	 * This is where we would most likely expect a non-existing device to
	 * show up by not setting SR_DRDY.
	 */
	if (wait_status(SR_DRDY, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return EIO;

	pio_write_8(&cmd->command, CMD_IDENTIFY_DRIVE);

	if (wait_status(0, ~SR_BSY, &status, TIMEOUT_PROBE) != EOK)
		return EIO;

	/* Read data from the disk buffer. */

	if ((status & SR_DRQ) != 0) {
		for (i = 0; i < block_size / 2; i++) {
			data = pio_read_16(&cmd->data_port);
			((uint16_t *) buf)[i] = data;
		}
	}

	if ((status & SR_ERR) != 0)
		return EIO;

	return EOK;
}

/** Read a physical from the device.
 *
 * @param disk_id	Device index (0 or 1)
 * @param blk_idx	Index of the first block.
 * @param blk_cnt	Number of blocks to transfer.
 * @param buf		Buffer for holding the data.
 *
 * @return EOK on success, EIO on error.
 */
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

	if (d->amode == am_chs) {
		/* Compute CHS coordinates. */
		c = blk_idx / (d->geom.heads * d->geom.sectors);
		idx = blk_idx % (d->geom.heads * d->geom.sectors);

		h = idx / d->geom.sectors;
		s = 1 + (idx % d->geom.sectors);
	} else {
		/* Compute LBA-28 coordinates. */
		s = blk_idx & 0xff;		/* bits 0-7 */
		c = (blk_idx >> 8) & 0xffff;	/* bits 8-23 */
		h = (blk_idx >> 24) & 0x0f;	/* bits 24-27 */
	}

	/* New value for Drive/Head register */
	drv_head =
	    ((disk_id != 0) ? DHR_DRV : 0) |
	    ((d->amode != am_chs) ? DHR_LBA : 0) |
	    (h & 0x0f);

	fibril_mutex_lock(&d->lock);

	/* Program a Read Sectors operation. */

	if (wait_status(0, ~SR_BSY, NULL, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	pio_write_8(&cmd->drive_head, drv_head);

	if (wait_status(SR_DRDY, ~SR_BSY, NULL, TIMEOUT_DRDY) != EOK) {
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	pio_write_8(&cmd->sector_count, 1);
	pio_write_8(&cmd->sector_number, s);
	pio_write_8(&cmd->cylinder_low, c & 0xff);
	pio_write_8(&cmd->cylinder_high, c >> 16);

	pio_write_8(&cmd->command, CMD_READ_SECTORS);

	if (wait_status(0, ~SR_BSY, &status, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	if ((status & SR_DRQ) != 0) {
		/* Read data from the device buffer. */

		for (i = 0; i < block_size / 2; i++) {
			data = pio_read_16(&cmd->data_port);
			((uint16_t *) buf)[i] = data;
		}
	}

	if ((status & SR_ERR) != 0)
		return EIO;

	fibril_mutex_unlock(&d->lock);
	return EOK;
}

/** Write a physical block to the device.
 *
 * @param disk_id	Device index (0 or 1)
 * @param blk_idx	Index of the first block.
 * @param blk_cnt	Number of blocks to transfer.
 * @param buf		Buffer holding the data to write.
 *
 * @return EOK on success, EIO on error.
 */
static int ata_bd_write_block(int disk_id, uint64_t blk_idx, size_t blk_cnt,
    const void *buf)
{
	size_t i;
	uint8_t status;
	uint64_t c, h, s;
	uint64_t idx;
	uint8_t drv_head;
	disk_t *d;

	d = &disk[disk_id];

	/* Check device bounds. */
	if (blk_idx >= d->blocks)
		return EINVAL;

	if (d->amode == am_chs) {
		/* Compute CHS coordinates. */
		c = blk_idx / (d->geom.heads * d->geom.sectors);
		idx = blk_idx % (d->geom.heads * d->geom.sectors);

		h = idx / d->geom.sectors;
		s = 1 + (idx % d->geom.sectors);
	} else {
		/* Compute LBA-28 coordinates. */
		s = blk_idx & 0xff;		/* bits 0-7 */
		c = (blk_idx >> 8) & 0xffff;	/* bits 8-23 */
		h = (blk_idx >> 24) & 0x0f;	/* bits 24-27 */
	}

	/* New value for Drive/Head register */
	drv_head =
	    ((disk_id != 0) ? DHR_DRV : 0) |
	    ((d->amode != am_chs) ? DHR_LBA : 0) |
	    (h & 0x0f);

	fibril_mutex_lock(&d->lock);

	/* Program a Write Sectors operation. */

	if (wait_status(0, ~SR_BSY, NULL, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	pio_write_8(&cmd->drive_head, drv_head);

	if (wait_status(SR_DRDY, ~SR_BSY, NULL, TIMEOUT_DRDY) != EOK) {
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	pio_write_8(&cmd->sector_count, 1);
	pio_write_8(&cmd->sector_number, s);
	pio_write_8(&cmd->cylinder_low, c & 0xff);
	pio_write_8(&cmd->cylinder_high, c >> 16);

	pio_write_8(&cmd->command, CMD_WRITE_SECTORS);

	if (wait_status(0, ~SR_BSY, &status, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	if ((status & SR_DRQ) != 0) {
		/* Write data to the device buffer. */

		for (i = 0; i < block_size / 2; i++) {
			pio_write_16(&cmd->data_port, ((uint16_t *) buf)[i]);
		}
	}

	fibril_mutex_unlock(&d->lock);

	if (status & SR_ERR)
		return EIO;

	return EOK;
}

/** Wait until some status bits are set and some are reset.
 *
 * Example: wait_status(SR_DRDY, ~SR_BSY) waits for SR_DRDY to become
 * set and SR_BSY to become reset.
 *
 * @param set		Combination if bits which must be all set.
 * @param n_reset	Negated combination of bits which must be all reset.
 * @param pstatus	Pointer where to store last read status or NULL.
 * @param timeout	Timeout in 10ms units.
 *
 * @return		EOK on success, EIO on timeout.
 */
static int wait_status(unsigned set, unsigned n_reset, uint8_t *pstatus,
    unsigned timeout)
{
	uint8_t status;
	int cnt;

	status = pio_read_8(&cmd->status);

	/*
	 * This is crude, yet simple. First try with 1us delays
	 * (most likely the device will respond very fast). If not,
	 * start trying every 10 ms.
	 */

	cnt = 100;
	while ((status & ~n_reset) != 0 || (status & set) != set) {
		async_usleep(1);
		--cnt;
		if (cnt <= 0) break;

		status = pio_read_8(&cmd->status);
	}

	cnt = timeout;
	while ((status & ~n_reset) != 0 || (status & set) != set) {
		async_usleep(10000);
		--cnt;
		if (cnt <= 0) break;

		status = pio_read_8(&cmd->status);
	}

	if (pstatus)
		*pstatus = status;

	if (cnt == 0)
		return EIO;

	return EOK;
}

/**
 * @}
 */
