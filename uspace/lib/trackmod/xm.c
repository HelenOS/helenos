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
 * @file Extended Module (.xm).
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include <types/common.h>

#include "byteorder.h"
#include "xm.h"
#include "trackmod.h"
#include "types/xm.h"

static char xm_id_text[] = "Extended Module: ";

/** Load XM order list
 *
 * @param xm_hdr XM header
 * @param module Module
 * @return EOK on success, EIO on format error, ENOMEM if out of memory.
 */
static errno_t trackmod_xm_load_order_list(xm_hdr_t *xm_hdr, trackmod_module_t *module)
{
	errno_t rc;
	size_t i;

	/* Order list */
	module->ord_list_len = uint16_t_le2host(xm_hdr->song_len);
	if (module->ord_list_len > xm_pat_ord_table_size) {
		/* Invalid song length */
		rc = EIO;
		goto error;
	}

	module->ord_list = calloc(sizeof(size_t), module->ord_list_len);
	if (module->ord_list == NULL) {
		printf("Out of memory.\n");
		rc = ENOMEM;
		goto error;
	}

	for (i = 0; i < module->ord_list_len; i++) {
		module->ord_list[i] = xm_hdr->pat_ord_table[i];
	}

	module->restart_pos = uint16_t_le2host(xm_hdr->restart_pos);
	if (module->restart_pos >= module->ord_list_len) {
		rc = EIO;
		goto error;
	}

	return EOK;
error:
	return rc;
}

/** Decode XM pattern.
 *
 * @param data    Packed pattern data
 * @param pattern Pattern to load to
 * @return	  EOK on success, EINVAL if there is error in the coded data.
 */
static errno_t trackmod_xm_decode_pattern(uint8_t *data, size_t dsize,
    trackmod_pattern_t *pattern)
{
	size_t cells;
	size_t i;
	size_t si;
	uint8_t mask;

	cells = pattern->rows * pattern->channels;
	si = 0;

	for (i = 0; i < cells; i++) {
		if (si >= dsize)
			return EINVAL;

		if ((data[si] & 0x80) != 0) {
			mask = data[si++] & 0x1f;
		} else {
			mask = 0x1f;
		}

		/* Note */
		if ((mask & 0x1) != 0) {
			if (si >= dsize)
				return EINVAL;
			pattern->data[i].note = data[si++] & 0x7f;
		}

		/* Instrument */
		if ((mask & 0x2) != 0) {
			if (si >= dsize)
				return EINVAL;
			pattern->data[i].instr = data[si++];
		}

		/* Volume */
		if ((mask & 0x4) != 0) {
			if (si >= dsize)
				return EINVAL;
			pattern->data[i].volume = data[si++];
		}

		/* Effect type */
		if ((mask & 0x8) != 0) {
			if (si >= dsize)
				return EINVAL;
			pattern->data[i].effect = (unsigned)data[si++] << 8;
		}

		/* Effect parameter */
		if ((mask & 0x10) != 0) {
			if (si >= dsize)
				return EINVAL;
			pattern->data[i].effect |= data[si++];
		}
	}

	/* Note: Ignoring any extra data */

	return EOK;
}

/** Load XM patterns
 *
 * @param file File
 * @param module Module
 * @return EOK on success, EIO on format error, ENOMEM if out of memory.
 */
