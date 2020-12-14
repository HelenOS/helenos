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
 * @file Protracker module (.mod).
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>

#include "byteorder.h"
#include "protracker.h"
#include "trackmod.h"
#include "types/protracker.h"

/** Sample tag decoding table entry */
typedef struct {
	/** Tag */
	const char *tag;
	/** Number of channels */
	unsigned channels;
} smptag_desc_t;

/** Sample tag decoding table */
static smptag_desc_t smp_tags[] = {
	{ .tag = "M.K.", .channels = 4 },
	{ .tag = "M!K!", .channels = 4 },
	{ .tag = "2CHN", .channels = 2 },
	{ .tag = "6CHN", .channels = 6 },
	{ .tag = "8CHN", .channels = 8 },
	{ .tag = "10CH", .channels = 10 },
	{ .tag = "12CH", .channels = 12 },
	{ .tag = "14CH", .channels = 14 },
	{ .tag = "16CH", .channels = 16 },
	{ .tag = "18CH", .channels = 18 },
	{ .tag = "20CH", .channels = 20 },
	{ .tag = "22CH", .channels = 22 },
	{ .tag = "24CH", .channels = 24 },
	{ .tag = "26CH", .channels = 26 },
	{ .tag = "28CH", .channels = 28 },
	{ .tag = "30CH", .channels = 30 },
	{ .tag = "32CH", .channels = 32 },
};

/** Decode sample tag.
 *
 * @param tag      Tag
 * @param channels Place to store number or channels.
 * @return         EOK on success, EINVAL if tag is not recognized.
 */
static errno_t smp_tag_decode(uint8_t *tag, size_t *channels)
{
	size_t nentries = sizeof(smp_tags) / sizeof(smptag_desc_t);
	size_t i;

	for (i = 0; i < nentries; i++) {
		if (memcmp(tag, smp_tags[i].tag, 4) == 0) {
			*channels = smp_tags[i].channels;
			return EOK;
		}
	}

	return EINVAL;
}

/** Get number of patterns stored in file.
 *
 * @param olist Order list
 * @return      Number of patterns in file
 */
static size_t order_list_get_npatterns(protracker_order_list_t *olist)
{
	size_t i;
	size_t max_pat;

	max_pat = 0;
	for (i = 0; i < protracker_olist_len; i++) {
		if (olist->order_list[i] > max_pat)
			max_pat = olist->order_list[i];
	}

	return 1 + max_pat;
}

/** Decode pattern cell.
 *
 * @param pattern Pattern
 * @param row     Row number
 * @param channel Channel number
 * @param cell    Place to store decoded cell
 */
static void protracker_decode_cell(uint32_t cdata, trackmod_cell_t *cell)
{
	uint32_t code;

	code = uint32_t_be2host(cdata);
	cell->period = (code >> (4 * 4)) & 0xfff;
	cell->instr = (((code >> (7 * 4)) & 0xf) << 4) |
	    ((code >> (3 * 4)) & 0xf);
	cell->effect = code & 0xfff;
}

/** Load Protracker patterns.
 *
 * @param f      File to read from
 * @param module Module being loaded to
 * @return       EOK on success, ENOMEM if out of memory, EIO on I/O error.
 */
static errno_t protracker_load_patterns(FILE *f, trackmod_module_t *module)
{
	size_t cells;
	size_t i, j;
	errno_t rc;
	size_t nread;
	uint32_t *buf = NULL;

	cells = module->channels * protracker_pattern_rows;
	buf = calloc(sizeof(uint32_t), cells);

	if (buf == NULL) {
		rc = ENOMEM;
		goto error;
	}

	for (i = 0; i < module->patterns; i++) {
		module->pattern[i].rows = protracker_pattern_rows;
		module->pattern[i].channels = module->channels;
		module->pattern[i].data = calloc(sizeof(trackmod_cell_t), cells);
		if (module->pattern[i].data == NULL) {
			rc = ENOMEM;
			goto error;
		}

		nread = fread(buf, sizeof(uint32_t), cells, f);
		if (nread != cells) {
			printf("Error reading pattern.\n");
			rc = EIO;
			goto error;
		}

		/* Decode cells */
		for (j = 0; j < cells; j++) {
			protracker_decode_cell(buf[j],
			    &module->pattern[i].data[j]);
		}
	}

	free(buf);
	return EOK;
error:
	free(buf);
	return rc;
}

/** Load protracker samples.
 *
 * @param f      File being read from
 * @param sample Sample header
 * @param module Module being loaded to
 * @return       EOk on success, ENOMEM if out of memory, EIO on I/O error.
 */
