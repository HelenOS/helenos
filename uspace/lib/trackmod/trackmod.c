/*
 * Copyright (c) 2014 Jiri Svoboda
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

/** @addtogroup trackmod
 * @{
 */
/**
 * @file Tracker module handling library.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "macros.h"
#include "trackmod.h"

/** Tunables */
enum {
	amp_factor = 16
};

/** Standard definitions set in stone */
enum {
	/** Base sample clock */
	base_clock = 8363 * 428,
	/** Maximum sample volume */
	vol_max = 63,
	/** Default TPR */
	def_tpr = 6,
	/** Default BPM */
	def_bpm = 125
};

static size_t trackmod_get_next_ord_idx(trackmod_modplay_t *);

/** Destroy sample.
 *
 * @param sample Sample
 */
static void trackmod_sample_destroy(trackmod_sample_t *sample)
{
	free(sample->data);
}

/** Destroy pattern.
 *
 * @param pattern Pattern
 */
static void trackmod_pattern_destroy(trackmod_pattern_t *pattern)
{
	free(pattern->data);
}

/** Create new empty module structure.
 *
 * @return New module
 */
trackmod_module_t *trackmod_module_new(void)
{
	return calloc(1, sizeof(trackmod_module_t));
}

/** Destroy module.
 *
 * @param module Module
 */
void trackmod_module_destroy(trackmod_module_t *module)
{
	size_t i;

	/* Destroy samples */
	if (module->sample != NULL) {
		for (i = 0; i < module->samples; i++)
			trackmod_sample_destroy(&module->sample[i]);
		free(module->sample);
	}

	/* Destroy patterns */
	if (module->pattern != NULL) {
		for (i = 0; i < module->patterns; i++)
			trackmod_pattern_destroy(&module->pattern[i]);
		free(module->pattern);
	}

	free(module->ord_list);
	free(module);
}

/** Return current pattern.
 *
 * @param modplay Module playback
 * @return        Pattern
 */
static trackmod_pattern_t *trackmod_cur_pattern(trackmod_modplay_t *modplay)
{
	unsigned pat_idx;

	pat_idx = modplay->module->ord_list[modplay->ord_idx];
	return &modplay->module->pattern[pat_idx];
}

/** Decode pattern cell.
 *
 * @param pattern Pattern
 * @param row     Row number
 * @param channel Channel number
 * @param cell    Place to store decoded cell
 */
static void trackmod_pattern_get_cell(trackmod_pattern_t *pattern,
    size_t row, size_t channel, trackmod_cell_t *cell)
{
	uint32_t code;

	code = pattern->data[row * pattern->channels + channel];
	cell->period = (code >> (4 * 4)) & 0xfff;
	cell->sample = (((code >> (7 * 4)) & 0xf) << 4) |
	    ((code >> (3 * 4)) & 0xf);
	cell->effect = code & 0xfff;
}

/** Process note (period, sample index)
 *
 * @param modplay Module playback
 * @param i       Channel number
 * @param cell    Cell
 */
static void trackmod_process_note(trackmod_modplay_t *modplay, size_t i,
    trackmod_cell_t *cell)
{
	trackmod_chan_t *chan = &modplay->chan[i];
	size_t smpidx;

	smpidx = (cell->sample - 1) % modplay->module->samples;
	chan->sample = &modplay->module->sample[smpidx];
	chan->smp_pos = 0;
	chan->lsmp = 0;
	chan->period = cell->period;
	chan->volume = modplay->chan[i].sample->def_vol;
}

/** Process Set volume effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_set_volume(trackmod_modplay_t *modplay, size_t chan,
    uint8_t param)
{
	modplay->chan[chan].volume = param & vol_max;
}

/** Process Pattern break effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_pattern_break(trackmod_modplay_t *modplay,
    size_t chan, uint8_t param)
{
	size_t next_idx;
	trackmod_pattern_t *next_pat;

	next_idx = trackmod_get_next_ord_idx(modplay);
	next_pat = &modplay->module->pattern[next_idx]; 

	modplay->pat_break = true;
	modplay->pat_break_row = param % next_pat->rows;
}

/** Process Set speed effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_set_speed(trackmod_modplay_t *modplay, size_t chan,
    uint8_t param)
{
	if (param > 0 && param < 32)
		modplay->tpr = param;
	else if (param > 0)
		modplay->bpm = param;
}

/** Process effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param cell    Cell
 */
