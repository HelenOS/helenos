/*
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup isa
 * @{
 */

/** @file
 * @brief DMA controller management
 */

#include <assert.h>
#include <bool.h>
#include <errno.h>
#include <fibril_synch.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <ddf/log.h>
#include "i8237.h"

/** DMA Slave controller I/O Address. */
#define DMA_CONTROLLER_FIRST_BASE  ((void *) 0x00)

/** DMA Master controller I/O Address. */
#define DMA_CONTROLLER_SECOND_BASE  ((void *) 0xc0)

/** Shared DMA page address register I/O address. */
#define DMA_CONTROLLER_PAGE_BASE  ((void *) 0x81)

#define DMA_STATUS_REQ(x)       (1 << (((x) % 4) + 4))
#define DMA_STATUS_COMPLETE(x)  (1 << ((x) % 4))

/** http://wiki.osdev.org/DMA: The only bit that works is COND (bit 2) */
#define DMA_COMMAND_COND  (1 << 2)  /**< Disables DMA controller */

#define DMA_SINGLE_MASK_CHAN_SEL_MASK   0x03
#define DMA_SINGLE_MASK_CHAN_SEL_SHIFT  0

#define DMA_SINGLE_MASK_CHAN_TO_REG(x) \
	(((x) & DMA_SINGLE_MASK_CHAN_SEL_MASK) << DMA_SINGLE_MASK_CHAN_SEL_SHIFT)

#define DMA_SINGLE_MASK_MASKED_FLAG  (1 << 2)

#define DMA_MODE_CHAN_SELECT_MASK   0x03
#define DMA_MODE_CHAN_SELECT_SHIFT  0

#define DMA_MODE_CHAN_TO_REG(x) \
	(((x) & DMA_MODE_CHAN_SELECT_MASK) << DMA_MODE_CHAN_SELECT_SHIFT)

#define DMA_MODE_CHAN_TRA_MASK       0x03
#define DMA_MODE_CHAN_TRA_SHIFT      2
#define DMA_MODE_CHAN_TRA_SELF_TEST  0
#define DMA_MODE_CHAN_TRA_WRITE      0x01
#define DMA_MODE_CHAN_TRA_READ       0x02

#define DMA_MODE_CHAN_AUTO_FLAG  (1 << 4)
#define DMA_MODE_CHAN_DOWN_FLAG  (1 << 5)

#define DMA_MODE_CHAN_MODE_MASK     0x03
#define DMA_MODE_CHAN_MODE_SHIFT    6
#define DMA_MODE_CHAN_MODE_DEMAND   0
#define DMA_MODE_CHAN_MODE_SINGLE   1
#define DMA_MODE_CHAN_MODE_BLOCK    2
#define DMA_MODE_CHAN_MODE_CASCADE  3

#define DMA_MULTI_MASK_CHAN(x)  (1 << ((x) % 4))

typedef struct {
	uint8_t channel_start0;
	uint8_t channel_count0;
	uint8_t channel_start1;
	uint8_t channel_count1;
	uint8_t channel_start2;
	uint8_t channel_count2;
	uint8_t channel_start3;
	uint8_t channel_count3;
	
	uint8_t command_status;
	
	/** Memory to memory transfers, NOT implemented on PCs */
	uint8_t request;
	uint8_t single_mask;
	uint8_t mode;
	uint8_t flip_flop;
	
	/*
	 * Master reset sets Flip-Flop low, clears status,
	 * sets all mask bits on.
	 *
	 * Intermediate is not implemented on PCs.
	 *
	 */
	uint8_t master_reset;
	uint8_t mask_reset;
	uint8_t multi_mask;
} dma_controller_regs_first_t;

typedef struct {
	uint8_t channel_start4;
	uint8_t reserved0;
	uint8_t channel_count4;
	uint8_t reserved1;
	uint8_t channel_start5;
	uint8_t reserved2;
	uint8_t channel_count5;
	uint8_t reserved3;
	uint8_t channel_start6;
	uint8_t reserved4;
	uint8_t channel_count6;
	uint8_t reserved5;
	uint8_t channel_start7;
	uint8_t reserved6;
	uint8_t channel_count7;
	
	uint8_t command_status;
	uint8_t reserved8;
	uint8_t request;
	uint8_t reserved9;
	uint8_t single_mask;
	uint8_t reserveda;
	uint8_t mode;
	uint8_t reservedb;
	uint8_t flip_flop;
	uint8_t reservedc;
	uint8_t master_reset;
	uint8_t reservedd;
	uint8_t multi_mask;
} dma_controller_regs_second_t;

