/*
 * Copyright (c) 2012 Petr Jerman
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

/** @file
 * AHCI SATA driver implementation.
 */

#include <as.h>
#include <errno.h>
#include <stdio.h>
#include <ddf/interrupt.h>
#include <ddf/log.h>
#include <device/hw_res.h>
#include <device/hw_res_parsed.h>
#include <pci_dev_iface.h>
#include <ahci_iface.h>
#include "ahci.h"
#include "ahci_hw.h"
#include "ahci_sata.h"

#define NAME  "ahci"

#define LO(ptr) \
	((uint32_t) (((uint64_t) ((uintptr_t) (ptr))) & 0xffffffff))

#define HI(ptr) \
	((uint32_t) (((uint64_t) ((uintptr_t) (ptr))) >> 32))

/** Interrupt pseudocode for a single port
 *
 * The interrupt handling works as follows:
 *
 * 1. Port interrupt status register is read
 *    (stored as arg2).
 * 2. If port interrupt is indicated, then:
 *    3. Port interrupt status register is cleared.
 *    4. Global interrupt status register is read
 *       and cleared (any potential interrupts from
 *       other ports are reasserted automatically).
 *    5. Port number is stored as arg1.
 *    6. The interrupt is accepted.
 *
 */
#define AHCI_PORT_CMDS(port) \
	{ \
		/* Read port interrupt status register */ \
		.cmd = CMD_PIO_READ_32, \
		.addr = NULL, \
		.dstarg = 2 \
	}, \
	{ \
		/* Check if port asserted interrupt */ \
		.cmd = CMD_PREDICATE, \
		.value = 5, \
		.srcarg = 2, \
	}, \
	{ \
		/* Clear port interrupt status register */ \
		.cmd = CMD_PIO_WRITE_A_32, \
		.addr = NULL, \
		.srcarg = 2 \
	}, \
	{ \
		/* Read global interrupt status register */ \
		.cmd = CMD_PIO_READ_32, \
		.addr = NULL, \
		.dstarg = 0 \
	}, \
	{ \
		/* Clear global interrupt status register */ \
		.cmd = CMD_PIO_WRITE_A_32, \
		.addr = NULL, \
		.srcarg = 0 \
	}, \
	{ \
		/* Indicate port interrupt assertion */ \
		.cmd = CMD_LOAD, \
		.value = (port), \
		.dstarg = 1 \
	}, \
	{ \
		/* Accept the interrupt */ \
		.cmd = CMD_ACCEPT \
	}

static errno_t get_sata_device_name(ddf_fun_t *, size_t, char *);
static errno_t get_num_blocks(ddf_fun_t *, uint64_t *);
static errno_t get_block_size(ddf_fun_t *, size_t *);
static errno_t read_blocks(ddf_fun_t *, uint64_t, size_t, void *);
static errno_t write_blocks(ddf_fun_t *, uint64_t, size_t, void *);

static errno_t ahci_identify_device(sata_dev_t *);
static errno_t ahci_set_highest_ultra_dma_mode(sata_dev_t *);
static errno_t ahci_rb_fpdma(sata_dev_t *, uintptr_t, uint64_t);
static errno_t ahci_wb_fpdma(sata_dev_t *, uintptr_t, uint64_t);

static void ahci_sata_devices_create(ahci_dev_t *, ddf_dev_t *);
static ahci_dev_t *ahci_ahci_create(ddf_dev_t *);
static void ahci_ahci_hw_start(ahci_dev_t *);

static errno_t ahci_dev_add(ddf_dev_t *);

static void ahci_get_model_name(uint16_t *, char *);

static fibril_mutex_t sata_devices_count_lock;
static int sata_devices_count = 0;

/*----------------------------------------------------------------------------*/
/*-- AHCI Interface ----------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

static ahci_iface_t ahci_interface = {
	.get_sata_device_name = &get_sata_device_name,
	.get_num_blocks = &get_num_blocks,
	.get_block_size = &get_block_size,
	.read_blocks = &read_blocks,
	.write_blocks = &write_blocks
};

static ddf_dev_ops_t ahci_ops = {
	.interfaces[AHCI_DEV_IFACE] = &ahci_interface
};

static driver_ops_t driver_ops = {
	.dev_add = &ahci_dev_add
};

static driver_t ahci_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

/** Get SATA structure from DDF function. */
static sata_dev_t *fun_sata_dev(ddf_fun_t *fun)
{
	return ddf_fun_data_get(fun);
}

/** Get AHCI structure from DDF device. */
static ahci_dev_t *dev_ahci_dev(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}

/** Get SATA device name.
 *
 * @param fun                  Device function handling the call.
 * @param sata_dev_name_length Length of the sata_dev_name buffer.
 * @param sata_dev_name        Buffer for SATA device name.
 *
 * @return EOK.
 *
 */
static errno_t get_sata_device_name(ddf_fun_t *fun,
    size_t sata_dev_name_length, char *sata_dev_name)
{
	sata_dev_t *sata = fun_sata_dev(fun);
	str_cpy(sata_dev_name, sata_dev_name_length, sata->model);
	return EOK;
}

/** Get Number of blocks in SATA device.
 *
 * @param fun    Device function handling the call.
 * @param blocks Return number of blocks in SATA device.
 *
 * @return EOK.
 *
 */
