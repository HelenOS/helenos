/*
 * Copyright (c) 2013 Jiri Svoboda
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
 * This driver supports CHS, 28-bit and 48-bit LBA addressing, as well as
 * PACKET devices. It only uses PIO transfers. There is no support DMA
 * or any other fancy features such as S.M.A.R.T, removable devices, etc.
 *
 * This driver is based on the ATA-1, ATA-2, ATA-3 and ATA/ATAPI-4 through 7
 * standards, as published by the ANSI, NCITS and INCITS standards bodies,
 * which are freely available. This driver contains no vendor-specific
 * code at this moment.
 *
 * The driver services a single controller which can have up to two disks
 * attached.
 */

#include <ddi.h>
#include <ddf/log.h>
#include <async.h>
#include <as.h>
#include <bd_srv.h>
#include <fibril_synch.h>
#include <scsi/mmc.h>
#include <scsi/sbc.h>
#include <scsi/spc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <str.h>
#include <inttypes.h>
#include <errno.h>
#include <byteorder.h>
#include <macros.h>

#include "ata_hw.h"
#include "ata_bd.h"
#include "main.h"

#define NAME       "ata_bd"

/** Number of defined legacy controller base addresses. */
#define LEGACY_CTLS 4

/**
 * Size of data returned from Identify Device or Identify Packet Device
 * command.
 */
static const size_t identify_data_size = 512;

static errno_t ata_bd_init_io(ata_ctrl_t *ctrl);
static void ata_bd_fini_io(ata_ctrl_t *ctrl);

static errno_t ata_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t ata_bd_close(bd_srv_t *);
static errno_t ata_bd_read_blocks(bd_srv_t *, uint64_t ba, size_t cnt, void *buf,
    size_t);
static errno_t ata_bd_read_toc(bd_srv_t *, uint8_t session, void *buf, size_t);
static errno_t ata_bd_write_blocks(bd_srv_t *, uint64_t ba, size_t cnt,
    const void *buf, size_t);
static errno_t ata_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t ata_bd_get_num_blocks(bd_srv_t *, aoff64_t *);
static errno_t ata_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);

static errno_t ata_rcmd_read(disk_t *disk, uint64_t ba, size_t cnt,
    void *buf);
static errno_t ata_rcmd_write(disk_t *disk, uint64_t ba, size_t cnt,
    const void *buf);
static errno_t ata_rcmd_flush_cache(disk_t *disk);
static errno_t disk_init(ata_ctrl_t *ctrl, disk_t *d, int disk_id);
static errno_t ata_identify_dev(disk_t *disk, void *buf);
static errno_t ata_identify_pkt_dev(disk_t *disk, void *buf);
static errno_t ata_cmd_packet(disk_t *disk, const void *cpkt, size_t cpkt_size,
    void *obuf, size_t obuf_size, size_t *rcvd_size);
static errno_t ata_pcmd_inquiry(disk_t *disk, void *obuf, size_t obuf_size,
    size_t *rcvd_size);
static errno_t ata_pcmd_read_12(disk_t *disk, uint64_t ba, size_t cnt,
    void *obuf, size_t obuf_size);
static errno_t ata_pcmd_read_capacity(disk_t *disk, uint64_t *nblocks,
    size_t *block_size);
static errno_t ata_pcmd_read_toc(disk_t *disk, uint8_t ses,
    void *obuf, size_t obuf_size);
static void disk_print_summary(disk_t *d);
static errno_t coord_calc(disk_t *d, uint64_t ba, block_coord_t *bc);
static void coord_sc_program(ata_ctrl_t *ctrl, const block_coord_t *bc,
    uint16_t scnt);
static errno_t wait_status(ata_ctrl_t *ctrl, unsigned set, unsigned n_reset,
    uint8_t *pstatus, unsigned timeout);

bd_ops_t ata_bd_ops = {
	.open = ata_bd_open,
	.close = ata_bd_close,
	.read_blocks = ata_bd_read_blocks,
	.read_toc = ata_bd_read_toc,
	.write_blocks = ata_bd_write_blocks,
	.get_block_size = ata_bd_get_block_size,
	.get_num_blocks = ata_bd_get_num_blocks,
	.sync_cache = ata_bd_sync_cache
};

