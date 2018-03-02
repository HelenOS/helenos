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
#include <stdbool.h>
#include <errno.h>
#include <ddi.h>
#include <ddf/log.h>
#include <fibril_synch.h>
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
		{ /* Channel 0 - Unusable*/
			.offset_reg_address = (uint8_t *) 0x00,
			.size_reg_address = (uint8_t *) 0x01,
			.page_reg_address = (uint8_t *) 0x87,
			.single_mask_address = (uint8_t *) 0x0a,
			.mode_address = (uint8_t *) 0x0b,
			.flip_flop_address = (uint8_t *) 0x0c,
		},
		{ /* Channel 1 */
			.offset_reg_address = (uint8_t *) 0x02,
			.size_reg_address = (uint8_t *) 0x03,
			.page_reg_address = (uint8_t *) 0x83,
			.single_mask_address = (uint8_t *) 0x0a,
			.mode_address = (uint8_t *) 0x0b,
			.flip_flop_address = (uint8_t *) 0x0c,
		},
		{ /* Channel 2 */
			.offset_reg_address = (uint8_t *) 0x04,
			.size_reg_address = (uint8_t *) 0x05,
			.page_reg_address = (uint8_t *) 0x81,
			.single_mask_address = (uint8_t *) 0x0a,
			.mode_address = (uint8_t *) 0x0b,
			.flip_flop_address = (uint8_t *) 0x0c,
		},
		{ /* Channel 3 */
			.offset_reg_address = (uint8_t *) 0x06,
			.size_reg_address = (uint8_t *) 0x07,
			.page_reg_address = (uint8_t *) 0x82,
			.single_mask_address = (uint8_t *) 0x0a,
			.mode_address = (uint8_t *) 0x0b,
			.flip_flop_address = (uint8_t *) 0x0c,
		},

		/* The second chip 16-bit */
		{ /* Channel 4 - Unusable */
			.offset_reg_address = (uint8_t *) 0xc0,
			.size_reg_address = (uint8_t *) 0xc2,
			.page_reg_address = (uint8_t *) 0x8f,
			.single_mask_address = (uint8_t *) 0xd4,
			.mode_address = (uint8_t *) 0xd6,
			.flip_flop_address = (uint8_t *) 0xd8,
		},
		{ /* Channel 5 */
			.offset_reg_address = (uint8_t *) 0xc4,
			.size_reg_address = (uint8_t *) 0xc6,
			.page_reg_address = (uint8_t *) 0x8b,
			.single_mask_address = (uint8_t *) 0xd4,
			.mode_address = (uint8_t *) 0xd6,
			.flip_flop_address = (uint8_t *) 0xd8,
		},
		{ /* Channel 6 */
			.offset_reg_address = (uint8_t *) 0xc8,
			.size_reg_address = (uint8_t *) 0xca,
			.page_reg_address = (uint8_t *) 0x89,
			.single_mask_address = (uint8_t *) 0xd4,
			.mode_address = (uint8_t *) 0xd6,
			.flip_flop_address = (uint8_t *) 0xd8,
		},
		{ /* Channel 7 */
			.offset_reg_address = (uint8_t *) 0xcc,
			.size_reg_address = (uint8_t *) 0xce,
			.page_reg_address = (uint8_t *) 0x8a,
			.single_mask_address = (uint8_t *) 0xd4,
			.mode_address = (uint8_t *) 0xd6,
			.flip_flop_address = (uint8_t *) 0xd8,
		},
	},

	.page_table = NULL,
	.first = NULL,
	.second = NULL,
	.initialized = false,
};

/** Initialize I/O access to DMA controller I/O ports.
 *
 * @param controller DMA Controller structure to initialize.
 *
 * @return Error code.
 *
 */
