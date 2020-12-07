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