static errno_t get_num_blocks(ddf_fun_t *fun, uint64_t *num_blocks)
{
	sata_dev_t *sata = fun_sata_dev(fun);
	*num_blocks = sata->blocks;
	return EOK;
}

/** Get SATA device block size.
 *
 * @param fun        Device function handling the call.
 * @param block_size Return block size.
 *
 * @return EOK.
 *
 */
static errno_t get_block_size(ddf_fun_t *fun, size_t *block_size)
{
	sata_dev_t *sata = fun_sata_dev(fun);
	*block_size = sata->block_size;
	return EOK;
}

/** Read data blocks into SATA device.
 *
 * @param fun      Device function handling the call.
 * @param blocknum Number of first block.
 * @param count    Number of blocks to read.
 * @param buf      Buffer for data.
 *
 * @return EOK if succeed, error code otherwise
 *
 */
static errno_t read_blocks(ddf_fun_t *fun, uint64_t blocknum,
    size_t count, void *buf)
{
	sata_dev_t *sata = fun_sata_dev(fun);

	uintptr_t phys;
	void *ibuf = AS_AREA_ANY;
	errno_t rc = dmamem_map_anonymous(sata->block_size, DMAMEM_4GiB,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &phys, &ibuf);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot allocate read buffer.");
		return rc;
	}

	memset(buf, 0, sata->block_size);

	fibril_mutex_lock(&sata->lock);

	for (size_t cur = 0; cur < count; cur++) {
		rc = ahci_rb_fpdma(sata, phys, blocknum + cur);
		if (rc != EOK)
			break;

		memcpy((void *) (((uint8_t *) buf) + (sata->block_size * cur)),
		    ibuf, sata->block_size);
	}

	fibril_mutex_unlock(&sata->lock);
	dmamem_unmap_anonymous(ibuf);

	return rc;
}

/** Write data blocks into SATA device.
 *
 * @param fun      Device function handling the call.
 * @param blocknum Number of first block.
 * @param count    Number of blocks to write.
 * @param buf      Buffer with data.
 *
 * @return EOK if succeed, error code otherwise
 *
 */
static errno_t write_blocks(ddf_fun_t *fun, uint64_t blocknum,
    size_t count, void *buf)
{
	sata_dev_t *sata = fun_sata_dev(fun);

	uintptr_t phys;
	void *ibuf = AS_AREA_ANY;
	errno_t rc = dmamem_map_anonymous(sata->block_size, DMAMEM_4GiB,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &phys, &ibuf);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot allocate write buffer.");
		return rc;
	}

	fibril_mutex_lock(&sata->lock);

	for (size_t cur = 0; cur < count; cur++) {
		memcpy(ibuf, (void *) (((uint8_t *) buf) + (sata->block_size * cur)),
		    sata->block_size);
		rc = ahci_wb_fpdma(sata, phys, blocknum + cur);
		if (rc != EOK)
			break;
	}

	fibril_mutex_unlock(&sata->lock);
	dmamem_unmap_anonymous(ibuf);

	return rc;
}

/*----------------------------------------------------------------------------*/
/*-- AHCI Commands -----------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/** Wait for interrupt event.
 *
 * @param sata SATA device structure.
 *
 * @return Value of interrupt state register.
 *
 */
static ahci_port_is_t ahci_wait_event(sata_dev_t *sata)
{
	fibril_mutex_lock(&sata->event_lock);

	sata->event_pxis = 0;
	while (sata->event_pxis == 0)
		fibril_condvar_wait(&sata->event_condvar, &sata->event_lock);

	ahci_port_is_t pxis = sata->event_pxis;

	if (ahci_port_is_permanent_error(pxis))
		sata->is_invalid_device = true;

	fibril_mutex_unlock(&sata->event_lock);

	return pxis;
}

/** Set AHCI registers for identifying SATA device.
 *
 * @param sata SATA device structure.
 * @param phys Physical address of working buffer.
 *
 */
static void ahci_identify_device_cmd(sata_dev_t *sata, uintptr_t phys)
{
	volatile sata_std_command_frame_t *cmd =
	    (sata_std_command_frame_t *) sata->cmd_table;

	cmd->fis_type = SATA_CMD_FIS_TYPE;
	cmd->c = SATA_CMD_FIS_COMMAND_INDICATOR;
	cmd->command = 0xec;
	cmd->features = 0;
	cmd->lba_lower = 0;
	cmd->device = 0;
	cmd->lba_upper = 0;
	cmd->features_upper = 0;
	cmd->count = 0;
	cmd->reserved1 = 0;
	cmd->control = 0;
	cmd->reserved2 = 0;

	volatile ahci_cmd_prdt_t *prdt =
	    (ahci_cmd_prdt_t *) (&sata->cmd_table[0x20]);

	prdt->data_address_low = LO(phys);
	prdt->data_address_upper = HI(phys);
	prdt->reserved1 = 0;
	prdt->dbc = SATA_IDENTIFY_DEVICE_BUFFER_LENGTH - 1;
	prdt->reserved2 = 0;
	prdt->ioc = 0;

	sata->cmd_header->prdtl = 1;
	sata->cmd_header->flags =
	    AHCI_CMDHDR_FLAGS_CLEAR_BUSY_UPON_OK |
	    AHCI_CMDHDR_FLAGS_2DWCMD;
	sata->cmd_header->bytesprocessed = 0;

	/* Run command. */
	sata->port->pxsact |= 1;
	sata->port->pxci |= 1;
}

