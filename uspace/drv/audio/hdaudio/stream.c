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
#include <bitops.h>
#include <byteorder.h>
#include <ddf/log.h>
#include <ddi.h>
#include <errno.h>
#include <str_error.h>
#include <macros.h>
#include <stdlib.h>

#include "hdactl.h"
#include "hdaudio.h"
#include "regif.h"
#include "spec/bdl.h"
#include "stream.h"

errno_t hda_stream_buffers_alloc(hda_t *hda, hda_stream_buffers_t **rbufs)
{
	void *bdl;
	void *buffer;
	uintptr_t buffer_phys;
	hda_stream_buffers_t *bufs = NULL;
	size_t i;
//	size_t j, k;
	errno_t rc;

	bufs = calloc(1, sizeof(hda_stream_buffers_t));
	if (bufs == NULL) {
		rc = ENOMEM;
		goto error;
	}

	bufs->nbuffers = 4;
	bufs->bufsize = 16384;

	/*
	 * BDL must be aligned to 128 bytes. If 64OK is not set,
	 * it must be within the 32-bit address space.
	 */
	bdl = AS_AREA_ANY;
	rc = dmamem_map_anonymous(bufs->nbuffers * sizeof(hda_buffer_desc_t),
	    hda->ctl->ok64bit ? 0 : DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE,
	    0, &bufs->bdl_phys, &bdl);
	if (rc != EOK)
		goto error;

	bufs->bdl = bdl;

	/* Allocate arrays of buffer pointers */

	bufs->buf = calloc(bufs->nbuffers, sizeof(void *));
	if (bufs->buf == NULL)
		goto error;

	bufs->buf_phys = calloc(bufs->nbuffers, sizeof(uintptr_t));
	if (bufs->buf_phys == NULL)
		goto error;

	/* Allocate buffers */
/*
	for (i = 0; i < bufs->nbuffers; i++) {
		buffer = AS_AREA_ANY;
		rc = dmamem_map_anonymous(bufs->bufsize,
		    bufs->hda->ctl->ok64bit ? 0 : DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE,
		    0, &buffer_phys, &buffer);
		if (rc != EOK)
			goto error;

		ddf_msg(LVL_NOTE, "Stream buf phys=0x%llx virt=%p",
		    (unsigned long long)buffer_phys, buffer);

		bufs->buf[i] = buffer;
		bufs->buf_phys[i] = buffer_phys;

		k = 0;
		for (j = 0; j < bufs->bufsize / 2; j++) {
			int16_t *bp = bufs->buf[i];
			bp[j] = (k > 128) ? -100 : 100;
			++k;
			if (k >= 256)
				k = 0;
		}
	}
*/
	/* audio_pcm_iface requires a single contiguous buffer */
	buffer = AS_AREA_ANY;
	rc = dmamem_map_anonymous(bufs->bufsize * bufs->nbuffers,
	    hda->ctl->ok64bit ? 0 : DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE,
	    0, &buffer_phys, &buffer);
	if (rc != EOK) {
		ddf_msg(LVL_NOTE, "dmamem_map_anon -> %s", str_error_name(rc));
		goto error;
	}

	for (i = 0; i < bufs->nbuffers; i++) {
		bufs->buf[i] = buffer + i * bufs->bufsize;
		bufs->buf_phys[i] = buffer_phys + i * bufs->bufsize;

		ddf_msg(LVL_NOTE, "Stream buf phys=0x%llx virt=%p",
		    (long long unsigned)(uintptr_t)bufs->buf[i],
		    (void *)bufs->buf_phys[i]);
/*		k = 0;
		for (j = 0; j < bufs->bufsize / 2; j++) {
			int16_t *bp = bufs->buf[i];
			bp[j] = (k > 128) ? -10000 : 10000;
			++k;
			if (k >= 256)
				k = 0;
		}
*/
	}

	/* Fill in BDL */
	for (i = 0; i < bufs->nbuffers; i++) {
		bufs->bdl[i].address = host2uint64_t_le(bufs->buf_phys[i]);
		bufs->bdl[i].length = host2uint32_t_le(bufs->bufsize);
		bufs->bdl[i].flags = BIT_V(uint32_t, bdf_ioc);
	}

	*rbufs = bufs;
	return EOK;
error:
	hda_stream_buffers_free(bufs);
	return ENOMEM;
}

