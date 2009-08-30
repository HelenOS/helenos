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
 * This driver supports CHS, 28-bit and 48-bit LBA addressing. It only uses
 * PIO transfers. There is no support DMA, the PACKET feature set or any other
 * fancy features such as S.M.A.R.T, removable devices, etc.
 *
 * This driver is based on the ATA-1, ATA-2, ATA-3 and ATA/ATAPI-4 through 7
 * standards, as published by the ANSI, NCITS and INCITS standards bodies,
 * which are freely available. This driver contains no vendor-specific
 * code at this moment.
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
#include <macros.h>

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
static int ata_bd_read_blocks(int disk_id, uint64_t ba, size_t cnt,
    void *buf);
static int ata_bd_write_blocks(int disk_id, uint64_t ba, size_t cnt,
    const void *buf);
static int ata_bd_read_block(int disk_id, uint64_t ba, size_t cnt,
    void *buf);
static int ata_bd_write_block(int disk_id, uint64_t ba, size_t cnt,
    const void *buf);
static int disk_init(disk_t *d, int disk_id);
static int drive_identify(int drive_id, void *buf);
static void disk_print_summary(disk_t *d);
static int coord_calc(disk_t *d, uint64_t ba, block_coord_t *bc);
static void coord_sc_program(const block_coord_t *bc, uint16_t scnt);
static int wait_status(unsigned set, unsigned n_reset, uint8_t *pstatus,
    unsigned timeout);

