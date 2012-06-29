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
#include <ipc/bd.h>
#include <async.h>
#include <as.h>
#include <fibril_synch.h>
#include <stdint.h>
#include <str.h>
#include <loc.h>
#include <sys/types.h>
#include <inttypes.h>
#include <errno.h>
#include <bool.h>
#include <byteorder.h>
#include <task.h>
#include <macros.h>

#include "ata_hw.h"
#include "ata_bd.h"

#define NAME       "ata_bd"
#define NAMESPACE  "bd"

/** Number of defined legacy controller base addresses. */
#define LEGACY_CTLS 4

/**
 * Size of data returned from Identify Device or Identify Packet Device
 * command.
 */
static const size_t identify_data_size = 512;

/** I/O base address of the command registers. */
static uintptr_t cmd_physical;
/** I/O base address of the control registers. */
static uintptr_t ctl_physical;

/** I/O base addresses for legacy (ISA-compatible) controllers. */
static ata_base_t legacy_base[LEGACY_CTLS] = {
	{ 0x1f0, 0x3f0 },
	{ 0x170, 0x370 },
	{ 0x1e8, 0x3e8 },
	{ 0x168, 0x368 }
};

static ata_cmd_t *cmd;
static ata_ctl_t *ctl;

/** Per-disk state. */
static disk_t disk[MAX_DISKS];

static void print_syntax(void);
static int ata_bd_init(void);
static void ata_bd_connection(ipc_callid_t iid, ipc_call_t *icall, void *);
static int ata_bd_read_blocks(int disk_id, uint64_t ba, size_t cnt,
    void *buf);
static int ata_bd_write_blocks(int disk_id, uint64_t ba, size_t cnt,
    const void *buf);
static int ata_rcmd_read(int disk_id, uint64_t ba, size_t cnt,
    void *buf);
static int ata_rcmd_write(int disk_id, uint64_t ba, size_t cnt,
    const void *buf);
static int disk_init(disk_t *d, int disk_id);
static int drive_identify(int drive_id, void *buf);
static int identify_pkt_dev(int dev_idx, void *buf);
static int ata_cmd_packet(int dev_idx, const void *cpkt, size_t cpkt_size,
    void *obuf, size_t obuf_size);
static int ata_pcmd_inquiry(int dev_idx, void *obuf, size_t obuf_size);
static int ata_pcmd_read_12(int dev_idx, uint64_t ba, size_t cnt,
    void *obuf, size_t obuf_size);
static int ata_pcmd_read_toc(int dev_idx, uint8_t ses,
    void *obuf, size_t obuf_size);
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
	unsigned ctl_num;
	char *eptr;

	printf(NAME ": ATA disk driver\n");

	if (argc > 1) {
		ctl_num = strtoul(argv[1], &eptr, 0);
		if (*eptr != '\0' || ctl_num == 0 || ctl_num > 4) {
			printf("Invalid argument.\n");
			print_syntax();
			return -1;
		}
	} else {
		ctl_num = 1;
	}

	cmd_physical = legacy_base[ctl_num - 1].cmd;
	ctl_physical = legacy_base[ctl_num - 1].ctl;

	printf("I/O address %p/%p\n", (void *) cmd_physical,
	    (void *) ctl_physical);

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
		
		snprintf(name, 16, "%s/ata%udisk%d", NAMESPACE, ctl_num, i);
		rc = loc_service_register(name, &disk[i].service_id);
		if (rc != EOK) {
			printf(NAME ": Unable to register device %s.\n", name);
			return rc;
		}
		++n_disks;
	}

	if (n_disks == 0) {
		printf("No disks detected.\n");
		return -1;
	}

	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}


static void print_syntax(void)
{
	printf("Syntax: " NAME " <controller_number>\n");
	printf("Controller number = 1..4\n");
}

