/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup libata
 * @{
 */

/**
 * @file
 * @brief ATA disk driver library
 *
 * This driver library implements the generic part of ATA/ATAPI. It is
 * meant to be used by a driver which implements the actual transport
 * (such as ISA, PCI).
 *
 * This driver libary supports CHS, 28-bit and 48-bit LBA addressing,
 * as well as PACKET devices. It supports PIO transfers and IRQ.
 *
 * There is no support DMA, S.M.A.R.T or removable devices.
 *
 * This driver is based on the ATA-1, ATA-2, ATA-3 and ATA/ATAPI-4 through 7
 * standards, as published by the ANSI, NCITS and INCITS standards bodies,
 * which are freely available. This driver contains no vendor-specific
 * code at this moment.
 */

#include <bd_srv.h>
#include <byteorder.h>
#include <errno.h>
#include <macros.h>
#include <scsi/mmc.h>
#include <scsi/sbc.h>
#include <scsi/spc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ata/ata.h"
#include "ata/ata_hw.h"

#define MSG_BUF_SIZE 256

/**
 * Size of data returned from Identify Device or Identify Packet Device
 * command.
 */
static const size_t identify_data_size = 512;
static void ata_msg_note(ata_channel_t *, const char *, ...);
static void ata_msg_debug(ata_channel_t *, const char *, ...);
static void ata_msg_warn(ata_channel_t *, const char *, ...);
static void ata_msg_error(ata_channel_t *, const char *, ...);
static errno_t ata_device_add(ata_device_t *);
static errno_t ata_device_remove(ata_device_t *);
static void ata_read_data_16(ata_channel_t *, uint16_t *, size_t);
static void ata_write_data_16(ata_channel_t *, uint16_t *, size_t);
static uint8_t ata_read_cmd_8(ata_channel_t *, uint16_t);
static void ata_write_cmd_8(ata_channel_t *, uint16_t, uint8_t);

static errno_t ata_bd_init_irq(ata_channel_t *);
static void ata_bd_fini_irq(ata_channel_t *);

static errno_t ata_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t ata_bd_close(bd_srv_t *);
static errno_t ata_bd_read_blocks(bd_srv_t *, uint64_t, size_t, void *, size_t);
static errno_t ata_bd_read_toc(bd_srv_t *, uint8_t, void *, size_t);
static errno_t ata_bd_write_blocks(bd_srv_t *, uint64_t, size_t, const void *,
    size_t);
static errno_t ata_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t ata_bd_get_num_blocks(bd_srv_t *, aoff64_t *);
static errno_t ata_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);

static errno_t ata_rcmd_read(ata_device_t *, uint64_t, size_t, void *);
static errno_t ata_rcmd_write(ata_device_t *, uint64_t, size_t,
    const void *);
static errno_t ata_rcmd_flush_cache(ata_device_t *);
static errno_t ata_device_init(ata_channel_t *, ata_device_t *, int);
static errno_t ata_identify_dev(ata_device_t *, void *);
static errno_t ata_identify_pkt_dev(ata_device_t *, void *);
static errno_t ata_cmd_packet(ata_device_t *, const void *, size_t, void *,
    size_t, size_t *);
static errno_t ata_pcmd_inquiry(ata_device_t *, void *, size_t, size_t *);
static errno_t ata_pcmd_read_12(ata_device_t *, uint64_t, size_t, void *, size_t);
static errno_t ata_pcmd_read_capacity(ata_device_t *, uint64_t *, size_t *);
static errno_t ata_pcmd_read_toc(ata_device_t *, uint8_t, void *, size_t);
static void disk_print_summary(ata_device_t *);
static size_t ata_disk_maxnb(ata_device_t *);
static errno_t coord_calc(ata_device_t *, uint64_t, block_coord_t *);
static void coord_sc_program(ata_channel_t *, const block_coord_t *, uint16_t);
static errno_t wait_status(ata_channel_t *, unsigned, unsigned, uint8_t *,
    unsigned);
static errno_t wait_irq(ata_channel_t *, uint8_t *, unsigned);
static void ata_dma_chan_setup(ata_device_t *, void *, size_t, ata_dma_dir_t);
static void ata_dma_chan_teardown(ata_device_t *);

static bd_ops_t ata_bd_ops = {
	.open = ata_bd_open,
	.close = ata_bd_close,
	.read_blocks = ata_bd_read_blocks,
	.read_toc = ata_bd_read_toc,
	.write_blocks = ata_bd_write_blocks,
	.get_block_size = ata_bd_get_block_size,
	.get_num_blocks = ata_bd_get_num_blocks,
	.sync_cache = ata_bd_sync_cache
};

static ata_device_t *bd_srv_device(bd_srv_t *bd)
{
	return (ata_device_t *)bd->srvs->sarg;
}

static int disk_dev_idx(ata_device_t *device)
{
	return (device->device_id & 1);
}

/** Create ATA channel.
 *
 * @param params Channel creation parameters
 * @param rchan Place to store pointer to new channel
 * @return EOK on success or an error code
 */
