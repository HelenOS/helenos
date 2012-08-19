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

#include <as.h>
#include <bool.h>
#include <ddi.h>
#include <devman.h>
#include <device/hw_res.h>
#include <libarch/ddi.h>
#include <libarch/barrier.h>
#include <str_error.h>
#include <audio_pcm_iface.h>

#include "ddf_log.h"
#include "dsp_commands.h"
#include "dsp.h"

/* Maximum allowed transfer size for ISA DMA transfers is 64kB */
#define MAX_BUFFER_SIZE (4 * 1024) // use 4kB for now

#ifndef DSP_RETRY_COUNT
#define DSP_RETRY_COUNT 100
#endif

#define DSP_RESET_RESPONSE 0xaa
#define DSP_RATE_LIMIT 45000

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

	const int ret = hw_res_dma_channel_setup(sess,
	    dsp->dma16_channel, pa, size,
	    DMA_MODE_READ | DMA_MODE_AUTO | DMA_MODE_ON_DEMAND);

	async_hangup(sess);
	return ret;
}

static inline int sb_setup_buffer(sb_dsp_t *dsp, size_t size)
{
	assert(dsp);
	if (size > MAX_BUFFER_SIZE || size == 0 || (size % 2) == 1)
		size = MAX_BUFFER_SIZE;
	void *buffer = NULL, *pa = NULL;
	int ret = dmamem_map_anonymous(size, AS_AREA_WRITE | AS_AREA_READ,
	    0, &pa, &buffer);
	if (ret != EOK) {
		ddf_log_error("Failed to allocate DMA buffer.");
		return ENOMEM;
	}

	ddf_log_verbose("Setup dma buffer at %p(%p).", buffer, pa, size);
	assert((uintptr_t)pa < (1 << 25));

	/* Setup 16 bit channel */
	ret = sb_setup_dma(dsp, (uintptr_t)pa, size);
	if (ret == EOK) {
		dsp->buffer.data = buffer;
		dsp->buffer.size = size;
	} else {
		ddf_log_error("Failed to setup DMA16 channel: %s.",
		    str_error(ret));
		dmamem_unmap_anonymous(buffer);
	}
	return ret;
}

static inline void sb_clear_buffer(sb_dsp_t *dsp)
{
	assert(dsp);
	dmamem_unmap_anonymous(dsp->buffer.data);
	dsp->buffer.data = NULL;
	dsp->buffer.size = 0;
}

static inline size_t sample_count(pcm_sample_format_t format, size_t byte_count)
{
	return byte_count / pcm_sample_format_size(format);
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
	dsp->status = DSP_STOPPED;
	dsp->ignore_interrupts = false;
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

#ifndef AUTO_DMA_MODE
	if (dsp->status == DSP_PLAYBACK) {
		sb_dsp_write(dsp, SINGLE_DMA_16B_DA);
		sb_dsp_write(dsp, dsp->active.mode);
		sb_dsp_write(dsp, (dsp->active.samples - 1) & 0xff);
		sb_dsp_write(dsp, (dsp->active.samples - 1) >> 8);
	}

	if (dsp->status == DSP_RECORDING) {
		sb_dsp_write(dsp, SINGLE_DMA_16B_AD);
		sb_dsp_write(dsp, dsp->active.mode);
		sb_dsp_write(dsp, (dsp->active.samples - 1) & 0xff);
		sb_dsp_write(dsp, (dsp->active.samples - 1) >> 8);
	}
#endif

	if (dsp->ignore_interrupts)
		return;

	dsp->active.frame_count +=
	    dsp->active.samples / ((dsp->active.mode & DSP_MODE_STEREO) ? 2 : 1);

	if (dsp->event_exchange) {
		switch (dsp->status) {
		case DSP_PLAYBACK:
			async_msg_1(dsp->event_exchange,
			    PCM_EVENT_FRAMES_PLAYED, dsp->active.frame_count);
			break;
		case DSP_RECORDING:
			async_msg_1(dsp->event_exchange,
			    PCM_EVENT_FRAMES_RECORDED, dsp->active.frame_count);
			break;
		default:
		case DSP_STOPPED:
			ddf_log_warning("Interrupt while DSP stopped and "
			    "event exchange present. Terminating exchange");
			async_exchange_end(dsp->event_exchange);
			dsp->event_exchange = NULL;
		}
	} else {
		ddf_log_warning("Interrupt with no event consumer.");
	}
}