typedef struct {
	uint8_t channel2;
	uint8_t channel3;
	uint8_t channel1;
	uint8_t reserved0;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t channel0;
	uint8_t reserved3;
	uint8_t channel6;
	uint8_t channel7;
	uint8_t channel5;
	uint8_t reserved4;
	uint8_t reserved5;
	uint8_t reserved6;
	uint8_t channel4;
} dma_page_regs_t;

/** Addresses needed to setup a DMA channel. */
typedef struct {
	ioport8_t *offset_reg_address;
	ioport8_t *size_reg_address;
	ioport8_t *page_reg_address;
	ioport8_t *single_mask_address;
	ioport8_t *mode_address;
	ioport8_t *flip_flop_address;
} dma_channel_t;

typedef struct {
	dma_channel_t channels[8];
	dma_page_regs_t *page_table;
	dma_controller_regs_first_t *first;
	dma_controller_regs_second_t *second;
	bool initialized;
} dma_controller_t;

static fibril_mutex_t guard = FIBRIL_MUTEX_INITIALIZER(guard);

/** Standard i8237 DMA controller.
 *
 * http://zet.aluzina.org/index.php/8237_DMA_controller#DMA_Channel_Registers
 *
 */
static dma_controller_t controller_8237 = {
	.channels = {
		/* The first chip 8-bit */
		{
			(uint8_t *) 0x00,
			(uint8_t *) 0x01,
			(uint8_t *) 0x87,
			(uint8_t *) 0x0a,
			(uint8_t *) 0x0b,
			(uint8_t *) 0x0c,
		},
		{
			(uint8_t *) 0x02,
			(uint8_t *) 0x03,
			(uint8_t *) 0x83,
			(uint8_t *) 0x0a,
			(uint8_t *) 0x0b,
			(uint8_t *) 0x0c,
		},
		{
			(uint8_t *) 0x04,
			(uint8_t *) 0x05,
			(uint8_t *) 0x81,
			(uint8_t *) 0x0a,
			(uint8_t *) 0x0b,
			(uint8_t *) 0x0c,
		},
		{
			(uint8_t *) 0x06,
			(uint8_t *) 0x07,
			(uint8_t *) 0x82,
			(uint8_t *) 0x0a,
			(uint8_t *) 0x0b,
			(uint8_t *) 0x0c,
		},
		
		/* The second chip 16-bit */
		{
			(uint8_t *) 0xc0,
			(uint8_t *) 0xc2,
			(uint8_t *) 0x8f,
			(uint8_t *) 0xd4,
			(uint8_t *) 0xd6,
			(uint8_t *) 0xd8,
		},
		{
			(uint8_t *) 0xc4,
			(uint8_t *) 0xc6,
			(uint8_t *) 0x8b,
			(uint8_t *) 0xd4,
			(uint8_t *) 0xd6,
			(uint8_t *) 0xd8,
		},
		{
			(uint8_t *) 0xc8,
			(uint8_t *) 0xca,
			(uint8_t *) 0x89,
			(uint8_t *) 0xd4,
			(uint8_t *) 0xd6,
			(uint8_t *) 0xd8,
		},
		{
			(uint8_t *) 0xcc,
			(uint8_t *) 0xce,
			(uint8_t *) 0x8a,
			(uint8_t *) 0xd4,
			(uint8_t *) 0xd6,
			(uint8_t *) 0xd8,
		},
	},
	
	.page_table = NULL,
	.first = NULL,
	.second = NULL,
	.initialized = false,
};

/* Initialize I/O access to DMA controller I/O ports.
 *
 * @param controller DMA Controller structure to initialize.
 *
 * @return Error code.
 *
 */
static inline int dma_controller_init(dma_controller_t *controller)
{
	assert(controller);
	int ret = pio_enable(DMA_CONTROLLER_PAGE_BASE, sizeof(dma_page_regs_t),
	    (void **) &controller->page_table);
	if (ret != EOK)
		return EIO;
	
	ret = pio_enable(DMA_CONTROLLER_FIRST_BASE,
	    sizeof(dma_controller_regs_first_t), (void **) &controller->first);
	if (ret != EOK)
		return EIO;
	
	ret = pio_enable(DMA_CONTROLLER_SECOND_BASE,
		sizeof(dma_controller_regs_second_t), (void **) &controller->second);
	if (ret != EOK)
		return EIO;
	
	controller->initialized = true;
	
	/* Reset the controller */
	pio_write_8(&controller->second->master_reset, 0xff);
	pio_write_8(&controller->first->master_reset, 0xff);
	
	return EOK;
}