errno_t ata_channel_create(ata_params_t *params, ata_channel_t **rchan)
{
	ata_channel_t *chan;
	int i;

	chan = calloc(1, sizeof(ata_channel_t));
	if (chan == NULL)
		return ENOMEM;

	chan->params = *params;
	ata_msg_debug(chan, "ata_channel_create()");

	fibril_mutex_initialize(&chan->lock);
	fibril_mutex_initialize(&chan->irq_lock);
	fibril_condvar_initialize(&chan->irq_cv);

	for (i = 0; i < MAX_DEVICES; i++)
		chan->device[i].chan = chan;

	*rchan = chan;
	return EOK;
}

/** Initialize ATA channel.
 *
 * @param params Channel creation parameters
 * @param rchan Place to store pointer to new channel
 * @return EOK on success or an error code
 */
errno_t ata_channel_initialize(ata_channel_t *chan)
{
	int i;
	errno_t rc;
	int n_disks;
	bool irq_inited = false;
	bool dev_added[MAX_DEVICES];

	for (i = 0; i < MAX_DEVICES; i++)
		dev_added[i] = false;

	ata_msg_debug(chan, "ata_channel_initialize()");

	rc = ata_bd_init_irq(chan);
	if (rc != EOK)
		return rc;

	irq_inited = true;

	for (i = 0; i < MAX_DEVICES; i++) {
		ata_msg_debug(chan, "Identify drive %d...", i);

		rc = ata_device_init(chan, &chan->device[i], i);

		if (rc == EOK) {
			disk_print_summary(&chan->device[i]);
		} else {
			ata_msg_debug(chan, "Not found.");
		}
	}

	n_disks = 0;

	for (i = 0; i < MAX_DEVICES; i++) {
		/* Skip unattached devices. */
		if (chan->device[i].present == false)
			continue;

		rc = ata_device_add(&chan->device[i]);
		if (rc != EOK) {
			ata_msg_error(chan, "Unable to add device %d.", i);
			goto error;
		}
		dev_added[i] = true;
		++n_disks;
	}

	if (n_disks == 0) {
		ata_msg_warn(chan, "No devices detected.");
		rc = ENOENT;
		goto error;
	}

	return EOK;
error:
	for (i = 0; i < MAX_DEVICES; i++) {
		if (dev_added[i]) {
			rc = ata_device_remove(&chan->device[i]);
			if (rc != EOK) {
				ata_msg_error(chan,
				    "Unable to remove device %d.", i);
			}
		}
	}
	if (irq_inited)
		ata_bd_fini_irq(chan);
	return rc;
}

/** Destroy ATA channel. */
errno_t ata_channel_destroy(ata_channel_t *chan)
{
	int i;
	errno_t rc;

	ata_msg_debug(chan, ": ata_channel_destroy()");

	fibril_mutex_lock(&chan->lock);

	for (i = 0; i < MAX_DEVICES; i++) {
		rc = ata_device_remove(&chan->device[i]);
		if (rc != EOK) {
			ata_msg_error(chan, "Unable to remove device %d.", i);
			break;
		}
	}

	ata_bd_fini_irq(chan);
	fibril_mutex_unlock(&chan->lock);

	return rc;
}

/** Add ATA device.
 *
 * @param d Device
 * @return EOK on success or an error code
 */
static errno_t ata_device_add(ata_device_t *d)
{
	bd_srvs_init(&d->bds);
	d->bds.ops = &ata_bd_ops;
	d->bds.sarg = (void *)d;

	return d->chan->params.add_device(d->chan->params.arg, d->device_id,
	    (void *)d);
}

/** Remove ATA device.
 *
 * @param d Device
 * @return EOK on success or an error code
 */
static errno_t ata_device_remove(ata_device_t *d)
{
	return d->chan->params.remove_device(d->chan->params.arg, d->device_id);
}

/** Read 16 bits from data port.
 *
 * @param chan ATA channel
 * @param buf Buffer to hold data
 * @param nwords Number of words to read
 */
static void ata_read_data_16(ata_channel_t *chan, uint16_t *buf,
    size_t nwords)
{
	chan->params.read_data_16(chan->params.arg, buf, nwords);
}

/** Write 16 bits to data port.
 *
 * @param chan ATA channel
 * @param data Data
 * @param nwords Number of words to write
 */
static void ata_write_data_16(ata_channel_t *chan, uint16_t *data,
    size_t nwords)
{
	chan->params.write_data_16(chan->params.arg, data, nwords);
}

/** Read 8 bits from 8-bit command port.
 *
 * @param chan ATA channel
 * @param port Port number
 * @return 8-bit register value
 */
static uint8_t ata_read_cmd_8(ata_channel_t *chan, uint16_t port)
{
	return chan->params.read_cmd_8(chan->params.arg, port);
}

/** Write 8 bits to 8-bit command port.
 *
 * @param chan ATA channel
 * @param port Port number
 * @param value Register value
 */
static void ata_write_cmd_8(ata_channel_t *chan, uint16_t port, uint8_t value)
{
	return chan->params.write_cmd_8(chan->params.arg, port, value);
}

/** Log a notice message.
 *
 * @param chan Channel
 * @param fmt Format
 */
static void ata_msg_note(ata_channel_t *chan, const char *fmt, ...)
{
	va_list ap;
	char buf[MSG_BUF_SIZE];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	chan->params.msg_note(chan->params.arg, buf);
}

/** Log a debug message.
 *
 * @param chan Channel
 * @param fmt Format
 */
static void ata_msg_debug(ata_channel_t *chan, const char *fmt, ...)
{
	va_list ap;
	char buf[MSG_BUF_SIZE];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	chan->params.msg_debug(chan->params.arg, buf);
}

