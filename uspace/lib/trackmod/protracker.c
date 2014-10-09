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

/** Sample tag decoding table entry*/
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
static int smp_tag_decode(uint8_t *tag, size_t *channels)
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

/** Load protracker module.
 *
 * @param fname   File name
 * @param rmodule Place to store pointer to newly loaded module.
 * @return        EOK on success, ENONEM if out of memory, EIO on I/O error
 *                or if any error is found in the format of the file.
 */
int trackmod_protracker_load(char *fname, trackmod_module_t **rmodule)
{
	FILE *f = NULL;
	trackmod_module_t *module = NULL;
	protracker_31smp_t mod31;
	protracker_15smp_t mod15;
	protracker_order_list_t *order_list;
	protracker_smp_t *sample;
	size_t nread;
	size_t samples;
	size_t channels;
	size_t patterns;
	size_t cells;
	size_t i, j;
	int rc;

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

		rc = fseek(f, sizeof(protracker_15smp_t), SEEK_SET);
		if (rc != 0) {
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

	module->samples = samples;
	module->sample = calloc(sizeof(trackmod_sample_t), samples);
	if (module->sample == NULL) {
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

	/* Load patterns */

	cells = channels * protracker_pattern_rows;

	for (i = 0; i < patterns; i++) {
		module->pattern[i].rows = protracker_pattern_rows;
		module->pattern[i].channels = channels;
		module->pattern[i].data = calloc(sizeof(uint32_t), cells);

		nread = fread(module->pattern[i].data,
		    sizeof(uint32_t), cells, f);
		if (nread != cells) {
			printf("Error reading pattern.\n");
			rc = EIO;
			goto error;
		}

		/* Convert byte order */
		for (j = 0; j < cells; j++) {
			module->pattern[i].data[j] = uint32_t_be2host(
			    module->pattern[i].data[j]);
		}
	}

	/* Load samples */
	for (i = 0; i < samples; i++) {
		module->sample[i].length =
		    uint16_t_be2host(sample[i].length) * 2;
		module->sample[i].data = calloc(1, module->sample[i].length);
		if (module->sample[i].data == NULL) {
			printf("Error allocating sample.\n");
			rc = ENOMEM;
			goto error;
		}

		nread = fread(module->sample[i].data, 1, module->sample[i].length,
			f);
		if (nread != module->sample[i].length) {
			printf("Error reading sample.\n");
			rc = EIO;
			goto error;
		}

		module->sample[i].def_vol = sample[i].def_vol;
		module->sample[i].loop_start =
		    uint16_t_be2host(sample[i].loop_start) * 2;
		module->sample[i].loop_len =
		    uint16_t_be2host(sample[i].loop_len) * 2;
		if (module->sample[i].loop_len <= 2)
			module->sample[i].loop_len = 0;
	}

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
