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
#include "protracker.h"
#include "trackmod.h"
#include "xm.h"

/** Tunables */
enum {
	amp_factor = 16
};

/** Standard definitions set in stone */
enum {
	/** Base sample clock */
	base_clock = 8363 * 428,
	/** Maximum sample volume */
	vol_max = 64,
	/** Minimum period */
	period_min = 113,
	/** Maxium period */
	period_max = 856
};

/** Table for finetune computation.
  *
  * Finetune is a number ft in [-8 .. 7]. The pitch should be adjusted by
  * ft/8 semitones. To adjust pitch by 1/8 semitone down we can mutiply the
  * period by 2^(1/12/8) =. 1.0072, one semitone up: 2^-(1/12/8) =. 0.9928,
  * to adjust by ft/8 semitones, multiply by 2^(-ft/12/8).
  *
  * finetune_factor[ft] := 10000 * 2^(-ft/12/8)
  * res_period = clip(period * fineture_factor[ft+8] / 10000)
  */
static unsigned finetune_factor[16] = {
	10595, 10518, 10443, 10368, 10293, 10219, 10145, 10072,
	10000,  9928,  9857,  9786,  9715,  9645,  9576,  9507
};

static unsigned period_table[12 * 8] = {
     907,900,894,887,881,875,868,862,856,850,844,838,832,826,820,814,
     808,802,796,791,785,779,774,768,762,757,752,746,741,736,730,725,
     720,715,709,704,699,694,689,684,678,675,670,665,660,655,651,646,
     640,636,632,628,623,619,614,610,604,601,597,592,588,584,580,575,
     570,567,563,559,555,551,547,543,538,535,532,528,524,520,516,513,
     508,505,502,498,494,491,487,484,480,477,474,470,467,463,460,457
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

/** Destroy instrument.
 *
 * @param instr Intrument
 */
static void trackmod_instr_destroy(trackmod_instr_t *instr)
{
	size_t i;

	for (i = 0; i < instr->samples; i++)
		trackmod_sample_destroy(&instr->sample[i]);
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
	if (module->instr != NULL) {
		for (i = 0; i < module->instrs; i++)
			trackmod_instr_destroy(&module->instr[i]);
		free(module->instr);
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

errno_t trackmod_module_load(char *fname, trackmod_module_t **rmodule)
{
	errno_t rc;

	rc = trackmod_xm_load(fname, rmodule);
	if (rc == EOK)
		return EOK;

	rc = trackmod_protracker_load(fname, rmodule);
	return rc;
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
	*cell = pattern->data[row * pattern->channels + channel];
}

/** Compute floor(a / b), and the remainder.
 *
 * Unlike standard integer division this rounds towars negative infinity,
 * not towards zero.
 *
 * @param a Dividend
 * @param b Divisor
 * @param quot Place to store 'quotient' (floor (a/b))
 * @param rem Place to store 'remainder' (a - floor(a/b) * b)
 */
static void divmod_floor(int a, int b, int *quot, int *rem)
{
	if (b < 0) {
		a = -a;
		b = -b;
	}

	if (a >= 0) {
		*quot = a / b;
		*rem = a % b;
	} else {
		*quot = - (-a + (b - 1)) / b;
		*rem = a - (*quot * b);
	}
}

/** Process note (period)
 *
 * @param modplay Module playback
 * @param i       Channel number
 * @param cell    Cell
 */
static void trackmod_process_note(trackmod_modplay_t *modplay, size_t i,
    trackmod_cell_t *cell)
{
	trackmod_chan_t *chan = &modplay->chan[i];
	int period;
	int pitch;
	int octave;
	int opitch;

	if (chan->sample == NULL)
		return;

	if (cell->period == 0) {
		pitch = 8 * (cell->note + chan->sample->rel_note) +
		    chan->sample->finetune;
		divmod_floor(pitch, 8 * 12, &octave, &opitch);

		if (octave >= 0)
			period = period_table[opitch] * 8 / (1 << octave);
		else
			period = period_table[opitch] * 8 * (1 << (-octave));
	} else {
		period = cell->period;
		period = period *
		    finetune_factor[chan->sample->finetune + 8] / 10000;
		if (period > period_max)
			period = period_max;
		if (period < period_min)
			period = period_min;
	}

	chan->period_new = period;
}

/** Process instrument number (this is what triggers the note playback)
 *
 * @param modplay Module playback
 * @param i       Channel number
 * @param cell    Cell
 */
static void trackmod_process_instr(trackmod_modplay_t *modplay, size_t i,
    trackmod_cell_t *cell)
{
	trackmod_chan_t *chan = &modplay->chan[i];
	trackmod_instr_t *instr;
	size_t iidx;
	size_t sidx;

	if (cell->instr == 0)
		return;

	iidx = (cell->instr - 1) % modplay->module->instrs;
	instr = &modplay->module->instr[iidx];
	sidx = instr->key_smp[cell->note] % instr->samples;
	chan->sample = &instr->sample[sidx];
	chan->smp_pos = 0;
	chan->lsmp = 0;

	chan->volume = modplay->chan[i].sample->def_vol;
}

/** Process keyoff note
 *
 * @param modplay Module playback
 * @param i       Channel number
 * @param cell    Cell
 */
static void trackmod_process_keyoff_note(trackmod_modplay_t *modplay, size_t i)
{
	trackmod_chan_t *chan = &modplay->chan[i];

	chan->sample = NULL;
	chan->period = 0;
	chan->smp_pos = 0;
	chan->lsmp = 0;
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
	modplay->chan[chan].volume = param % (vol_max + 1);
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
	unsigned row;

	/* Strangely the parameter is BCD */
	row = (param >> 4) * 10 + (param & 0xf);

	next_idx = trackmod_get_next_ord_idx(modplay);
	next_pat = &modplay->module->pattern[next_idx]; 

	modplay->pat_break = true;
	modplay->pat_break_row = row % next_pat->rows;
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

/** Process Fine volume slide down effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_fine_vol_slide_down(trackmod_modplay_t *modplay,
    size_t chan, uint8_t param)
{
	int nv;

	nv = modplay->chan[chan].volume - param;
	if (nv < 0)
		nv = 0;
	modplay->chan[chan].volume = nv;
}

/** Process Fine volume slide up effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_fine_vol_slide_up(trackmod_modplay_t *modplay,
    size_t chan, uint8_t param)
{
	int nv;

	nv = modplay->chan[chan].volume + param;
	if (nv > vol_max)
		nv = vol_max;
	modplay->chan[chan].volume = nv;
}

/** Process Volume slide effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_vol_slide(trackmod_modplay_t *modplay,
    size_t chan, uint8_t param)
{
	if ((param & 0xf0) != 0)
		modplay->chan[chan].vol_slide = param >> 4;
	else
		modplay->chan[chan].vol_slide = -(int)(param & 0xf);
}

/** Process Volume slide down effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_vol_slide_down(trackmod_modplay_t *modplay,
    size_t chan, uint8_t param4)
{
	modplay->chan[chan].vol_slide = -(int)param4;
}

/** Process Volume slide up effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_vol_slide_up(trackmod_modplay_t *modplay,
    size_t chan, uint8_t param4)
{
	modplay->chan[chan].vol_slide = param4;
}

/** Process Fine portamento down effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_fine_porta_down(trackmod_modplay_t *modplay,
    size_t chan, uint8_t param)
{
	int np;

	np = modplay->chan[chan].period + param;
	if (np > period_max)
		np = period_max;
	modplay->chan[chan].period = np;
}

/** Process Fine portamento up effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_fine_porta_up(trackmod_modplay_t *modplay,
    size_t chan, uint8_t param)
{
	int np;

	np = modplay->chan[chan].period - param;
	if (np < period_min)
		np = period_min;
	modplay->chan[chan].period = np;
}

/** Process Portamento down effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_porta_down(trackmod_modplay_t *modplay,
    size_t chan, uint8_t param)
{
	modplay->chan[chan].portamento = -(int)param;
}

/** Process Portamento up effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_porta_up(trackmod_modplay_t *modplay,
    size_t chan, uint8_t param)
{
	modplay->chan[chan].portamento = param;
}

/** Process Tone portamento effect.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param param   Effect parameter
 */
static void trackmod_effect_tone_porta(trackmod_modplay_t *modplay,
    size_t chan, uint8_t param)
{
	/* Set up tone portamento effect */
	modplay->chan[chan].portamento = param;
	if (modplay->chan[chan].period_new != 0)
		modplay->chan[chan].period_tgt = modplay->chan[chan].period_new;

	/* Prevent going directly to new period */
	modplay->chan[chan].period_new = 0;
}

/** Process volume column.
 *
 * @param modplay Module playback
 * @param chan    Channel number
 * @param cell    Cell
 */
static void trackmod_process_volume(trackmod_modplay_t *modplay, size_t chan,
    trackmod_cell_t *cell)
{
	uint8_t param4;

	if (cell->volume >= 0x10 && cell->volume <= 0x10 + vol_max)
		trackmod_effect_set_volume(modplay, chan, cell->volume - 0x10);

	param4 = cell->volume & 0xf;

	switch (cell->volume & 0xf0) {
	case 0x60:
		trackmod_effect_vol_slide_down(modplay, chan, param4);
		break;
	case 0x70:
		trackmod_effect_vol_slide_up(modplay, chan, param4);
		break;
	case 0x80:
		trackmod_effect_fine_vol_slide_down(modplay, chan, param4);
		break;
	case 0x90:
		trackmod_effect_fine_vol_slide_up(modplay, chan, param4);
		break;
	case 0xf0:
		trackmod_effect_tone_porta(modplay, chan, param4 << 4);
		break;
	default:
		break;
	}
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
	uint8_t param4;

	param8 = cell->effect & 0xff;

	switch (cell->effect & 0xf00) {
	case 0x100:
		trackmod_effect_porta_up(modplay, chan, param8);
		break;
	case 0x200:
		trackmod_effect_porta_down(modplay, chan, param8);
		break;
	case 0x300:
		trackmod_effect_tone_porta(modplay, chan, param8);
		break;
	case 0xa00:
		trackmod_effect_vol_slide(modplay, chan, param8);
		break;
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

	param4 = cell->effect & 0xf;

	switch (cell->effect & 0xff0) {
	case 0xe10:
		trackmod_effect_fine_porta_up(modplay, chan, param4);
		break;
	case 0xe20:
		trackmod_effect_fine_porta_down(modplay, chan, param4);
		break;
	case 0xea0:
		trackmod_effect_fine_vol_slide_up(modplay, chan, param4);
		break;
	case 0xeb0:
		trackmod_effect_fine_vol_slide_down(modplay, chan, param4);
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
	modplay->chan[chan].period_new = 0;

	trackmod_process_instr(modplay, chan, cell);

	if (cell->period != 0 || (cell->note != 0 && cell->note != keyoff_note)) {
		trackmod_process_note(modplay, chan, cell);
	} else if (cell->note == keyoff_note && cell->instr == 0) {
		trackmod_process_keyoff_note(modplay, chan);
	}

	trackmod_process_volume(modplay, chan, cell);
	trackmod_process_effect(modplay, chan, cell);

	if (modplay->chan[chan].period_new != 0)
		modplay->chan[chan].period = modplay->chan[chan].period_new;
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

	if (modplay->debug)
		printf("%02zx: ", modplay->row);

	for (i = 0; i < modplay->module->channels; i++) {
		trackmod_pattern_get_cell(pattern, modplay->row, i, &cell);

		if (modplay->debug) {
			printf("%4d %02x %02x %03x |", cell.period ?
			    cell.period : cell.note, cell.instr,
			    cell.volume, cell.effect);
		}

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
		ord_idx = modplay->module->restart_pos;

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

/** Clear effects at end of row. */
static void trackmod_clear_effects(trackmod_modplay_t *modplay)
{
	size_t i;

	for (i = 0; i < modplay->module->channels; i++) {
		modplay->chan[i].vol_slide = 0;
		modplay->chan[i].portamento = 0;
	}
}

/** Process effects at beginning of tick. */
static void trackmod_process_tick(trackmod_modplay_t *modplay)
{
	trackmod_chan_t *chan;
	size_t i;
	int nv;
	int np;

	for (i = 0; i < modplay->module->channels; i++) {
		chan = &modplay->chan[i];

		/* Volume slides */
		nv = (int)chan->volume + chan->vol_slide;
		if (nv < 0)
			nv = 0;
		if (nv > vol_max)
			nv = vol_max;

		chan->volume = nv;

		/* Portamentos */
		if (chan->period_tgt == 0) {
			/* Up or down portamento */
			np = (int)chan->period - chan->portamento;
		} else {
			/* Tone portamento */
			if (chan->period_tgt < chan->period)
				np = max((int)chan->period_tgt, (int)chan->period - chan->portamento);
			else
				np = min((int)chan->period_tgt, (int)chan->period + chan->portamento);
		}

/*		if (np < period_min)
			np = period_min;
		if (np > period_max)
			np = period_max;
*/
		modplay->chan[i].period = np;
	}
}

/** Advance to next row.
 *
 * @param modplay Module playback
 */
static void trackmod_next_row(trackmod_modplay_t *modplay)
{
	trackmod_pattern_t *pattern;

	/* Clear effect state at end of row */
	trackmod_clear_effects(modplay);

	pattern = trackmod_cur_pattern(modplay);

	modplay->tick = 0;
	++modplay->row;
	if (modplay->row >= pattern->rows || modplay->pat_break)
		trackmod_next_pattern(modplay);

	trackmod_process_tick(modplay);
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
	else
		trackmod_process_tick(modplay);
}

/** Create module playback object.
 *
 * @param module   Module
 * @param smp_freq Sampling frequency
 * @param rmodplay Place to store pointer to module playback object
 */
errno_t trackmod_modplay_create(trackmod_module_t *module,
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

	modplay->tpr = module->def_tpr;
	modplay->bpm = module->def_bpm;

	modplay->chan = calloc(module->channels,
	    sizeof(trackmod_chan_t));
	if (modplay->chan == NULL)
		goto error;

	trackmod_process_tick(modplay);
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

/** Get sample frame.
 *
 * Get frame at the specified sample position.
 *
 * @param sample Sample
 * @param pos	 Position (frame index)
 * @return	 Frame value
 */
int trackmod_sample_get_frame(trackmod_sample_t *sample, size_t pos)
{
	int8_t *i8p;
	int16_t *i16p;

	if (sample->bytes_smp == 1) {
		i8p = (int8_t *)sample->data;
		return i8p[pos];
	} else {
		/* chan->sample->bytes_smp == 2 */
		i16p = (int16_t *)sample->data;
		return i16p[pos] / 256; /* XXX Retain full precision */
	}
}

/** Advance sample position to next frame.
 *
 * @param chan Channel playback
 */
static void chan_smp_next_frame(trackmod_chan_t *chan)
{
	chan->lsmp = trackmod_sample_get_frame(chan->sample, chan->smp_pos);
	++chan->smp_pos;

	switch (chan->sample->loop_type) {
	case tl_pingpong_loop:
		/** XXX Pingpong loop */
	case tl_no_loop:
		/* No loop */
		if (chan->smp_pos >= chan->sample->length) {
			chan->sample = NULL;
			chan->smp_pos = 0;
		}
		break;
	case tl_forward_loop:
		/** Forward loop */
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

	if (chan->sample == NULL || chan->period == 0)
		return 0;

	/*
	 * Linear interpolation. Note this is slightly simplified:
	 * We ignore the half-sample offset and the boundary condition
	 * at the end of the sample (we should extend with zero).
	 */
	sl = (int)chan->lsmp * amp_factor * chan->volume / vol_max;
	sn = (int)trackmod_sample_get_frame(chan->sample, chan->smp_pos) *
	    amp_factor * chan->volume / vol_max;

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