/** Log a warning message.
 *
 * @param chan Channel
 * @param fmt Format
 */
static void ata_msg_warn(ata_channel_t *chan, const char *fmt, ...)
{
	va_list ap;
	char buf[MSG_BUF_SIZE];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	chan->params.msg_warn(chan->params.arg, buf);
}

/** Log an error message.
 *
 * @param chan Channel
 * @param fmt Format
 */
static void ata_msg_error(ata_channel_t *chan, const char *fmt, ...)
{
	va_list ap;
	char buf[MSG_BUF_SIZE];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	chan->params.msg_error(chan->params.arg, buf);
}

/** Print one-line device summary. */
static void disk_print_summary(ata_device_t *d)
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

	ata_msg_note(d->chan, "%s: %s %" PRIu64 " blocks%s", d->model, atype,
	    d->blocks, cap);
cleanup:
	free(atype);
	free(cap);
}

/** Initialize IRQ. */
static errno_t ata_bd_init_irq(ata_channel_t *chan)
{
	if (!chan->params.have_irq)
		return EOK;

	return chan->params.irq_enable(chan->params.arg);
}

/** Clean up IRQ. */
static void ata_bd_fini_irq(ata_channel_t *chan)
{
	if (!chan->params.have_irq)
		return;

	(void)chan->params.irq_disable(chan->params.arg);
}

/** Initialize a device.
 *
 * Probes for a device, determines its parameters and initializes
 * the device structure.
 */
static errno_t ata_device_init(ata_channel_t *chan, ata_device_t *d,
    int device_id)
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

	d->device_id = device_id;
	d->present = false;

	/* Try identify command. */
	rc = ata_identify_dev(d, &idata);
	if (rc == EOK) {
		/* Success. It's a register (non-packet) device. */
		ata_msg_debug(chan, "ATA register-only device found.");
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
		bc = ((uint16_t)ata_read_cmd_8(chan, REG_CYLINDER_HIGH) << 8) |
		    ata_read_cmd_8(chan, REG_CYLINDER_LOW);

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
			ata_msg_error(chan, "Device inquiry failed.");
			d->present = false;
			return EIO;
		}

		/* Check device type. */
		if (INQUIRY_PDEV_TYPE(inq_data.pqual_devtype) != SCSI_DEV_CD_DVD)
			ata_msg_warn(chan, "Peripheral device type is not CD-ROM.");

		rc = ata_pcmd_read_capacity(d, &nblocks, &block_size);
		if (rc != EOK) {
			ata_msg_error(chan, "Read capacity command failed.");
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
	ata_device_t *device = bd_srv_device(bd);
	size_t maxnb;
	size_t nb;
	errno_t rc;

	if (size < cnt * device->block_size) {
		rc = EINVAL;
		goto error;
	}

	/* Maximum number of blocks to transfer at the same time */
	maxnb = ata_disk_maxnb(device);
	while (cnt > 0) {
		nb = min(maxnb, cnt);
		if (device->dev_type == ata_reg_dev) {
			rc = ata_rcmd_read(device, ba, nb, buf);
		} else {
			rc = ata_pcmd_read_12(device, ba, nb, buf,
			    device->block_size);
		}

		if (rc != EOK)
			goto error;

		ba += nb;
		cnt -= nb;
		buf += device->block_size * nb;
	}

	return EOK;
error:
	ata_msg_debug(device->chan, "ata_bd_read_blocks: rc=%d", rc);
	return rc;
}

/** Read TOC from device. */
static errno_t ata_bd_read_toc(bd_srv_t *bd, uint8_t session, void *buf, size_t size)
{
	ata_device_t *device = bd_srv_device(bd);

	return ata_pcmd_read_toc(device, session, buf, size);
}

/** Write multiple blocks to the device. */
static errno_t ata_bd_write_blocks(bd_srv_t *bd, uint64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	ata_device_t *device = bd_srv_device(bd);
	size_t maxnb;
	size_t nb;
	errno_t rc;

	if (device->dev_type != ata_reg_dev)
		return ENOTSUP;

	if (size < cnt * device->block_size)
		return EINVAL;

	/* Maximum number of blocks to transfer at the same time */
	maxnb = ata_disk_maxnb(device);
	while (cnt > 0) {
		nb = min(maxnb, cnt);
		rc = ata_rcmd_write(device, ba, nb, buf);
		if (rc != EOK)
			return rc;

		ba += nb;
		cnt -= nb;
		buf += device->block_size * nb;
	}

	return EOK;
}

/** Get device block size. */
static errno_t ata_bd_get_block_size(bd_srv_t *bd, size_t *rbsize)
{
	ata_device_t *device = bd_srv_device(bd);

	*rbsize = device->block_size;
	return EOK;
}

/** Get device number of blocks. */
static errno_t ata_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	ata_device_t *device = bd_srv_device(bd);

	*rnb = device->blocks;
	return EOK;
}

/** Flush cache. */
static errno_t ata_bd_sync_cache(bd_srv_t *bd, uint64_t ba, size_t cnt)
{
	ata_device_t *device = bd_srv_device(bd);

	/* ATA cannot flush just some blocks, we just flush everything. */
	(void)ba;
	(void)cnt;

	return ata_rcmd_flush_cache(device);
}