unsigned sb_dsp_query_cap(sb_dsp_t *dsp, audio_cap_t cap)
{
	switch(cap) {
	case AUDIO_CAP_RECORD:
	case AUDIO_CAP_PLAYBACK:
	case AUDIO_CAP_INTERRUPT:
		return 1;
	case AUDIO_CAP_MAX_BUFFER:
		return MAX_BUFFER_SIZE;
	case AUDIO_CAP_INTERRUPT_MIN_FRAMES:
		return 1;
	case AUDIO_CAP_INTERRUPT_MAX_FRAMES:
		return 16535;
	case AUDIO_CAP_BUFFER_POS:
	default:
		return 0;
	}
}

int sb_dsp_test_format(sb_dsp_t *dsp, unsigned *channels, unsigned *rate,
  pcm_sample_format_t *format)
{
	int ret = EOK;
	if (*channels == 0 || *channels > 2) {
		*channels = 2;
		ret = ELIMIT;
	}
	if (*rate > DSP_RATE_LIMIT) {
		*rate = DSP_RATE_LIMIT;
		ret = ELIMIT;
	}
	//TODO 8bit DMA supports 8bit formats
	if (*format != PCM_SAMPLE_SINT16_LE && *format != PCM_SAMPLE_UINT16_LE) {
		*format = pcm_sample_format_is_signed(*format) ?
		    PCM_SAMPLE_SINT16_LE : PCM_SAMPLE_UINT16_LE;
		ret = ELIMIT;
	}
	return ret;
}

int sb_dsp_get_buffer(sb_dsp_t *dsp, void **buffer, size_t *size)
{
	assert(dsp);
	assert(size);

	/* buffer is already setup by for someone, refuse to work until
	 * it's released */
	if (dsp->buffer.data)
		return EBUSY;

	const int ret = sb_setup_buffer(dsp, *size);
	if (ret == EOK) {
		ddf_log_debug("Providing buffer: %p, %zu B.",
		    dsp->buffer.data, dsp->buffer.size);

		if (buffer)
			*buffer = dsp->buffer.data;
		if (size)
			*size = dsp->buffer.size;
	}
	return ret;
}

int sb_dsp_set_event_session(sb_dsp_t *dsp, async_sess_t *session)
{
	assert(dsp);
	assert(session);
	if (dsp->event_session)
		return EBUSY;
	dsp->event_session = session;
	ddf_log_debug("Set event session.");
	return EOK;
}

int sb_dsp_release_buffer(sb_dsp_t *dsp)
{
	assert(dsp);
	sb_clear_buffer(dsp);
	async_exchange_end(dsp->event_exchange);
	dsp->event_exchange = NULL;
	if (dsp->event_session)
		async_hangup(dsp->event_session);
	dsp->event_session = NULL;
	ddf_log_debug("DSP buffer released.");
	return EOK;
}

int sb_dsp_start_playback(sb_dsp_t *dsp, unsigned frames,
    unsigned channels, unsigned sampling_rate, pcm_sample_format_t format)
{
	assert(dsp);

	if (!dsp->buffer.data)
		return EINVAL;

	/* Check supported parameters */
	ddf_log_debug("Requested playback: %u frames, %uHz, %s, %u channel(s).",
	    frames, sampling_rate, pcm_sample_format_str(format), channels);
	if (sb_dsp_test_format(dsp, &channels, &sampling_rate, &format) != EOK)
		return ENOTSUP;

	/* Client requested regular interrupts */
	if (frames) {
		if (!dsp->event_session)
			return EINVAL;
		dsp->event_exchange = async_exchange_begin(dsp->event_session);
		if (!dsp->event_exchange)
			return ENOMEM;
		dsp->ignore_interrupts = false;
	}

	const bool sign = (format == PCM_SAMPLE_SINT16_LE);

	sb_dsp_write(dsp, SET_SAMPLING_RATE_OUTPUT);
	sb_dsp_write(dsp, sampling_rate >> 8);
	sb_dsp_write(dsp, sampling_rate & 0xff);

	ddf_log_verbose("Sample rate: %hhx:%hhx.",
	    sampling_rate >> 8, sampling_rate & 0xff);

#ifdef AUTO_DMA_MODE
	sb_dsp_write(dsp, AUTO_DMA_16B_DA_FIFO);
#else
	sb_dsp_write(dsp, SINGLE_DMA_16B_DA);
#endif

	dsp->active.mode = 0
	    | (sign ? DSP_MODE_SIGNED : 0)
	    | (channels == 2 ? DSP_MODE_STEREO : 0);
	dsp->active.samples = frames * channels;
	dsp->active.frame_count = 0;

	sb_dsp_write(dsp, dsp->active.mode);
	sb_dsp_write(dsp, (dsp->active.samples - 1) & 0xff);
	sb_dsp_write(dsp, (dsp->active.samples - 1) >> 8);

	ddf_log_verbose("Playback started, interrupt every %u samples "
	    "(~1/%u sec)", dsp->active.samples,
	    sampling_rate / (dsp->active.samples * channels));

	dsp->status = DSP_PLAYBACK;

	return EOK;
}

