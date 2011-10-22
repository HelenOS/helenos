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
 * @brief DSP helper functions implementation
 */

#include <libarch/ddi.h>
#include <str_error.h>

#include "dma.h"
#include "dma_controller.h"
#include "ddf_log.h"
#include "dsp_commands.h"
#include "dsp.h"

#define BUFFER_SIZE (PAGE_SIZE / 4)
#define PLAY_BLOCK_SIZE (BUFFER_SIZE / 2)

#ifndef DSP_RETRY_COUNT
#define DSP_RETRY_COUNT 100
#endif

#define DSP_RESET_RESPONSE 0xaa
#define SB_DMA_CHAN_16 5
#define SB_DMA_CHAN_8 1

#define AUTO_DMA_MODE

#ifdef AUTO_DMA_MODE
#undef AUTO_DMA_MODE
#define AUTO_DMA_MODE true
#else
#define AUTO_DMA_MODE false
#endif

static inline int sb_dsp_read(sb_dsp_t *dsp, uint8_t *data)
{
	assert(data);
	assert(dsp);
	uint8_t status;
	size_t attempts = DSP_RETRY_COUNT;
	do {
		status = pio_read_8(&dsp->regs->dsp_read_status);
	} while (--attempts && ((status & DSP_READ_READY) == 0));

	if ((status & DSP_READ_READY) == 0)
		return EIO;

	*data = pio_read_8(&dsp->regs->dsp_data_read);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static inline int sb_dsp_write(sb_dsp_t *dsp, uint8_t data)
{
	assert(dsp);
	uint8_t status;
	size_t attempts = DSP_RETRY_COUNT;
	do {
		status = pio_read_8(&dsp->regs->dsp_write);
	} while (--attempts && ((status & DSP_WRITE_BUSY) != 0));

	if ((status & DSP_WRITE_BUSY))
		return EIO;

	pio_write_8(&dsp->regs->dsp_write, data);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static inline void sb_dsp_reset(sb_dsp_t *dsp)
{
	assert(dsp);
	/* Reset DSP, see Chapter 2 of Sound Blaster HW programming guide */
	pio_write_8(&dsp->regs->dsp_reset, 1);
	udelay(3); /* Keep reset for 3 us */
	pio_write_8(&dsp->regs->dsp_reset, 0);
}
/*----------------------------------------------------------------------------*/
static inline int sb_setup_buffer(sb_dsp_t *dsp)
{
	assert(dsp);
	uint8_t *buffer = malloc24(BUFFER_SIZE);
	if (buffer == NULL) {
		ddf_log_error("Failed to allocate buffer.\n");
		return ENOMEM;
	}

	const uintptr_t pa = addr_to_phys(buffer);
	assert(pa < (1 << 25));
//	assert((pa & 0xffff) == 0);
	/* Set 16 bit channel */
	const int ret = dma_setup_channel(SB_DMA_CHAN_16, pa, BUFFER_SIZE);
	if (ret == EOK) {
		dsp->buffer.data = buffer;
		dsp->buffer.position = buffer;
		dsp->buffer.size = BUFFER_SIZE;
		bzero(buffer, BUFFER_SIZE);
		dma_prepare_channel(
		    SB_DMA_CHAN_16, false, AUTO_DMA_MODE, BLOCK_DMA);
		/* Set 8bit channel */
		const int ret = dma_setup_channel(SB_DMA_CHAN_8, pa, BUFFER_SIZE);
		if (ret == EOK) {
			dma_prepare_channel(
			    SB_DMA_CHAN_8, false, AUTO_DMA_MODE, BLOCK_DMA);
		} else {
			ddf_log_warning("Failed to setup DMA8 channel: %s.\n",
			    str_error(ret));
		}
	} else {
		ddf_log_error("Failed to setup DMA16 channel %s.\n",
		    str_error(ret));
		free24(buffer);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
static inline void sb_clear_buffer(sb_dsp_t *dsp)
{
	free24(dsp->buffer.data);
	dsp->buffer.data = NULL;
	dsp->buffer.position = NULL;
	dsp->buffer.size = 0;
}
/*----------------------------------------------------------------------------*/
int sb_dsp_init(sb_dsp_t *dsp, sb16_regs_t *regs)
{
	assert(dsp);
	dsp->regs = regs;
	sb_dsp_reset(dsp);
	/* "DSP takes about 100 microseconds to initialize itself" */
	udelay(100);
	uint8_t response;
	const int ret = sb_dsp_read(dsp, &response);
	if (ret != EOK) {
		ddf_log_error("Failed to read DSP reset response value.\n");
		return ret;
	}
	if (response != DSP_RESET_RESPONSE) {
		ddf_log_error("Invalid DSP reset response: %x.\n", response);
		return EIO;
	}

	/* Get DSP version number */
	sb_dsp_write(dsp, DSP_VERSION);
	sb_dsp_read(dsp, &dsp->version.major);
	sb_dsp_read(dsp, &dsp->version.minor);

	return ret;
}
/*----------------------------------------------------------------------------*/
void sb_dsp_interrupt(sb_dsp_t *dsp)
{
	assert(dsp);
	/* We don't really care about the mode of transport, so ack both */
	if (dsp->version.major >= 4) {
		/* ACK dma16 transfer interrupt */
		pio_read_8(&dsp->regs->dma16_ack);
	}
	/* ACK dma8 transfer interrupt */
	pio_read_8(&dsp->regs->dsp_read_status);

	static size_t interrupt_count = 0;

	const size_t remain_size = dsp->playing.size -
	    (dsp->playing.position - dsp->playing.data);

	ddf_log_note("Interrupt count %zu, remaining: %zu.\n",
	    ++interrupt_count, remain_size);
	if (remain_size == 0) {
		ddf_log_note("Nothing more to play");
		if (AUTO_DMA_MODE) {
			sb_dsp_write(dsp, DMA_16B_EXIT);
		}
		sb_clear_buffer(dsp);
		return;
	}
	if (remain_size <= PLAY_BLOCK_SIZE) {
		ddf_log_note("Last %zu bytes to play.\n", remain_size);
		/* This is the last block */
		memcpy(dsp->buffer.position, dsp->playing.position, remain_size);
		dsp->playing.position += remain_size;
		dsp->buffer.position += remain_size;
		sb_dsp_write(dsp, SINGLE_DMA_16B_DA);
		sb_dsp_write(dsp, dsp->playing.mode);
		sb_dsp_write(dsp, (remain_size - 1) & 0xff);
		sb_dsp_write(dsp, (remain_size - 1) >> 8);
		return;
	}
	ddf_log_note("Playing full block.\n");
	memcpy(dsp->buffer.position, dsp->playing.position, PLAY_BLOCK_SIZE);
	dsp->playing.position += PLAY_BLOCK_SIZE;
	dsp->buffer.position += PLAY_BLOCK_SIZE;
	/* Wrap around */
	if (dsp->buffer.position == (dsp->buffer.data + dsp->buffer.size))
		dsp->buffer.position = dsp->buffer.data;
	if (!AUTO_DMA_MODE) {
		sb_dsp_write(dsp, SINGLE_DMA_16B_DA_FIFO);
		sb_dsp_write(dsp, dsp->playing.mode);
		sb_dsp_write(dsp, (PLAY_BLOCK_SIZE - 1) & 0xff);
		sb_dsp_write(dsp, (PLAY_BLOCK_SIZE - 1) >> 8);
	}

}
/*----------------------------------------------------------------------------*/
int sb_dsp_play_direct(sb_dsp_t *dsp, const uint8_t *data, size_t size,
    unsigned sampling_rate, unsigned channels, unsigned bit_depth)
{
	assert(dsp);
	if (channels != 1 || bit_depth != 8)
		return EIO;
	/* In microseconds */
	const unsigned wait_period = 1000000 / sampling_rate;
	while (size--) {
		pio_write_8(&dsp->regs->dsp_write, DIRECT_8B_OUTPUT);
		pio_write_8(&dsp->regs->dsp_write, *data++);
		udelay(wait_period);
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
int sb_dsp_play(sb_dsp_t *dsp, const uint8_t *data, size_t size,
    uint16_t sampling_rate, unsigned channels, unsigned bit_depth)
{
	assert(dsp);
	if (!data)
		return EOK;

	/* Check supported parameters */
	if (bit_depth != 8 && bit_depth != 16)
		return ENOTSUP;
	if (channels != 1 && channels != 2)
		return ENOTSUP;

	ddf_log_debug("Buffer prepare.\n");
	const int ret = sb_setup_buffer(dsp);
	if (ret != EOK)
		return ret;

	const size_t play_size =
	    size < PLAY_BLOCK_SIZE ? size : PLAY_BLOCK_SIZE;
	const size_t copy_size =
	    size < BUFFER_SIZE ? size : BUFFER_SIZE;


	dsp->playing.data = data;
	dsp->playing.position = data + copy_size;
	dsp->playing.size = size;
	dsp->playing.mode =
	    (bit_depth == 16 ? 0x10 : 0) | (channels == 2 ? 0x20 : 0);
	ddf_log_debug("Setting mode %hhx.\n", dsp->playing.mode);

	ddf_log_debug("Playing sound: %zu(%zu) bytes.\n", play_size, size);
	memcpy(dsp->buffer.data, dsp->playing.data, copy_size);

	sb_dsp_write(dsp, SET_SAMPLING_RATE_OUTPUT);
	sb_dsp_write(dsp, sampling_rate >> 8);
	sb_dsp_write(dsp, sampling_rate & 0xff);

	ddf_log_debug("Set sampling rate %hhx:%hhx.\n",
	    sampling_rate >> 8, sampling_rate & 0xff);

	if (AUTO_DMA_MODE) {
		sb_dsp_write(dsp, AUTO_DMA_16B_DA_FIFO);
	} else {
		sb_dsp_write(dsp, SINGLE_DMA_16B_DA_FIFO);
	}
	sb_dsp_write(dsp, dsp->playing.mode);
	sb_dsp_write(dsp, (play_size - 1) & 0xff);
	sb_dsp_write(dsp, (play_size - 1) >> 8);

	return EOK;
}
/**
 * @}
 */
