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
#include <devman.h>
#include <ddf/interrupt.h>
#include <ddf/log.h>
#include <device/hw_res_parsed.h>
#include <device/pci.h>
#include <sysinfo.h>
#include <ipc/irc.h>
#include <ns.h>
#include <ahci_iface.h>
#include "ahci.h"
#include "ahci_hw.h"
#include "ahci_sata.h"

#define NAME  "ahci"

#define AHCI_TIMER_TICKS  1000000000

#define LO(ptr) \
	((uint32_t) (((uint64_t) ((uintptr_t) (ptr))) & 0xffffffff))

#define HI(ptr) \
	((uint32_t) (((uint64_t) ((uintptr_t) (ptr))) >> 32))

static int ahci_get_sata_device_name(ddf_fun_t *, size_t, char *);
static int ahci_get_num_blocks(ddf_fun_t *, uint64_t *);
static int ahci_get_block_size(ddf_fun_t *, size_t *);
static int ahci_read_blocks(ddf_fun_t *, uint64_t, size_t, void *);
static int ahci_write_blocks(ddf_fun_t *, uint64_t, size_t, void *);

static int ahci_identify_device(sata_dev_t *);
static int ahci_set_highest_ultra_dma_mode(sata_dev_t *);
static int ahci_rb_fpdma(sata_dev_t *, void *, uint64_t);
static int ahci_wb_fpdma(sata_dev_t *, void *, uint64_t);

static void ahci_sata_devices_create(ahci_dev_t *, ddf_dev_t *);
static ahci_dev_t *ahci_ahci_create(ddf_dev_t *);
static void ahci_ahci_init(ahci_dev_t *);

static int ahci_dev_add(ddf_dev_t *);

static void ahci_get_model_name(uint16_t *, char *);
static int ahci_pciintel_enable_interrupt(int);

static fibril_mutex_t sata_devices_count_lock;
static int sata_devices_count = 0;

