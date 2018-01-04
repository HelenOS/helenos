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

/** @addtogroup hdaudio
 * @{
 */
/** @file High Definition Audio stream
 */

#ifndef STREAM_H
#define STREAM_H

#include "hdaudio.h"
#include "spec/bdl.h"

typedef enum {
	/** Input Stream */
	sdir_input,
	/** Output Stream */
	sdir_output,
	/** Bidirectional Stream */
	sdir_bidi
} hda_stream_dir_t;

typedef struct hda_stream_buffers {
	/** Number of buffers */
	size_t nbuffers;
	/** Buffer size */
	size_t bufsize;
	/** Buffer Descriptor List */
	hda_buffer_desc_t *bdl;
	/** Physical address of BDL */
	uintptr_t bdl_phys;
	/** Buffers */
	void **buf;
	/** Physical addresses of buffers */
	uintptr_t *buf_phys;
} hda_stream_buffers_t;

typedef struct hda_stream {
	hda_t *hda;
	/** Stream ID */
	uint8_t sid;
	/** Stream descriptor index */
	uint8_t sdid;
	/** Direction */
	hda_stream_dir_t dir;
	/** Buffers */
	hda_stream_buffers_t *buffers;
	/** Stream format */
	uint32_t fmt;
} hda_stream_t;

extern errno_t hda_stream_buffers_alloc(hda_t *, hda_stream_buffers_t **);
extern void hda_stream_buffers_free(hda_stream_buffers_t *);
extern hda_stream_t *hda_stream_create(hda_t *, hda_stream_dir_t,
    hda_stream_buffers_t *, uint32_t);
extern void hda_stream_destroy(hda_stream_t *);
extern void hda_stream_start(hda_stream_t *);
extern void hda_stream_stop(hda_stream_t *);
extern void hda_stream_reset(hda_stream_t *);

#endif

/** @}
 */
