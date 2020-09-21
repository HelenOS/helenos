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
 * @file Waveform Audio File Format (WAVE).
 */

#include <assert.h>
#include <byteorder.h>
#include <errno.h>
#include <macros.h>
#include <mem.h>
#include <riff/chunk.h>
#include <riff/rwave.h>
#include <stdlib.h>

/** Encode format chunk data.
 *
 * @params params	WAVE parameters
 * @params fmt		Pointer to fmt chunk data structure to fill in
 */
static void rwave_encode_fmt(rwave_params_t *params, rwave_fmt_t *fmt)
{
	int bytes_smp;

	bytes_smp = (params->bits_smp + 7) / 8;

	fmt->format_tag = host2uint16_t_le(WFMT_PCM);
	fmt->channels = host2uint16_t_le(params->channels);
	fmt->smp_sec = host2uint32_t_le(params->smp_freq);
	fmt->avg_bytes_sec = host2uint32_t_le(bytes_smp * params->smp_freq *
	    params->channels);
	fmt->block_align = host2uint16_t_le(bytes_smp);
	fmt->bits_smp = host2uint16_t_le(params->bits_smp);
}

/** Decode format chunk data.
 *
 * @param fmt	 Pointer to format chunk data
 * @param params WAVE parameters to fill in
 *
 * @return EOK on success, EINVAL if format is not supported.
 */
static int rwave_decode_fmt(rwave_fmt_t *fmt, rwave_params_t *params)
{
	uint16_t fmt_tag;

	fmt_tag = uint16_t_le2host(fmt->format_tag);
	printf("fmt_tag=0x%x\n", fmt_tag);
	if (fmt_tag != WFMT_PCM)
		return EINVAL;

	params->channels = uint16_t_le2host(fmt->channels);
	params->smp_freq = uint32_t_le2host(fmt->smp_sec);
	params->bits_smp = uint16_t_le2host(fmt->bits_smp);

	return EOK;
}

/** Open WAVE file for writing.
 *
 * @param fname  File name
 * @param params WAVE file parameters
 * @param rww    Place to store pointer to WAVE writer
 *
 * @return EOK on success, EIO on I/O error, ENOMEM if out of memory.
 */
errno_t rwave_wopen(const char *fname, rwave_params_t *params, rwavew_t **rww)
{
	riff_wchunk_t fmt;
	rwave_fmt_t rwfmt;
	errno_t rc;
	rwavew_t *ww;

	rwave_encode_fmt(params, &rwfmt);

	ww = calloc(1, sizeof(rwavew_t));
	if (ww == NULL) {
		rc = ENOMEM;
		goto error;
	}

	ww->bufsize = 4096;
	ww->buf = calloc(1, ww->bufsize);
	if (ww->buf == NULL) {
		rc = ENOMEM;
		goto error;
	}

	/* Make a copy of parameters */
	ww->params = *params;

	rc = riff_wopen(fname, &ww->rw);
	if (rc != EOK) {
		assert(rc == EIO || rc == ENOMEM);
		goto error;
	}

	rc = riff_wchunk_start(ww->rw, CKID_RIFF, &ww->wave);
	if (rc != EOK)
		goto error;

	rc = riff_write_uint32(ww->rw, FORM_WAVE);
	if (rc != EOK)
		goto error;

	rc = riff_wchunk_start(ww->rw, CKID_fmt, &fmt);
	if (rc != EOK)
		goto error;

	rc = riff_write(ww->rw, &rwfmt, sizeof(rwfmt));
	if (rc != EOK)
		goto error;

	rc = riff_wchunk_end(ww->rw, &fmt);
	if (rc != EOK)
		goto error;

	rc = riff_wchunk_start(ww->rw, CKID_data, &ww->data);
	if (rc != EOK)
		goto error;

	*rww = ww;
	return EOK;
error:
	if (ww != NULL)
		free(ww->buf);
	if (ww->rw != NULL)
		riff_wclose(ww->rw);
	free(ww);
	return rc;
}

/** Write samples to WAVE file.
 *
 * @param ww    WAVE writer
 * @param data  Pointer to data
 * @param bytes Number of bytes to write
 *
 * @return EOK on success, EIO on I/O error, ENOTSUP if sample format is
 *         not supported.
 */
errno_t rwave_write_samples(rwavew_t *ww, void *data, size_t bytes)
{
	size_t i;
	uint16_t *d16, *b16;
	size_t now;
	errno_t rc;

	/* Convert sample data to little endian */

	while (bytes > 0) {
		now = min(bytes, ww->bufsize);

		switch (ww->params.bits_smp / 8) {
		case 1:
			memcpy(ww->buf, data, now);
			break;
		case 2:
			b16 = (uint16_t *)ww->buf;
			d16 = (uint16_t *)data;
			for (i = 0; i < now / 2; i++) {
				b16[i] = host2uint16_t_le(d16[i]);
			}
			break;
		default:
			return ENOTSUP;
		}

		rc = riff_write(ww->rw, ww->buf, now);
		if (rc != EOK) {
			assert(rc == EIO);
			return rc;
		}

		bytes -= now;
		data += now;
	}

	return EOK;
}