static inline errno_t dma_controller_init(dma_controller_t *controller)
{
	assert(controller);
	errno_t ret = pio_enable(DMA_CONTROLLER_PAGE_BASE, sizeof(dma_page_regs_t),
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

/** Helper function. Channels 4,5,6, and 7 are 8 bit DMA.
 * @pram channel DMA channel.
 * @reutrn True, if channel is 4,5,6, or 7, false otherwise.
 */
static inline bool is_dma16(unsigned channel)
{
	return (channel >= 4) && (channel < 8);
}

/** Helper function. Channels 0,1,2, and 3 are 8 bit DMA.
 * @pram channel DMA channel.
 * @reutrn True, if channel is 0,1,2, or 3, false otherwise.
 */
static inline bool is_dma8(unsigned channel)
{
	return (channel < 4);
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
 */
errno_t dma_channel_setup(unsigned int channel, uint32_t pa, uint32_t size,
    uint8_t mode)
{
	if (!is_dma8(channel) && !is_dma16(channel))
		return ENOENT;

	if ((channel == 0) || (channel == 4))
		return ENOTSUP;

	/* DMA is limited to 24bit addresses. */
	if (pa >= (1 << 24))
		return EINVAL;

	/* 8 bit channels use only 4 bits from the page register. */
	if (is_dma8(channel) && (pa >= (1 << 20)))
		return EINVAL;

	/* Buffers cannot cross 64K page boundaries */
	if ((pa & 0xffff0000) != ((pa + size - 1) & 0xffff0000))
		return EINVAL;

	fibril_mutex_lock(&guard);

	if (!controller_8237.initialized)
		dma_controller_init(&controller_8237);

	if (!controller_8237.initialized) {
		fibril_mutex_unlock(&guard);
		return EIO;
	}

	/* 16 bit transfers are a bit special */
	ddf_msg(LVL_DEBUG, "Unspoiled address %#" PRIx32 " (size %" PRIu32 ")",
	    pa, size);
	if (is_dma16(channel)) {
		/* Size must be aligned to 16 bits */
		if ((size & 1) != 0) {
			fibril_mutex_unlock(&guard);
			return EINVAL;
		}
		/* Size is in 2byte words */
		size >>= 1;
		/* Address is fun: lower 16 bits need to be shifted by 1 */
		pa = ((pa & 0xffff) >> 1) | (pa & 0xff0000);
	}

	const dma_channel_t dma_channel = controller_8237.channels[channel];

	ddf_msg(LVL_DEBUG, "Setting channel %u to address %#" PRIx32 " "
	    "(size %" PRIu32 "), mode %hhx.", channel, pa, size, mode);

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

/** Query remaining buffer size.
 *
 * @param channel DMA Channel 1, 2, 3 for 8 bit transfers,
 *                    5, 6, 7 for 16 bit.
 * @param size    Place to store number of bytes pending in the assigned buffer.
 *
 * @return Error code.
 */
errno_t dma_channel_remain(unsigned channel, size_t *size)
{
	assert(size);
	if (!is_dma8(channel) && !is_dma16(channel))
		return ENOENT;

	if ((channel == 0) || (channel == 4))
		return ENOTSUP;

	fibril_mutex_lock(&guard);
	if (!controller_8237.initialized) {
		fibril_mutex_unlock(&guard);
		return EIO;
	}

	const dma_channel_t dma_channel = controller_8237.channels[channel];
	/* Get size - reset flip-flop */
	pio_write_8(dma_channel.flip_flop_address, 0);

	/* Low byte */
	const uint8_t value_low = pio_read_8(dma_channel.size_reg_address);
	ddf_msg(LVL_DEBUG2, "Read size low byte: %p:%x.",
	    dma_channel.size_reg_address, value_low);

	/* High byte */
	const uint8_t value_high = pio_read_8(dma_channel.size_reg_address);
	ddf_msg(LVL_DEBUG2, "Read size high byte: %p:%x.",
	    dma_channel.size_reg_address, value_high);
	fibril_mutex_unlock(&guard);

	uint16_t remain = (value_high << 8 | value_low) ;
	/* 16 bit DMA size is in words,
	 * the upper bits are bogus for 16bit transfers so we need to get
	 * rid of them. Using limited type works well.*/
	if (is_dma16(channel))
		remain <<= 1;
	*size =  is_dma16(channel) ? remain + 2: remain + 1;
	return EOK;
}
/**
 * @}
 */