static void trackmod_process_effect(trackmod_modplay_t *modplay, size_t chan,
    trackmod_cell_t *cell)
{
	uint8_t param8;

	param8 = cell->effect & 0xff;

	switch (cell->effect & 0xf00) {
	case 0xc00:
		trackmod_effect_set_volume(modplay, chan, param8);
		break;
	case 0xd00:
		trackmod_effect_pattern_break(modplay, chan, param8);
		break;
	case 0xf00:
		trackmod_effect_set_speed(modplay, chan, param8);
		break;
	default:
		break;
	}
}

/** Process pattern cell.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param cell    Cell
 */
static void trackmod_process_cell(trackmod_modplay_t *modplay, size_t chan,
    trackmod_cell_t *cell)
{
	if (cell->period != 0 && cell->sample != 0)
		trackmod_process_note(modplay, chan, cell);

	trackmod_process_effect(modplay, chan, cell);
}

/** Process pattern row.
 *
 * @param modplay Module playback
 */
static void trackmod_process_row(trackmod_modplay_t *modplay)
{
	trackmod_pattern_t *pattern;
	trackmod_cell_t cell;
	size_t i;

	pattern = trackmod_cur_pattern(modplay);

	for (i = 0; i < modplay->module->channels; i++) {
		trackmod_pattern_get_cell(pattern, modplay->row, i, &cell);
		if (modplay->debug)
			printf("%4d %02x %03x |", cell.period, cell.sample, cell.effect);
		trackmod_process_cell(modplay, i, &cell);
	}

	if (modplay->debug)
		printf("\n");
}

/** Get next order list index.
 *
 * @param modplay Module playback
 * @return        Next order list index
 */
static size_t trackmod_get_next_ord_idx(trackmod_modplay_t *modplay)
{
	size_t ord_idx;

	ord_idx = modplay->ord_idx + 1;
	if (ord_idx >= modplay->module->ord_list_len)
		ord_idx = 0; /* XXX */

	return ord_idx;
}

/** Advance to next pattern.
 *
 * @param modplay Module playback
 */
static void trackmod_next_pattern(trackmod_modplay_t *modplay)
{
	if (modplay->debug)
		printf("Next pattern\n");

	modplay->row = 0;
	modplay->ord_idx = trackmod_get_next_ord_idx(modplay);

	/* If we are doing a pattern break */
	if (modplay->pat_break) {
		modplay->row = modplay->pat_break_row;
		modplay->pat_break = false;
	}
}

/** Advance to next row.
 *
 * @param modplay Module playback
 */
static void trackmod_next_row(trackmod_modplay_t *modplay)
{
	trackmod_pattern_t *pattern;

	pattern = trackmod_cur_pattern(modplay);

	modplay->tick = 0;
	++modplay->row;
	if (modplay->row >= pattern->rows || modplay->pat_break)
		trackmod_next_pattern(modplay);

	trackmod_process_row(modplay);
}

/** Advance to next tick.
 *
 * @param modplay Module playback
 */
static void trackmod_next_tick(trackmod_modplay_t *modplay)
{
	modplay->smp = 0;
	++modplay->tick;
	if (modplay->tick >= modplay->tpr)
		trackmod_next_row(modplay);
}

/** Create module playback object.
 *
 * @param module   Module
 * @param smp_freq Sampling frequency
 * @param rmodplay Place to store pointer to module playback object
 */
int trackmod_modplay_create(trackmod_module_t *module,
    unsigned smp_freq, trackmod_modplay_t **rmodplay)
{
	trackmod_modplay_t *modplay = NULL;

	modplay = calloc(1, sizeof(trackmod_modplay_t));
	if (modplay == NULL)
		goto error;

	modplay->module = module;
	modplay->smp_freq = smp_freq;
	modplay->frame_size = sizeof(int16_t);
	modplay->ord_idx = 0;
	modplay->row = 0;
	modplay->tick = 0;
	modplay->smp = 0;

	modplay->tpr = def_tpr;
	modplay->bpm = def_bpm;

	modplay->chan = calloc(module->channels,
	    sizeof(trackmod_chan_t));
	if (modplay->chan == NULL)
		goto error;

	trackmod_process_row(modplay);

	*rmodplay = modplay;
	return EOK;
error:
	if (modplay != NULL)
		free(modplay->chan);
	free(modplay);
	return ENOMEM;
}