/** Setup DMA channel to specified place and mode.
 *
 * @param channel DMA Channel 1, 2, 3 for 8 bit transfers,
 *                    5, 6, 7 for 16 bit.
 * @param pa      Physical address of the buffer. Must be < 16 MB
 *                for 16 bit and < 1 MB for 8 bit transfers.
 * @param size    DMA buffer size, limited to 64 KB.
 * @param mode    Mode of the DMA channel:
 *                - Read or Write
 *                - Allow automatic reset
 *                - Use address decrement instead of increment
 *                - Use SINGLE/BLOCK/ON DEMAND transfer mode
 *
 * @return Error code.
 *
 */
int dma_setup_channel(unsigned int channel, uint32_t pa, uint16_t size,
    uint8_t mode)
{
	if ((channel == 0) || (channel == 4))
		return ENOTSUP;
	
	if (channel > 7)
		return ENOENT;
	
	/* DMA is limited to 24bit addresses. */
	if (pa >= (1 << 24))
		return EINVAL;
	
	/* 8 bit channels use only 4 bits from the page register. */
	if ((channel > 0) && (channel < 4) && (pa >= (1 << 20)))
		return EINVAL;
	
	fibril_mutex_lock(&guard);
	
	if (!controller_8237.initialized)
		dma_controller_init(&controller_8237);
	
	if (!controller_8237.initialized) {
		fibril_mutex_unlock(&guard);
		return EIO;
	}
	
	/* 16 bit transfers are a bit special */
	ddf_msg(LVL_DEBUG, "Unspoiled address: %p and size: %zu.", pa, size);
	if (channel > 4) {
		/* Size must be aligned to 16 bits */
		if ((size & 1) != 0) {
			fibril_mutex_unlock(&guard);
			return EINVAL;
		}
		
		size >>= 1;
		
		/* Address is fun: lower 16 bits need to be shifted by 1 */
		pa = ((pa & 0xffff) >> 1) | (pa & 0xff0000);
	}
	
	const dma_channel_t dma_channel = controller_8237.channels[channel];
	
	ddf_msg(LVL_DEBUG, "Setting channel %u, to address %p(%zu), mode %hhx.",
	    channel, pa, size, mode);
	
	/* Mask DMA request */
	uint8_t value = DMA_SINGLE_MASK_CHAN_TO_REG(channel) |
	    DMA_SINGLE_MASK_MASKED_FLAG;
	pio_write_8(dma_channel.single_mask_address, value);
	
	/* Set mode */
	value = DMA_MODE_CHAN_TO_REG(channel) | mode;
	ddf_msg(LVL_DEBUG2, "Writing mode byte: %p:%hhx.",
	    dma_channel.mode_address, value);
	pio_write_8(dma_channel.mode_address, value);
	
	/* Set address - reset flip-flop */
	pio_write_8(dma_channel.flip_flop_address, 0);
	
	/* Low byte */
	value = pa & 0xff;
	ddf_msg(LVL_DEBUG2, "Writing address low byte: %p:%hhx.",
	    dma_channel.offset_reg_address, value);
	pio_write_8(dma_channel.offset_reg_address, value);
	
	/* High byte */
	value = (pa >> 8) & 0xff;
	ddf_msg(LVL_DEBUG2, "Writing address high byte: %p:%hhx.",
	    dma_channel.offset_reg_address, value);
	pio_write_8(dma_channel.offset_reg_address, value);
	
	/* Page address - third byte */
	value = (pa >> 16) & 0xff;
	ddf_msg(LVL_DEBUG2, "Writing address page byte: %p:%hhx.",
	    dma_channel.page_reg_address, value);
	pio_write_8(dma_channel.page_reg_address, value);
	
	/* Set size - reset flip-flop */
	pio_write_8(dma_channel.flip_flop_address, 0);
	
	/* Low byte */
	value = (size - 1) & 0xff;
	ddf_msg(LVL_DEBUG2, "Writing size low byte: %p:%hhx.",
	    dma_channel.size_reg_address, value);
	pio_write_8(dma_channel.size_reg_address, value);
	
	/* High byte */
	value = ((size - 1) >> 8) & 0xff;
	ddf_msg(LVL_DEBUG2, "Writing size high byte: %p:%hhx.",
	    dma_channel.size_reg_address, value);
	pio_write_8(dma_channel.size_reg_address, value);
	
	/* Unmask DMA request */
	value = DMA_SINGLE_MASK_CHAN_TO_REG(channel);
	pio_write_8(dma_channel.single_mask_address, value);
	
	fibril_mutex_unlock(&guard);
	
	return EOK;
}

/**
 * @}
 */