static disk_t *bd_srv_disk(bd_srv_t *bd)
{
	return (disk_t *)bd->srvs->sarg;
}

static int disk_dev_idx(disk_t *disk)
{
	return (disk->disk_id & 1);
}

/** Initialize ATA controller. */
errno_t ata_ctrl_init(ata_ctrl_t *ctrl, ata_base_t *res)
{
	int i;
	errno_t rc;
	int n_disks;

	ddf_msg(LVL_DEBUG, "ata_ctrl_init()");

	fibril_mutex_initialize(&ctrl->lock);
	ctrl->cmd_physical = res->cmd;
	ctrl->ctl_physical = res->ctl;

	ddf_msg(LVL_NOTE, "I/O address %p/%p", (void *) ctrl->cmd_physical,
	    (void *) ctrl->ctl_physical);

	rc = ata_bd_init_io(ctrl);
	if (rc != EOK)
		return rc;

	for (i = 0; i < MAX_DISKS; i++) {
		ddf_msg(LVL_NOTE, "Identify drive %d...", i);

		rc = disk_init(ctrl, &ctrl->disk[i], i);

		if (rc == EOK) {
			disk_print_summary(&ctrl->disk[i]);
		} else {
			ddf_msg(LVL_NOTE, "Not found.");
		}
	}

	n_disks = 0;

	for (i = 0; i < MAX_DISKS; i++) {
		/* Skip unattached drives. */
		if (ctrl->disk[i].present == false)
			continue;

		rc = ata_fun_create(&ctrl->disk[i]);
		if (rc != EOK) {
			ddf_msg(LVL_ERROR, "Unable to create function for "
			    "disk %d.", i);
			goto error;
		}
		++n_disks;
	}

	if (n_disks == 0) {
		ddf_msg(LVL_WARN, "No disks detected.");
		rc = EIO;
		goto error;
	}

	return EOK;
error:
	for (i = 0; i < MAX_DISKS; i++) {
		if (ata_fun_remove(&ctrl->disk[i]) != EOK) {
			ddf_msg(LVL_ERROR, "Unable to clean up function for "
			    "disk %d.", i);
		}
	}
	ata_bd_fini_io(ctrl);
	return rc;
}

/** Remove ATA controller. */
errno_t ata_ctrl_remove(ata_ctrl_t *ctrl)
{
	int i;
	errno_t rc;

	ddf_msg(LVL_DEBUG, ": ata_ctrl_remove()");

	fibril_mutex_lock(&ctrl->lock);

	for (i = 0; i < MAX_DISKS; i++) {
		rc = ata_fun_remove(&ctrl->disk[i]);
		if (rc != EOK) {
			ddf_msg(LVL_ERROR, "Unable to clean up function for "
			    "disk %d.", i);
			return rc;
		}
	}

	ata_bd_fini_io(ctrl);
	fibril_mutex_unlock(&ctrl->lock);

	return EOK;
}

/** Surprise removal of ATA controller. */
errno_t ata_ctrl_gone(ata_ctrl_t *ctrl)
{
	int i;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "ata_ctrl_gone()");

	fibril_mutex_lock(&ctrl->lock);

	for (i = 0; i < MAX_DISKS; i++) {
		rc = ata_fun_unbind(&ctrl->disk[i]);
		if (rc != EOK) {
			ddf_msg(LVL_ERROR, "Unable to clean up function for "
			    "disk %d.", i);
			return rc;
		}
	}

	ata_bd_fini_io(ctrl);
	fibril_mutex_unlock(&ctrl->lock);

	return EOK;
}