static errno_t trackmod_xm_load_patterns(FILE *f, trackmod_module_t *module)
{
	size_t i;
	size_t hdr_size;
	uint8_t pack_type;
	size_t rows;
	size_t data_size;
	ssize_t nread;
	xm_pattern_t pattern;
	uint8_t *buf = NULL;
	long seek_amount;
	errno_t rc;
	int ret;

	module->pattern = calloc(sizeof(trackmod_pattern_t), module->patterns);
	if (module->pattern == NULL) {
		rc = ENOMEM;
		goto error;
	}

	for (i = 0; i < module->patterns; i++) {
		ret = fread(&pattern, 1, sizeof(xm_pattern_t), f);
		if (ret != sizeof(xm_pattern_t)) {
			rc = EIO;
			goto error;
		}

		hdr_size = (size_t)uint32_t_le2host(pattern.hdr_size);
		pack_type = pattern.pack_type;
		rows = uint16_t_le2host(pattern.rows);
		data_size = uint16_t_le2host(pattern.data_size);

		if (pack_type != 0) {
			rc = EIO;
			goto error;
		}

		/* Jump to end of pattern header */
		seek_amount = (long)hdr_size - (long)sizeof(xm_pattern_t);
		if (fseek(f, seek_amount, SEEK_CUR) < 0) {
			rc = EIO;
			goto error;
		}

		module->pattern[i].rows = rows;
		module->pattern[i].channels = module->channels;
		module->pattern[i].data = calloc(sizeof(trackmod_cell_t),
		    rows * module->channels);

		if (module->pattern[i].data == NULL) {
			rc = ENOMEM;
			goto error;
		}

		buf = calloc(1, data_size);
		if (buf == NULL) {
			rc = ENOMEM;
			goto error;
		}

		nread = fread(buf, 1, data_size, f);
		if (nread != (ssize_t)data_size) {
			rc = EIO;
			goto error;
		}

		rc = trackmod_xm_decode_pattern(buf, data_size,
		    &module->pattern[i]);
		if (rc != EOK)
			goto error;

		free(buf);
		buf = NULL;
	}

	return EOK;
error:
	free(buf);
	return rc;
}

/** Decode XM sample data.
 *
 * XM sample data is delta-encoded. Undo the delta encoding and convert
 * byte order.
 */
static void trackmod_xm_decode_sample_data(trackmod_sample_t *sample)
{
	size_t i;
	int8_t *i8p;
	int16_t *i16p;
	int8_t cur8;
	int16_t cur16;

	if (sample->bytes_smp == 1) {
		cur8 = 0;
		i8p = (int8_t *)sample->data;

		for (i = 0; i < sample->length; i++) {
			cur8 = cur8 + i8p[i];
			i8p[i] = cur8;
		}
	} else {
		cur16 = 0;
		i16p = (int16_t *)sample->data;

		for (i = 0; i < sample->length; i++) {
			cur16 = cur16 + (int16_t)uint16_t_le2host(i16p[i]);
			i16p[i] = cur16;
		}
	}
}

/** Load XM instruments
 *
 * @param file File
 * @param module Module
 * @return EOK on success, EIO on format error, ENOMEM if out of memory.
 */
static errno_t trackmod_xm_load_instruments(xm_hdr_t *xm_hdr, FILE *f,
    trackmod_module_t *module)
{
	size_t i, j;
	xm_instr_t instr;
	xm_instr_ext_t instrx;
	xm_smp_t smp;
	size_t samples;
	size_t instr_size;
	size_t smp_size;
	size_t smp_hdr_size = 0;  /* GCC false alarm on uninitialized */
	ssize_t nread;
	uint8_t ltype;
	trackmod_sample_t *sample;
	void *smp_data;
	long pos;
	errno_t rc;
	int ret;

	module->instrs = uint16_t_le2host(xm_hdr->instruments);
	module->instr = calloc(module->instrs, sizeof(trackmod_instr_t));
	if (module->instr == NULL)
		return ENOMEM;

	for (i = 0; i < module->instrs; i++) {
		pos = ftell(f);
		ret = fread(&instr, 1, sizeof(xm_instr_t), f);
		if (ret != sizeof(xm_instr_t)) {
			rc = EIO;
			goto error;
		}

		samples = uint16_t_le2host(instr.samples);
		instr_size = (size_t)uint32_t_le2host(instr.size);

		if (samples > 0) {
			ret = fread(&instrx, 1, sizeof(xm_instr_ext_t), f);
			if (ret != sizeof(xm_instr_ext_t)) {
				rc = EIO;
				goto error;
			}

			smp_hdr_size = uint32_t_le2host(instrx.smp_hdr_size);

			for (j = 0; j < xm_smp_note_size; j++) {
				module->instr[i].key_smp[j] =
				    instrx.smp_note[j];
			}

			module->instr[i].samples = samples;
			module->instr[i].sample = calloc(samples,
			    sizeof(trackmod_sample_t));
			if (module->instr[i].sample == NULL) {
				rc = ENOMEM;
				goto error;
			}
		}

		if (fseek(f, pos + instr_size, SEEK_SET) < 0) {
			rc = EIO;
			goto error;
		}

		for (j = 0; j < samples; j++) {
			sample = &module->instr[i].sample[j];
			pos = ftell(f);

			ret = fread(&smp, 1, sizeof(xm_smp_t), f);
			if (ret != sizeof(xm_smp_t)) {
				rc = EIO;
				goto error;
			}

			smp_size = (size_t)uint32_t_le2host(smp.length);

			smp_data = calloc(smp_size, 1);
			if (smp_data == NULL) {
				rc = ENOMEM;
				goto error;
			}

			if (fseek(f, pos + smp_hdr_size, SEEK_SET) < 0) {
				rc = EIO;
				goto error;
			}

			nread = fread(smp_data, 1, smp_size, f);
			if (nread != (ssize_t)smp_size) {
				rc = EIO;
				goto error;
			}

			if (smp.smp_type & (1 << xmst_16_bit)) {
				sample->bytes_smp = 2;
			} else {
				sample->bytes_smp = 1;
			}

			sample->data = smp_data;
			sample->length = smp_size / sample->bytes_smp;

			ltype = smp.smp_type & 0x3;
			switch (ltype) {
			case xmsl_no_loop:
				sample->loop_type = tl_no_loop;
				break;
			case xmsl_forward_loop:
				sample->loop_type = tl_forward_loop;
				break;
			case xmsl_pingpong_loop:
				sample->loop_type = tl_pingpong_loop;
				break;
			default:
				rc = EIO;
				goto error;
			}

			sample->loop_start =
			    uint32_t_le2host(smp.loop_start) / sample->bytes_smp;
			sample->loop_len =
			    uint32_t_le2host(smp.loop_len) / sample->bytes_smp;
			sample->def_vol = 0x40;
			sample->rel_note = smp.rel_note;
			sample->finetune = smp.finetune / 2;

			trackmod_xm_decode_sample_data(sample);
		}
	}

	return EOK;
error:
	return rc;
}