/** Close WAVE file for writing.
 *
 * @param ww WAVE writer
 * @return EOK on success, EIO on I/O error - in which case @a ww is destroyed
 *         anyway.
 */
errno_t rwave_wclose(rwavew_t *ww)
{
	errno_t rc;

	rc = riff_wchunk_end(ww->rw, &ww->wave);
	if (rc == EOK)
		rc = riff_wchunk_end(ww->rw, &ww->data);

	rc = riff_wclose(ww->rw);

	ww->rw = NULL;
	free(ww->buf);
	free(ww);

	return rc;
}

/** Open WAVE file for reading.
 *
 * @param fname  File name
 * @param params WAVE file parameters
 * @param rwaver Place to store pointer to new wave reader
 *
 * @return EOK on success, EIO on I/O error, ENOMEM if out of memory
 */
errno_t rwave_ropen(const char *fname, rwave_params_t *params, rwaver_t **rwr)
{
	rwaver_t *wr = NULL;
	uint32_t form_id;
	riff_rchunk_t fmt;
	rwave_fmt_t wfmt;
	size_t nread;
	errno_t rc;

	wr = calloc(1, sizeof(rwaver_t));
	if (wr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = riff_ropen(fname, &wr->wave, &wr->rr);
	if (rc != EOK) {
		assert(rc == EIO || rc == ENOMEM);
		goto error;
	}

	if (wr->wave.ckid != CKID_RIFF) {
		printf("Not RIFF file\n");
		rc = ENOMEM;
		goto error;
	}

	rc = riff_read_uint32(&wr->wave, &form_id);
	if (rc != EOK) {
		assert(rc == EIO);
		goto error;
	}

	if (form_id != FORM_WAVE) {
		printf("wrong form ID\n");
		rc = EIO;
		goto error;
	}

	rc = riff_rchunk_start(&wr->wave, &fmt);
	if (rc != EOK) {
		assert(rc == EIO);
		goto error;
	}

	if (fmt.ckid != CKID_fmt) {
		printf("not fmt chunk\n");
		rc = ENOMEM;
		goto error;
	}

	rc = riff_read(&fmt, &wfmt, sizeof(rwave_fmt_t), &nread);
	if (rc != EOK) {
		printf("error reading fmt chunk\n");
		assert(rc == EIO || rc == ELIMIT);
		rc = EIO;
		goto error;
	}

	if (nread < sizeof(rwave_fmt_t)) {
		rc = EIO;
		goto error;
	}

	rc = riff_rchunk_end(&fmt);
	if (rc != EOK) {
		assert(rc == EIO);
		goto error;
	}

	rc = rwave_decode_fmt(&wfmt, params);
	if (rc != EOK) {
		printf("decode fmt fail\n");
		assert(rc == EINVAL);
		rc = EIO;
		goto error;
	}

	rc = riff_rchunk_start(&wr->wave, &wr->data);
	if (rc != EOK) {
		assert(rc == EIO);
		goto error;
	}

	if (wr->data.ckid != CKID_data) {
		printf("not data ckid\n");
		rc = EIO;
		goto error;
	}

	*rwr = wr;
	return EOK;
error:
	if (wr != NULL && wr->rr != NULL)
		riff_rclose(wr->rr);
	free(wr);
	return rc;
}

/** Read samples from WAVE file.
 *
 * @param wr    WAVE reader
 * @param buf   Buffer for storing the data
 * @param bytes Number of bytes to read
 * @param nread Place to store number of bytes actually read (>= 0).
 *
 * @return EOK if zero or more bytes successfully read and @a *nread is set,
 *         EIO on I/O error.
 */
errno_t rwave_read_samples(rwaver_t *wr, void *buf, size_t bytes, size_t *nread)
{
	errno_t rc;

	rc = riff_read(&wr->data, buf, bytes, nread);
	if (rc != EOK) {
		assert(rc == EIO || rc == ELIMIT);
		return EIO;
	}

	return EOK;
}

/** Close WAVE file for reading.
 *
 * @param wr WAVE reader
 *
 * @return EOK on success, EIO on I/O error in which case @a wr is destroyed
 *         anyway.
 */
errno_t rwave_rclose(rwaver_t *wr)
{
	errno_t rc;

	rc = riff_rchunk_end(&wr->wave);
	if (rc != EOK) {
		assert(rc == EIO);
		goto error;
	}

	riff_rclose(wr->rr);
	free(wr);
	return EOK;
error:
	return rc;
}

/** @}
 */