int sb_dsp_stop_playback(sb_dsp_t *dsp)
{
	assert(dsp);
	sb_dsp_write(dsp, DMA_16B_EXIT);
	ddf_log_debug("Stopping playback on buffer.");
	async_msg_0(dsp->event_exchange, PCM_EVENT_PLAYBACK_TERMINATED);
	async_exchange_end(dsp->event_exchange);
	dsp->event_exchange = NULL;
	return EOK;
}

int sb_dsp_start_record(sb_dsp_t *dsp, unsigned frames,
    unsigned channels, unsigned sampling_rate, pcm_sample_format_t format)
{
	assert(dsp);
	if (!dsp->buffer.data)
		return EINVAL;

	/* Check supported parameters */
	ddf_log_debug("Requested record: %u frames, %uHz, %s, %u channel(s).",
	    frames, sampling_rate, pcm_sample_format_str(format), channels);
	if (sb_dsp_test_format(dsp, &channels, &sampling_rate, &format) != EOK)
		return ENOTSUP;

	/* client requested regular interrupts */
	if (frames) {
		if (!dsp->event_session)
			return EINVAL;
		dsp->event_exchange = async_exchange_begin(dsp->event_session);
		if (!dsp->event_exchange)
			return ENOMEM;
		dsp->ignore_interrupts = false;
	}

	const bool sign = (format == PCM_SAMPLE_SINT16_LE);

	sb_dsp_write(dsp, SET_SAMPLING_RATE_OUTPUT);
	sb_dsp_write(dsp, sampling_rate >> 8);
	sb_dsp_write(dsp, sampling_rate & 0xff);

	ddf_log_verbose("Sampling rate: %hhx:%hhx.",
	    sampling_rate >> 8, sampling_rate & 0xff);

#ifdef AUTO_DMA_MODE
	sb_dsp_write(dsp, AUTO_DMA_16B_AD_FIFO);
#else
	sb_dsp_write(dsp, SINGLE_DMA_16B_AD);
#endif

	dsp->active.mode = 0 |
	    (sign ? DSP_MODE_SIGNED : 0) |
	    (channels == 2 ? DSP_MODE_STEREO : 0);
	dsp->active.samples = frames * channels;
	dsp->active.frame_count = 0;

	sb_dsp_write(dsp, dsp->active.mode);
	sb_dsp_write(dsp, (dsp->active.samples - 1) & 0xff);
	sb_dsp_write(dsp, (dsp->active.samples - 1) >> 8);

	ddf_log_verbose("Recording started started, interrupt every %u samples "
	    "(~1/%u sec)", dsp->active.samples,
	    sampling_rate / (dsp->active.samples * channels));
	dsp->status = DSP_RECORDING;

	return EOK;
}

int sb_dsp_stop_record(sb_dsp_t *dsp)
{
	assert(dsp);
	sb_dsp_write(dsp, DMA_16B_EXIT);
	ddf_log_debug("Stopped recording");
	async_msg_0(dsp->event_exchange, PCM_EVENT_RECORDING_TERMINATED);
	async_exchange_end(dsp->event_exchange);
	dsp->event_exchange = NULL;
	return EOK;
}
/**
 * @}
 */