/** Set AHCI registers for identifying packet SATA device.
 *
 * @param sata SATA device structure.
 * @param phys Physical address of working buffer.
 *
 */
static void ahci_identify_packet_device_cmd(sata_dev_t *sata, uintptr_t phys)
{
	volatile sata_std_command_frame_t *cmd =
	    (sata_std_command_frame_t *) sata->cmd_table;

	cmd->fis_type = SATA_CMD_FIS_TYPE;
	cmd->c = SATA_CMD_FIS_COMMAND_INDICATOR;
	cmd->command = 0xa1;
	cmd->features = 0;
	cmd->lba_lower = 0;
	cmd->device = 0;
	cmd->lba_upper = 0;
	cmd->features_upper = 0;
	cmd->count = 0;
	cmd->reserved1 = 0;
	cmd->control = 0;
	cmd->reserved2 = 0;

	volatile ahci_cmd_prdt_t *prdt =
	    (ahci_cmd_prdt_t *) (&sata->cmd_table[0x20]);

	prdt->data_address_low = LO(phys);
	prdt->data_address_upper = HI(phys);
	prdt->reserved1 = 0;
	prdt->dbc = SATA_IDENTIFY_DEVICE_BUFFER_LENGTH - 1;
	prdt->reserved2 = 0;
	prdt->ioc = 0;

	sata->cmd_header->prdtl = 1;
	sata->cmd_header->flags =
	    AHCI_CMDHDR_FLAGS_CLEAR_BUSY_UPON_OK |
	    AHCI_CMDHDR_FLAGS_2DWCMD;
	sata->cmd_header->bytesprocessed = 0;

	/* Run command. */
	sata->port->pxsact |= 1;
	sata->port->pxci |= 1;
}

/** Fill device identification in SATA device structure.
 *
 * @param sata SATA device structure.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t ahci_identify_device(sata_dev_t *sata)
{
	if (sata->is_invalid_device) {
		ddf_msg(LVL_ERROR,
		    "Identify command device on invalid device");
		return EINTR;
	}

	uintptr_t phys;
	sata_identify_data_t *idata = AS_AREA_ANY;
	errno_t rc = dmamem_map_anonymous(SATA_IDENTIFY_DEVICE_BUFFER_LENGTH,
	    DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0, &phys,
	    (void *) &idata);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot allocate buffer to identify device.");
		return rc;
	}

	memset(idata, 0, SATA_IDENTIFY_DEVICE_BUFFER_LENGTH);

	fibril_mutex_lock(&sata->lock);

	ahci_identify_device_cmd(sata, phys);
	ahci_port_is_t pxis = ahci_wait_event(sata);

	if (sata->is_invalid_device) {
		ddf_msg(LVL_ERROR,
		    "Unrecoverable error during ata identify device");
		goto error;
	}

	if (ahci_port_is_tfes(pxis)) {
		ahci_identify_packet_device_cmd(sata, phys);
		pxis = ahci_wait_event(sata);

		if ((sata->is_invalid_device) || (ahci_port_is_error(pxis))) {
			ddf_msg(LVL_ERROR,
			    "Unrecoverable error during ata identify packet device");
			goto error;
		}

		sata->is_packet_device = true;
	}

	ahci_get_model_name(idata->model_name, sata->model);

	/*
	 * Due to QEMU limitation (as of 2012-06-22),
	 * only NCQ FPDMA mode is supported.
	 */
	if ((idata->sata_cap & sata_np_cap_ncq) == 0) {
		ddf_msg(LVL_ERROR, "%s: NCQ must be supported", sata->model);
		goto error;
	}

	uint16_t logsec = idata->physical_logic_sector_size;
	if ((logsec & 0xc000) == 0x4000) {
		/* Length of sector may be larger than 512 B */
		if (logsec & 0x0100) {
			/* Size of sector is larger than 512 B */
			ddf_msg(LVL_ERROR,
			    "%s: Sector length other than 512 B not supported",
			    sata->model);
			goto error;
		}

		if ((logsec & 0x0200) && ((logsec & 0x000f) != 0)) {
			/* Physical sectors per logical sector is greather than 1 */
			ddf_msg(LVL_ERROR,
			    "%s: Sector length other than 512 B not supported",
			    sata->model);
			goto error;
		}
	}

	if (sata->is_packet_device) {
		/*
		 * Due to QEMU limitation (as of 2012-06-22),
		 * only NCQ FPDMA mode supported - block size is
		 * 512 B, not 2048 B!
		 */
		sata->block_size = SATA_DEFAULT_SECTOR_SIZE;
		sata->blocks = 0;
	} else {
		sata->block_size = SATA_DEFAULT_SECTOR_SIZE;

		if ((idata->caps & sata_rd_cap_lba) == 0) {
			ddf_msg(LVL_ERROR, "%s: LBA for NCQ must be supported",
			    sata->model);
			goto error;
		} else if ((idata->cmd_set1 & sata_cs1_addr48) == 0) {
			sata->blocks = (uint32_t) idata->total_lba28_0 |
			    ((uint32_t) idata->total_lba28_1 << 16);
		} else {
			/* Device supports LBA-48 addressing. */
			sata->blocks = (uint64_t) idata->total_lba48_0 |
			    ((uint64_t) idata->total_lba48_1 << 16) |
			    ((uint64_t) idata->total_lba48_2 << 32) |
			    ((uint64_t) idata->total_lba48_3 << 48);
		}
	}

	uint8_t udma_mask = idata->udma & 0x007f;
	sata->highest_udma_mode = (uint8_t) -1;
	if (udma_mask == 0) {
		ddf_msg(LVL_ERROR,
		    "%s: UDMA mode for NCQ FPDMA mode must be supported",
		    sata->model);
		goto error;
	} else {
		for (uint8_t i = 0; i < 7; i++) {
			if (udma_mask & (1 << i))
				sata->highest_udma_mode = i;
		}
	}

	fibril_mutex_unlock(&sata->lock);
	dmamem_unmap_anonymous(idata);

	return EOK;