/** Print one-line device summary. */
static void disk_print_summary(disk_t *d)
{
	uint64_t mbytes;
	char *atype = NULL;
	char *cap = NULL;
	int rc;

	if (d->dev_type == ata_reg_dev) {
		switch (d->amode) {
		case am_chs:
			rc = asprintf(&atype, "CHS %u cylinders, %u heads, "
			    "%u sectors", d->geom.cylinders, d->geom.heads,
			    d->geom.sectors);
			if (rc < 0) {
				/* Out of memory */
				atype = NULL;
			}
			break;
		case am_lba28:
			atype = str_dup("LBA-28");
			break;
		case am_lba48:
			atype = str_dup("LBA-48");
			break;
		}
	} else {
		atype = str_dup("PACKET");
	}

	if (atype == NULL)
		return;

	mbytes = d->blocks / (2 * 1024);
	if (mbytes > 0) {
		rc = asprintf(&cap, " %" PRIu64 " MB.", mbytes);
		if (rc < 0) {
			cap = NULL;
			goto cleanup;
		}
	}

	ddf_msg(LVL_NOTE, "%s: %s %" PRIu64 " blocks%s", d->model, atype,
	    d->blocks, cap);
cleanup:
	free(atype);
	free(cap);
}

/** Enable device I/O. */
static errno_t ata_bd_init_io(ata_ctrl_t *ctrl)
{
	errno_t rc;
	void *vaddr;

	rc = pio_enable((void *) ctrl->cmd_physical, sizeof(ata_cmd_t), &vaddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot initialize device I/O space.");
		return rc;
	}

	ctrl->cmd = vaddr;

	rc = pio_enable((void *) ctrl->ctl_physical, sizeof(ata_ctl_t), &vaddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot initialize device I/O space.");
		return rc;
	}

	ctrl->ctl = vaddr;

	return EOK;
}

/** Clean up device I/O. */
static void ata_bd_fini_io(ata_ctrl_t *ctrl)
{
	(void) ctrl;
	/* XXX TODO */
}

/** Initialize a disk.
 *
 * Probes for a disk, determines its parameters and initializes
 * the disk structure.
 */
static errno_t disk_init(ata_ctrl_t *ctrl, disk_t *d, int disk_id)
{
	identify_data_t idata;
	uint8_t model[40];
	scsi_std_inquiry_data_t inq_data;
	size_t isize;
	uint16_t w;
	uint8_t c;
	uint16_t bc;
	uint64_t nblocks;
	size_t block_size;
	size_t pos, len;
	errno_t rc;
	unsigned i;

	d->ctrl = ctrl;
	d->disk_id = disk_id;
	d->present = false;
	d->afun = NULL;

	/* Try identify command. */
	rc = ata_identify_dev(d, &idata);
	if (rc == EOK) {
		/* Success. It's a register (non-packet) device. */
		ddf_msg(LVL_NOTE, "ATA register-only device found.");
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
		bc = ((uint16_t)pio_read_8(&ctrl->cmd->cylinder_high) << 8) |
		    pio_read_8(&ctrl->cmd->cylinder_low);

		if (bc == PDEV_SIGNATURE_BC) {
			rc = ata_identify_pkt_dev(d, &idata);
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
		if (c >= 0x80)
			c = '?';

		chr_encode(c, d->model, &pos, 40);
	}
	d->model[pos] = '\0';

	if (d->dev_type == ata_pkt_dev) {
		/* Send inquiry. */
		rc = ata_pcmd_inquiry(d, &inq_data, sizeof(inq_data), &isize);
		if (rc != EOK || isize < sizeof(inq_data)) {
			ddf_msg(LVL_ERROR, "Device inquiry failed.");
			d->present = false;
			return EIO;
		}

		/* Check device type. */
		if (INQUIRY_PDEV_TYPE(inq_data.pqual_devtype) != SCSI_DEV_CD_DVD)
			ddf_msg(LVL_WARN, "Peripheral device type is not CD-ROM.");

		rc = ata_pcmd_read_capacity(d, &nblocks, &block_size);
		if (rc != EOK) {
			ddf_msg(LVL_ERROR, "Read capacity command failed.");
			d->present = false;
			return EIO;
		}

		d->blocks = nblocks;
		d->block_size = block_size;
	} else {
		/* Assume register Read always uses 512-byte blocks. */
		d->block_size = 512;
	}

	d->present = true;
	return EOK;
}

static errno_t ata_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	return EOK;
}

static errno_t ata_bd_close(bd_srv_t *bd)
{
	return EOK;
}