/** Destroy module playback object.
 *
 * @param modplay Module playback
 */
void trackmod_modplay_destroy(trackmod_modplay_t *modplay)
{
	free(modplay->chan);
	free(modplay);
}

/** Get number of samples per tick.
 *
 * @param modplay Module playback
 * @return        Number of samples per tick
 */
static size_t samples_per_tick(trackmod_modplay_t *modplay)
{
	return modplay->smp_freq * 10 / 4 / modplay->bpm;
}

/** Get number of samples remaining in current tick.
 *
 * @param modplay Module playback
 * @return        Number of remaining samples in tick
 */
static size_t samples_remain_tick(trackmod_modplay_t *modplay)
{
	/* XXX Integer samples per tick is a simplification */
	return samples_per_tick(modplay) - modplay->smp;
}

/** Advance sample position to next frame.
 *
 * @param chan Channel playback
 */
static void chan_smp_next_frame(trackmod_chan_t *chan)
{
	chan->lsmp = chan->sample->data[chan->smp_pos];
	++chan->smp_pos;

	if (chan->sample->loop_len == 0) {
		/* No looping */
		if (chan->smp_pos >= chan->sample->length) {
			chan->sample = NULL;
			chan->smp_pos = 0;
		}
	} else {
		/** Looping */
		if (chan->smp_pos >= chan->sample->loop_start +
		    chan->sample->loop_len) {
			chan->smp_pos = chan->sample->loop_start;
		}
	}
}

/** Render next sample on channel.
 *
 * @param modplay Module playback
 * @param cidx    Channel number
 * @return        Sample value
 */
static int trackmod_chan_next_sample(trackmod_modplay_t *modplay,
    size_t cidx)
{
	int sl, sn;
	int period, clk;
	int s;

	trackmod_chan_t *chan = &modplay->chan[cidx];

	if (chan->sample == NULL)
		return 0;

	/*
	 * Linear interpolation. Note this is slightly simplified:
	 * We ignore the half-sample offset and the boundary condition
	 * at the end of the sample (we should extend with zero).
	 */
	sl = (int)chan->lsmp * amp_factor * chan->volume / vol_max;
	sn = (int)chan->sample->data[chan->smp_pos] * amp_factor *
	    chan->volume / vol_max;

	period = (int)chan->period;
	clk = (int)chan->smp_clk;

	s = (sl * (period - clk) + sn * clk) / period;

	chan->smp_clk += base_clock / modplay->smp_freq;
	while (chan->sample != NULL && chan->smp_clk >= chan->period) {
		chan->smp_clk -= chan->period;
		chan_smp_next_frame(chan);
	}

	return s;
}

/** Render a segment of samples contained entirely within a tick.
 *
 * @param modplay Module playback
 * @param buffer  Buffer for storing audio data
 * @param bufsize Size of @a buffer in bytes
 */
static void get_samples_within_tick(trackmod_modplay_t *modplay,
    void *buffer, size_t bufsize)
{
	size_t nsamples;
	size_t smpidx;
	size_t chan;
	int s;

	nsamples = bufsize / modplay->frame_size;

	for (smpidx = 0; smpidx < nsamples; smpidx++) {
		s = 0;
		for (chan = 0; chan < modplay->module->channels; chan++) {
			s += trackmod_chan_next_sample(modplay, chan);
		}

		((int16_t *)buffer)[smpidx] = s;
	}

	modplay->smp += nsamples;
}

/** Render a segment of samples.
 *
 * @param modplay Module playback
 * @param buffer  Buffer for storing audio data
 * @param bufsize Size of @a buffer in bytes
 */
void trackmod_modplay_get_samples(trackmod_modplay_t *modplay,
    void *buffer, size_t bufsize)
{
	size_t nsamples;
	size_t rsmp;
	size_t now;

	nsamples = bufsize / modplay->frame_size;
	while (nsamples > 0) {
		rsmp = samples_remain_tick(modplay);
		if (rsmp == 0)
			trackmod_next_tick(modplay);

		rsmp = samples_remain_tick(modplay);
		now = min(rsmp, nsamples);

		get_samples_within_tick(modplay, buffer,
		    now * modplay->frame_size);
		nsamples -= now;
		buffer += now * modplay->frame_size;
	}
}

/** @}
 */
