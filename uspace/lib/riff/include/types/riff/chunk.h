/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libriff
 * @{
 */
/**
 * @file RIFF chunk types.
 */

#ifndef RIFF_TYPES_CHUNK_H
#define RIFF_TYPES_CHUNK_H

#include <stdint.h>
#include <stdio.h>

typedef uint32_t riff_ckid_t;
typedef uint32_t riff_cksize_t;
typedef uint32_t riff_ltype_t;

/** RIFF writer */
typedef struct {
	FILE *f;
	/** Chunk start offset */
	long ckstart;
} riffw_t;

/** RIFF reader */
typedef struct {
	FILE *f;
	/** Current file position */
	long pos;
} riffr_t;

/** RIFF chunk for reading */
typedef struct {
	riffr_t *riffr;
	long ckstart;
	riff_ckid_t ckid;
	riff_cksize_t cksize;
} riff_rchunk_t;

/** RIFF chunk for writing */
typedef struct {
	long ckstart;
} riff_wchunk_t;

/** RIFF chunk info */
typedef struct {
	long ckstart;
	riff_ckid_t ckid;
	riff_cksize_t cksize;
} riff_ckinfo_t;

enum {
	/** RIFF chunk ID */
	CKID_RIFF = 0x46464952,
	/** LIST chunk ID */
	CKID_LIST = 0x5453494C,

	/** WAVE RIFF form ID */
	FORM_WAVE = 0x45564157,
	/** fmt chunk ID */
	CKID_fmt = 0x20746d66,
	/** data chunk ID */
	CKID_data = 0x61746164,

	/** PCM wave format */
	WFMT_PCM = 0x0001
};

#endif

/** @}
 */