/** Read multiple blocks from the device. */
static errno_t ata_bd_read_blocks(bd_srv_t *bd, uint64_t ba, size_t cnt,
    void *buf, size_t size)
{
	disk_t *disk = bd_srv_disk(bd);
	errno_t rc;

	if (size < cnt * disk->block_size)
		return EINVAL;

	while (cnt > 0) {
		if (disk->dev_type == ata_reg_dev) {
			rc = ata_rcmd_read(disk, ba, 1, buf);
		} else {
			rc = ata_pcmd_read_12(disk, ba, 1, buf,
			    disk->block_size);
		}

		if (rc != EOK)
			return rc;

		++ba;
		--cnt;
		buf += disk->block_size;
	}

	return EOK;
}

/** Read TOC from device. */
static errno_t ata_bd_read_toc(bd_srv_t *bd, uint8_t session, void *buf, size_t size)
{
	disk_t *disk = bd_srv_disk(bd);

	return ata_pcmd_read_toc(disk, session, buf, size);
}

/** Write multiple blocks to the device. */
static errno_t ata_bd_write_blocks(bd_srv_t *bd, uint64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	disk_t *disk = bd_srv_disk(bd);
	errno_t rc;

	if (disk->dev_type != ata_reg_dev)
		return ENOTSUP;

	if (size < cnt * disk->block_size)
		return EINVAL;

	while (cnt > 0) {
		rc = ata_rcmd_write(disk, ba, 1, buf);
		if (rc != EOK)
			return rc;

		++ba;
		--cnt;
		buf += disk->block_size;
	}

	return EOK;
}

/** Get device block size. */
static errno_t ata_bd_get_block_size(bd_srv_t *bd, size_t *rbsize)
{
	disk_t *disk = bd_srv_disk(bd);

	*rbsize = disk->block_size;
	return EOK;
}

/** Get device number of blocks. */
static errno_t ata_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	disk_t *disk = bd_srv_disk(bd);

	*rnb = disk->blocks;
	return EOK;
}

/** Flush cache. */
static errno_t ata_bd_sync_cache(bd_srv_t *bd, uint64_t ba, size_t cnt)
{
	disk_t *disk = bd_srv_disk(bd);

	/* ATA cannot flush just some blocks, we just flush everything. */
	(void)ba;
	(void)cnt;

	return ata_rcmd_flush_cache(disk);
}

/** PIO data-in command protocol. */
static errno_t ata_pio_data_in(disk_t *disk, void *obuf, size_t obuf_size,
    size_t blk_size, size_t nblocks)
{
	ata_ctrl_t *ctrl = disk->ctrl;
	uint16_t data;
	size_t i;
	uint8_t status;

	/* XXX Support multiple blocks */
	assert(nblocks == 1);
	assert(blk_size % 2 == 0);

	if (wait_status(ctrl, 0, ~SR_BSY, &status, TIMEOUT_BSY) != EOK)
		return EIO;

	if ((status & SR_DRQ) != 0) {
		/* Read data from the device buffer. */

		for (i = 0; i < blk_size / 2; i++) {
			data = pio_read_16(&ctrl->cmd->data_port);
			((uint16_t *) obuf)[i] = data;
		}
	}

	if ((status & SR_ERR) != 0)
		return EIO;

	return EOK;
}

/** PIO data-out command protocol. */
static errno_t ata_pio_data_out(disk_t *disk, const void *buf, size_t buf_size,
    size_t blk_size, size_t nblocks)
{
	ata_ctrl_t *ctrl = disk->ctrl;
	size_t i;
	uint8_t status;

	/* XXX Support multiple blocks */
	assert(nblocks == 1);
	assert(blk_size % 2 == 0);

	if (wait_status(ctrl, 0, ~SR_BSY, &status, TIMEOUT_BSY) != EOK)
		return EIO;

	if ((status & SR_DRQ) != 0) {
		/* Write data to the device buffer. */

		for (i = 0; i < blk_size / 2; i++) {
			pio_write_16(&ctrl->cmd->data_port, ((uint16_t *) buf)[i]);
		}
	}

	if (status & SR_ERR)
		return EIO;

	return EOK;
}

/** PIO non-data command protocol. */
static errno_t ata_pio_nondata(disk_t *disk)
{
	ata_ctrl_t *ctrl = disk->ctrl;
	uint8_t status;

	if (wait_status(ctrl, 0, ~SR_BSY, &status, TIMEOUT_BSY) != EOK)
		return EIO;

	if (status & SR_ERR)
		return EIO;

	return EOK;
}

