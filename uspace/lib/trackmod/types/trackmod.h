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
 * @file Tracker module handling library types.
 */

#ifndef TYPES_TRACKMOD_H
#define TYPES_TRACKMOD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
	max_key = 96,
	keyoff_note = 97
};

typedef enum {
	/** No loop */
	tl_no_loop,
	/** Forward loop */
	tl_forward_loop,
	/** Pingpong loop */
	tl_pingpong_loop
} trackmod_looptype_t;

/** Sample */
typedef struct {
	/** Length in frames */
	size_t length;
	/** Bytes per sample */
	size_t bytes_smp;
	/** Sample data */
	void *data;
	/** Loop type */
	trackmod_looptype_t loop_type;
	/** Loop start position in frames */
	size_t loop_start;
	/** Loop length in frames (> 0) */
	size_t loop_len;
	/** Default volume (0..63) */
	uint8_t def_vol;
	/** Relative note */
	int rel_note;
	/** Finetune value (-8..7) in 1/8 semitones */
	int finetune;
} trackmod_sample_t;

/** Instrument */
typedef struct {
	/** Number of samples */
	size_t samples;
	/** Samples */
	trackmod_sample_t *sample;
	/** Sample index for each key */
	int key_smp[max_key];
} trackmod_instr_t;

/** Pattern cell */
typedef struct {
	/** Note */
	unsigned note;
	/** Sample period */
	unsigned period;
	/** Instrument number */
	unsigned instr;
	/** Volume */
	uint8_t volume;
	/** Effect */
	uint16_t effect;
} trackmod_cell_t;

/** Pattern */
typedef struct {
	/** Number of rows */
	size_t rows;
	/** Number of channels */
	size_t channels;
	/** Pattern data */
	trackmod_cell_t *data;
} trackmod_pattern_t;

/** Module. */
typedef struct {
	/** Number of channels */
	size_t channels;
	/** Number of samples */
	size_t instrs;
	/** Instruments */
	trackmod_instr_t *instr;
	/** Number of patterns */
	size_t patterns;
	/** Patterns */
	trackmod_pattern_t *pattern;
	/** Order list length */
	size_t ord_list_len;
	/** Order list */
	size_t *ord_list;
	/** Restart pos */
	size_t restart_pos;
	/** Default BPM */
	unsigned def_bpm;
	/** Default TPR */
	unsigned def_tpr;
} trackmod_module_t;

/** Channel playback */
typedef struct {
	trackmod_sample_t *sample;
	/** Value of sample before current position */
	int8_t lsmp;
	/** Sample position (in frames) */
	size_t smp_pos;
	/** Sample position (clock ticks within frame) */
	size_t smp_clk;
	/** Current period */
	unsigned period;
	/** Period after note was processed, zero if no note */
	unsigned period_new;
	/** Volume */
	uint8_t volume;
	/** Volume slide amount */
	int vol_slide;
	/** Portamento amount (positive for tone and up portamento,
	 * negative for down portamento.
	 */
	int portamento;
	/** Tone portamento target period. */
	unsigned period_tgt;
} trackmod_chan_t;

/** Module playback. */
typedef struct {
	/** Module */
	trackmod_module_t *module;
	/** Sampling frequency */
	unsigned smp_freq;
	/** Frame size (bytes per sample * channels) */
	size_t frame_size;

	/** Current position, order list index */
	size_t ord_idx;
	/** Current position, row within pattern */
	size_t row;
	/** Current position, tick within row */
	unsigned tick;
	/** Current position, sample within tick */
	unsigned smp;

	/** Channel playback state */
	trackmod_chan_t *chan;

	/** BPM (beats per minute) */
	unsigned bpm;
	/** TPR (ticks per row) */
	unsigned tpr;

	/** If true, break from pattern at end of current row */
	bool pat_break;
	/** If @c pat_break is true, row number where to jump in next pattern */
	size_t pat_break_row;
	/** Debug mode, print messages to stdout. */
	bool debug;
} trackmod_modplay_t;

#endif

/** @}
 */