/** Print one-line device summary. */
static void disk_print_summary(disk_t *d)
{
	uint64_t mbytes;

	printf("%s: ", d->model);

	if (d->dev_type == ata_reg_dev) {
		switch (d->amode) {
		case am_chs:
			printf("CHS %u cylinders, %u heads, %u sectors",
			    disk->geom.cylinders, disk->geom.heads,
			    disk->geom.sectors);
			break;
		case am_lba28:
			printf("LBA-28");
			break;
		case am_lba48:
			printf("LBA-48");
			break;
		}
	} else {
		printf("PACKET");
	}

	printf(" %" PRIu64 " blocks", d->blocks);

	mbytes = d->blocks / (2 * 1024);
	if (mbytes > 0)
		printf(" %" PRIu64 " MB.", mbytes);

	printf("\n");
}

/** Register driver and enable device I/O. */
static int ata_bd_init(void)
{
	async_set_client_connection(ata_bd_connection);
	int rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf("%s: Unable to register driver.\n", NAME);
		return rc;
	}
	
	void *vaddr;
	rc = pio_enable((void *) cmd_physical, sizeof(ata_cmd_t), &vaddr);
	if (rc != EOK) {
		printf("%s: Could not initialize device I/O space.\n", NAME);
		return rc;
	}
	
	cmd = vaddr;
	
	rc = pio_enable((void *) ctl_physical, sizeof(ata_ctl_t), &vaddr);
	if (rc != EOK) {
		printf("%s: Could not initialize device I/O space.\n", NAME);
		return rc;
	}
	
	ctl = vaddr;
	
	return EOK;
}