/*----------------------------------------------------------------------------*/
/*-- AHCI Interface ----------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

static ahci_iface_t ahci_interface = {
	.get_sata_device_name = &ahci_get_sata_device_name,
	.get_num_blocks = &ahci_get_num_blocks,
	.get_block_size = &ahci_get_block_size,
	.read_blocks = &ahci_read_blocks,
	.write_blocks = &ahci_write_blocks
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

static int ahci_get_sata_device_name(ddf_fun_t *fun,
    size_t sata_dev_name_length, char *sata_dev_name)
{
	sata_dev_t *sata = (sata_dev_t *) fun->driver_data;
	str_cpy(sata_dev_name, sata_dev_name_length, sata->model);
	return EOK;
}

static int ahci_get_num_blocks(ddf_fun_t *fun, uint64_t *num_blocks)
{
	sata_dev_t *sata = (sata_dev_t *) fun->driver_data;
	*num_blocks = sata->blocks;
	return EOK;
}

static int ahci_get_block_size(ddf_fun_t *fun, size_t *block_size)
{
	sata_dev_t *sata = (sata_dev_t *) fun->driver_data;
	*block_size = sata->block_size;
	return EOK;
}

static int ahci_read_blocks(ddf_fun_t *fun, uint64_t blocknum,
    size_t count, void *buf)
{
	int rc = EOK;
	sata_dev_t *sata = (sata_dev_t *) fun->driver_data;
	
	void *phys;
	void *ibuf;
	
	dmamem_map_anonymous(sata->block_size, AS_AREA_READ | AS_AREA_WRITE,
	    0, &phys, (void **) &ibuf);
	bzero(buf, sata->block_size);
	
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

static int ahci_write_blocks(ddf_fun_t *fun, uint64_t blocknum,
    size_t count, void *buf)
{
	int rc = EOK;
	sata_dev_t *sata = (sata_dev_t *) fun->driver_data;
	
	void *phys;
	void *ibuf;
	
	dmamem_map_anonymous(sata->block_size, AS_AREA_READ | AS_AREA_WRITE,
	    0, &phys, (void **) &ibuf);
	
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

static void ahci_identify_device_cmd(sata_dev_t *sata, void *phys)
{
	volatile std_command_frame_t *cmd =
	    (std_command_frame_t *) sata->cmd_table;
	
	cmd->fis_type = 0x27;
	cmd->c = 0x80;
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
	prdt->dbc = 511;
	prdt->reserved2 = 0;
	prdt->ioc = 0;
	
	sata->cmd_header->prdtl = 1;
	sata->cmd_header->flags = 0x402;
	sata->cmd_header->bytesprocessed = 0;
	
	sata->port->pxsact |= 1;
	sata->port->pxci |= 1;
}

static void ahci_identify_packet_device_cmd(sata_dev_t *sata, void *phys)
{
	volatile std_command_frame_t *cmd =
	    (std_command_frame_t *) sata->cmd_table;
	
	cmd->fis_type = 0x27;
	cmd->c = 0x80;
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
	prdt->dbc = 511;
	prdt->reserved2 = 0;
	prdt->ioc = 0;
	
	sata->cmd_header->prdtl = 1;
	sata->cmd_header->flags = 0x402;
	sata->cmd_header->bytesprocessed = 0;
	
	sata->port->pxsact |= 1;
	sata->port->pxci |= 1;
}

static int ahci_identify_device(sata_dev_t *sata)
{
	if (sata->invalid_device) {
		ddf_msg(LVL_ERROR,
		    "Identify command device on invalid device");
		return EINTR;
	}
	
	void *phys;
	identify_data_t *idata;
	
	dmamem_map_anonymous(512, AS_AREA_READ | AS_AREA_WRITE, 0, &phys,
	    (void **) &idata);
	bzero(idata, 512);
	
	fibril_mutex_lock(&sata->lock);
	
	ahci_identify_device_cmd(sata, phys);
	
	fibril_mutex_lock(&sata->event_lock);
	fibril_condvar_wait(&sata->event_condvar, &sata->event_lock);
	fibril_mutex_unlock(&sata->event_lock);
	
	ahci_port_is_t pxis = sata->shadow_pxis;
	sata->shadow_pxis.u32 &= ~pxis.u32;
	
	if (sata->invalid_device) {
		ddf_msg(LVL_ERROR,
		    "Unrecoverable error during ata identify device");
		goto error;
	}
	
	if (ahci_port_is_tfes(pxis)) {
		ahci_identify_packet_device_cmd(sata, phys);
		
		fibril_mutex_lock(&sata->event_lock);
		fibril_condvar_wait(&sata->event_condvar, &sata->event_lock);
		fibril_mutex_unlock(&sata->event_lock);
		
		pxis = sata->shadow_pxis;
		sata->shadow_pxis.u32 &= ~pxis.u32;
		
		if ((sata->invalid_device) || (ahci_port_is_error(pxis))) {
			ddf_msg(LVL_ERROR,
			    "Unrecoverable error during ata identify packet device");
			goto error;
		}
		
		sata->packet_device = true;
	} else
		sata->packet_device = false;
	
	ahci_get_model_name(idata->model_name, sata->model);
	
	/*
	 * Due to QEMU limitation (as of 2012-06-22),
	 * only NCQ FPDMA mode is supported.
	 */
	if ((idata->sata_cap & np_cap_ncq) == 0) {
		ddf_msg(LVL_ERROR, "%s: NCQ must be supported", sata->model);
		goto error;
	}
	
	if (sata->packet_device) {
		/*
		 * Due to QEMU limitation (as of 2012-06-22),
		 * only NCQ FPDMA mode supported - block size is 512 B,
		 * not 2048 B!
		 */
		sata->block_size = 512;
		sata->blocks = 0;
	} else {
		sata->block_size = 512;
		
		if ((idata->caps & rd_cap_lba) == 0) {
			ddf_msg(LVL_ERROR, "%s: LBA for NCQ must be supported",
			    sata->model);
			goto error;
		} else if ((idata->cmd_set1 & cs1_addr48) == 0) {
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
	if (udma_mask == 0) {
		ddf_msg(LVL_ERROR,
		    "%s: UDMA mode for NCQ FPDMA mode must be supported",
		    sata->model);
		goto error;
	} else {
		for (unsigned int i = 0; i < 7; i++) {
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

static void ahci_set_mode_cmd(sata_dev_t *sata, void* phys, uint8_t mode)
{
	volatile std_command_frame_t *cmd =
	    (std_command_frame_t *) sata->cmd_table;
	
	cmd->fis_type = 0x27;
	cmd->c = 0x80;
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
	
	volatile ahci_cmd_prdt_t* prdt =
	    (ahci_cmd_prdt_t *) (&sata->cmd_table[0x20]);
	
	prdt->data_address_low = LO(phys);
	prdt->data_address_upper = HI(phys);
	prdt->reserved1 = 0;
	prdt->dbc = 511;
	prdt->reserved2 = 0;
	prdt->ioc = 0;
	
	sata->cmd_header->prdtl = 1;
	sata->cmd_header->flags = 0x402;
	sata->cmd_header->bytesprocessed = 0;
	
	sata->port->pxsact |= 1;
	sata->port->pxci |= 1;
}

static int ahci_set_highest_ultra_dma_mode(sata_dev_t *sata)
{
	if (sata->invalid_device) {
		ddf_msg(LVL_ERROR,
		    "%s: Setting highest UDMA mode on invalid device",
		    sata->model);
		return EINTR;
	}
	
	void *phys;
	identify_data_t *idata;
	
	dmamem_map_anonymous(512, AS_AREA_READ | AS_AREA_WRITE, 0, &phys,
	    (void **) &idata);
	bzero(idata, 512);
	
	fibril_mutex_lock(&sata->lock);
	
	uint8_t mode = 0x40 | (sata->highest_udma_mode & 0x07);
	ahci_set_mode_cmd(sata, phys, mode);
	
	fibril_mutex_lock(&sata->event_lock);
	fibril_condvar_wait(&sata->event_condvar, &sata->event_lock);
	fibril_mutex_unlock(&sata->event_lock);
	
	ahci_port_is_t pxis = sata->shadow_pxis;
	sata->shadow_pxis.u32 &= ~pxis.u32;
	
	if (sata->invalid_device) {
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

static void ahci_rb_fpdma_cmd(sata_dev_t *sata, void *phys, uint64_t blocknum)
{
	volatile ncq_command_frame_t *cmd =
	    (ncq_command_frame_t *) sata->cmd_table;
	
	cmd->fis_type = 0x27;
	cmd->c = 0x80;
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
	sata->cmd_header->flags = 0x405;
	sata->cmd_header->bytesprocessed = 0;
	
	sata->port->pxsact |= 1;
	sata->port->pxci |= 1;
}

static int ahci_rb_fpdma(sata_dev_t *sata, void *phys, uint64_t blocknum)
{
	if (sata->invalid_device) {
		ddf_msg(LVL_ERROR,
		    "%s: FPDMA read from invalid device", sata->model);
		return EINTR;
	}
	
	ahci_rb_fpdma_cmd(sata, phys, blocknum);
	
	fibril_mutex_lock(&sata->event_lock);
	fibril_condvar_wait(&sata->event_condvar, &sata->event_lock);
	fibril_mutex_unlock(&sata->event_lock);
	
	ahci_port_is_t pxis = sata->shadow_pxis;
	sata->shadow_pxis.u32 &= ~pxis.u32;
	
	if ((sata->invalid_device) || (ahci_port_is_error(pxis))_ {
		ddf_msg(LVL_ERROR,
		    "%s: Unrecoverable error during FPDMA read", sata->model);
		return EINTR;
	}
	
	return EOK;
}

static void ahci_wb_fpdma_cmd(sata_dev_t *sata, void *phys, uint64_t blocknum)
{
	volatile ncq_command_frame_t *cmd =
	    (ncq_command_frame_t *) sata->cmd_table;
	
	cmd->fis_type = 0x27;
	cmd->c = 0x80;
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
	
	volatile ahci_cmd_prdt_t * prdt =
	    (ahci_cmd_prdt_t *) (&sata->cmd_table[0x20]);
	
	prdt->data_address_low = LO(phys);
	prdt->data_address_upper = HI(phys);
	prdt->reserved1 = 0;
	prdt->dbc = sata->block_size - 1;
	prdt->reserved2 = 0;
	prdt->ioc = 0;
	
	sata->cmd_header->prdtl = 1;
	sata->cmd_header->flags = 0x445;
	sata->cmd_header->bytesprocessed = 0;
	
	sata->port->pxsact |= 1;
	sata->port->pxci |= 1;
}

static int ahci_wb_fpdma(sata_dev_t *sata, void *phys, uint64_t blocknum)
{
	if (sata->invalid_device) {
		ddf_msg(LVL_ERROR,
		    "%s: FPDMA write to invalid device", sata->model);
		return EINTR;
	}
	
	ahci_wb_fpdma_cmd(sata, phys, blocknum);
	
	fibril_mutex_lock(&sata->event_lock);
	fibril_condvar_wait(&sata->event_condvar, &sata->event_lock);
	fibril_mutex_unlock(&sata->event_lock);
	
	ahci_port_is_t pxis = sata->shadow_pxis;
	sata->shadow_pxis.u32 &= ~pxis.u32;
	
	if ((sata->invalid_device) || (ahci_port_is_error(pxis))) {
		ddf_msg(LVL_ERROR,
		    "%s: Unrecoverable error during fpdma write", sata->model);
		return EINTR;
	}
	
	return EOK;
}

/*----------------------------------------------------------------------------*/
/*-- Interrupts and timer unified handling -----------------------------------*/
/*----------------------------------------------------------------------------*/

static irq_pio_range_t ahci_ranges[] = {
	{
		.base = 0,
		.size = 32,
	}
};

static irq_cmd_t ahci_cmds[] = {
	{
		/* Disable interrupt - interrupt is deasserted in qemu 1.0.1 */
		.cmd = CMD_MEM_WRITE_32,
		.addr = NULL,
		.value = AHCI_GHC_GHC_AE
	},
	{
		/* Clear interrupt status register - for vbox and real hw */
		.cmd = CMD_MEM_REWRITE_32,
		.addr = NULL
	},
	{
		.cmd = CMD_ACCEPT
	}
};

/** Unified AHCI interrupt and timer interrupt handler.
 *
 * @param ahci     AHCI device.
 * @param is_timer Indicate timer interrupt.
 *
 */
static void ahci_interrupt_or_timer(ahci_dev_t *ahci, bool is_timer)
{
	/*
	 * Get current value of hardware interrupt state register,
	 * clear hardware register (write to clear behavior).
	 */
	ahci_ghc_is_t is;
	
	is.u32 = ahci->memregs->ghc.is;
	ahci->memregs->ghc.is = is.u32;
	is.u32 = ahci->memregs->ghc.is;
	
	uint32_t port_event_flags = 0;
	uint32_t port_mask = 1;
	for (unsigned int i = 0; i < 32; i++) {
		/*
		 * Get current value of hardware port interrupt state register,
		 * clear hardware register (write to clear behavior).
		 */
		ahci_port_is_t pxis;
		
		pxis.u32 = ahci->memregs->ports[i].pxis;
		ahci->memregs->ports[i].pxis = pxis.u32;
		
		sata_dev_t *sata = (sata_dev_t *) ahci->sata_devs[i];
		if (sata != NULL) {
			/* add value to shadow copy of port interrupt state register. */
			sata->shadow_pxis.u32 |= pxis.u32;
			
			/* Evaluate port event. */
			if ((ahci_port_is_end_of_operation(pxis)) ||
			    (ahci_port_is_error(pxis)))
				port_event_flags |= port_mask;
			
			if (ahci_port_is_permanent_error(pxis))
				sata->invalid_device = true;
		}
		
		port_mask <<= 1;
	}
	
	port_mask = 1;
	for (unsigned int i = 0; i < 32; i++) {
		sata_dev_t *sata = (sata_dev_t *) ahci->sata_devs[i];
		if ((port_event_flags & port_mask) && (sata != NULL)) {
			fibril_mutex_lock(&sata->event_lock);
			fibril_condvar_signal(&sata->event_condvar);
			fibril_mutex_unlock(&sata->event_lock);
		}
		
		port_mask <<= 1;
	}
}

/** AHCI timer interrupt handler.
 *
 * @param arg Pointer to AHCI device.
 *
 */
static void ahci_timer(void *arg)
{
	ahci_dev_t *ahci = (ahci_dev_t *) arg;
	
	ahci_interrupt_or_timer(ahci, 1);
	fibril_timer_set(ahci->timer, AHCI_TIMER_TICKS, ahci_timer, ahci);
}

/** AHCI interrupt handler.
 *
 * @param dev Pointer to device driver handler.
 *
 */
static void ahci_interrupt(ddf_dev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	ahci_dev_t *ahci = (ahci_dev_t *) dev->driver_data;
	
	ahci_interrupt_or_timer(ahci, 0);
	
	/* Enable interrupt. */
	ahci->memregs->ghc.ghc |= 2;
}

/*----------------------------------------------------------------------------*/
/*-- AHCI and SATA device creating and initializing routines -----------------*/
/*----------------------------------------------------------------------------*/

static sata_dev_t *ahci_sata_device_allocate(volatile ahci_port_t *port)
{
	size_t size = 4096;
	void* phys = NULL;
	void* virt_fb = NULL;
	void* virt_cmd = NULL;
	void* virt_table = NULL;
	
	sata_dev_t *sata = malloc(sizeof(sata_dev_t));
	if (sata == NULL)
		return NULL;
	
	bzero(sata, sizeof(sata_dev_t));
	
	sata->port = port;
	
	/* Allocate and init retfis structure. */
	int rc = dmamem_map_anonymous(size, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &phys, &virt_fb);
	if (rc != EOK)
		goto error_retfis;
	
	bzero(virt_fb, size);
	sata->port->pxfbu = HI(phys);
	sata->port->pxfb = LO(phys);
	
	/* Allocate and init command header structure. */
	rc = dmamem_map_anonymous(size, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &phys, &virt_cmd);
	
	if (rc != EOK)
		goto error_cmd;
	
	bzero(virt_cmd, size);
	sata->port->pxclbu = HI(phys);
	sata->port->pxclb = LO(phys);
	sata->cmd_header = (ahci_cmdhdr_t *) virt_cmd;
	
	/* Allocate and init command table structure. */
	rc = dmamem_map_anonymous(size, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &phys, &virt_table);
	
	if (rc != EOK)
		goto error_table;
	
	bzero(virt_table, size);
	sata->cmd_header->cmdtableu = HI(phys);
	sata->cmd_header->cmdtable = LO(phys);
	sata->cmd_table = (uint32_t*) virt_table;
	
	return sata;
	
error_table:
	dmamem_unmap(virt_cmd, size);
error_cmd:
	dmamem_unmap(virt_fb, size);
error_retfis:
	free(sata);
	return NULL;
	
	/*
	 * Deleting of pointers in memory hardware mapped register
	 * unneccessary, hardware port is not in operational state.
	 */
}

static int ahci_sata_device_create(ahci_dev_t *ahci, ddf_dev_t *dev,
    volatile ahci_port_t *port, unsigned int port_num)
{
	ddf_fun_t *fun = NULL;
	sata_dev_t *sata = ahci_sata_device_allocate(port);
	
	if (sata == NULL)
		return EINTR;
	
	/* Set pointers between SATA and AHCI structures. */
	sata->ahci = ahci;
	sata->port_num = port_num;
	ahci->sata_devs[port_num] = sata;
	
	fibril_mutex_initialize(&sata->lock);
	fibril_mutex_initialize(&sata->event_lock);
	fibril_condvar_initialize(&sata->event_condvar);
	
	/* Initialize SATA port operational registers. */
	sata->port->pxis = 0;
	sata->port->pxie = 0xffffffff;
	sata->port->pxserr = 0;
	sata->port->pxcmd |= 0x10;
	sata->port->pxcmd |= 0x01;
	
	if (ahci_identify_device(sata) != EOK)
		goto error;
	
	if (ahci_set_highest_ultra_dma_mode(sata) != EOK)
		goto error;
	
	/* Add sata device to system. */
	char sata_dev_name[1024];
	snprintf(sata_dev_name, 1024, "ahci_%u", sata_devices_count);
	
	fibril_mutex_lock(&sata_devices_count_lock);
	sata_devices_count++;
	fibril_mutex_unlock(&sata_devices_count_lock);
	
	fun = ddf_fun_create(dev, fun_exposed, sata_dev_name);
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function.");
		goto error;
	}
	
	fun->ops = &ahci_ops;
	fun->driver_data = sata;
	int rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function.");
		goto error;
	}
	
	return EOK;
	
error:
	sata->invalid_device = true;
	if (fun != NULL)
		ddf_fun_destroy(fun);
	
	return EINTR;
}

static void ahci_sata_devices_create(ahci_dev_t *ahci, ddf_dev_t *dev)
{
	for (unsigned int port_num = 0; port_num < 32; port_num++) {
		/* Active ports only */
		if (!(ahci->memregs->ghc.pi & (1 << port_num)))
			continue;
		
		volatile ahci_port_t *port = ahci->memregs->ports + port_num;
		
		/* Active devices only */
		if ((port->pxssts & 0x0f) != 3)
			continue;
		
		ahci_sata_device_create(ahci, dev, port, port_num);
	}
}

static ahci_dev_t *ahci_ahci_create(ddf_dev_t *dev)
{
	ahci_dev_t *ahci = malloc(sizeof(ahci_dev_t));
	if (!ahci)
		return NULL;
	
	bzero(ahci, sizeof(ahci_dev_t));
	
	ahci->dev = dev;
	
	hw_res_list_parsed_t hw_res_parsed;
	hw_res_list_parsed_init(&hw_res_parsed);
	if (hw_res_get_list_parsed(dev->parent_sess, &hw_res_parsed, 0) != EOK)
		goto error_get_res_parsed;
	
	/* Register interrupt handler */
	ahci_ranges[0].base = (size_t) hw_res_parsed.mem_ranges.ranges[0].address;
	ahci_ranges[0].size = sizeof(ahci_dev_t);
	ahci_cmds[0].addr =
	    ((uint32_t *) (size_t) hw_res_parsed.mem_ranges.ranges[0].address) + 1;
	ahci_cmds[1].addr =
	    ((uint32_t *) (size_t) hw_res_parsed.mem_ranges.ranges[0].address) + 2;
	
	irq_code_t ct;
	ct.cmdcount = 3;
	ct.cmds = ahci_cmds;
	ct.rangecount = 1;
	ct.ranges = ahci_ranges;
	
	int rc = register_interrupt_handler(dev, hw_res_parsed.irqs.irqs[0],
	    ahci_interrupt, &ct);
	
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed register_interrupt_handler function.");
		goto error_register_interrupt_handler;
	}
	
	if (ahci_pciintel_enable_interrupt(hw_res_parsed.irqs.irqs[0]) != EOK) {
		ddf_msg(LVL_ERROR, "Failed enable interupt.");
		goto error_enable_interrupt;
	}
	
	/* Map AHCI register. */
	physmem_map((void *) (size_t) (hw_res_parsed.mem_ranges.ranges[0].address),
	    8, AS_AREA_READ | AS_AREA_WRITE, (void **) &ahci->memregs);
	hw_res_list_parsed_clean(&hw_res_parsed);
	
	if (ahci->memregs == NULL)
		goto error_map_registers;
	
	ahci->timer = fibril_timer_create();
	
	return ahci;
	
error_map_registers:
error_enable_interrupt:
error_register_interrupt_handler:
	hw_res_list_parsed_clean(&hw_res_parsed);
error_get_res_parsed:
	return NULL;
}

static void ahci_ahci_init(ahci_dev_t *ahci)
{
	/* Enable interrupt and bus mastering */
	ahci_pcireg_cmd_t cmd;
	
	pci_config_space_read_16(ahci->dev->parent_sess, AHCI_PCI_CMD, &cmd.u16);
	cmd.id = 0;
	cmd.bme = 1;
	pci_config_space_write_16(ahci->dev->parent_sess, AHCI_PCI_CMD, cmd.u16);
	
	/* Set master latency timer */
	pci_config_space_write_8(ahci->dev->parent_sess, AHCI_PCI_MLT, 32);
	
	/* Disable command completion coalescing feature. */
	ahci_ghc_ccc_ctl_t ccc;
	ccc.u32 = ahci->memregs->ghc.ccc_ctl;
	ccc.en = 0;
	ahci->memregs->ghc.ccc_ctl = ccc.u32;
	
	/* Enable AHCI and interrupt. */
	ahci->memregs->ghc.ghc = AHCI_GHC_GHC_AE | AHCI_GHC_GHC_IE; 
	
	/* Enable timer. */
	fibril_timer_set(ahci->timer, AHCI_TIMER_TICKS, ahci_timer, ahci);
}

static int ahci_dev_add(ddf_dev_t *dev)	
{
	dev->parent_sess = devman_parent_device_connect(EXCHANGE_SERIALIZE,
	    dev->handle, IPC_FLAG_BLOCKING);
	if (dev->parent_sess == NULL)
		return EINTR;
	
	ahci_dev_t *ahci = ahci_ahci_create(dev);
	if (ahci == NULL)
		return EINTR;
	
	dev->driver_data = ahci;
	ahci_ahci_init(ahci);
	ahci_sata_devices_create(ahci, dev);
	
	return EOK;
}

/*----------------------------------------------------------------------------*/
/*-- Helpers and utilities ---------------------------------------------------*/
/*----------------------------------------------------------------------------*/

static void ahci_get_model_name(uint16_t *src, char *dst)
{
	uint8_t model[40];
	bzero(model, 40);
	
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

static int ahci_pciintel_enable_interrupt(int irq)
{
	async_sess_t *irc_sess = NULL;
	irc_sess = service_connect_blocking(EXCHANGE_SERIALIZE, SERVICE_IRC, 0, 0);
	
	if (!irc_sess)
		return EINTR;
	
	async_exch_t *exch = async_exchange_begin(irc_sess);
	const int rc = async_req_1_0(exch, IRC_ENABLE_INTERRUPT, irq);
	async_exchange_end(exch);
	
	async_hangup(irc_sess);
	return rc;
}

/*----------------------------------------------------------------------------*/
/*-- AHCI Main routine -------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
	printf("%s: HelenOS AHCI device driver\n", NAME);
	ddf_log_init(NAME, LVL_ERROR);
	fibril_mutex_initialize(&sata_devices_count_lock);
	return ddf_driver_main(&ahci_driver);
}
