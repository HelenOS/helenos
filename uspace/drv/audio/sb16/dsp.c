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

#include <devman.h>
#include <device/hw_res.h>
#include <libarch/ddi.h>
#include <libarch/barrier.h>
#include <str_error.h>
#include <bool.h>

#include "dma.h"
#include "ddf_log.h"
#include "dsp_commands.h"
#include "dsp.h"

#define BUFFER_ID 1
#define BUFFER_SIZE (PAGE_SIZE)
#define PLAY_BLOCK_SIZE (BUFFER_SIZE / 2)

#ifndef DSP_RETRY_COUNT
#define DSP_RETRY_COUNT 100
#endif

#define DSP_RESET_RESPONSE 0xaa

#define AUTO_DMA_MODE

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

static inline void sb_dsp_reset(sb_dsp_t *dsp)
{
	assert(dsp);
	/* Reset DSP, see Chapter 2 of Sound Blaster HW programming guide */
	pio_write_8(&dsp->regs->dsp_reset, 1);
	udelay(3); /* Keep reset for 3 us */
	pio_write_8(&dsp->regs->dsp_reset, 0);
}

static inline int sb_setup_dma(sb_dsp_t *dsp, uintptr_t pa, size_t size)
{
	async_sess_t *sess = devman_parent_device_connect(EXCHANGE_ATOMIC,
	    dsp->sb_dev->handle, IPC_FLAG_BLOCKING);
	if (!sess)
		return ENOMEM;

	const int ret = hw_res_dma_channel_setup(sess,
	    dsp->dma16_channel, pa, size,
	    DMA_MODE_READ | DMA_MODE_AUTO | DMA_MODE_ON_DEMAND);
	async_hangup(sess);
	return ret;
}

static inline int sb_setup_buffer(sb_dsp_t *dsp, size_t size)
{
	assert(dsp);
	if (size > BUFFER_SIZE || size == 0 || (size % 2) == 1)
		size = BUFFER_SIZE;
	uint8_t *buffer = dma_create_buffer24(size);
	if (buffer == NULL) {
		ddf_log_error("Failed to allocate DMA buffer.");
		return ENOMEM;
	}

	const uintptr_t pa = addr_to_phys(buffer);
	assert(pa < (1 << 25));
	/* Set 16 bit channel */
	const int ret = sb_setup_dma(dsp, pa, size);
	if (ret == EOK) {
		dsp->buffer.data = buffer;
		dsp->buffer.size = size;
		bzero(dsp->buffer.data, dsp->buffer.size);
	} else {
		ddf_log_error("Failed to setup DMA16 channel: %s.",
		    str_error(ret));
		dma_destroy_buffer(buffer);
	}
	return ret;
}

static inline void sb_clear_buffer(sb_dsp_t *dsp)
{
	dma_destroy_buffer(dsp->buffer.data);
	dsp->buffer.data = NULL;
	dsp->buffer.size = 0;
}

static inline size_t sample_count(unsigned sample_size, size_t byte_count)
{
	if (sample_size == 16) {
		return byte_count / 2;
	}
	return byte_count;
}

int sb_dsp_init(sb_dsp_t *dsp, sb16_regs_t *regs, ddf_dev_t *dev,
    int dma8, int dma16)
{
	assert(dsp);
	dsp->regs = regs;
	dsp->dma8_channel = dma8;
	dsp->dma16_channel = dma16;
	dsp->event_session = NULL;
	dsp->event_exchange = NULL;
	dsp->sb_dev = dev;
	sb_dsp_reset(dsp);
	/* "DSP takes about 100 microseconds to initialize itself" */
	udelay(100);
	uint8_t response;
	const int ret = sb_dsp_read(dsp, &response);
	if (ret != EOK) {
		ddf_log_error("Failed to read DSP reset response value.");
		return ret;
	}
	if (response != DSP_RESET_RESPONSE) {
		ddf_log_error("Invalid DSP reset response: %x.", response);
		return EIO;
	}

	/* Get DSP version number */
	sb_dsp_write(dsp, DSP_VERSION);
	sb_dsp_read(dsp, &dsp->version.major);
	sb_dsp_read(dsp, &dsp->version.minor);

	return ret;
}

void sb_dsp_interrupt(sb_dsp_t *dsp)
{
	assert(dsp);
	if (dsp->event_exchange) {
		ddf_log_verbose("Sending interrupt event.");
		async_msg_0(dsp->event_exchange, IPC_FIRST_USER_METHOD);
	} else {
		ddf_log_warning("Interrupt with no event consumer.");
	}
#ifndef AUTO_DMA_MODE
	sb_dsp_write(dsp, SINGLE_DMA_16B_DA);
	sb_dsp_write(dsp, dsp->playing.mode);
	sb_dsp_write(dsp, (dsp->playing.samples - 1) & 0xff);
	sb_dsp_write(dsp, (dsp->playing.samples - 1) >> 8);
#endif
}