static errno_t protracker_load_samples(FILE *f, protracker_smp_t *smp,
    trackmod_module_t *module)
{
	errno_t rc;
	size_t i;
	uint8_t ftval;
	size_t nread;
	trackmod_sample_t *sample;

	for (i = 0; i < module->instrs; i++) {
		module->instr[i].samples = 1;
		module->instr[i].sample = calloc(1, sizeof(trackmod_sample_t));
		if (module->instr[i].sample == NULL) {
			printf("Error allocating sample.\n");
			rc = ENOMEM;
			goto error;
		}

		sample = &module->instr[i].sample[0];
		sample->length =
		    uint16_t_be2host(smp[i].length) * 2;
		sample->bytes_smp = 1;
		sample->data = calloc(1, sample->length);
		if (sample->data == NULL) {
			printf("Error allocating sample.\n");
			rc = ENOMEM;
			goto error;
		}

		nread = fread(sample->data, 1, sample->length, f);
		if (nread != sample->length) {
			printf("Error reading sample.\n");
			rc = EIO;
			goto error;
		}

		sample->def_vol = smp[i].def_vol;

		sample->loop_start =
		    uint16_t_be2host(smp[i].loop_start) * 2;
		sample->loop_len =
		    uint16_t_be2host(smp[i].loop_len) * 2;
		if (sample->loop_len <= 2)
			sample->loop_type = tl_no_loop;
		else
			sample->loop_type = tl_forward_loop;

		/* Finetune is a 4-bit signed value. */
		ftval = smp[i].finetune & 0x0f;
		sample->finetune =
		    (ftval & 0x8) ? (ftval & 0x7) - 8 : ftval;
	}

	return EOK;
error:
	return rc;
}

/** Load protracker module.
 *
 * @param fname   File name
 * @param rmodule Place to store pointer to newly loaded module.
 * @return        EOK on success, ENONEM if out of memory, EIO on I/O error
 *                or if any error is found in the format of the file.
 */
errno_t trackmod_protracker_load(char *fname, trackmod_module_t **rmodule)
{
	FILE *f = NULL;
	trackmod_module_t *module = NULL;
	protracker_31smp_t mod31;
	protracker_15smp_t mod15;
	protracker_order_list_t *order_list;
	protracker_smp_t *sample;
	size_t samples;
	size_t channels;
	size_t patterns;
	size_t i;
	size_t nread;
	errno_t rc;

	f = fopen(fname, "rb");
	if (f == NULL) {
		printf("Error opening file.\n");
		rc = EIO;
		goto error;
	}

	nread = fread(&mod31, 1, sizeof(protracker_31smp_t), f);
	if (nread < sizeof(protracker_15smp_t)) {
		printf("File too small.\n");
		rc = EIO;
		goto error;
	}

	if (nread == sizeof(protracker_31smp_t)) {
		/* Could be 31-sample variant */
		rc = smp_tag_decode(mod31.sample_tag, &channels);
		if (rc != EOK) {
			samples = 15;
			channels = 4;
		} else {
			samples = 31;
		}
	} else {
		samples = 15;
		channels = 4;
	}

	if (samples == 15) {
		memcpy(&mod15, &mod31, sizeof(protracker_15smp_t));

		if (fseek(f, sizeof(protracker_15smp_t), SEEK_SET) < 0) {
			printf("Error seeking.\n");
			rc = EIO;
			goto error;
		}

		order_list = &mod15.order_list;
		sample = mod15.sample;
	} else {
		order_list = &mod31.order_list;
		sample = mod31.sample;
	}

	patterns = order_list_get_npatterns(order_list);

	module = trackmod_module_new();
	if (module == NULL) {
		printf("Out of memory.\n");
		rc = ENOMEM;
		goto error;
	}

	module->channels = channels;

	module->instrs = samples;
	module->instr = calloc(sizeof(trackmod_instr_t), samples);
	if (module->instr == NULL) {
		printf("Out of memory.\n");
		rc = ENOMEM;
		goto error;
	}

	module->patterns = patterns;
	module->pattern = calloc(sizeof(trackmod_pattern_t), patterns);
	if (module->pattern == NULL) {
		printf("Out of memory.\n");
		rc = ENOMEM;
		goto error;
	}

	/* Order list */
	module->ord_list_len = order_list->order_list_len;
	module->ord_list = calloc(sizeof(size_t), module->ord_list_len);
	if (module->ord_list == NULL) {
		printf("Out of memory.\n");
		rc = ENOMEM;
		goto error;
	}

	for (i = 0; i < order_list->order_list_len; i++) {
		module->ord_list[i] = order_list->order_list[i];
	}

	/* The 'mark' byte may or may not contain a valid restart position */
	if (order_list->mark < order_list->order_list_len) {
		module->restart_pos = order_list->mark;
	}

	/* Load patterns */
	rc = protracker_load_patterns(f, module);
	if (rc != EOK)
		goto error;

	/* Load samples */
	rc = protracker_load_samples(f, sample, module);
	if (rc != EOK)
		goto error;

	(void) fclose(f);

	module->def_bpm = protracker_def_bpm;
	module->def_tpr = protracker_def_tpr;

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