/** PIO data-in command protocol. */
static errno_t ata_pio_data_in(ata_device_t *device, void *obuf, size_t obuf_size,
    size_t blk_size, size_t nblocks)
{
	ata_channel_t *chan = device->chan;
	uint8_t status;
	errno_t rc;

	assert(nblocks > 0);
	assert(blk_size % 2 == 0);

	while (nblocks > 0) {
		if (chan->params.have_irq)
			rc = wait_irq(chan, &status, TIMEOUT_BSY);
		else
			rc = wait_status(chan, 0, ~SR_BSY, &status, TIMEOUT_BSY);

		if (rc != EOK) {
			ata_msg_debug(chan, "wait_irq/wait_status failed");
			return EIO;
		}

		if ((status & SR_DRQ) == 0) {
			ata_msg_debug(chan, "DRQ == 0");
			break;
		}

		/* Read data from the device buffer. */
		ata_read_data_16(chan, (uint16_t *)obuf, blk_size / 2);
		obuf += blk_size;

		--nblocks;
	}

	if ((status & SR_ERR) != 0) {
		ata_msg_debug(chan, "status & SR_ERR != 0");
		return EIO;
	}
	if (nblocks > 0) {
		ata_msg_debug(chan, "remaining nblocks = %zu", nblocks);
		return EIO;
	}

	return EOK;
}

/** PIO data-out command protocol. */
static errno_t ata_pio_data_out(ata_device_t *device, const void *buf, size_t buf_size,
    size_t blk_size, size_t nblocks)
{
	ata_channel_t *chan = device->chan;
	uint8_t status;
	errno_t rc;

	assert(nblocks > 0);
	assert(blk_size % 2 == 0);

	rc = wait_status(chan, 0, ~SR_BSY, &status, TIMEOUT_BSY);
	if (rc != EOK)
		return EIO;

	while (nblocks > 0) {
		if ((status & SR_DRQ) == 0) {
			ata_msg_debug(chan, "pio_data_out: unexpected DRQ=0");
			break;
		}

		/* Write data to the device buffer. */
		ata_write_data_16(chan, (uint16_t *)buf, blk_size / 2);
		buf += blk_size;

		if (chan->params.have_irq)
			rc = wait_irq(chan, &status, TIMEOUT_BSY);
		else
			rc = wait_status(chan, 0, ~SR_BSY, &status, TIMEOUT_BSY);
		if (rc != EOK)
			return EIO;

		--nblocks;
	}

	if (status & SR_ERR)
		return EIO;
	if (nblocks > 0)
		return EIO;

	return EOK;
}

/** PIO non-data command protocol. */
static errno_t ata_pio_nondata(ata_device_t *device)
{
	ata_channel_t *chan = device->chan;
	uint8_t status;
	errno_t rc;

	if (chan->params.have_irq)
		rc = wait_irq(chan, &status, TIMEOUT_BSY);
	else
		rc = wait_status(chan, 0, ~SR_BSY, &status, TIMEOUT_BSY);

	if (rc != EOK)
		return EIO;

	if (status & SR_ERR)
		return EIO;

	return EOK;
}

/** DMA command protocol.
 *
 * @param device ATA device
 * @param cmd Command code
 * @param buf Data buffer
 * @param buf_size Data buffer size in bytes
 * @param dir DMA direction
 *
 * @return EOK on success or an error code
 */
static errno_t ata_dma_proto(ata_device_t *device, uint8_t cmd, void *buf,
    size_t buf_size, ata_dma_dir_t dir)
{
	ata_channel_t *chan = device->chan;
	uint8_t status;
	errno_t rc;

	/* Set up DMA channel */
	ata_dma_chan_setup(device, buf, buf_size, dir);

	ata_write_cmd_8(chan, REG_COMMAND, cmd);

	if (chan->params.have_irq)
		rc = wait_irq(chan, &status, TIMEOUT_BSY);
	else
		rc = wait_status(chan, 0, ~SR_BSY, &status, TIMEOUT_BSY);

	/* Tear down DMA channel */
	ata_dma_chan_teardown(device);

	if (rc != EOK) {
		ata_msg_debug(chan, "wait_irq/wait_status failed");
		return EIO;
	}

	if ((status & SR_ERR) != 0) {
		ata_msg_debug(chan, "status & SR_ERR != 0");
		return EIO;
	}

	return EOK;
}

/** Issue IDENTIFY DEVICE command.
 *
 * Reads @c identify data into the provided buffer. This is used to detect
 * whether an ATA device is present and if so, to determine its parameters.
 *
 * @param device	Device
 * @param buf		Pointer to a 512-byte buffer.
 *
 * @return		ETIMEOUT on timeout (this can mean the device is
 *			not present). EIO if device responds with error.
 */
