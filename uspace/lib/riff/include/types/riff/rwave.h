/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
