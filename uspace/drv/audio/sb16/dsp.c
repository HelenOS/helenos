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
/*----------------------------------------------------------------------------*/
static inline int sb_setup_buffer(sb_dsp_t *dsp)
{
	assert(dsp);
	uint8_t *buffer = dma_create_buffer24(BUFFER_SIZE);
	if (buffer == NULL) {
		ddf_log_error("Failed to allocate buffer.\n");
		return ENOMEM;
	}

	const uintptr_t pa = addr_to_phys(buffer);
	assert(pa < (1 << 25));
	/* Set 16 bit channel */
	const int ret = sb_setup_dma(dsp, pa, BUFFER_SIZE);
	if (ret == EOK) {
		dsp->buffer.data = buffer;
		dsp->buffer.position = buffer;
		dsp->buffer.size = BUFFER_SIZE;
		bzero(buffer, BUFFER_SIZE);
	} else {
		ddf_log_error("Failed to setup DMA16 channel %s.\n",
		    str_error(ret));
		dma_destroy_buffer(buffer);
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
static inline void sb_clear_buffer(sb_dsp_t *dsp)
{
	dma_destroy_buffer(dsp->buffer.data);
	dsp->buffer.data = NULL;
	dsp->buffer.position = NULL;
	dsp->buffer.size = 0;
}
/*----------------------------------------------------------------------------*/
static inline size_t sample_count(unsigned sample_size, size_t byte_count)
{
	if (sample_size == 16) {
		return byte_count / 2;
	}
	return byte_count;
}
/*----------------------------------------------------------------------------*/
int sb_dsp_init(sb_dsp_t *dsp, sb16_regs_t *regs, ddf_dev_t *dev,
    int dma8, int dma16)
{
	assert(dsp);
	dsp->regs = regs;
	dsp->dma8_channel = dma8;
	dsp->dma16_channel = dma16;
	dsp->sb_dev = dev;
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
#if 0
	assert(dsp);
	const size_t remain_size =
	    dsp->playing.size - (dsp->playing.position - dsp->playing.data);

	if (remain_size == 0) {
#ifdef AUTO_DMA_MODE
		sb_dsp_write(dsp, DMA_16B_EXIT);
#endif
		ddf_log_note("Nothing more to play");
		sb_clear_buffer(dsp);
		return;
	}
	if (remain_size <= PLAY_BLOCK_SIZE) {
		ddf_log_note("Last %zu bytes to play.\n", remain_size);
		/* This is the last block */
		memcpy(dsp->buffer.position, dsp->playing.position, remain_size);
		write_barrier();
		dsp->playing.position += remain_size;
		dsp->buffer.position += remain_size;
		const size_t samples =
		    sample_count(dsp->playing.mode, remain_size);
		sb_dsp_write(dsp, SINGLE_DMA_16B_DA);
		sb_dsp_write(dsp, dsp->playing.mode);
		sb_dsp_write(dsp, (samples - 1) & 0xff);
		sb_dsp_write(dsp, (samples - 1) >> 8);
		return;
	}
	/* Copy new data */
	memcpy(dsp->buffer.position, dsp->playing.position, PLAY_BLOCK_SIZE);
	write_barrier();
	/* Adjust position */
	dsp->playing.position += PLAY_BLOCK_SIZE;
	dsp->buffer.position += PLAY_BLOCK_SIZE;
	/* Wrap around */
	if (dsp->buffer.position == (dsp->buffer.data + dsp->buffer.size))
		dsp->buffer.position = dsp->buffer.data;
#ifndef AUTO_DMA_MODE
	const size_t samples = sample_count(dsp->playing.mode, PLAY_BLOCK_SIZE);
	sb_dsp_write(dsp, SINGLE_DMA_16B_DA_FIFO);
	sb_dsp_write(dsp, dsp->playing.mode);
	sb_dsp_write(dsp, (samples - 1) & 0xff);
	sb_dsp_write(dsp, (samples - 1) >> 8);
#endif
#endif
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
int sb_dsp_play(sb_dsp_t *dsp, const void *data, size_t size,
    uint16_t sampling_rate, unsigned channels, unsigned sample_size)
{
	assert(dsp);
	if (!data)
		return EOK;

	/* Check supported parameters */
	if (sample_size != 16) // FIXME We only support 16 bit playback
		return ENOTSUP;
	if (channels != 1 && channels != 2)
		return ENOTSUP;

	ddf_log_debug("Buffer prepare.\n");
	const int ret = sb_setup_buffer(dsp);
	if (ret != EOK)
		return ret;

	const size_t copy_size = size < BUFFER_SIZE ? size : BUFFER_SIZE;
	const size_t play_size =
	    size < PLAY_BLOCK_SIZE ? size : PLAY_BLOCK_SIZE;

	dsp->playing.data = data;
	dsp->playing.position = data + copy_size;
	dsp->playing.size = size;
	dsp->playing.mode = 0;

		dsp->playing.mode |= DSP_MODE_SIGNED;
	if (channels == 2)
		dsp->playing.mode |= DSP_MODE_STEREO;

	const size_t samples = sample_count(sample_size, play_size);

	ddf_log_debug("Playing %s sound: %zu(%zu) bytes => %zu samples.\n",
	    mode_to_str(dsp->playing.mode), play_size, size, samples);

	memcpy(dsp->buffer.data, dsp->playing.data, copy_size);
	write_barrier();

	sb_dsp_write(dsp, SET_SAMPLING_RATE_OUTPUT);
	sb_dsp_write(dsp, sampling_rate >> 8);
	sb_dsp_write(dsp, sampling_rate & 0xff);

	ddf_log_debug("Sampling rate: %hhx:%hhx.\n",
	    sampling_rate >> 8, sampling_rate & 0xff);

#ifdef AUTO_DMA_MODE
	if (play_size < size) {
		sb_dsp_write(dsp, AUTO_DMA_16B_DA_FIFO);
	} else {
		sb_dsp_write(dsp, SINGLE_DMA_16B_DA_FIFO);
	}
#else
	sb_dsp_write(dsp, SINGLE_DMA_16B_DA_FIFO);
#endif

	sb_dsp_write(dsp, dsp->playing.mode);
	sb_dsp_write(dsp, (samples - 1) & 0xff);
	sb_dsp_write(dsp, (samples - 1) >> 8);

	return EOK;
}
/*----------------------------------------------------------------------------*/
int sb_dsp_get_buffer(sb_dsp_t *dsp, void **buffer, size_t *size, unsigned *id)
{
	assert(dsp);
	const int ret = sb_setup_buffer(dsp);
	ddf_log_debug("Providing buffer(%u): %p, %zu.\n",
	    BUFFER_ID, dsp->buffer.data, dsp->buffer.size);
	if (ret == EOK && buffer)
		*buffer = dsp->buffer.data;
	if (ret == EOK && size)
		*size = dsp->buffer.size;
	if (ret == EOK && id)
		*id = BUFFER_ID;
	return ret;
}
/*----------------------------------------------------------------------------*/
int sb_dsp_release_buffer(sb_dsp_t *dsp, unsigned id)
{
	assert(dsp);
	if (id != BUFFER_ID)
		return ENOENT;
	sb_clear_buffer(dsp);
	return EOK;
}
/*----------------------------------------------------------------------------*/
int sb_dsp_start_playback(sb_dsp_t *dsp, unsigned id, unsigned sampling_rate,
    unsigned sample_size, unsigned channels, bool sign)
{
	assert(dsp);

	/* Check supported parameters */
	ddf_log_debug("Starting playback on buffer(%u): rate: %u, size: %u, "
	    " channels: %u, signed: %s.\n", id, sampling_rate, sample_size,
	    channels, sign ? "YES" : "NO" );
	if (id != BUFFER_ID)
		return ENOENT;
	if (sample_size != 16) // FIXME We only support 16 bit playback
		return ENOTSUP;
	if (channels != 1 && channels != 2)
		return ENOTSUP;
	if (sampling_rate > 44100)
		return ENOTSUP;


	sb_dsp_write(dsp, SET_SAMPLING_RATE_OUTPUT);
	sb_dsp_write(dsp, sampling_rate >> 8);
	sb_dsp_write(dsp, sampling_rate & 0xff);

	ddf_log_debug("Sampling rate: %hhx:%hhx.\n",
	    sampling_rate >> 8, sampling_rate & 0xff);

#ifdef AUTO_DMA_MODE
	sb_dsp_write(dsp, AUTO_DMA_16B_DA_FIFO);
#else
	sb_dsp_write(dsp, SINGLE_DMA_16B_DA_FIFO);
#endif

	const uint8_t mode =
	    (sign ? DSP_MODE_SIGNED : 0) | (channels == 2 ? DSP_MODE_STEREO : 0);
	sb_dsp_write(dsp, mode);

	const uint16_t samples = sample_count(sample_size, PLAY_BLOCK_SIZE);
	sb_dsp_write(dsp, (samples - 1) & 0xff);
	sb_dsp_write(dsp, (samples - 1) >> 8);

	return EOK;
	return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
int sb_dsp_stop_playback(sb_dsp_t *dsp, unsigned id)
{
	assert(dsp);
	if (id != BUFFER_ID)
		return ENOENT;
	sb_dsp_write(dsp, DMA_16B_EXIT);
	return EOK;
}
/*----------------------------------------------------------------------------*/
int sb_dsp_start_record(sb_dsp_t *dsp, unsigned id, unsigned sample_rate,
    unsigned sample_size, unsigned channels, bool sign)
{
	return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
int sb_dsp_stop_record(sb_dsp_t *dsp, unsigned id)
{
	return ENOTSUP;
}
/**
 * @}
 */