/** Load extended module.
 *
 * @param fname   File name
 * @param rmodule Place to store pointer to newly loaded module.
 * @return        EOK on success, ENONEM if out of memory, EIO on I/O error
 *                or if any error is found in the format of the file.
 */
errno_t trackmod_xm_load(char *fname, trackmod_module_t **rmodule)
{
	FILE *f = NULL;
	trackmod_module_t *module = NULL;
	xm_hdr_t xm_hdr;
	size_t nread;
	size_t hdr_size;
	errno_t rc;

	f = fopen(fname, "rb");
	if (f == NULL) {
		printf("Error opening file.\n");
		rc = EIO;
		goto error;
	}

	nread = fread(&xm_hdr, 1, sizeof(xm_hdr_t), f);
	if (nread < sizeof(xm_hdr_t)) {
		printf("File too small.\n");
		rc = EIO;
		goto error;
	}

	if (memcmp(xm_hdr.id_text, xm_id_text, xm_id_text_size) != 0) {
		rc = EIO;
		goto error;
	}

	module = trackmod_module_new();
	if (module == NULL) {
		printf("Out of memory.\n");
		rc = ENOMEM;
		goto error;
	}

	module->channels = uint16_t_le2host(xm_hdr.channels);
	module->patterns = uint16_t_le2host(xm_hdr.patterns);
	module->ord_list_len = uint16_t_le2host(xm_hdr.song_len);

	hdr_size = (size_t)uint32_t_le2host(xm_hdr.hdr_size) +
	    offsetof(xm_hdr_t, hdr_size);

	module->def_bpm = uint16_t_le2host(xm_hdr.def_bpm);
	module->def_tpr = uint16_t_le2host(xm_hdr.def_tempo);

	/* Jump to end of file header */
	if (fseek(f, hdr_size, SEEK_SET) < 0) {
		rc = EIO;
		goto error;
	}

	rc = trackmod_xm_load_order_list(&xm_hdr, module);
	if (rc != EOK)
		goto error;

	rc = trackmod_xm_load_patterns(f, module);
	if (rc != EOK)
		goto error;

	rc = trackmod_xm_load_instruments(&xm_hdr, f, module);
	if (rc != EOK)
		goto error;

	(void) fclose(f);

	*rmodule = module;
	return EOK;
error:
	if (module != NULL)
		trackmod_module_destroy(module);
	if (f != NULL)
		(void) fclose(f);
	return rc;
}

/** @}
 */
