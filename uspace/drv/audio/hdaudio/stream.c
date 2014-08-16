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

#include <as.h>
#include <ddf/log.h>
#include <ddi.h>
#include <errno.h>
#include <stdlib.h>

#include "hdactl.h"
#include "hdaudio.h"
#include "regif.h"
#include "spec/bdl.h"
#include "stream.h"

static int hda_stream_buffers_alloc(hda_stream_t *stream)
{
	void *bdl;
	void *buffer;
	uintptr_t buffer_phys;
	size_t i;
	size_t j, k;
	int rc;

	stream->nbuffers = 2;
	stream->bufsize = 16384;

	/*
	 * BDL must be aligned to 128 bytes. If 64OK is not set,
	 * it must be within the 32-bit address space.
	 */
	bdl = AS_AREA_ANY;
	rc = dmamem_map_anonymous(stream->nbuffers * sizeof(hda_buffer_desc_t),
	    stream->hda->ctl->ok64bit ? 0 : DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE,
	    0, &stream->bdl_phys, &bdl);
	if (rc != EOK)
		goto error;

	stream->bdl = bdl;

	/* Allocate arrays of buffer pointers */

	stream->buf = calloc(stream->nbuffers, sizeof(void *));
	if (stream->buf == NULL)
		goto error;

	stream->buf_phys = calloc(stream->nbuffers, sizeof(uintptr_t));
	if (stream->buf_phys == NULL)
		goto error;

	/* Allocate buffers */

	for (i = 0; i < stream->nbuffers; i++) {
		buffer = AS_AREA_ANY;
		rc = dmamem_map_anonymous(stream->bufsize,
		    stream->hda->ctl->ok64bit ? 0 : DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE,
		    0, &buffer_phys, &buffer);
		if (rc != EOK)
			goto error;

		stream->buf[i] = buffer;
		stream->buf_phys[i] = buffer_phys;

		k = 0;
		for (j = 0; j < stream->bufsize / 2; j++) {
			int16_t *bp = stream->buf[i];
			bp[j] = (k > 128) ? -10000 : 10000;
			++k;
			if (k >= 256)
				k = 0;
		}
	}

	/* Fill in BDL */
	for (i = 0; i < stream->nbuffers; i++) {
		stream->bdl[i].address = stream->buf_phys[i];
		stream->bdl[i].length = stream->bufsize;
		stream->bdl[i].flags = 0;
	}

	return EOK;
error:
	return ENOMEM;
}

static void hda_stream_desc_configure(hda_stream_t *stream)
{
	hda_sdesc_regs_t *sdregs;

	sdregs = &stream->hda->regs->sdesc[stream->sdid];
	hda_reg32_write(&sdregs->cbl, stream->nbuffers * stream->bufsize);
	hda_reg16_write(&sdregs->lvi, stream->nbuffers - 1);
	hda_reg16_write(&sdregs->fmt, stream->fmt);
}

static void hda_stream_set_run(hda_stream_t *stream, bool run)
{
	uint32_t ctl;
	hda_sdesc_regs_t *sdregs;

	sdregs = &stream->hda->regs->sdesc[stream->sdid];

	ctl = hda_reg8_read(&sdregs->ctl1);
	ctl = ctl | 2 /* XXX Run */;
	hda_reg8_write(&sdregs->ctl1, ctl);
}

hda_stream_t *hda_stream_create(hda_t *hda, hda_stream_dir_t dir,
    uint32_t fmt)
{
	hda_stream_t *stream;
	int rc;

	stream = calloc(1, sizeof(hda_stream_t));
	if (stream == NULL)
		return NULL;

	stream->hda = hda;
	stream->dir = dir;
	stream->sid = 1; /* XXX Allocate this */
	stream->sdid = hda->ctl->iss; /* XXX Allocate - First output SDESC */
	stream->fmt = fmt;

	ddf_msg(LVL_NOTE, "snum=%d sdidx=%d", stream->sid, stream->sdid);

	ddf_msg(LVL_NOTE, "Allocate buffers");
	rc = hda_stream_buffers_alloc(stream);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_NOTE, "Configure stream descriptor");
	hda_stream_desc_configure(stream);
	return stream;
error:
	return NULL;
}

void hda_stream_destroy(hda_stream_t *stream)
{
	ddf_msg(LVL_NOTE, "hda_stream_destroy()");
	free(stream);
}

void hda_stream_start(hda_stream_t *stream)
{
	hda_stream_set_run(stream, true);
}

/** @}
 */
