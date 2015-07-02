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

/** @addtogroup liblabel
 * @{
 */
/**
 * @file GUID Partition Table label.
 */

#include <block.h>
#include <byteorder.h>
#include <errno.h>
#include <mem.h>
#include <stdlib.h>

#include "std/gpt.h"
#include "gpt.h"

static int gpt_open(service_id_t, label_t **);
static int gpt_create(service_id_t, label_t **);
static void gpt_close(label_t *);
static int gpt_destroy(label_t *);
static int gpt_get_info(label_t *, label_info_t *);
static label_part_t *gpt_part_first(label_t *);
static label_part_t *gpt_part_next(label_part_t *);
static void gpt_part_get_info(label_part_t *, label_part_info_t *);
static int gpt_part_create(label_t *, label_part_spec_t *, label_part_t **);
static int gpt_part_destroy(label_part_t *);

static int gpt_pte_to_part(label_t *, gpt_entry_t *, int);

const uint8_t efi_signature[8] = {
	/* "EFI PART" in ASCII */
	0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54
};

label_ops_t gpt_label_ops = {
	.open = gpt_open,
	.create = gpt_create,
	.close = gpt_close,
	.destroy = gpt_destroy,
	.get_info = gpt_get_info,
	.part_first = gpt_part_first,
	.part_next = gpt_part_next,
	.part_get_info = gpt_part_get_info,
	.part_create = gpt_part_create,
	.part_destroy = gpt_part_destroy
};

static int gpt_open(service_id_t sid, label_t **rlabel)
{
	label_t *label = NULL;
	gpt_header_t *gpt_hdr = NULL;
	gpt_entry_t *eptr;
	uint8_t *etable = NULL;
	size_t bsize;
	uint32_t num_entries;
	uint32_t esize;
	uint32_t bcnt;
	uint64_t ba;
	uint32_t entry;
	uint64_t ba_min, ba_max;
	int i;
	int rc;

	rc = block_get_bsize(sid, &bsize);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	if (bsize < 512 || (bsize % 512) != 0) {
		rc = EINVAL;
		goto error;
	}

	gpt_hdr = calloc(1, bsize);
	if (gpt_hdr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = block_read_direct(sid, GPT_HDR_BA, 1, gpt_hdr);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	label = calloc(1, sizeof(label_t));
	if (label == NULL)
		return ENOMEM;

	list_initialize(&label->parts);

	for (i = 0; i < 8; ++i) {
		if (gpt_hdr->efi_signature[i] != efi_signature[i]) {
			rc = EINVAL;
			goto error;
		}
	}

	num_entries = uint32_t_le2host(gpt_hdr->num_entries);
	esize = uint32_t_le2host(gpt_hdr->entry_size);
	bcnt = (num_entries + esize - 1) / esize;
	ba = uint64_t_le2host(gpt_hdr->entry_lba);
	ba_min = uint64_t_le2host(gpt_hdr->first_usable_lba);
	ba_max = uint64_t_le2host(gpt_hdr->last_usable_lba);

	if (num_entries < 1) {
		rc = EINVAL;
		goto error;
	}

	if (esize < sizeof(gpt_entry_t)) {
		rc = EINVAL;
		goto error;
	}

	if (ba_max < ba_min) {
		rc = EINVAL;
		goto error;
	}

	etable = calloc(num_entries, esize);
	if (etable == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = block_read_direct(sid, ba, bcnt, etable);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	for (entry = 0; entry < num_entries; entry++) {
		eptr = (gpt_entry_t *)(etable + entry * esize);
		rc = gpt_pte_to_part(label, eptr, entry + 1);
		if (rc != EOK)
			goto error;
	}

	free(etable);
	etable = NULL;
	free(gpt_hdr);
	gpt_hdr = NULL;

	label->ops = &gpt_label_ops;
	label->ltype = lt_gpt;
	label->ablock0 = ba_min;
	label->anblocks = ba_max - ba_min + 1;
	*rlabel = label;
	return EOK;
error:
	free(etable);
	free(gpt_hdr);
	free(label);
	return rc;
}

static int gpt_create(service_id_t sid, label_t **rlabel)
{
	return EOK;
}

static void gpt_close(label_t *label)
{
	free(label);
}

static int gpt_destroy(label_t *label)
{
	return EOK;
}

static int gpt_get_info(label_t *label, label_info_t *linfo)
{
	memset(linfo, 0, sizeof(label_info_t));
	linfo->dcnt = dc_label;
	linfo->ltype = lt_gpt;
	linfo->ablock0 = label->ablock0;
	linfo->anblocks = label->anblocks;
	return EOK;
}

static label_part_t *gpt_part_first(label_t *label)
{
	link_t *link;

	link = list_first(&label->parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, label_part_t, llabel);
}

static label_part_t *gpt_part_next(label_part_t *part)
{
	link_t *link;

	link = list_next(&part->llabel, &part->label->parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, label_part_t, llabel);
}

static void gpt_part_get_info(label_part_t *part, label_part_info_t *pinfo)
{
	pinfo->index = part->index;
	pinfo->block0 = part->block0;
	pinfo->nblocks = part->nblocks;
}

static int gpt_part_create(label_t *label, label_part_spec_t *pspec,
    label_part_t **rpart)
{
	return ENOTSUP;
}

static int gpt_part_destroy(label_part_t *part)
{
	return ENOTSUP;
}

static int gpt_pte_to_part(label_t *label, gpt_entry_t *pte, int index)
{
	label_part_t *part;
	bool present;
	uint64_t b0, b1;
	int i;

	present = false;
	for (i = 0; i < 8; i++)
		if (pte->part_type[i] != 0x00)
			present = true;

	if (!present)
		return EOK;

	part = calloc(1, sizeof(label_part_t));
	if (part == NULL)
		return ENOMEM;

	b0 = uint64_t_le2host(pte->start_lba);
	b1 = uint64_t_le2host(pte->end_lba);
	if (b1 <= b0)
		return EINVAL;

	part->index = index;
	part->block0 = b0;
	part->nblocks = b1 - b0 + 1;

	part->label = label;
	list_append(&part->llabel, &label->parts);
	return EOK;
}

/** @}
 */