int sb_dsp_get_buffer(sb_dsp_t *dsp, void **buffer, size_t *size, unsigned *id)
{
	assert(dsp);
	assert(size);

	/* buffer is already setup by for someone, refuse to work until
	 * it's released */
	if (dsp->buffer.data)
		return EBUSY;

	const int ret = sb_setup_buffer(dsp, *size);
	if (ret == EOK) {
		ddf_log_debug("Providing buffer(%u): %p, %zu B.",
		    BUFFER_ID, dsp->buffer.data, dsp->buffer.size);

		if (buffer)
			*buffer = dsp->buffer.data;
		if (size)
			*size = dsp->buffer.size;
		if (id)
			*id = BUFFER_ID;
	}
	return ret;
}

int sb_dsp_set_event_session(sb_dsp_t *dsp, unsigned id, async_sess_t *session)
{
	assert(dsp);
	assert(session);
	if (id != BUFFER_ID)
		return ENOENT;
	if (dsp->event_session)
		return EBUSY;
	dsp->event_session = session;
	ddf_log_debug("Set event session.");
	return EOK;
}

int sb_dsp_release_buffer(sb_dsp_t *dsp, unsigned id)
{
	assert(dsp);
	if (id != BUFFER_ID)
		return ENOENT;
	sb_clear_buffer(dsp);
	if (dsp->event_exchange)
		async_exchange_end(dsp->event_exchange);
	dsp->event_exchange = NULL;
	if (dsp->event_session)
		async_hangup(dsp->event_session);
	dsp->event_session = NULL;
	ddf_log_debug("DSP buffer released.");
	return EOK;
}

int sb_dsp_start_playback(sb_dsp_t *dsp, unsigned id, unsigned parts,
    unsigned sampling_rate, unsigned sample_size, unsigned channels, bool sign)
{
	assert(dsp);

	if (!dsp->event_session)
		return EINVAL;

	/* Play block size must be even number (we use DMA 16)*/
	if (dsp->buffer.size % (parts * 2))
		return EINVAL;

	const unsigned play_block_size = dsp->buffer.size / parts;

	/* Check supported parameters */
	ddf_log_debug("Starting playback on buffer(%u): rate: %u, size: %u, "
	    " channels: %u, signed: %s.", id, sampling_rate, sample_size,
	    channels, sign ? "YES" : "NO" );
	if (id != BUFFER_ID)
		return ENOENT;
	if (sample_size != 16) // FIXME We only support 16 bit playback
		return ENOTSUP;
	if (channels != 1 && channels != 2)
		return ENOTSUP;
	if (sampling_rate > 44100)
		return ENOTSUP;

	dsp->event_exchange = async_exchange_begin(dsp->event_session);
	if (!dsp->event_exchange)
		return ENOMEM;

	sb_dsp_write(dsp, SET_SAMPLING_RATE_OUTPUT);
	sb_dsp_write(dsp, sampling_rate >> 8);
	sb_dsp_write(dsp, sampling_rate & 0xff);

	ddf_log_debug("Sampling rate: %hhx:%hhx.",
	    sampling_rate >> 8, sampling_rate & 0xff);

#ifdef AUTO_DMA_MODE
	sb_dsp_write(dsp, AUTO_DMA_16B_DA_FIFO);
#else
	sb_dsp_write(dsp, SINGLE_DMA_16B_DA_FIFO);
#endif

	dsp->playing.mode = 0 |
	    (sign ? DSP_MODE_SIGNED : 0) | (channels == 2 ? DSP_MODE_STEREO : 0);
	sb_dsp_write(dsp, dsp->playing.mode);

	dsp->playing.samples = sample_count(sample_size, play_block_size);
	sb_dsp_write(dsp, (dsp->playing.samples - 1) & 0xff);
	sb_dsp_write(dsp, (dsp->playing.samples - 1) >> 8);

	return EOK;
}

int sb_dsp_stop_playback(sb_dsp_t *dsp, unsigned id)
{
	assert(dsp);
	if (id != BUFFER_ID)
		return ENOENT;
	async_exchange_end(dsp->event_exchange);
	sb_dsp_write(dsp, DMA_16B_EXIT);
	return EOK;
}

int sb_dsp_start_record(sb_dsp_t *dsp, unsigned id, unsigned sample_rate,
    unsigned sample_size, unsigned channels, bool sign)
{
	return ENOTSUP;
}

int sb_dsp_stop_record(sb_dsp_t *dsp, unsigned id)
{
	return ENOTSUP;
}
/**
 * @}
 */