void hda_stream_buffers_free(hda_stream_buffers_t *bufs)
{
	if (bufs == NULL)
		return;

	/* XXX */
	free(bufs);
}

static void hda_stream_desc_configure(hda_stream_t *stream)
{
	hda_sdesc_regs_t *sdregs;
	hda_stream_buffers_t *bufs = stream->buffers;
	uint8_t ctl1;
	uint8_t ctl3;

	ctl3 = (stream->sid << 4);
	ctl1 = 0x4;

	sdregs = &stream->hda->regs->sdesc[stream->sdid];
	hda_reg8_write(&sdregs->ctl3, ctl3);
	hda_reg8_write(&sdregs->ctl1, ctl1);
	hda_reg32_write(&sdregs->cbl, bufs->nbuffers * bufs->bufsize);
	hda_reg16_write(&sdregs->lvi, bufs->nbuffers - 1);
	hda_reg16_write(&sdregs->fmt, stream->fmt);
	hda_reg32_write(&sdregs->bdpl, LOWER32(bufs->bdl_phys));
	hda_reg32_write(&sdregs->bdpu, UPPER32(bufs->bdl_phys));
}

static void hda_stream_set_run(hda_stream_t *stream, bool run)
{
	uint32_t ctl;
	hda_sdesc_regs_t *sdregs;

	sdregs = &stream->hda->regs->sdesc[stream->sdid];

	ctl = hda_reg8_read(&sdregs->ctl1);
	if (run)
		ctl = ctl | BIT_V(uint8_t, sdctl1_run);
	else
		ctl = ctl & ~BIT_V(uint8_t, sdctl1_run);

	hda_reg8_write(&sdregs->ctl1, ctl);
}

static void hda_stream_reset_noinit(hda_stream_t *stream)
{
	uint32_t ctl;
	hda_sdesc_regs_t *sdregs;

	sdregs = &stream->hda->regs->sdesc[stream->sdid];

	ctl = hda_reg8_read(&sdregs->ctl1);
	ctl = ctl | BIT_V(uint8_t, sdctl1_srst);
	hda_reg8_write(&sdregs->ctl1, ctl);

	async_usleep(100 * 1000);

	ctl = hda_reg8_read(&sdregs->ctl1);
	ctl = ctl & ~BIT_V(uint8_t, sdctl1_srst);
	hda_reg8_write(&sdregs->ctl1, ctl);

	async_usleep(100 * 1000);
}

hda_stream_t *hda_stream_create(hda_t *hda, hda_stream_dir_t dir,
    hda_stream_buffers_t *bufs, uint32_t fmt)
{
	hda_stream_t *stream;
	uint8_t sdid;

	stream = calloc(1, sizeof(hda_stream_t));
	if (stream == NULL)
		return NULL;

	sdid = 0;

	switch (dir) {
	case sdir_input:
		sdid = 0; /* XXX Allocate - first input SDESC */
		break;
	case sdir_output:
		sdid = hda->ctl->iss; /* XXX Allocate - First output SDESC */
		break;
	case sdir_bidi:
		sdid = hda->ctl->iss + hda->ctl->oss; /* XXX Allocate - First bidi SDESC */
		break;
	}

	stream->hda = hda;
	stream->dir = dir;
	stream->sid = 1; /* XXX Allocate this */
	stream->sdid = sdid;
	stream->fmt = fmt;
	stream->buffers = bufs;

	ddf_msg(LVL_NOTE, "snum=%d sdidx=%d", stream->sid, stream->sdid);

	ddf_msg(LVL_NOTE, "Configure stream descriptor");
	hda_stream_desc_configure(stream);
	return stream;
}

void hda_stream_destroy(hda_stream_t *stream)
{
	ddf_msg(LVL_NOTE, "hda_stream_destroy()");
	hda_stream_reset_noinit(stream);
	free(stream);
}

void hda_stream_start(hda_stream_t *stream)
{
	ddf_msg(LVL_NOTE, "hda_stream_start()");
	hda_stream_set_run(stream, true);
}

void hda_stream_stop(hda_stream_t *stream)
{
	ddf_msg(LVL_NOTE, "hda_stream_stop()");
	hda_stream_set_run(stream, false);
}

void hda_stream_reset(hda_stream_t *stream)
{
	ddf_msg(LVL_NOTE, "hda_stream_reset()");
	hda_stream_reset_noinit(stream);
	hda_stream_desc_configure(stream);
}

/** @}
 */
