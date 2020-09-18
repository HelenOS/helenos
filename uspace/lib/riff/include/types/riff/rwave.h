/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup libriff
 * @{
 */
/**
 * @file Waveform Audio File Format (WAVE) types.
 */

#ifndef RIFF_TYPES_WAVE_H
#define RIFF_TYPES_WAVE_H

#include <stdint.h>
#include <types/riff/chunk.h>

/** WAVE format chunk data
 *
 * Actual data structure in the RIFF file
 */
typedef struct {
	/** Format category */
	uint16_t format_tag;
	/** Number of channels */
	uint16_t channels;
	/** Sampling rate */
	uint32_t smp_sec;
	/** For buffer estimation */
	uint32_t avg_bytes_sec;
	/** Data block size */
	uint16_t block_align;
	/** Bits per sample (PCM only) */
	uint16_t bits_smp;
} rwave_fmt_t;

/** RIFF WAVE params
 *
 * Used by the API
 */
typedef struct {
	/** Number of channels */
	int channels;
	/** Number of bits per sample */
	int bits_smp;
	/** Sample frequency in Hz */
	int smp_freq;
} rwave_params_t;

/** RIFF WAVE writer */
typedef struct {
	/** RIFF writer */
	riffw_t *rw;
	/** Buffer size in bytes */
	size_t bufsize;
	/** Conversion buffer */
	void *buf;
	/** WAVE file parameters */
	rwave_params_t params;
	/** RIFF WAVE chunk */
	riff_wchunk_t wave;
	/** data chunk */
	riff_wchunk_t data;
} rwavew_t;

/** RIFF WAVE reader */
typedef struct {
	/** RIFF reader */
	riffr_t *rr;
	/** RIFF WAVE chunk */
	riff_rchunk_t wave;
	/** data chunk */
	riff_rchunk_t data;
} rwaver_t;

#endif

/** @}
 */
