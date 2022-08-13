/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
