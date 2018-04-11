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
#include <stdbool.h>
#include <ddf/driver.h>
#include <ddi.h>
#include <device/hw_res.h>
#include <libarch/ddi.h>
#include <libarch/barrier.h>
#include <macros.h>
#include <str_error.h>
#include <audio_pcm_iface.h>

#include "ddf_log.h"
#include "dsp_commands.h"
#include "dsp.h"

/* Maximum allowed transfer size for ISA DMA transfers is 64kB */
#define MAX_BUFFER_SIZE (64 * 1024)

#ifndef DSP_RETRY_COUNT
#define DSP_RETRY_COUNT 100
#endif

#define DSP_RESET_RESPONSE 0xaa

/* These are only for SB16 (DSP4.00+) */
#define DSP_RATE_UPPER_LIMIT 44100
#define DSP_RATE_LOWER_LIMIT 5000

#define AUTO_DMA_MODE

static inline const char *dsp_state_to_str(dsp_state_t state)
{
	static const char *state_names[] = {
		[DSP_PLAYBACK_ACTIVE_EVENTS] = "PLAYBACK w/ EVENTS",
		[DSP_CAPTURE_ACTIVE_EVENTS] = "CAPTURE w/ EVENTS",
		[DSP_PLAYBACK_NOEVENTS] = "PLAYBACK w/o EVENTS",
		[DSP_CAPTURE_NOEVENTS] = "CAPTURE w/o EVENTS",
		[DSP_PLAYBACK_TERMINATE] = "PLAYBACK TERMINATE",
		[DSP_CAPTURE_TERMINATE] = "CAPTURE TERMINATE",
		[DSP_READY] = "READY",
		[DSP_NO_BUFFER] = "NO BUFFER",
	};
	if ((size_t)state < ARRAY_SIZE(state_names))
		return state_names[state];
	return "UNKNOWN";
}


static inline void dsp_change_state(sb_dsp_t *dsp, dsp_state_t state)
{
	assert(dsp);
	ddf_log_verbose("Changing state from %s to %s",
	    dsp_state_to_str(dsp->state), dsp_state_to_str(state));
	dsp->state = state;
}

static inline errno_t dsp_read(sb_dsp_t *dsp, uint8_t *data)
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

static inline errno_t dsp_write(sb_dsp_t *dsp, uint8_t data)
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

static inline void dsp_reset(sb_dsp_t *dsp)
{
	assert(dsp);
	/* Reset DSP, see Chapter 2 of Sound Blaster HW programming guide */
	pio_write_8(&dsp->regs->dsp_reset, 1);
	udelay(3); /* Keep reset for 3 us */
	pio_write_8(&dsp->regs->dsp_reset, 0);
	/* "DSP takes about 100 microseconds to initialize itself" */
	udelay(100);
}

static inline void dsp_start_current_active(sb_dsp_t *dsp, uint8_t command)
{
	dsp_write(dsp, command);
	dsp_write(dsp, dsp->active.mode);
	dsp_write(dsp, (dsp->active.samples - 1) & 0xff);
	dsp_write(dsp, (dsp->active.samples - 1) >> 8);
}

static inline void dsp_set_sampling_rate(sb_dsp_t *dsp, unsigned rate)
{
	dsp_write(dsp, SET_SAMPLING_RATE_OUTPUT);
	uint8_t rate_lo = rate & 0xff;
	uint8_t rate_hi = rate >> 8;
	dsp_write(dsp, rate_hi);
	dsp_write(dsp, rate_lo);
	ddf_log_verbose("Sampling rate: %hhx:%hhx.", rate_hi, rate_lo);
}

static inline void dsp_report_event(sb_dsp_t *dsp, pcm_event_t event)
{
	assert(dsp);
	if (!dsp->event_exchange)
		ddf_log_warning("No one listening for event %u", event);
	async_msg_1(dsp->event_exchange, event, dsp->active.frame_count);
}

static inline errno_t setup_dma(sb_dsp_t *dsp, uintptr_t pa, size_t size)
{
	async_sess_t *sess = ddf_dev_parent_sess_get(dsp->sb_dev);

	return hw_res_dma_channel_setup(sess,
	    dsp->dma16_channel, pa, size,
	    DMA_MODE_READ | DMA_MODE_AUTO | DMA_MODE_ON_DEMAND);
}