error:
	fibril_mutex_unlock(&sata->lock);
	dmamem_unmap_anonymous(idata);

	return EINTR;
}

/** Set AHCI registers for setting SATA device transfer mode.
 *
 * @param sata SATA device structure.
 * @param phys Physical address of working buffer.
 * @param mode Required mode.
 *
 */
static void ahci_set_mode_cmd(sata_dev_t *sata, uintptr_t phys, uint8_t mode)
{
	volatile sata_std_command_frame_t *cmd =
	    (sata_std_command_frame_t *) sata->cmd_table;

	cmd->fis_type = SATA_CMD_FIS_TYPE;
	cmd->c = SATA_CMD_FIS_COMMAND_INDICATOR;
	cmd->command = 0xef;
	cmd->features = 0x03;
	cmd->lba_lower = 0;
	cmd->device = 0;
	cmd->lba_upper = 0;
	cmd->features_upper = 0;
	cmd->count = mode;
	cmd->reserved1 = 0;
	cmd->control = 0;
	cmd->reserved2 = 0;

	volatile ahci_cmd_prdt_t *prdt =
	    (ahci_cmd_prdt_t *) (&sata->cmd_table[0x20]);

	prdt->data_address_low = LO(phys);
	prdt->data_address_upper = HI(phys);
	prdt->reserved1 = 0;
	prdt->dbc = SATA_SET_FEATURE_BUFFER_LENGTH - 1;
	prdt->reserved2 = 0;
	prdt->ioc = 0;

	sata->cmd_header->prdtl = 1;
	sata->cmd_header->flags =
	    AHCI_CMDHDR_FLAGS_CLEAR_BUSY_UPON_OK |
	    AHCI_CMDHDR_FLAGS_2DWCMD;
	sata->cmd_header->bytesprocessed = 0;

	/* Run command. */
	sata->port->pxsact |= 1;
	sata->port->pxci |= 1;
}

/** Set highest ultra DMA mode supported by SATA device.
 *
 * @param sata SATA device structure.
 *
 * @return EOK if succeed, error code otherwise
 *
 */
static errno_t ahci_set_highest_ultra_dma_mode(sata_dev_t *sata)
{
	if (sata->is_invalid_device) {
		ddf_msg(LVL_ERROR,
		    "%s: Setting highest UDMA mode on invalid device",
		    sata->model);
		return EINTR;
	}

	if (sata->highest_udma_mode == (uint8_t) -1) {
		ddf_msg(LVL_ERROR,
		    "%s: No AHCI UDMA support.", sata->model);
		return EINTR;
	}

	if (sata->highest_udma_mode > 6) {
		ddf_msg(LVL_ERROR,
		    "%s: Unknown AHCI UDMA mode.", sata->model);
		return EINTR;
	}

	uintptr_t phys;
	sata_identify_data_t *idata = AS_AREA_ANY;
	errno_t rc = dmamem_map_anonymous(SATA_SET_FEATURE_BUFFER_LENGTH,
	    DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0, &phys,
	    (void *) &idata);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot allocate buffer for device set mode.");
		return rc;
	}

	memset(idata, 0, SATA_SET_FEATURE_BUFFER_LENGTH);

	fibril_mutex_lock(&sata->lock);

	uint8_t mode = 0x40 | (sata->highest_udma_mode & 0x07);
	ahci_set_mode_cmd(sata, phys, mode);
	ahci_port_is_t pxis = ahci_wait_event(sata);

	if (sata->is_invalid_device) {
		ddf_msg(LVL_ERROR,
		    "%s: Unrecoverable error during set highest UDMA mode",
		    sata->model);
		goto error;
	}

	if (ahci_port_is_error(pxis)) {
		ddf_msg(LVL_ERROR,
		    "%s: Error during set highest UDMA mode", sata->model);
		goto error;
	}

	fibril_mutex_unlock(&sata->lock);
	dmamem_unmap_anonymous(idata);

	return EOK;

error:
	fibril_mutex_unlock(&sata->lock);
	dmamem_unmap_anonymous(idata);

	return EINTR;
}

/** Set AHCI registers for reading one sector from the SATA device using FPDMA.
 *
 * @param sata     SATA device structure.
 * @param phys     Physical address of buffer for sector data.
 * @param blocknum Block number to read.
 *
 */