/** Issue IDENTIFY DEVICE command.
 *
 * Reads @c identify data into the provided buffer. This is used to detect
 * whether an ATA device is present and if so, to determine its parameters.
 *
 * @param disk		Disk
 * @param buf		Pointer to a 512-byte buffer.
 *
 * @return		ETIMEOUT on timeout (this can mean the device is
 *			not present). EIO if device responds with error.
 */
static errno_t ata_identify_dev(disk_t *disk, void *buf)
{
	ata_ctrl_t *ctrl = disk->ctrl;
	uint8_t status;
	uint8_t drv_head;

	drv_head = ((disk_dev_idx(disk) != 0) ? DHR_DRV : 0);

	if (wait_status(ctrl, 0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return ETIMEOUT;

	pio_write_8(&ctrl->cmd->drive_head, drv_head);

	/*
	 * Do not wait for DRDY to be set in case this is a packet device.
	 * We determine whether the device is present by waiting for DRQ to be
	 * set after issuing the command.
	 */
	if (wait_status(ctrl, 0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return ETIMEOUT;

	pio_write_8(&ctrl->cmd->command, CMD_IDENTIFY_DRIVE);

	if (wait_status(ctrl, 0, ~SR_BSY, &status, TIMEOUT_PROBE) != EOK)
		return ETIMEOUT;

	/*
	 * If ERR is set, this may be a packet device, so return EIO to cause
	 * the caller to check for one.
	 */
	if ((status & SR_ERR) != 0)
		return EIO;

	/*
	 * For probing purposes we need to wait for some status bit to become
	 * active - otherwise we could be fooled just by receiving all zeroes.
	 */
	if (wait_status(ctrl, SR_DRQ, ~SR_BSY, &status, TIMEOUT_PROBE) != EOK)
		return ETIMEOUT;

	return ata_pio_data_in(disk, buf, identify_data_size,
	    identify_data_size, 1);
}

/** Issue Identify Packet Device command.
 *
 * Reads @c identify data into the provided buffer. This is used to detect
 * whether an ATAPI device is present and if so, to determine its parameters.
 *
 * @param disk		Disk
 * @param buf		Pointer to a 512-byte buffer.
 */
static errno_t ata_identify_pkt_dev(disk_t *disk, void *buf)
{
	ata_ctrl_t *ctrl = disk->ctrl;
	uint8_t drv_head;

	drv_head = ((disk_dev_idx(disk) != 0) ? DHR_DRV : 0);

	if (wait_status(ctrl, 0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return EIO;

	pio_write_8(&ctrl->cmd->drive_head, drv_head);

	/* For ATAPI commands we do not need to wait for DRDY. */
	if (wait_status(ctrl, 0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return EIO;

	pio_write_8(&ctrl->cmd->command, CMD_IDENTIFY_PKT_DEV);

	return ata_pio_data_in(disk, buf, identify_data_size,
	    identify_data_size, 1);
}

/** Issue packet command (i. e. write a command packet to the device).
 *
 * Only data-in commands are supported (e.g. inquiry, read).
 *
 * @param disk		Disk
 * @param obuf		Buffer for storing data read from device
 * @param obuf_size	Size of obuf in bytes
 * @param rcvd_size	Place to store number of bytes read or @c NULL
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_cmd_packet(disk_t *disk, const void *cpkt, size_t cpkt_size,
    void *obuf, size_t obuf_size, size_t *rcvd_size)
{
	ata_ctrl_t *ctrl = disk->ctrl;
	size_t i;
	uint8_t status;
	uint8_t drv_head;
	size_t data_size;
	uint16_t val;

	fibril_mutex_lock(&ctrl->lock);

	/* New value for Drive/Head register */
	drv_head =
	    ((disk_dev_idx(disk) != 0) ? DHR_DRV : 0);

	if (wait_status(ctrl, 0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK) {
		fibril_mutex_unlock(&ctrl->lock);
		return EIO;
	}

	pio_write_8(&ctrl->cmd->drive_head, drv_head);

	if (wait_status(ctrl, 0, ~(SR_BSY | SR_DRQ), NULL, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&ctrl->lock);
		return EIO;
	}

	/* Byte count <- max. number of bytes we can read in one transfer. */
	pio_write_8(&ctrl->cmd->cylinder_low, 0xfe);
	pio_write_8(&ctrl->cmd->cylinder_high, 0xff);

	pio_write_8(&ctrl->cmd->command, CMD_PACKET);

	if (wait_status(ctrl, SR_DRQ, ~SR_BSY, &status, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&ctrl->lock);
		return EIO;
	}

	/* Write command packet. */
	for (i = 0; i < (cpkt_size + 1) / 2; i++)
		pio_write_16(&ctrl->cmd->data_port, ((uint16_t *) cpkt)[i]);

	if (wait_status(ctrl, 0, ~SR_BSY, &status, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&ctrl->lock);
		return EIO;
	}

	if ((status & SR_DRQ) == 0) {
		fibril_mutex_unlock(&ctrl->lock);
		return EIO;
	}

	/* Read byte count. */
	data_size = (uint16_t) pio_read_8(&ctrl->cmd->cylinder_low) +
	    ((uint16_t) pio_read_8(&ctrl->cmd->cylinder_high) << 8);

	/* Check whether data fits into output buffer. */
	if (data_size > obuf_size) {
		/* Output buffer is too small to store data. */
		fibril_mutex_unlock(&ctrl->lock);
		return EIO;
	}

	/* Read data from the device buffer. */
	for (i = 0; i < (data_size + 1) / 2; i++) {
		val = pio_read_16(&ctrl->cmd->data_port);
		((uint16_t *) obuf)[i] = val;
	}

	fibril_mutex_unlock(&ctrl->lock);

	if (status & SR_ERR)
		return EIO;

	if (rcvd_size != NULL)
		*rcvd_size = data_size;
	return EOK;
}

/** Issue ATAPI Inquiry.
 *
 * @param disk		Disk
 * @param obuf		Buffer for storing inquiry data read from device
 * @param obuf_size	Size of obuf in bytes
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_pcmd_inquiry(disk_t *disk, void *obuf, size_t obuf_size,
    size_t *rcvd_size)
{
	uint8_t cpb[12];
	scsi_cdb_inquiry_t *cp = (scsi_cdb_inquiry_t *)cpb;
	errno_t rc;

	memset(cpb, 0, sizeof(cpb));

	/*
	 * For SFF 8020 compliance the inquiry must be padded to 12 bytes
	 * and allocation length must fit in one byte.
	 */
	cp->op_code = SCSI_CMD_INQUIRY;

	/* Allocation length */
	cp->alloc_len = host2uint16_t_be(min(obuf_size, 0xff));

	rc = ata_cmd_packet(disk, cpb, sizeof(cpb), obuf, obuf_size, rcvd_size);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Issue ATAPI read capacity(10) command.
 *
 * @param disk		Disk
 * @param nblocks	Place to store number of blocks
 * @param block_size	Place to store block size
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_pcmd_read_capacity(disk_t *disk, uint64_t *nblocks,
    size_t *block_size)
{
	scsi_cdb_read_capacity_10_t cdb;
	scsi_read_capacity_10_data_t data;
	size_t rsize;
	errno_t rc;

	memset(&cdb, 0, sizeof(cdb));
	cdb.op_code = SCSI_CMD_READ_CAPACITY_10;

	rc = ata_cmd_packet(disk, &cdb, sizeof(cdb), &data, sizeof(data), &rsize);
	if (rc != EOK)
		return rc;

	if (rsize != sizeof(data))
		return EIO;

	*nblocks = uint32_t_be2host(data.last_lba) + 1;
	*block_size = uint32_t_be2host(data.block_size);

	return EOK;
}

/** Issue ATAPI read(12) command.
 *
 * Output buffer must be large enough to hold the data, otherwise the
 * function will fail.
 *
 * @param disk		Disk
 * @param ba		Starting block address
 * @param cnt		Number of blocks to read
 * @param obuf		Buffer for storing inquiry data read from device
 * @param obuf_size	Size of obuf in bytes
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_pcmd_read_12(disk_t *disk, uint64_t ba, size_t cnt,
    void *obuf, size_t obuf_size)
{
	scsi_cdb_read_12_t cp;
	errno_t rc;

	if (ba > UINT32_MAX)
		return EINVAL;

	memset(&cp, 0, sizeof(cp));

	cp.op_code = SCSI_CMD_READ_12;
	cp.lba = host2uint32_t_be(ba);
	cp.xfer_len = host2uint32_t_be(cnt);

	rc = ata_cmd_packet(disk, &cp, sizeof(cp), obuf, obuf_size, NULL);
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
 * @param disk		Disk
 * @param session	Starting session
 * @param obuf		Buffer for storing inquiry data read from device
 * @param obuf_size	Size of obuf in bytes
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_pcmd_read_toc(disk_t *disk, uint8_t session, void *obuf,
    size_t obuf_size)
{
	uint8_t cpb[12];
	scsi_cdb_read_toc_t *cp = (scsi_cdb_read_toc_t *)cpb;
	errno_t rc;

	memset(cpb, 0, sizeof(cpb));

	cp->op_code = SCSI_CMD_READ_TOC;
	cp->msf = 0;
	cp->format = 0x01; /* 0x01 = multi-session mode */
	cp->track_sess_no = session;
	cp->alloc_len = host2uint16_t_be(obuf_size);
	cp->control = 0x40; /* 0x01 = multi-session mode (shifted to MSB) */

	rc = ata_cmd_packet(disk, cpb, sizeof(cpb), obuf, obuf_size, NULL);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Read a physical block from the device.
 *
 * @param disk		Disk
 * @param ba		Address the first block.
 * @param cnt		Number of blocks to transfer.
 * @param buf		Buffer for holding the data.
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_rcmd_read(disk_t *disk, uint64_t ba, size_t blk_cnt,
    void *buf)
{
	ata_ctrl_t *ctrl = disk->ctrl;
	uint8_t drv_head;
	block_coord_t bc;
	errno_t rc;

	/* Silence warning. */
	memset(&bc, 0, sizeof(bc));

	/* Compute block coordinates. */
	if (coord_calc(disk, ba, &bc) != EOK)
		return EINVAL;

	/* New value for Drive/Head register */
	drv_head =
	    ((disk_dev_idx(disk) != 0) ? DHR_DRV : 0) |
	    ((disk->amode != am_chs) ? DHR_LBA : 0) |
	    (bc.h & 0x0f);

	fibril_mutex_lock(&ctrl->lock);

	/* Program a Read Sectors operation. */

	if (wait_status(ctrl, 0, ~SR_BSY, NULL, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&ctrl->lock);
		return EIO;
	}

	pio_write_8(&ctrl->cmd->drive_head, drv_head);

	if (wait_status(ctrl, SR_DRDY, ~SR_BSY, NULL, TIMEOUT_DRDY) != EOK) {
		fibril_mutex_unlock(&ctrl->lock);
		return EIO;
	}

	/* Program block coordinates into the device. */
	coord_sc_program(ctrl, &bc, 1);

	pio_write_8(&ctrl->cmd->command, disk->amode == am_lba48 ?
	    CMD_READ_SECTORS_EXT : CMD_READ_SECTORS);

	rc = ata_pio_data_in(disk, buf, blk_cnt * disk->block_size,
	    disk->block_size, blk_cnt);

	fibril_mutex_unlock(&ctrl->lock);

	return rc;
}

/** Write a physical block to the device.
 *
 * @param disk		Disk
 * @param ba		Address of the first block.
 * @param cnt		Number of blocks to transfer.
 * @param buf		Buffer holding the data to write.
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_rcmd_write(disk_t *disk, uint64_t ba, size_t cnt,
    const void *buf)
{
	ata_ctrl_t *ctrl = disk->ctrl;
	uint8_t drv_head;
	block_coord_t bc;
	errno_t rc;

	/* Silence warning. */
	memset(&bc, 0, sizeof(bc));

	/* Compute block coordinates. */
	if (coord_calc(disk, ba, &bc) != EOK)
		return EINVAL;

	/* New value for Drive/Head register */
	drv_head =
	    ((disk_dev_idx(disk) != 0) ? DHR_DRV : 0) |
	    ((disk->amode != am_chs) ? DHR_LBA : 0) |
	    (bc.h & 0x0f);

	fibril_mutex_lock(&ctrl->lock);

	/* Program a Write Sectors operation. */

	if (wait_status(ctrl, 0, ~SR_BSY, NULL, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&ctrl->lock);
		return EIO;
	}

	pio_write_8(&ctrl->cmd->drive_head, drv_head);

	if (wait_status(ctrl, SR_DRDY, ~SR_BSY, NULL, TIMEOUT_DRDY) != EOK) {
		fibril_mutex_unlock(&ctrl->lock);
		return EIO;
	}

	/* Program block coordinates into the device. */
	coord_sc_program(ctrl, &bc, 1);

	pio_write_8(&ctrl->cmd->command, disk->amode == am_lba48 ?
	    CMD_WRITE_SECTORS_EXT : CMD_WRITE_SECTORS);

	rc = ata_pio_data_out(disk, buf, cnt * disk->block_size,
	    disk->block_size, cnt);

	fibril_mutex_unlock(&ctrl->lock);
	return rc;
}

/** Flush cached data to nonvolatile storage.
 *
 * @param disk		Disk
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_rcmd_flush_cache(disk_t *disk)
{
	ata_ctrl_t *ctrl = disk->ctrl;
	uint8_t drv_head;
	errno_t rc;

	/* New value for Drive/Head register */
	drv_head =
	    (disk_dev_idx(disk) != 0) ? DHR_DRV : 0;

	fibril_mutex_lock(&ctrl->lock);

	/* Program a Flush Cache operation. */

	if (wait_status(ctrl, 0, ~SR_BSY, NULL, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&ctrl->lock);
		return EIO;
	}

	pio_write_8(&ctrl->cmd->drive_head, drv_head);

	if (wait_status(ctrl, SR_DRDY, ~SR_BSY, NULL, TIMEOUT_DRDY) != EOK) {
		fibril_mutex_unlock(&ctrl->lock);
		return EIO;
	}

	pio_write_8(&ctrl->cmd->command, CMD_FLUSH_CACHE);

	rc = ata_pio_nondata(disk);

	fibril_mutex_unlock(&ctrl->lock);
	return rc;
}

/** Calculate block coordinates.
 *
 * Calculates block coordinates in the best coordinate system supported
 * by the device. These can be later programmed into the device using
 * @c coord_sc_program().
 *
 * @return EOK on success or EINVAL if block index is past end of device.
 */
static errno_t coord_calc(disk_t *d, uint64_t ba, block_coord_t *bc)
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
 *
 * @param ctrl		Controller
 * @param bc		Block coordinates
 * @param scnt		Sector count
 */
static void coord_sc_program(ata_ctrl_t *ctrl, const block_coord_t *bc,
    uint16_t scnt)
{
	ata_cmd_t *cmd = ctrl->cmd;

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
 * Example: wait_status(ctrl, SR_DRDY, ~SR_BSY, ...) waits for SR_DRDY to become
 * set and SR_BSY to become reset.
 *
 * @param ctrl		Controller
 * @param set		Combination if bits which must be all set.
 * @param n_reset	Negated combination of bits which must be all reset.
 * @param pstatus	Pointer where to store last read status or NULL.
 * @param timeout	Timeout in 10ms units.
 *
 * @return		EOK on success, EIO on timeout.
 */
static errno_t wait_status(ata_ctrl_t *ctrl, unsigned set, unsigned n_reset,
    uint8_t *pstatus, unsigned timeout)
{
	uint8_t status;
	int cnt;

	status = pio_read_8(&ctrl->cmd->status);

	/*
	 * This is crude, yet simple. First try with 1us delays
	 * (most likely the device will respond very fast). If not,
	 * start trying every 10 ms.
	 */

	cnt = 100;
	while ((status & ~n_reset) != 0 || (status & set) != set) {
		--cnt;
		if (cnt <= 0)
			break;

		status = pio_read_8(&ctrl->cmd->status);
	}

	cnt = timeout;
	while ((status & ~n_reset) != 0 || (status & set) != set) {
		async_usleep(10000);
		--cnt;
		if (cnt <= 0)
			break;

		status = pio_read_8(&ctrl->cmd->status);
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