/** Block device connection handler */
static void ata_bd_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	void *fs_va = NULL;
	ipc_callid_t callid;
	ipc_call_t call;
	sysarg_t method;
	service_id_t dsid;
	size_t comm_size;	/**< Size of the communication area. */
	unsigned int flags;
	int retval;
	uint64_t ba;
	size_t cnt;
	int disk_id, i;

	/* Get the device service ID. */
	dsid = IPC_GET_ARG1(*icall);

	/* Determine which disk device is the client connecting to. */
	disk_id = -1;
	for (i = 0; i < MAX_DISKS; i++)
		if (disk[i].service_id == dsid)
			disk_id = i;

	if (disk_id < 0 || disk[disk_id].present == false) {
		async_answer_0(iid, EINVAL);
		return;
	}

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	async_answer_0(iid, EOK);

	if (!async_share_out_receive(&callid, &comm_size, &flags)) {
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
			if (cnt * disk[disk_id].block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = ata_bd_read_blocks(disk_id, ba, cnt, fs_va);
			break;
		case BD_WRITE_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			if (cnt * disk[disk_id].block_size > comm_size) {
				retval = ELIMIT;
				break;
			}
			retval = ata_bd_write_blocks(disk_id, ba, cnt, fs_va);
			break;
		case BD_GET_BLOCK_SIZE:
			async_answer_1(callid, EOK, disk[disk_id].block_size);
			continue;
		case BD_GET_NUM_BLOCKS:
			async_answer_2(callid, EOK, LOWER32(disk[disk_id].blocks),
			    UPPER32(disk[disk_id].blocks));
			continue;
		case BD_READ_TOC:
			cnt = IPC_GET_ARG1(call);
			if (disk[disk_id].dev_type == ata_pkt_dev)
				retval = ata_pcmd_read_toc(disk_id, cnt, fs_va,
				    disk[disk_id].block_size);
			else
				retval = EINVAL;
			break;
		default:
			retval = EINVAL;
			break;
		}
		async_answer_0(callid, retval);
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
	ata_inquiry_data_t inq_data;
	uint16_t w;
	uint8_t c;
	uint16_t bc;
	size_t pos, len;
	int rc;
	unsigned i;

	d->present = false;
	fibril_mutex_initialize(&d->lock);

	/* Try identify command. */
	rc = drive_identify(disk_id, &idata);
	if (rc == EOK) {
		/* Success. It's a register (non-packet) device. */
		printf("ATA register-only device found.\n");
		d->dev_type = ata_reg_dev;
	} else if (rc == EIO) {
		/*
		 * There is something, but not a register device. Check to see
		 * whether the IDENTIFY command left the packet signature in
		 * the registers in case this is a packet device.
		 *
		 * According to the ATA specification, the LBA low and
		 * interrupt reason registers should be set to 0x01. However,
		 * there are many devices that do not follow this and only set
		 * the byte count registers. So, only check these.
		 */
		bc = ((uint16_t)pio_read_8(&cmd->cylinder_high) << 8) |
		    pio_read_8(&cmd->cylinder_low);

		if (bc == PDEV_SIGNATURE_BC) {
			rc = identify_pkt_dev(disk_id, &idata);
			if (rc == EOK) {
				/* We have a packet device. */
				d->dev_type = ata_pkt_dev;
			} else {
				return EIO;
			}
		} else {
			/* Nope. Something's there, but not recognized. */
			return EIO;
		}
	} else {
		/* Operation timed out. That means there is no device there. */
		return EIO;
	}

	if (d->dev_type == ata_pkt_dev) {
		/* Packet device */
		d->amode = 0;

		d->geom.cylinders = 0;
		d->geom.heads = 0;
		d->geom.sectors = 0;

		d->blocks = 0;
	} else if ((idata.caps & rd_cap_lba) == 0) {
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

	if (d->dev_type == ata_pkt_dev) {
		/* Send inquiry. */
		rc = ata_pcmd_inquiry(0, &inq_data, sizeof(inq_data));
		if (rc != EOK) {
			printf("Device inquiry failed.\n");
			d->present = false;
			return EIO;
		}

		/* Check device type. */
		if (INQUIRY_PDEV_TYPE(inq_data.pdev_type) != PDEV_TYPE_CDROM)
			printf("Warning: Peripheral device type is not CD-ROM.\n");

		/* Assume 2k block size for now. */
		d->block_size = 2048;
	} else {
		/* Assume register Read always uses 512-byte blocks. */
		d->block_size = 512;
	}

	d->present = true;
	return EOK;
}

/** Read multiple blocks from the device. */
static int ata_bd_read_blocks(int disk_id, uint64_t ba, size_t cnt,
    void *buf) {

	int rc;

	while (cnt > 0) {
		if (disk[disk_id].dev_type == ata_reg_dev)
			rc = ata_rcmd_read(disk_id, ba, 1, buf);
		else
			rc = ata_pcmd_read_12(disk_id, ba, 1, buf,
			    disk[disk_id].block_size);

		if (rc != EOK)
			return rc;

		++ba;
		--cnt;
		buf += disk[disk_id].block_size;
	}

	return EOK;
}

/** Write multiple blocks to the device. */
static int ata_bd_write_blocks(int disk_id, uint64_t ba, size_t cnt,
    const void *buf) {

	int rc;

	if (disk[disk_id].dev_type != ata_reg_dev)
		return ENOTSUP;

	while (cnt > 0) {
		rc = ata_rcmd_write(disk_id, ba, 1, buf);
		if (rc != EOK)
			return rc;

		++ba;
		--cnt;
		buf += disk[disk_id].block_size;
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
 *
 * @return		ETIMEOUT on timeout (this can mean the device is
 *			not present). EIO if device responds with error.
 */
static int drive_identify(int disk_id, void *buf)
{
	uint16_t data;
	uint8_t status;
	uint8_t drv_head;
	size_t i;

	drv_head = ((disk_id != 0) ? DHR_DRV : 0);

	if (wait_status(0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return ETIMEOUT;

	pio_write_8(&cmd->drive_head, drv_head);

	/*
	 * Do not wait for DRDY to be set in case this is a packet device.
	 * We determine whether the device is present by waiting for DRQ to be
	 * set after issuing the command.
	 */
	if (wait_status(0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return ETIMEOUT;

	pio_write_8(&cmd->command, CMD_IDENTIFY_DRIVE);

	if (wait_status(0, ~SR_BSY, &status, TIMEOUT_PROBE) != EOK)
		return ETIMEOUT;

	/*
	 * If ERR is set, this may be a packet device, so return EIO to cause
	 * the caller to check for one.
	 */
	if ((status & SR_ERR) != 0) {
		return EIO;
	}

	if (wait_status(SR_DRQ, ~SR_BSY, &status, TIMEOUT_PROBE) != EOK)
		return ETIMEOUT;

	/* Read data from the disk buffer. */

	for (i = 0; i < identify_data_size / 2; i++) {
		data = pio_read_16(&cmd->data_port);
		((uint16_t *) buf)[i] = data;
	}

	return EOK;
}

/** Issue Identify Packet Device command.
 *
 * Reads @c identify data into the provided buffer. This is used to detect
 * whether an ATAPI device is present and if so, to determine its parameters.
 *
 * @param dev_idx	Device index, 0 or 1.
 * @param buf		Pointer to a 512-byte buffer.
 */
static int identify_pkt_dev(int dev_idx, void *buf)
{
	uint16_t data;
	uint8_t status;
	uint8_t drv_head;
	size_t i;

	drv_head = ((dev_idx != 0) ? DHR_DRV : 0);

	if (wait_status(0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return EIO;

	pio_write_8(&cmd->drive_head, drv_head);

	/* For ATAPI commands we do not need to wait for DRDY. */
	if (wait_status(0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return EIO;

	pio_write_8(&cmd->command, CMD_IDENTIFY_PKT_DEV);

	if (wait_status(0, ~SR_BSY, &status, TIMEOUT_BSY) != EOK)
		return EIO;

	/* Read data from the device buffer. */

	if ((status & SR_DRQ) != 0) {
		for (i = 0; i < identify_data_size / 2; i++) {
			data = pio_read_16(&cmd->data_port);
			((uint16_t *) buf)[i] = data;
		}
	}

	if ((status & SR_ERR) != 0)
		return EIO;

	return EOK;
}

/** Issue packet command (i. e. write a command packet to the device).
 *
 * Only data-in commands are supported (e.g. inquiry, read).
 *
 * @param dev_idx	Device index (0 or 1)
 * @param obuf		Buffer for storing data read from device
 * @param obuf_size	Size of obuf in bytes
 *
 * @return EOK on success, EIO on error.
 */
static int ata_cmd_packet(int dev_idx, const void *cpkt, size_t cpkt_size,
    void *obuf, size_t obuf_size)
{
	size_t i;
	uint8_t status;
	uint8_t drv_head;
	disk_t *d;
	size_t data_size;
	uint16_t val;

	d = &disk[dev_idx];
	fibril_mutex_lock(&d->lock);

	/* New value for Drive/Head register */
	drv_head =
	    ((dev_idx != 0) ? DHR_DRV : 0);

	if (wait_status(0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK) {
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	pio_write_8(&cmd->drive_head, drv_head);

	if (wait_status(0, ~(SR_BSY|SR_DRQ), NULL, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	/* Byte count <- max. number of bytes we can read in one transfer. */
	pio_write_8(&cmd->cylinder_low, 0xfe);
	pio_write_8(&cmd->cylinder_high, 0xff);

	pio_write_8(&cmd->command, CMD_PACKET);

	if (wait_status(SR_DRQ, ~SR_BSY, &status, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	/* Write command packet. */
	for (i = 0; i < (cpkt_size + 1) / 2; i++)
		pio_write_16(&cmd->data_port, ((uint16_t *) cpkt)[i]);

	if (wait_status(0, ~SR_BSY, &status, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	if ((status & SR_DRQ) == 0) {
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	/* Read byte count. */
	data_size = (uint16_t) pio_read_8(&cmd->cylinder_low) +
	    ((uint16_t) pio_read_8(&cmd->cylinder_high) << 8);

	/* Check whether data fits into output buffer. */
	if (data_size > obuf_size) {
		/* Output buffer is too small to store data. */
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	/* Read data from the device buffer. */
	for (i = 0; i < (data_size + 1) / 2; i++) {
		val = pio_read_16(&cmd->data_port);
		((uint16_t *) obuf)[i] = val;
	}

	if (status & SR_ERR) {
		fibril_mutex_unlock(&d->lock);
		return EIO;
	}

	fibril_mutex_unlock(&d->lock);

	return EOK;
}

/** Issue ATAPI Inquiry.
 *
 * @param dev_idx	Device index (0 or 1)
 * @param obuf		Buffer for storing inquiry data read from device
 * @param obuf_size	Size of obuf in bytes
 *
 * @return EOK on success, EIO on error.
 */
static int ata_pcmd_inquiry(int dev_idx, void *obuf, size_t obuf_size)
{
	ata_pcmd_inquiry_t cp;
	int rc;

	memset(&cp, 0, sizeof(cp));

	cp.opcode = PCMD_INQUIRY;
	cp.alloc_len = min(obuf_size, 0xff); /* Allocation length */

	rc = ata_cmd_packet(0, &cp, sizeof(cp), obuf, obuf_size);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Issue ATAPI read(12) command.
 *
 * Output buffer must be large enough to hold the data, otherwise the
 * function will fail.
 *
 * @param dev_idx	Device index (0 or 1)
 * @param ba		Starting block address
 * @param cnt		Number of blocks to read
 * @param obuf		Buffer for storing inquiry data read from device
 * @param obuf_size	Size of obuf in bytes
 *
 * @return EOK on success, EIO on error.
 */
static int ata_pcmd_read_12(int dev_idx, uint64_t ba, size_t cnt,
    void *obuf, size_t obuf_size)
{
	ata_pcmd_read_12_t cp;
	int rc;

	if (ba > UINT32_MAX)
		return EINVAL;

	memset(&cp, 0, sizeof(cp));

	cp.opcode = PCMD_READ_12;
	cp.ba = host2uint32_t_be(ba);
	cp.nblocks = host2uint32_t_be(cnt);

	rc = ata_cmd_packet(0, &cp, sizeof(cp), obuf, obuf_size);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Issue ATAPI read TOC command.
 *
 * Read TOC in 'multi-session' format (first and last session number
 * with last session LBA).
 *
 * http://suif.stanford.edu/~csapuntz/specs/INF-8020.PDF page 171
 *
 * Output buffer must be large enough to hold the data, otherwise the
 * function will fail.
 *
 * @param dev_idx	Device index (0 or 1)
 * @param session	Starting session
 * @param obuf		Buffer for storing inquiry data read from device
 * @param obuf_size	Size of obuf in bytes
 *
 * @return EOK on success, EIO on error.
 */
static int ata_pcmd_read_toc(int dev_idx, uint8_t session, void *obuf,
    size_t obuf_size)
{
	ata_pcmd_read_toc_t cp;
	int rc;

	memset(&cp, 0, sizeof(cp));

	cp.opcode = PCMD_READ_TOC;
	cp.msf = 0;
	cp.format = 0x01; /* 0x01 = multi-session mode */
	cp.start = session;
	cp.size = host2uint16_t_be(obuf_size);
	cp.oldformat = 0x40; /* 0x01 = multi-session mode (shifted to MSB) */
	
	rc = ata_cmd_packet(0, &cp, sizeof(cp), obuf, obuf_size);
	if (rc != EOK)
		return rc;
	
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
static int ata_rcmd_read(int disk_id, uint64_t ba, size_t blk_cnt,
    void *buf)
{
	size_t i;
	uint16_t data;
	uint8_t status;
	uint8_t drv_head;
	disk_t *d;
	block_coord_t bc;

	d = &disk[disk_id];
	
	/* Silence warning. */
	memset(&bc, 0, sizeof(bc));

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

		for (i = 0; i < disk[disk_id].block_size / 2; i++) {
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
static int ata_rcmd_write(int disk_id, uint64_t ba, size_t cnt,
    const void *buf)
{
	size_t i;
	uint8_t status;
	uint8_t drv_head;
	disk_t *d;
	block_coord_t bc;

	d = &disk[disk_id];
	
	/* Silence warning. */
	memset(&bc, 0, sizeof(bc));

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

		for (i = 0; i < disk[disk_id].block_size / 2; i++) {
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