static void ahci_rb_fpdma_cmd(sata_dev_t *sata, uintptr_t phys,
    uint64_t blocknum)
{
	volatile sata_ncq_command_frame_t *cmd =
	    (sata_ncq_command_frame_t *) sata->cmd_table;

	cmd->fis_type = SATA_CMD_FIS_TYPE;
	cmd->c = SATA_CMD_FIS_COMMAND_INDICATOR;
	cmd->command = 0x60;
	cmd->tag = 0;
	cmd->control = 0;

	cmd->reserved1 = 0;
	cmd->reserved2 = 0;
	cmd->reserved3 = 0;
	cmd->reserved4 = 0;
	cmd->reserved5 = 0;
	cmd->reserved6 = 0;

	cmd->sector_count_low = 1;
	cmd->sector_count_high = 0;

	cmd->lba0 = blocknum & 0xff;
	cmd->lba1 = (blocknum >> 8) & 0xff;
	cmd->lba2 = (blocknum >> 16) & 0xff;
	cmd->lba3 = (blocknum >> 24) & 0xff;
	cmd->lba4 = (blocknum >> 32) & 0xff;
	cmd->lba5 = (blocknum >> 40) & 0xff;

	volatile ahci_cmd_prdt_t *prdt =
	    (ahci_cmd_prdt_t *) (&sata->cmd_table[0x20]);

	prdt->data_address_low = LO(phys);
	prdt->data_address_upper = HI(phys);
	prdt->reserved1 = 0;
	prdt->dbc = sata->block_size - 1;
	prdt->reserved2 = 0;
	prdt->ioc = 0;

	sata->cmd_header->prdtl = 1;
	sata->cmd_header->flags =
	    AHCI_CMDHDR_FLAGS_CLEAR_BUSY_UPON_OK |
	    AHCI_CMDHDR_FLAGS_5DWCMD;
	sata->cmd_header->bytesprocessed = 0;

	sata->port->pxsact |= 1;
	sata->port->pxci |= 1;
}

/** Read one sector from the SATA device using FPDMA.
 *
 * @param sata     SATA device structure.
 * @param phys     Physical address of buffer for sector data.
 * @param blocknum Block number to read.
 *
 * @return EOK if succeed, error code otherwise
 *
 */
static errno_t ahci_rb_fpdma(sata_dev_t *sata, uintptr_t phys, uint64_t blocknum)
{
	if (sata->is_invalid_device) {
		ddf_msg(LVL_ERROR,
		    "%s: FPDMA read from invalid device", sata->model);
		return EINTR;
	}

	ahci_rb_fpdma_cmd(sata, phys, blocknum);
	ahci_port_is_t pxis = ahci_wait_event(sata);

	if ((sata->is_invalid_device) || (ahci_port_is_error(pxis))) {
		ddf_msg(LVL_ERROR,
		    "%s: Unrecoverable error during FPDMA read", sata->model);
		return EINTR;
	}

	return EOK;
}

/** Set AHCI registers for writing one sector to the SATA device, use FPDMA.
 *
 * @param sata     SATA device structure.
 * @param phys     Physical address of buffer with sector data.
 * @param blocknum Block number to write.
 *
 * @return EOK if succeed, error code otherwise
 *
 */
static void ahci_wb_fpdma_cmd(sata_dev_t *sata, uintptr_t phys,
    uint64_t blocknum)
{
	volatile sata_ncq_command_frame_t *cmd =
	    (sata_ncq_command_frame_t *) sata->cmd_table;

	cmd->fis_type = SATA_CMD_FIS_TYPE;
	cmd->c = SATA_CMD_FIS_COMMAND_INDICATOR;
	cmd->command = 0x61;
	cmd->tag = 0;
	cmd->control = 0;

	cmd->reserved1 = 0;
	cmd->reserved2 = 0;
	cmd->reserved3 = 0;
	cmd->reserved4 = 0;
	cmd->reserved5 = 0;
	cmd->reserved6 = 0;

	cmd->sector_count_low = 1;
	cmd->sector_count_high = 0;

	cmd->lba0 = blocknum & 0xff;
	cmd->lba1 = (blocknum >> 8) & 0xff;
	cmd->lba2 = (blocknum >> 16) & 0xff;
	cmd->lba3 = (blocknum >> 24) & 0xff;
	cmd->lba4 = (blocknum >> 32) & 0xff;
	cmd->lba5 = (blocknum >> 40) & 0xff;

	volatile ahci_cmd_prdt_t *prdt =
	    (ahci_cmd_prdt_t *) (&sata->cmd_table[0x20]);

	prdt->data_address_low = LO(phys);
	prdt->data_address_upper = HI(phys);
	prdt->reserved1 = 0;
	prdt->dbc = sata->block_size - 1;
	prdt->reserved2 = 0;
	prdt->ioc = 0;

	sata->cmd_header->prdtl = 1;
	sata->cmd_header->flags =
	    AHCI_CMDHDR_FLAGS_CLEAR_BUSY_UPON_OK |
	    AHCI_CMDHDR_FLAGS_WRITE |
	    AHCI_CMDHDR_FLAGS_5DWCMD;
	sata->cmd_header->bytesprocessed = 0;

	sata->port->pxsact |= 1;
	sata->port->pxci |= 1;
}

/** Write one sector into the SATA device, use FPDMA.
 *
 * @param sata     SATA device structure.
 * @param phys     Physical addres of buffer with sector data.
 * @param blocknum Block number to write.
 *
 * @return EOK if succeed, error code otherwise
 *
 */
