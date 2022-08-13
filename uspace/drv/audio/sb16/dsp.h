/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvaudiosb16
 * @{
 */
/** @file
 * @brief Sound Blaster Digital Sound Processor (DSP) helper functions.
 */
#ifndef DRV_AUDIO_SB16_DSP_H
#define DRV_AUDIO_SB16_DSP_H

#include <ddf/driver.h>
#include <libarch/ddi.h>
#include <errno.h>
#include <pcm/sample_format.h>
#include <audio_pcm_iface.h>

#include "registers.h"
typedef enum {
	DSP_PLAYBACK_ACTIVE_EVENTS,
	DSP_CAPTURE_ACTIVE_EVENTS,
	DSP_PLAYBACK_NOEVENTS,
	DSP_CAPTURE_NOEVENTS,
	DSP_PLAYBACK_TERMINATE,
	DSP_CAPTURE_TERMINATE,
	DSP_READY,
	DSP_NO_BUFFER,
} dsp_state_t;

typedef struct sb_dsp {
	sb16_regs_t *regs;
	int dma8_channel;
	int dma16_channel;
	struct {
		uint8_t major;
		uint8_t minor;
	} version;
	struct {
		uint8_t *data;
		size_t size;
	} buffer;
	struct {
		uint8_t mode;
		uint16_t samples;
		unsigned frame_count;
	} active;
	dsp_state_t state;
	async_sess_t *event_session;
	async_exch_t *event_exchange;
	ddf_dev_t *sb_dev;
} sb_dsp_t;

errno_t sb_dsp_init(sb_dsp_t *dsp, sb16_regs_t *regs, ddf_dev_t *dev,
    int dma8, int dma16);
void sb_dsp_interrupt(sb_dsp_t *dsp);
unsigned sb_dsp_query_cap(sb_dsp_t *dsp, audio_cap_t cap);
errno_t sb_dsp_get_buffer_position(sb_dsp_t *dsp, size_t *size);
errno_t sb_dsp_test_format(sb_dsp_t *dsp, unsigned *channels, unsigned *rate,
    pcm_sample_format_t *format);
errno_t sb_dsp_get_buffer(sb_dsp_t *dsp, void **buffer, size_t *size);
errno_t sb_dsp_set_event_session(sb_dsp_t *dsp, async_sess_t *session);
async_sess_t *sb_dsp_get_event_session(sb_dsp_t *dsp);
errno_t sb_dsp_release_buffer(sb_dsp_t *dsp);
errno_t sb_dsp_start_playback(sb_dsp_t *dsp, unsigned frames,
    unsigned channels, unsigned sample_rate, pcm_sample_format_t format);
errno_t sb_dsp_stop_playback(sb_dsp_t *dsp, bool immediate);
errno_t sb_dsp_start_capture(sb_dsp_t *dsp, unsigned frames,
    unsigned channels, unsigned sample_rate, pcm_sample_format_t format);
errno_t sb_dsp_stop_capture(sb_dsp_t *dsp, bool immediate);

#endif
/**
 * @}
 */
