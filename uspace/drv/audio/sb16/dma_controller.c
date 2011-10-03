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
/** @addtogroup drvaudiosb16
 * @{
 */
/** @file
 * @brief DMA memory management
 */
#include <assert.h>
#include <errno.h>

#include "dma_controller.h"

#define DMA_CONTROLLER_FIRST_BASE 0x0
typedef struct dma_controller_regs_first {
	uint8_t channel_start0;
	uint8_t channel_count0;
	uint8_t channel_start1;
	uint8_t channel_count1;
	uint8_t channel_start2;
	uint8_t channel_count2;
	uint8_t channel_start3;
	uint8_t channel_count3;

	uint8_t command_status;
#define DMA_STATUS_REQ(x) (1 << ((x % 4) + 4))
#define DMA_STATUS_COMPLETE(x) (1 << (x % 4))
/* http://wiki.osdev.org/DMA: The only bit that works is COND(bit 2) */
#define DMA_COMMAND_COND (1 << 2) /* Disables DMA controller */

	uint8_t request; /* Memory to memory transfers, NOT implemented on PCs*/
	uint8_t single_mask;
#define DMA_SINGLE_MASK_CHAN_SELECT_MASK (0x3)
#define DMA_SINGLE_MASK_CHAN_SELECT_SHIFT (0)
#define DMA_SINGLE_MASK_MASK_ON_FLAG (1 << 2)

	uint8_t mode;
#define DMA_MODE_CHAN_SELECT_MASK (0x3)
#define DMA_MODE_CHAN_SELECT_SHIFT (0)
#define DMA_MODE_CHAN_TRA_MASK (0x3)
#define DMA_MODE_CHAN_TRA_SHIFT (2)
#define DMA_MODE_CHAN_TRA_SELF_TEST (0)
#define DMA_MODE_CHAN_TRA_WRITE (1)
#define DMA_MODE_CHAN_TRA_READ (2)
#define DMA_MODE_CHAN_AUTO_FLAG (1 << 4)
#define DMA_MODE_CHAN_DOWN_FLAG (1 << 5)
#define DMA_MODE_CHAN_MOD_MASK (0x3)
#define DMA_MODE_CHAN_MOD_SHIFT (6)
#define DMA_MODE_CHAN_MOD_DEMAND (0)
#define DMA_MODE_CHAN_MOD_SINGLE (1)
#define DMA_MODE_CHAN_MOD_BLOCK (2)
#define DMA_MODE_CHAN_MOD_CASCADE (3)

	uint8_t flip_flop;
	uint8_t master_reset; /* Intermediate is not implemented on PCs */
	uint8_t mask_reset;
/* Master reset sets Flip-Flop low, clears status,sets all mask bits on */

	uint8_t multi_mask;
#define DMA_MULTI_MASK_CHAN(x) (1 << (x % 4))

} dma_controller_regs_first_t;

#define DMA_CONTROLLER_SECOND_BASE 0xc0
/* See dma_controller_regs_first_t for register values */
typedef struct dma_controller_regs_second {
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
	uint8_t master_reset_intermediate;
	uint8_t reservedd;
	uint8_t multi_mask;
} dma_controller_regs_second_t;

#define DMA_CONTROLLER_PAGE_BASE 0x81
typedef struct dma_page_regs {
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

typedef struct dma_channel {
	uint8_t offset_reg_address;
	uint8_t size_reg_address;
	uint8_t page_reg_address;
} dma_channel_t;

typedef struct dma_controller {
	dma_channel_t channel[8];
	dma_page_regs_t *page_table;
	dma_controller_regs_first_t *first;
	dma_controller_regs_second_t *second;
} dma_controller_t;

dma_controller_t controller_8237 = {
	.channel = {
	    { 0x00, 0x01, 0x87 }, { 0x02, 0x03, 0x83 },
	    { 0x04, 0x05, 0x81 }, { 0x06, 0x07, 0x82 },
	    { 0xc0, 0xc2, 0x8f }, { 0xc4, 0xc6, 0x8b },
	    { 0xc8, 0xca, 0x89 }, { 0xcc, 0xce, 0x8a } },
	.page_table = NULL,
	.first = NULL,
	.second = NULL,
};

static inline dma_controller_t *dma_controller_init()
{
	return NULL;
}
/*----------------------------------------------------------------------------*/
int dma_setup_channel(unsigned channel, uintptr_t pa, size_t size)
{
	static dma_controller_t *controller = NULL;
	if (!controller)
		return EIO;
	return ENOTSUP;
}
/**
 * @}
 */