static errno_t ahci_wb_fpdma(sata_dev_t *sata, uintptr_t phys, uint64_t blocknum)
{
	if (sata->is_invalid_device) {
		ddf_msg(LVL_ERROR,
		    "%s: FPDMA write to invalid device", sata->model);
		return EINTR;
	}

	ahci_wb_fpdma_cmd(sata, phys, blocknum);
	ahci_port_is_t pxis = ahci_wait_event(sata);

	if ((sata->is_invalid_device) || (ahci_port_is_error(pxis))) {
		ddf_msg(LVL_ERROR,
		    "%s: Unrecoverable error during FPDMA write", sata->model);
		return EINTR;
	}

	return EOK;
}

/*----------------------------------------------------------------------------*/
/*-- Interrupts handling -----------------------------------------------------*/
/*----------------------------------------------------------------------------*/

static irq_pio_range_t ahci_ranges[] = {
	{
		.base = 0,
		.size = 0,
	}
};

static irq_cmd_t ahci_cmds[] = {
	AHCI_PORT_CMDS(0),
	AHCI_PORT_CMDS(1),
	AHCI_PORT_CMDS(2),
	AHCI_PORT_CMDS(3),
	AHCI_PORT_CMDS(4),
	AHCI_PORT_CMDS(5),
	AHCI_PORT_CMDS(6),
	AHCI_PORT_CMDS(7),
	AHCI_PORT_CMDS(8),
	AHCI_PORT_CMDS(9),
	AHCI_PORT_CMDS(10),
	AHCI_PORT_CMDS(11),
	AHCI_PORT_CMDS(12),
	AHCI_PORT_CMDS(13),
	AHCI_PORT_CMDS(14),
	AHCI_PORT_CMDS(15),
	AHCI_PORT_CMDS(16),
	AHCI_PORT_CMDS(17),
	AHCI_PORT_CMDS(18),
	AHCI_PORT_CMDS(19),
	AHCI_PORT_CMDS(20),
	AHCI_PORT_CMDS(21),
	AHCI_PORT_CMDS(22),
	AHCI_PORT_CMDS(23),
	AHCI_PORT_CMDS(24),
	AHCI_PORT_CMDS(25),
	AHCI_PORT_CMDS(26),
	AHCI_PORT_CMDS(27),
	AHCI_PORT_CMDS(28),
	AHCI_PORT_CMDS(29),
	AHCI_PORT_CMDS(30),
	AHCI_PORT_CMDS(31)
};

/** AHCI interrupt handler.
 *
 * @param icall The IPC call structure.
 * @param dev   DDF device structure.
 *
 */
static void ahci_interrupt(ipc_call_t *icall, ddf_dev_t *dev)
{
	ahci_dev_t *ahci = dev_ahci_dev(dev);
	unsigned int port = IPC_GET_ARG1(*icall);
	ahci_port_is_t pxis = IPC_GET_ARG2(*icall);

	if (port >= AHCI_MAX_PORTS)
		return;

	sata_dev_t *sata = (sata_dev_t *) ahci->sata_devs[port];
	if (sata == NULL)
		return;

	/* Evaluate port event */
	if ((ahci_port_is_end_of_operation(pxis)) ||
	    (ahci_port_is_error(pxis))) {
		fibril_mutex_lock(&sata->event_lock);

		sata->event_pxis = pxis;
		fibril_condvar_signal(&sata->event_condvar);

		fibril_mutex_unlock(&sata->event_lock);
	}
}

/*----------------------------------------------------------------------------*/
/*-- AHCI and SATA device creating and initializing routines -----------------*/
/*----------------------------------------------------------------------------*/

/** Allocate SATA device structure with buffers for hardware.
 *
 * @param port AHCI port structure
 *
 * @return SATA device structure if succeed, NULL otherwise.
 *
 */
static sata_dev_t *ahci_sata_allocate(ahci_dev_t *ahci, volatile ahci_port_t *port)
{
	size_t size = 4096;
	uintptr_t phys = 0;
	void *virt_fb = AS_AREA_ANY;
	void *virt_cmd = AS_AREA_ANY;
	void *virt_table = AS_AREA_ANY;
	ddf_fun_t *fun;

	fun = ddf_fun_create(ahci->dev, fun_exposed, NULL);

	sata_dev_t *sata = ddf_fun_data_alloc(fun, sizeof(sata_dev_t));
	if (sata == NULL)
		return NULL;

	sata->fun = fun;
	sata->port = port;

	/* Allocate and init retfis structure. */
	errno_t rc = dmamem_map_anonymous(size, DMAMEM_4GiB,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &phys, &virt_fb);
	if (rc != EOK)
		goto error_retfis;

	memset(virt_fb, 0, size);
	sata->port->pxfbu = HI(phys);
	sata->port->pxfb = LO(phys);

	/* Allocate and init command header structure. */
	rc = dmamem_map_anonymous(size, DMAMEM_4GiB,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &phys, &virt_cmd);
	if (rc != EOK)
		goto error_cmd;

	memset(virt_cmd, 0, size);
	sata->port->pxclbu = HI(phys);
	sata->port->pxclb = LO(phys);
	sata->cmd_header = (ahci_cmdhdr_t *) virt_cmd;

	/* Allocate and init command table structure. */
	rc = dmamem_map_anonymous(size, DMAMEM_4GiB,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &phys, &virt_table);
	if (rc != EOK)
		goto error_table;

	memset(virt_table, 0, size);
	sata->cmd_header->cmdtableu = HI(phys);
	sata->cmd_header->cmdtable = LO(phys);
	sata->cmd_table = (uint32_t *) virt_table;

	return sata;

error_table:
	dmamem_unmap(virt_cmd, size);
error_cmd:
	dmamem_unmap(virt_fb, size);
error_retfis:
	free(sata);
	return NULL;
}