static errno_t ata_identify_dev(ata_device_t *device, void *buf)
{
	ata_channel_t *chan = device->chan;
	uint8_t status;
	uint8_t drv_head;

	drv_head = ((disk_dev_idx(device) != 0) ? DHR_DRV : 0);

	if (wait_status(chan, 0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return ETIMEOUT;

	ata_write_cmd_8(chan, REG_DRIVE_HEAD, drv_head);

	/*
	 * Do not wait for DRDY to be set in case this is a packet device.
	 * We determine whether the device is present by waiting for DRQ to be
	 * set after issuing the command.
	 */
	if (wait_status(chan, 0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return ETIMEOUT;

	ata_write_cmd_8(chan, REG_COMMAND, CMD_IDENTIFY_DRIVE);

	/*
	 * For probing purposes we need to wait for some status bit to become
	 * active - otherwise we could be fooled just by receiving all zeroes.
	 */
	if (wait_status(chan, SR_DRQ, ~SR_BSY, &status, TIMEOUT_PROBE) != EOK) {
		if ((status & SR_ERR) == 0) {
			/* Probably no device at all */
			return ETIMEOUT;
		}
	}

	return ata_pio_data_in(device, buf, identify_data_size,
	    identify_data_size, 1);
}

/** Issue Identify Packet Device command.
 *
 * Reads @c identify data into the provided buffer. This is used to detect
 * whether an ATAPI device is present and if so, to determine its parameters.
 *
 * @param device	Device
 * @param buf		Pointer to a 512-byte buffer.
 */
static errno_t ata_identify_pkt_dev(ata_device_t *device, void *buf)
{
	ata_channel_t *chan = device->chan;
	uint8_t drv_head;

	drv_head = ((disk_dev_idx(device) != 0) ? DHR_DRV : 0);

	if (wait_status(chan, 0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return EIO;

	ata_write_cmd_8(chan, REG_DRIVE_HEAD, drv_head);

	/* For ATAPI commands we do not need to wait for DRDY. */
	if (wait_status(chan, 0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK)
		return EIO;

	ata_write_cmd_8(chan, REG_COMMAND, CMD_IDENTIFY_PKT_DEV);

	return ata_pio_data_in(device, buf, identify_data_size,
	    identify_data_size, 1);
}

/** Read data using PIO during a PACKET command.
 *
 * @param device Device
 * @param obuf Output buffer
 * @param obuf_size Output buffer size
 * @param rcvd_size Place to store number of bytes actually transferred
 *                  or @c NULL
 */
static errno_t ata_packet_pio_data_in(ata_device_t *device, void *obuf,
    size_t obuf_size, size_t *rcvd_size)
{
	ata_channel_t *chan = device->chan;
	size_t data_size;
	size_t remain;
	uint8_t status;
	errno_t rc;

	remain = obuf_size;
	while (remain > 0) {
		if (chan->params.have_irq)
			rc = wait_irq(chan, &status, TIMEOUT_BSY);
		else
			rc = wait_status(chan, 0, ~SR_BSY, &status, TIMEOUT_BSY);

		if (rc != EOK) {
			fibril_mutex_unlock(&chan->lock);
			return EIO;
		}

		if ((status & SR_DRQ) == 0)
			break;

		/* Read byte count. */
		data_size = (uint16_t) ata_read_cmd_8(chan, REG_CYLINDER_LOW) +
		    ((uint16_t) ata_read_cmd_8(chan, REG_CYLINDER_HIGH) << 8);

		/* Check whether data fits into output buffer. */
		if (data_size > obuf_size) {
			/* Output buffer is too small to store data. */
			fibril_mutex_unlock(&chan->lock);
			return EIO;
		}

		/* Read data from the device buffer. */
		ata_read_data_16(chan, obuf, (data_size + 1) / 2);
		obuf += data_size;

		remain -= data_size;
	}

	if (chan->params.have_irq)
		rc = wait_irq(chan, &status, TIMEOUT_BSY);
	else
		rc = wait_status(chan, 0, ~SR_BSY, &status, TIMEOUT_BSY);

	if (status & SR_ERR)
		return EIO;

	if (rcvd_size != NULL)
		*rcvd_size = obuf_size - remain;

	return EOK;
}

/** Transfer data using DMA during PACKET command.
 *
 * @param device Device
 * @param buf Buffer
 * @param buf_size Buffer size
 * @param dir DMA direction
 *
 * @return EOK on success or an error code
 */
static errno_t ata_packet_dma(ata_device_t *device, void *buf, size_t buf_size,
    ata_dma_dir_t dir)
{
	ata_channel_t *chan = device->chan;
	uint8_t status;
	errno_t rc;

	if (chan->params.have_irq)
		rc = wait_irq(chan, &status, TIMEOUT_BSY);
	else
		rc = wait_status(chan, 0, ~SR_BSY, &status, TIMEOUT_BSY);

	if (rc != EOK) {
		ata_msg_debug(chan, "wait_irq/wait_status failed");
		return EIO;
	}

	if ((status & SR_ERR) != 0) {
		ata_msg_debug(chan, "status & SR_ERR != 0");
		return EIO;
	}

	return EOK;
}

/** Issue packet command (i. e. write a command packet to the device).
 *
 * Only data-in commands are supported (e.g. inquiry, read).
 *
 * @param device	Device
 * @param obuf		Buffer for storing data read from device
 * @param obuf_size	Size of obuf in bytes
 * @param rcvd_size	Place to store number of bytes read or @c NULL
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_cmd_packet(ata_device_t *device, const void *cpkt, size_t cpkt_size,
    void *obuf, size_t obuf_size, size_t *rcvd_size)
{
	ata_channel_t *chan = device->chan;
	uint8_t status;
	uint8_t drv_head;
	errno_t rc;

	fibril_mutex_lock(&chan->lock);

	/* New value for Drive/Head register */
	drv_head =
	    ((disk_dev_idx(device) != 0) ? DHR_DRV : 0);

	if (wait_status(chan, 0, ~SR_BSY, NULL, TIMEOUT_PROBE) != EOK) {
		fibril_mutex_unlock(&chan->lock);
		return EIO;
	}

	ata_write_cmd_8(chan, REG_DRIVE_HEAD, drv_head);

	if (wait_status(chan, 0, ~(SR_BSY | SR_DRQ), NULL, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&chan->lock);
		return EIO;
	}

	if (chan->params.use_dma) {
		/* Set up DMA channel */
		ata_dma_chan_setup(device, obuf, obuf_size, ata_dma_read);
		ata_write_cmd_8(chan, REG_FEATURES, 0x01); // XXX
	} else {
		/*
		 * Byte count <- max. number of bytes we can read in one
		 * PIO transfer.
		 */
		ata_write_cmd_8(chan, REG_CYLINDER_LOW, 0xfe);
		ata_write_cmd_8(chan, REG_CYLINDER_HIGH, 0xff);
	}

	ata_write_cmd_8(chan, REG_COMMAND, CMD_PACKET);

	if (wait_status(chan, SR_DRQ, ~SR_BSY, &status, TIMEOUT_BSY) != EOK) {
		if (chan->params.use_dma)
			ata_dma_chan_teardown(device);
		fibril_mutex_unlock(&chan->lock);
		return EIO;
	}

	/* Write command packet. */
	ata_write_data_16(chan, ((uint16_t *) cpkt), (cpkt_size + 1) / 2);

	if (chan->params.use_dma) {
		/* Read data using DMA */
		rc = ata_packet_dma(device, obuf, obuf_size, ata_dma_read);
		if (rc == EOK && rcvd_size != NULL)
			*rcvd_size = obuf_size;
		ata_dma_chan_teardown(device);
	} else {
		/* Read data using PIO */
		rc = ata_packet_pio_data_in(device, obuf, obuf_size, rcvd_size);
	}

	fibril_mutex_unlock(&chan->lock);

	if (rc != EOK)
		return rc;

	return EOK;
}

/** Issue ATAPI Inquiry.
 *
 * @param device	Device
 * @param obuf		Buffer for storing inquiry data read from device
 * @param obuf_size	Size of obuf in bytes
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_pcmd_inquiry(ata_device_t *device, void *obuf, size_t obuf_size,
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

	rc = ata_cmd_packet(device, cpb, sizeof(cpb), obuf, obuf_size, rcvd_size);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Issue ATAPI read capacity(10) command.
 *
 * @param device	Device
 * @param nblocks	Place to store number of blocks
 * @param block_size	Place to store block size
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_pcmd_read_capacity(ata_device_t *device, uint64_t *nblocks,
    size_t *block_size)
{
	scsi_cdb_read_capacity_10_t cdb;
	scsi_read_capacity_10_data_t data;
	size_t rsize;
	errno_t rc;

	memset(&cdb, 0, sizeof(cdb));
	cdb.op_code = SCSI_CMD_READ_CAPACITY_10;

	rc = ata_cmd_packet(device, &cdb, sizeof(cdb), &data, sizeof(data), &rsize);
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
 * @param device	Device
 * @param ba		Starting block address
 * @param cnt		Number of blocks to read
 * @param obuf		Buffer for storing inquiry data read from device
 * @param obuf_size	Size of obuf in bytes
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_pcmd_read_12(ata_device_t *device, uint64_t ba, size_t cnt,
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

	rc = ata_cmd_packet(device, &cp, sizeof(cp), obuf, obuf_size, NULL);
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
 * @param device	Device
 * @param session	Starting session
 * @param obuf		Buffer for storing inquiry data read from device
 * @param obuf_size	Size of obuf in bytes
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_pcmd_read_toc(ata_device_t *device, uint8_t session,
    void *obuf, size_t obuf_size)
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

	rc = ata_cmd_packet(device, cpb, sizeof(cpb), obuf, obuf_size, NULL);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Read a physical block from the device.
 *
 * @param device	Device
 * @param ba		Address the first block.
 * @param cnt		Number of blocks to transfer.
 * @param buf		Buffer for holding the data.
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_rcmd_read(ata_device_t *device, uint64_t ba, size_t blk_cnt,
    void *buf)
{
	ata_channel_t *chan = device->chan;
	uint8_t drv_head;
	block_coord_t bc;
	uint8_t cmd;
	errno_t rc;

	/* Silence warning. */
	memset(&bc, 0, sizeof(bc));

	/* Compute block coordinates. */
	if (coord_calc(device, ba, &bc) != EOK) {
		ata_msg_note(chan, "ata_rcmd_read() -> coord_calc failed");
		return EINVAL;
	}

	/* New value for Drive/Head register */
	drv_head =
	    ((disk_dev_idx(device) != 0) ? DHR_DRV : 0) |
	    ((device->amode != am_chs) ? DHR_LBA : 0) |
	    (bc.h & 0x0f);

	fibril_mutex_lock(&chan->lock);

	/* Program a Read Sectors operation. */

	if (wait_status(chan, 0, ~SR_BSY, NULL, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&chan->lock);
		ata_msg_note(chan, "ata_rcmd_read() -> wait_status failed");
		return EIO;
	}

	ata_write_cmd_8(chan, REG_DRIVE_HEAD, drv_head);

	if (wait_status(chan, SR_DRDY, ~SR_BSY, NULL, TIMEOUT_DRDY) != EOK) {
		fibril_mutex_unlock(&chan->lock);
		ata_msg_note(chan, "ata_rcmd_read() -> wait_status 2 failed");
		return EIO;
	}

	/* Program block coordinates into the device. */
	coord_sc_program(chan, &bc, blk_cnt);

	if (chan->params.use_dma) {
		cmd = (device->amode == am_lba48) ? CMD_READ_DMA_EXT :
		    CMD_READ_DMA;

		rc = ata_dma_proto(device, cmd, buf,
		    blk_cnt * device->block_size, ata_dma_read);
		if (rc != EOK) {
			ata_msg_note(chan, "ata_rcmd_read() -> dma_proto->%d",
			    rc);
		}
	} else {
		ata_write_cmd_8(chan, REG_COMMAND, device->amode == am_lba48 ?
		    CMD_READ_SECTORS_EXT : CMD_READ_SECTORS);

		rc = ata_pio_data_in(device, buf, blk_cnt * device->block_size,
		    device->block_size, blk_cnt);
		if (rc != EOK) {
			ata_msg_note(chan, "ata_rcmd_read() -> pio_data_in->%d",
			    rc);
		}
	}

	fibril_mutex_unlock(&chan->lock);

	return rc;
}

/** Write a physical block to the device.
 *
 * @param device	Device
 * @param ba		Address of the first block.
 * @param cnt		Number of blocks to transfer.
 * @param buf		Buffer holding the data to write.
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_rcmd_write(ata_device_t *device, uint64_t ba, size_t cnt,
    const void *buf)
{
	ata_channel_t *chan = device->chan;
	uint8_t drv_head;
	block_coord_t bc;
	uint8_t cmd;
	errno_t rc;

	/* Silence warning. */
	memset(&bc, 0, sizeof(bc));

	/* Compute block coordinates. */
	if (coord_calc(device, ba, &bc) != EOK)
		return EINVAL;

	/* New value for Drive/Head register */
	drv_head =
	    ((disk_dev_idx(device) != 0) ? DHR_DRV : 0) |
	    ((device->amode != am_chs) ? DHR_LBA : 0) |
	    (bc.h & 0x0f);

	fibril_mutex_lock(&chan->lock);

	/* Program a Write Sectors operation. */

	if (wait_status(chan, 0, ~SR_BSY, NULL, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&chan->lock);
		return EIO;
	}

	ata_write_cmd_8(chan, REG_DRIVE_HEAD, drv_head);

	if (wait_status(chan, SR_DRDY, ~SR_BSY, NULL, TIMEOUT_DRDY) != EOK) {
		fibril_mutex_unlock(&chan->lock);
		return EIO;
	}

	/* Program block coordinates into the device. */
	coord_sc_program(chan, &bc, cnt);

	if (chan->params.use_dma) {
		cmd = (device->amode == am_lba48) ? CMD_WRITE_DMA_EXT :
		    CMD_WRITE_DMA;

		rc = ata_dma_proto(device, cmd, (void *)buf,
		    cnt * device->block_size, ata_dma_write);
		if (rc != EOK) {
			ata_msg_note(chan, "ata_rcmd_write() -> dma_proto->%d",
			    rc);
		}
	} else {
		ata_write_cmd_8(chan, REG_COMMAND, device->amode == am_lba48 ?
		    CMD_WRITE_SECTORS_EXT : CMD_WRITE_SECTORS);

		rc = ata_pio_data_out(device, buf, cnt * device->block_size,
		    device->block_size, cnt);
		if (rc != EOK) {
			ata_msg_note(chan,
			    "ata_rcmd_read() -> pio_data_out->%d", rc);
		}
	}

	fibril_mutex_unlock(&chan->lock);
	return rc;
}

/** Flush cached data to nonvolatile storage.
 *
 * @param device	Device
 *
 * @return EOK on success, EIO on error.
 */
static errno_t ata_rcmd_flush_cache(ata_device_t *device)
{
	ata_channel_t *chan = device->chan;
	uint8_t drv_head;
	errno_t rc;

	/* New value for Drive/Head register */
	drv_head =
	    (disk_dev_idx(device) != 0) ? DHR_DRV : 0;

	fibril_mutex_lock(&chan->lock);

	/* Program a Flush Cache operation. */

	if (wait_status(chan, 0, ~SR_BSY, NULL, TIMEOUT_BSY) != EOK) {
		fibril_mutex_unlock(&chan->lock);
		return EIO;
	}

	ata_write_cmd_8(chan, REG_DRIVE_HEAD, drv_head);

	if (wait_status(chan, SR_DRDY, ~SR_BSY, NULL, TIMEOUT_DRDY) != EOK) {
		fibril_mutex_unlock(&chan->lock);
		return EIO;
	}

	ata_write_cmd_8(chan, REG_COMMAND, CMD_FLUSH_CACHE);

	rc = ata_pio_nondata(device);

	fibril_mutex_unlock(&chan->lock);
	return rc;
}

/** Get the maximum number of blocks to be transferred in one I/O
 *
 * @param d Device
 * @return Maximum number of blocks
 */
static size_t ata_disk_maxnb(ata_device_t *d)
{
	size_t maxnb;
	size_t dma_maxnb;

	maxnb = 0;

	if (d->dev_type == ata_pkt_dev) {
		/* Could be more depending on SCSI command support */
		maxnb = 0x100;
	} else {
		switch (d->amode) {
		case am_chs:
		case am_lba28:
			maxnb = 0x100;
			break;
		case am_lba48:
			maxnb = 0x10000;
			break;
		}
	}

	/*
	 * If using DMA, this needs to be further restricted not to
	 * exceed DMA buffer size.
	 */
	dma_maxnb = d->chan->params.max_dma_xfer / d->block_size;
	if (dma_maxnb < maxnb)
		maxnb = dma_maxnb;

	return maxnb;
}

/** Calculate block coordinates.
 *
 * Calculates block coordinates in the best coordinate system supported
 * by the device. These can be later programmed into the device using
 * @c coord_sc_program().
 *
 * @return EOK on success or EINVAL if block index is past end of device.
 */
static errno_t coord_calc(ata_device_t *d, uint64_t ba, block_coord_t *bc)
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
 * @param chan		Channel
 * @param bc		Block coordinates
 * @param scnt		Sector count
 */
static void coord_sc_program(ata_channel_t *chan, const block_coord_t *bc,
    uint16_t scnt)
{
	if (bc->amode == am_lba48) {
		/* Write high-order bits. */
		ata_write_cmd_8(chan, REG_SECTOR_COUNT, scnt >> 8);
		ata_write_cmd_8(chan, REG_SECTOR_NUMBER, bc->c3);
		ata_write_cmd_8(chan, REG_CYLINDER_LOW, bc->c4);
		ata_write_cmd_8(chan, REG_CYLINDER_HIGH, bc->c5);
	}

	/* Write low-order bits. */
	ata_write_cmd_8(chan, REG_SECTOR_COUNT, scnt & 0x00ff);
	ata_write_cmd_8(chan, REG_SECTOR_NUMBER, bc->c0);
	ata_write_cmd_8(chan, REG_CYLINDER_LOW, bc->c1);
	ata_write_cmd_8(chan, REG_CYLINDER_HIGH, bc->c2);
}

/** Wait until some status bits are set and some are reset.
 *
 * Example: wait_status(chan, SR_DRDY, ~SR_BSY, ...) waits for SR_DRDY to become
 * set and SR_BSY to become reset.
 *
 * @param chan		Channel
 * @param set		Combination if bits which must be all set.
 * @param n_reset	Negated combination of bits which must be all reset.
 * @param pstatus	Pointer where to store last read status or NULL.
 * @param timeout	Timeout in 10ms units.
 *
 * @return		EOK on success, EIO on timeout.
 */
static errno_t wait_status(ata_channel_t *chan, unsigned set, unsigned n_reset,
    uint8_t *pstatus, unsigned timeout)
{
	uint8_t status;
	int cnt;

	status = ata_read_cmd_8(chan, REG_STATUS);

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

		status = ata_read_cmd_8(chan, REG_STATUS);
	}

	cnt = timeout;
	while ((status & ~n_reset) != 0 || (status & set) != set) {
		fibril_usleep(10000);
		--cnt;
		if (cnt <= 0)
			break;

		status = ata_read_cmd_8(chan, REG_STATUS);
	}

	if (pstatus)
		*pstatus = status;

	if (cnt == 0)
		return EIO;

	return EOK;
}

/** Wait for IRQ and return status.
 *
 * @param chan		Channel
 * @param pstatus	Pointer where to store last read status or NULL.
 * @param timeout	Timeout in 10ms units.
 *
 * @return		EOK on success, EIO on timeout.
 */
static errno_t wait_irq(ata_channel_t *chan, uint8_t *pstatus, unsigned timeout)
{
	fibril_mutex_lock(&chan->irq_lock);
	while (!chan->irq_fired)
		fibril_condvar_wait(&chan->irq_cv, &chan->irq_lock);

	chan->irq_fired = false;
	*pstatus = chan->irq_status;
	fibril_mutex_unlock(&chan->irq_lock);
	return EOK;
}

/** Set up DMA channel.
 *
 * @param device ATA device
 * @param buf Data buffer
 * @param buf_size Data buffer size in bytes
 * @param dir DMA direction
 */
static void ata_dma_chan_setup(ata_device_t *device, void *buf,
    size_t buf_size, ata_dma_dir_t dir)
{
	device->chan->params.dma_chan_setup(device->chan->params.arg,
	    buf, buf_size, dir);
}

/** Tear down DMA channel.
 *
 * @param device ATA device
 */
static void ata_dma_chan_teardown(ata_device_t *device)
{
	device->chan->params.dma_chan_teardown(device->chan->params.arg);
}

/** Interrupt handler.
 *
 * @param chan ATA channel
 * @param status Status read by interrupt handler
 */
void ata_channel_irq(ata_channel_t *chan, uint8_t status)
{
	fibril_mutex_lock(&chan->irq_lock);
	chan->irq_fired = true;
	chan->irq_status = status;
	fibril_mutex_unlock(&chan->irq_lock);
	fibril_condvar_broadcast(&chan->irq_cv);
}

/** Block device connection handler */
void ata_connection(ipc_call_t *icall, ata_device_t *device)
{
	bd_conn(icall, &device->bds);
}

/**
 * @}
 */