static inline errno_t setup_buffer(sb_dsp_t *dsp, size_t size)
{
	assert(dsp);

	if ((size > MAX_BUFFER_SIZE) || (size == 0) || ((size % 2) == 1))
		size = MAX_BUFFER_SIZE;

	uintptr_t pa = 0;
	void *buffer = AS_AREA_ANY;

	errno_t ret = dmamem_map_anonymous(size, DMAMEM_16MiB | 0x0000ffff,
	    AS_AREA_WRITE | AS_AREA_READ, 0, &pa, &buffer);
	if (ret != EOK) {
		ddf_log_error("Failed to allocate DMA buffer.");
		return ENOMEM;
	}

	ddf_log_verbose("Setup DMA buffer at %p (%zu) %zu.", buffer, pa, size);
	assert(pa < (1 << 24));

	/* Setup 16 bit channel */
	ret = setup_dma(dsp, pa, size);
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

errno_t sb_dsp_init(sb_dsp_t *dsp, sb16_regs_t *regs, ddf_dev_t *dev,
    int dma8, int dma16)
{
	assert(dsp);
	dsp->regs = regs;
	dsp->dma8_channel = dma8;
	dsp->dma16_channel = dma16;
	dsp->event_session = NULL;
	dsp->event_exchange = NULL;
	dsp->sb_dev = dev;
	dsp->state = DSP_NO_BUFFER;
	dsp_reset(dsp);
	uint8_t response;
	const errno_t ret = dsp_read(dsp, &response);
	if (ret != EOK) {
		ddf_log_error("Failed to read DSP reset response value.");
		return ret;
	}
	if (response != DSP_RESET_RESPONSE) {
		ddf_log_error("Invalid DSP reset response: %x.", response);
		return EIO;
	}

	/* Get DSP version number */
	dsp_write(dsp, DSP_VERSION);
	dsp_read(dsp, &dsp->version.major);
	dsp_read(dsp, &dsp->version.minor);

	return ret;
}

void sb_dsp_interrupt(sb_dsp_t *dsp)
{
	assert(dsp);

	dsp->active.frame_count +=
	    dsp->active.samples / ((dsp->active.mode & DSP_MODE_STEREO) ? 2 : 1);

	switch (dsp->state) {
	case DSP_PLAYBACK_ACTIVE_EVENTS:
		dsp_report_event(dsp, PCM_EVENT_FRAMES_PLAYED);
	case DSP_PLAYBACK_NOEVENTS:
#ifndef AUTO_DMA_MODE
		dsp_start_current_active(dsp, SINGLE_DMA_16B_DA);
#endif
		break;
	case DSP_CAPTURE_ACTIVE_EVENTS:
		dsp_report_event(dsp, PCM_EVENT_FRAMES_CAPTURED);
	case DSP_CAPTURE_NOEVENTS:
#ifndef AUTO_DMA_MODE
		dsp_start_current_active(dsp, SINGLE_DMA_16B_DA);
#endif
		break;
	case DSP_CAPTURE_TERMINATE:
		dsp_change_state(dsp, DSP_READY);
		dsp_report_event(dsp, PCM_EVENT_CAPTURE_TERMINATED);
		async_exchange_end(dsp->event_exchange);
		dsp->event_exchange = NULL;
		break;
	case DSP_PLAYBACK_TERMINATE:
		dsp_change_state(dsp, DSP_READY);
		dsp_report_event(dsp, PCM_EVENT_PLAYBACK_TERMINATED);
		async_exchange_end(dsp->event_exchange);
		dsp->event_exchange = NULL;
		break;
	default:
		ddf_log_warning("Interrupt while DSP not active (%s)",
		    dsp_state_to_str(dsp->state));
	}
}

unsigned sb_dsp_query_cap(sb_dsp_t *dsp, audio_cap_t cap)
{
	ddf_log_verbose("Querying cap %s", audio_pcm_cap_str(cap));
	switch (cap) {
	case AUDIO_CAP_CAPTURE:
	case AUDIO_CAP_PLAYBACK:
	case AUDIO_CAP_INTERRUPT:
	case AUDIO_CAP_BUFFER_POS:
		return 1;
	case AUDIO_CAP_MAX_BUFFER:
		return MAX_BUFFER_SIZE;
	case AUDIO_CAP_INTERRUPT_MIN_FRAMES:
		return 1;
	case AUDIO_CAP_INTERRUPT_MAX_FRAMES:
		return 16535;
	default:
		return -1;
	}
}

errno_t sb_dsp_get_buffer_position(sb_dsp_t *dsp, size_t *pos)
{
	if (dsp->state == DSP_NO_BUFFER)
		return ENOENT;

	assert(dsp->buffer.data);
	async_sess_t *sess = ddf_dev_parent_sess_get(dsp->sb_dev);

	// TODO: Assumes DMA 16
	size_t remain;
	errno_t rc = hw_res_dma_channel_remain(sess, dsp->dma16_channel, &remain);
	if (rc == EOK) {
		*pos = dsp->buffer.size - remain;
	}
	return rc;
}

errno_t sb_dsp_test_format(sb_dsp_t *dsp, unsigned *channels, unsigned *rate,
    pcm_sample_format_t *format)
{
	errno_t ret = EOK;
	if (*channels == 0 || *channels > 2) {
		*channels = 2;
		ret = ELIMIT;
	}
	//TODO 8bit DMA supports 8bit formats
	if (*format != PCM_SAMPLE_SINT16_LE && *format != PCM_SAMPLE_UINT16_LE) {
		*format = pcm_sample_format_is_signed(*format) ?
		    PCM_SAMPLE_SINT16_LE : PCM_SAMPLE_UINT16_LE;
		ret = ELIMIT;
	}
	if (*rate > DSP_RATE_UPPER_LIMIT) {
		*rate = DSP_RATE_UPPER_LIMIT;
		ret = ELIMIT;
	}
	if (*rate < DSP_RATE_LOWER_LIMIT) {
		*rate = DSP_RATE_LOWER_LIMIT;
		ret = ELIMIT;
	}
	return ret;
}

errno_t sb_dsp_set_event_session(sb_dsp_t *dsp, async_sess_t *session)
{
	assert(dsp);
	if (dsp->event_session && session)
		return EBUSY;
	dsp->event_session = session;
	ddf_log_debug("Set event session to %p.", session);
	return EOK;
}

async_sess_t *sb_dsp_get_event_session(sb_dsp_t *dsp)
{
	assert(dsp);
	ddf_log_debug("Get event session: %p.", dsp->event_session);
	return dsp->event_session;
}

errno_t sb_dsp_get_buffer(sb_dsp_t *dsp, void **buffer, size_t *size)
{
	assert(dsp);
	assert(size);

	/* buffer is already setup by for someone, refuse to work until
	 * it's released */
	if (dsp->state != DSP_NO_BUFFER)
		return EBUSY;
	assert(dsp->buffer.data == NULL);

	const errno_t ret = setup_buffer(dsp, *size);
	if (ret == EOK) {
		ddf_log_debug("Providing buffer: %p, %zu B.",
		    dsp->buffer.data, dsp->buffer.size);

		if (buffer)
			*buffer = dsp->buffer.data;
		if (size)
			*size = dsp->buffer.size;
		dsp_change_state(dsp, DSP_READY);
	}
	return ret;
}

errno_t sb_dsp_release_buffer(sb_dsp_t *dsp)
{
	assert(dsp);
	if (dsp->state != DSP_READY)
		return EINVAL;
	assert(dsp->buffer.data);
	dmamem_unmap_anonymous(dsp->buffer.data);
	dsp->buffer.data = NULL;
	dsp->buffer.size = 0;
	ddf_log_debug("DSP buffer released.");
	dsp_change_state(dsp, DSP_NO_BUFFER);
	return EOK;
}

errno_t sb_dsp_start_playback(sb_dsp_t *dsp, unsigned frames,
    unsigned channels, unsigned sampling_rate, pcm_sample_format_t format)
{
	assert(dsp);

	if (!dsp->buffer.data || dsp->state != DSP_READY)
		return EINVAL;

	/* Check supported parameters */
	ddf_log_debug("Requested playback: %u frames, %uHz, %s, %u channel(s).",
	    frames, sampling_rate, pcm_sample_format_str(format), channels);
	if (sb_dsp_test_format(dsp, &channels, &sampling_rate, &format) != EOK)
		return ENOTSUP;

	/* Client requested regular events */
	if (frames && !dsp->event_session)
		return EINVAL;

	if (dsp->event_session) {
		dsp->event_exchange = async_exchange_begin(dsp->event_session);
		if (!dsp->event_exchange)
			return ENOMEM;
	}

	dsp->active.mode = 0 |
	    (pcm_sample_format_is_signed(format) ? DSP_MODE_SIGNED : 0) |
	    (channels == 2 ? DSP_MODE_STEREO : 0);
	dsp->active.samples = frames * channels;
	dsp->active.frame_count = 0;

	dsp_set_sampling_rate(dsp, sampling_rate);

#ifdef AUTO_DMA_MODE
	dsp_start_current_active(dsp, AUTO_DMA_16B_DA_FIFO);
#else
	dsp_start_current_active(dsp, SINGLE_DMA_16B_DA);
#endif

	ddf_log_verbose("Playback started, event every %u samples",
	    dsp->active.samples);

	dsp_change_state(dsp,
	    frames ? DSP_PLAYBACK_ACTIVE_EVENTS : DSP_PLAYBACK_NOEVENTS);
	if (dsp->state == DSP_PLAYBACK_ACTIVE_EVENTS)
		dsp_report_event(dsp, PCM_EVENT_PLAYBACK_STARTED);

	return EOK;
}

errno_t sb_dsp_stop_playback(sb_dsp_t *dsp, bool immediate)
{
	assert(dsp);
	if ((dsp->state == DSP_PLAYBACK_NOEVENTS ||
	    dsp->state == DSP_PLAYBACK_ACTIVE_EVENTS) &&
	    immediate) {
		dsp_write(dsp, DMA_16B_PAUSE);
		dsp_reset(dsp);
		ddf_log_debug("Stopped playback");
		dsp_change_state(dsp, DSP_READY);
		if (dsp->event_exchange) {
			dsp_report_event(dsp, PCM_EVENT_PLAYBACK_TERMINATED);
			async_exchange_end(dsp->event_exchange);
			dsp->event_exchange = NULL;
		}
		return EOK;
	}
	if (dsp->state == DSP_PLAYBACK_ACTIVE_EVENTS) {
		/* Stop after current fragment */
		assert(!immediate);
		dsp_write(dsp, DMA_16B_EXIT);
		ddf_log_debug("Last playback fragment");
		dsp_change_state(dsp, DSP_PLAYBACK_TERMINATE);
		return EOK;
	}
	return EINVAL;
}

errno_t sb_dsp_start_capture(sb_dsp_t *dsp, unsigned frames,
    unsigned channels, unsigned sampling_rate, pcm_sample_format_t format)
{
	assert(dsp);
	if (!dsp->buffer.data || dsp->state != DSP_READY)
		return EINVAL;

	/* Check supported parameters */
	ddf_log_debug("Requested capture: %u frames, %uHz, %s, %u channel(s).",
	    frames, sampling_rate, pcm_sample_format_str(format), channels);
	if (sb_dsp_test_format(dsp, &channels, &sampling_rate, &format) != EOK)
		return ENOTSUP;

	/* Client requested regular events */
	if (frames && !dsp->event_session)
		return EINVAL;

	if (dsp->event_session) {
		dsp->event_exchange = async_exchange_begin(dsp->event_session);
		if (!dsp->event_exchange)
			return ENOMEM;
	}

	dsp->active.mode = 0 |
	    (pcm_sample_format_is_signed(format) ? DSP_MODE_SIGNED : 0) |
	    (channels == 2 ? DSP_MODE_STEREO : 0);
	dsp->active.samples = frames * channels;
	dsp->active.frame_count = 0;

	dsp_set_sampling_rate(dsp, sampling_rate);

#ifdef AUTO_DMA_MODE
	dsp_start_current_active(dsp, AUTO_DMA_16B_AD_FIFO);
#else
	dsp_start_current_active(dsp, SINGLE_DMA_16B_AD);
#endif

	ddf_log_verbose("Capture started started, event every %u samples",
	    dsp->active.samples);
	dsp_change_state(dsp,
	    frames ? DSP_CAPTURE_ACTIVE_EVENTS : DSP_CAPTURE_NOEVENTS);
	if (dsp->state == DSP_CAPTURE_ACTIVE_EVENTS)
		dsp_report_event(dsp, PCM_EVENT_CAPTURE_STARTED);
	return EOK;
}

errno_t sb_dsp_stop_capture(sb_dsp_t *dsp, bool immediate)
{
	assert(dsp);
	if ((dsp->state == DSP_CAPTURE_NOEVENTS ||
	    dsp->state == DSP_CAPTURE_ACTIVE_EVENTS) &&
	    immediate) {
		dsp_write(dsp, DMA_16B_PAUSE);
		dsp_reset(dsp);
		ddf_log_debug("Stopped capture fragment");
		dsp_change_state(dsp, DSP_READY);
		if (dsp->event_exchange) {
			dsp_report_event(dsp, PCM_EVENT_CAPTURE_TERMINATED);
			async_exchange_end(dsp->event_exchange);
			dsp->event_exchange = NULL;
		}
		return EOK;
	}
	if (dsp->state == DSP_CAPTURE_ACTIVE_EVENTS) {
		/* Stop after current fragment */
		assert(!immediate);
		dsp_write(dsp, DMA_16B_EXIT);
		ddf_log_debug("Last capture fragment");
		dsp_change_state(dsp, DSP_CAPTURE_TERMINATE);
		return EOK;
	}
	return EINVAL;
}
/**
 * @}
 */