/** Initialize and start SATA hardware device.
 *
 * @param sata SATA device structure.
 *
 */
static void ahci_sata_hw_start(sata_dev_t *sata)
{
	ahci_port_cmd_t pxcmd;

	pxcmd.u32 = sata->port->pxcmd;

	/* Frame receiver disabled. */
	pxcmd.fre = 0;

	/* Disable process the command list. */
	pxcmd.st = 0;

	sata->port->pxcmd = pxcmd.u32;

	/* Clear interrupt status. */
	sata->port->pxis = 0xffffffff;

	/* Clear error status. */
	sata->port->pxserr = 0xffffffff;

	/* Enable all interrupts. */
	sata->port->pxie = 0xffffffff;

	/* Frame receiver enabled. */
	pxcmd.fre = 1;

	/* Enable process the command list. */
	pxcmd.st = 1;

	sata->port->pxcmd = pxcmd.u32;
}

/** Create and initialize connected SATA structure device
 *
 * @param ahci     AHCI device structure.
 * @param dev      DDF device structure.
 * @param port     AHCI port structure.
 * @param port_num Number of AHCI port with existing SATA device.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t ahci_sata_create(ahci_dev_t *ahci, ddf_dev_t *dev,
    volatile ahci_port_t *port, unsigned int port_num)
{
	ddf_fun_t *fun = NULL;
	errno_t rc;

	sata_dev_t *sata = ahci_sata_allocate(ahci, port);
	if (sata == NULL)
		return EINTR;

	/* Set pointers between SATA and AHCI structures. */
	sata->ahci = ahci;
	sata->port_num = port_num;
	ahci->sata_devs[port_num] = sata;

	/* Initialize synchronization structures */
	fibril_mutex_initialize(&sata->lock);
	fibril_mutex_initialize(&sata->event_lock);
	fibril_condvar_initialize(&sata->event_condvar);

	ahci_sata_hw_start(sata);

	/* Identify device. */
	if (ahci_identify_device(sata) != EOK)
		goto error;

	/* Set required UDMA mode */
	if (ahci_set_highest_ultra_dma_mode(sata) != EOK)
		goto error;

	/* Add device to the system */
	char sata_dev_name[16];
	snprintf(sata_dev_name, 16, "ahci_%u", sata_devices_count);

	fibril_mutex_lock(&sata_devices_count_lock);
	sata_devices_count++;
	fibril_mutex_unlock(&sata_devices_count_lock);

	rc = ddf_fun_set_name(sata->fun, sata_dev_name);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed setting function name.");
		goto error;
	}

	ddf_fun_set_ops(fun, &ahci_ops);

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function.");
		goto error;
	}

	return EOK;

error:
	sata->is_invalid_device = true;
	if (fun != NULL)
		ddf_fun_destroy(fun);

	return EINTR;
}

/** Create and initialize all SATA structure devices for connected SATA drives.
 *
 * @param ahci AHCI device structure.
 * @param dev  DDF device structure.
 *
 */
static void ahci_sata_devices_create(ahci_dev_t *ahci, ddf_dev_t *dev)
{
	for (unsigned int port_num = 0; port_num < AHCI_MAX_PORTS; port_num++) {
		/* Active ports only */
		if (!(ahci->memregs->ghc.pi & (1 << port_num)))
			continue;

		volatile ahci_port_t *port = ahci->memregs->ports + port_num;

		/* Active devices only */
		ahci_port_ssts_t pxssts;
		pxssts.u32 = port->pxssts;
		if (pxssts.det != AHCI_PORT_SSTS_DET_ACTIVE)
			continue;

		ahci_sata_create(ahci, dev, port, port_num);
	}
}

/** Create AHCI device structure, intialize it and register interrupt routine.
 *
 * @param dev DDF device structure.
 *
 * @return AHCI device structure if succeed, NULL otherwise.
 *
 */