int main(int argc, char **argv)
{
	char name[16];
	int i, rc;
	int n_disks;

	printf(NAME ": ATA disk driver\n");

	printf("I/O address 0x%p/0x%p\n", ctl_physical, cmd_physical);

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
	uint64_t mbytes;

	printf("%s: ", d->model);

	switch (d->amode) {
	case am_chs:
		printf("CHS %u cylinders, %u heads, %u sectors",
		    disk->geom.cylinders, disk->geom.heads, disk->geom.sectors);
		break;
	case am_lba28:
		printf("LBA-28");
		break;
	case am_lba48:
		printf("LBA-48");
		break;
	}

	printf(" %llu blocks", d->blocks, d->blocks / (2 * 1024));

	mbytes = d->blocks / (2 * 1024);
	if (mbytes > 0)
		printf(" %llu MB.", mbytes);

	printf("\n");
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
	uint64_t ba;
	size_t cnt;
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
		case BD_READ_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			if (cnt * block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = ata_bd_read_blocks(disk_id, ba, cnt, fs_va);
			break;
		case BD_WRITE_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			if (cnt * block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = ata_bd_write_blocks(disk_id, ba, cnt, fs_va);
			break;
		case BD_GET_BLOCK_SIZE:
			ipc_answer_1(callid, EOK, block_size);
			continue;
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
	unsigned i;

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
	} else if ((idata.cmd_set1 & cs1_addr48) == 0) {
		/* Device only supports LBA-28 addressing. */
		d->amode = am_lba28;

		d->geom.cylinders = 0;
		d->geom.heads = 0;
		d->geom.sectors = 0;

		d->blocks =
		     (uint32_t) idata.total_lba28_0 | 
		    ((uint32_t) idata.total_lba28_1 << 16);
	} else {
		/* Device supports LBA-48 addressing. */
		d->amode = am_lba48;

		d->geom.cylinders = 0;
		d->geom.heads = 0;
		d->geom.sectors = 0;

		d->blocks =
		     (uint64_t) idata.total_lba48_0 |
		    ((uint64_t) idata.total_lba48_1 << 16) |
		    ((uint64_t) idata.total_lba48_2 << 32) | 
		    ((uint64_t) idata.total_lba48_3 << 48);
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

/** Read multiple blocks from the device. */
static int ata_bd_read_blocks(int disk_id, uint64_t ba, size_t cnt,
    void *buf) {

	int rc;

	while (cnt > 0) {
		rc = ata_bd_read_block(disk_id, ba, 1, buf);
		if (rc != EOK)
			return rc;

		++ba;
		--cnt;
		buf += block_size;
	}

	return EOK;
}

/** Write multiple blocks to the device. */
static int ata_bd_write_blocks(int disk_id, uint64_t ba, size_t cnt,
    const void *buf) {

	int rc;

	while (cnt > 0) {
		rc = ata_bd_write_block(disk_id, ba, 1, buf);
		if (rc != EOK)
			return rc;

		++ba;
		--cnt;
		buf += block_size;
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
 * @param ba		Address the first block.
 * @param cnt		Number of blocks to transfer.
 * @param buf		Buffer for holding the data.
 *
 * @return EOK on success, EIO on error.
 */
static int ata_bd_read_block(int disk_id, uint64_t ba, size_t blk_cnt,
    void *buf)
{
	size_t i;
	uint16_t data;
	uint8_t status;
	uint8_t drv_head;
	disk_t *d;
	block_coord_t bc;

	d = &disk[disk_id];
	bc.h = 0;	/* Silence warning. */

	/* Compute block coordinates. */
	if (coord_calc(d, ba, &bc) != EOK)
		return EINVAL;

	/* New value for Drive/Head register */
	drv_head =
	    ((disk_id != 0) ? DHR_DRV : 0) |
	    ((d->amode != am_chs) ? DHR_LBA : 0) |
	    (bc.h & 0x0f);

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

	/* Program block coordinates into the device. */
	coord_sc_program(&bc, 1);

	pio_write_8(&cmd->command, d->amode == am_lba48 ?
	    CMD_READ_SECTORS_EXT : CMD_READ_SECTORS);

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
 * @param ba		Address of the first block.
 * @param cnt		Number of blocks to transfer.
 * @param buf		Buffer holding the data to write.
 *
 * @return EOK on success, EIO on error.
 */
static int ata_bd_write_block(int disk_id, uint64_t ba, size_t cnt,
    const void *buf)
{
	size_t i;
	uint8_t status;
	uint8_t drv_head;
	disk_t *d;
	block_coord_t bc;

	d = &disk[disk_id];
	bc.h = 0;	/* Silence warning. */

	/* Compute block coordinates. */
	if (coord_calc(d, ba, &bc) != EOK)
		return EINVAL;

	/* New value for Drive/Head register */
	drv_head =
	    ((disk_id != 0) ? DHR_DRV : 0) |
	    ((d->amode != am_chs) ? DHR_LBA : 0) |
	    (bc.h & 0x0f);

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

	/* Program block coordinates into the device. */
	coord_sc_program(&bc, 1);

	pio_write_8(&cmd->command, d->amode == am_lba48 ?
	    CMD_WRITE_SECTORS_EXT : CMD_WRITE_SECTORS);

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

/** Calculate block coordinates.
 *
 * Calculates block coordinates in the best coordinate system supported
 * by the device. These can be later programmed into the device using
 * @c coord_sc_program().
 *
 * @return EOK on success or EINVAL if block index is past end of device.
 */
static int coord_calc(disk_t *d, uint64_t ba, block_coord_t *bc)
{
	uint64_t c;
	uint64_t idx;

	/* Check device bounds. */
	if (ba >= d->blocks)
		return EINVAL;

	bc->amode = d->amode;

	switch (d->amode) {
	case am_chs:
		/* Compute CHS coordinates. */
		c = ba / (d->geom.heads * d->geom.sectors);
		idx = ba % (d->geom.heads * d->geom.sectors);

		bc->cyl_lo = c & 0xff;
		bc->cyl_hi = (c >> 8) & 0xff;
		bc->h      = (idx / d->geom.sectors) & 0x0f;
		bc->sector = (1 + (idx % d->geom.sectors)) & 0xff;
		break;

	case am_lba28:
		/* Compute LBA-28 coordinates. */
		bc->c0 = ba & 0xff;		/* bits 0-7 */
		bc->c1 = (ba >> 8) & 0xff;	/* bits 8-15 */
		bc->c2 = (ba >> 16) & 0xff;	/* bits 16-23 */
		bc->h  = (ba >> 24) & 0x0f;	/* bits 24-27 */
		break;

	case am_lba48:
		/* Compute LBA-48 coordinates. */
		bc->c0 = ba & 0xff;		/* bits 0-7 */
		bc->c1 = (ba >> 8) & 0xff;	/* bits 8-15 */
		bc->c2 = (ba >> 16) & 0xff;	/* bits 16-23 */
		bc->c3 = (ba >> 24) & 0xff;	/* bits 24-31 */
		bc->c4 = (ba >> 32) & 0xff;	/* bits 32-39 */
		bc->c5 = (ba >> 40) & 0xff;	/* bits 40-47 */
		bc->h  = 0;
		break;
	}

	return EOK;
}

/** Program block coordinates and sector count into ATA registers.
 *
 * Note that bc->h must be programmed separately into the device/head register.
 */
static void coord_sc_program(const block_coord_t *bc, uint16_t scnt)
{
	if (bc->amode == am_lba48) {
		/* Write high-order bits. */
		pio_write_8(&cmd->sector_count, scnt >> 8);
		pio_write_8(&cmd->sector_number, bc->c3);
		pio_write_8(&cmd->cylinder_low, bc->c4);
		pio_write_8(&cmd->cylinder_high, bc->c5);
	}

	/* Write low-order bits. */
	pio_write_8(&cmd->sector_count, scnt & 0x00ff);
	pio_write_8(&cmd->sector_number, bc->c0);
	pio_write_8(&cmd->cylinder_low, bc->c1);
	pio_write_8(&cmd->cylinder_high, bc->c2);
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