static ahci_dev_t *ahci_ahci_create(ddf_dev_t *dev)
{
	ahci_dev_t *ahci = ddf_dev_data_alloc(dev, sizeof(ahci_dev_t));
	if (!ahci)
		return NULL;

	/* Connect to parent device */
	ahci->parent_sess = ddf_dev_parent_sess_get(dev);
	if (ahci->parent_sess == NULL)
		return NULL;

	ahci->dev = dev;

	hw_res_list_parsed_t hw_res_parsed;
	hw_res_list_parsed_init(&hw_res_parsed);
	if (hw_res_get_list_parsed(ahci->parent_sess, &hw_res_parsed, 0) != EOK)
		goto error_get_res_parsed;

	/* Map AHCI registers. */
	ahci->memregs = AS_AREA_ANY;

	physmem_map(RNGABS(hw_res_parsed.mem_ranges.ranges[0]),
	    AHCI_MEMREGS_PAGES_COUNT, AS_AREA_READ | AS_AREA_WRITE,
	    (void *) &ahci->memregs);
	if (ahci->memregs == NULL)
		goto error_map_registers;

	/* Register interrupt handler */
	ahci_ranges[0].base = RNGABS(hw_res_parsed.mem_ranges.ranges[0]);
	ahci_ranges[0].size = sizeof(ahci_memregs_t);

	for (unsigned int port = 0; port < AHCI_MAX_PORTS; port++) {
		size_t base = port * 7;

		ahci_cmds[base].addr =
		    ((uint32_t *) RNGABSPTR(hw_res_parsed.mem_ranges.ranges[0])) +
		    AHCI_PORTS_REGISTERS_OFFSET + port * AHCI_PORT_REGISTERS_SIZE +
		    AHCI_PORT_IS_REGISTER_OFFSET;
		ahci_cmds[base + 2].addr = ahci_cmds[base].addr;

		ahci_cmds[base + 3].addr =
		    ((uint32_t *) RNGABSPTR(hw_res_parsed.mem_ranges.ranges[0])) +
		    AHCI_GHC_IS_REGISTER_OFFSET;
		ahci_cmds[base + 4].addr = ahci_cmds[base + 3].addr;
	}

	irq_code_t ct;
	ct.cmdcount = sizeof(ahci_cmds) / sizeof(irq_cmd_t);
	ct.cmds = ahci_cmds;
	ct.rangecount = sizeof(ahci_ranges) / sizeof(irq_pio_range_t);
	ct.ranges = ahci_ranges;

	cap_irq_handle_t irq_cap;
	errno_t rc = register_interrupt_handler(dev,
	    hw_res_parsed.irqs.irqs[0], ahci_interrupt, &ct, &irq_cap);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed registering interrupt handler.");
		goto error_register_interrupt_handler;
	}

	rc = hw_res_enable_interrupt(ahci->parent_sess,
	    hw_res_parsed.irqs.irqs[0]);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed enable interupt.");
		goto error_enable_interrupt;
	}

	hw_res_list_parsed_clean(&hw_res_parsed);
	return ahci;

error_enable_interrupt:
	unregister_interrupt_handler(dev, irq_cap);

error_register_interrupt_handler:
	// FIXME: unmap physical memory

error_map_registers:
	hw_res_list_parsed_clean(&hw_res_parsed);

error_get_res_parsed:
	free(ahci);
	return NULL;
}

/** Initialize and start AHCI hardware device.
 *
 * @param ahci AHCI device.
 *
 */
static void ahci_ahci_hw_start(ahci_dev_t *ahci)
{
	/* Disable command completion coalescing feature */
	ahci_ghc_ccc_ctl_t ccc;

	ccc.u32 = ahci->memregs->ghc.ccc_ctl;
	ccc.en = 0;
	ahci->memregs->ghc.ccc_ctl = ccc.u32;

	/* Set master latency timer. */
	pci_config_space_write_8(ahci->parent_sess, AHCI_PCI_MLT, 32);

	/* Enable PCI interrupt and bus mastering */
	ahci_pcireg_cmd_t cmd;

	pci_config_space_read_16(ahci->parent_sess, AHCI_PCI_CMD, &cmd.u16);
	cmd.id = 0;
	cmd.bme = 1;
	pci_config_space_write_16(ahci->parent_sess, AHCI_PCI_CMD, cmd.u16);

	/* Enable AHCI and interrupt. */
	ahci->memregs->ghc.ghc = AHCI_GHC_GHC_AE | AHCI_GHC_GHC_IE;
}

/** AHCI device driver initialization
 *
 * Create and initialize all SATA structure devices for connected
 * SATA drives.
 *
 * @param dev DDF device structure.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t ahci_dev_add(ddf_dev_t *dev)
{
	ahci_dev_t *ahci = ahci_ahci_create(dev);
	if (ahci == NULL)
		goto error;

	/* Start AHCI hardware. */
	ahci_ahci_hw_start(ahci);

	/* Create device structures for sata devices attached to AHCI. */
	ahci_sata_devices_create(ahci, dev);

	return EOK;

error:
	return EINTR;
}

/*----------------------------------------------------------------------------*/
/*-- Helpers and utilities ---------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/** Convert SATA model name
 *
 * Convert SATA model name from machine format returned by
 * identify device command to human readable form.
 *
 * @param src Source buffer with device name in machine format.
 * @param dst Buffer for human readable string, minimum size is 41 chars.
 *
 */
static void ahci_get_model_name(uint16_t *src, char *dst)
{
	uint8_t model[40];
	memset(model, 0, 40);

	for (unsigned int i = 0; i < 20; i++) {
		uint16_t w = src[i];
		model[2 * i] = w >> 8;
		model[2 * i + 1] = w & 0x00ff;
	}

	uint32_t len = 40;
	while ((len > 0) && (model[len - 1] == 0x20))
		len--;

	size_t pos = 0;
	for (unsigned int i = 0; i < len; i++) {
		uint8_t c = model[i];
		if (c >= 0x80)
			c = '?';

		chr_encode(c, dst, &pos, 40);
	}

	dst[pos] = '\0';
}

/*----------------------------------------------------------------------------*/
/*-- AHCI Main routine -------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
	printf("%s: HelenOS AHCI device driver\n", NAME);
	ddf_log_init(NAME);
	fibril_mutex_initialize(&sata_devices_count_lock);
	return ddf_driver_main(&ahci_driver);
}
