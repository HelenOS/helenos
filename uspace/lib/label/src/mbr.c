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
 * @file Master Boot Record label
 */

#include <block.h>
#include <byteorder.h>
#include <errno.h>
#include <mem.h>
#include <stdlib.h>

#include "std/mbr.h"
#include "mbr.h"

static int mbr_open(service_id_t, label_t **);
static int mbr_create(service_id_t, label_t **);
static void mbr_close(label_t *);
static int mbr_destroy(label_t *);
static int mbr_get_info(label_t *, label_info_t *);
static label_part_t *mbr_part_first(label_t *);
static label_part_t *mbr_part_next(label_part_t *);
static void mbr_part_get_info(label_part_t *, label_part_info_t *);
static int mbr_part_create(label_t *, label_part_spec_t *, label_part_t **);
static int mbr_part_destroy(label_part_t *);

static void mbr_unused_pte(mbr_pte_t *);
static int mbr_part_to_pte(label_part_t *, mbr_pte_t *);
static int mbr_pte_to_part(label_t *, mbr_pte_t *, int);
static int mbr_pte_update(label_t *, mbr_pte_t *, int);

label_ops_t mbr_label_ops = {
	.open = mbr_open,
	.create = mbr_create,
	.close = mbr_close,
	.destroy = mbr_destroy,
	.get_info = mbr_get_info,
	.part_first = mbr_part_first,
	.part_next = mbr_part_next,
	.part_get_info = mbr_part_get_info,
	.part_create = mbr_part_create,
	.part_destroy = mbr_part_destroy
};

static int mbr_open(service_id_t sid, label_t **rlabel)
{
	label_t *label = NULL;
	mbr_br_block_t *mbr = NULL;
	mbr_pte_t *eptr;
	uint16_t sgn;
	size_t bsize;
	aoff64_t nblocks;
	uint32_t entry;
	int rc;

	rc = block_get_bsize(sid, &bsize);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = block_get_nblocks(sid, &nblocks);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	if (bsize < 512 || (bsize % 512) != 0) {
		rc = EINVAL;
		goto error;
	}

	if (nblocks < mbr_ablock0) {
		rc = EINVAL;
		goto error;
	}

	mbr = calloc(1, bsize);
	if (mbr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = block_read_direct(sid, mbr_ba, 1, mbr);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	label = calloc(1, sizeof(label_t));
	if (label == NULL)
		return ENOMEM;

	list_initialize(&label->parts);

	/* Verify boot record signature */
	sgn = uint16_t_le2host(mbr->signature);
	if (sgn != mbr_br_signature) {
		rc = EIO;
		goto error;
	}

	for (entry = 0; entry < mbr_nprimary; entry++) {
		eptr = &mbr->pte[entry];
		rc = mbr_pte_to_part(label, eptr, entry + 1);
		if (rc != EOK)
			goto error;
	}

	free(mbr);
	mbr = NULL;

	label->ops = &mbr_label_ops;
	label->ltype = lt_mbr;
	label->ablock0 = mbr_ablock0;
	label->anblocks = nblocks - mbr_ablock0;
	label->pri_entries = mbr_nprimary;
	*rlabel = label;
	return EOK;
error:
	free(mbr);
	free(label);
	return rc;
}

static int mbr_create(service_id_t sid, label_t **rlabel)
{
	return EOK;
}

static void mbr_close(label_t *label)
{
	free(label);
}

static int mbr_destroy(label_t *label)
{
	return EOK;
}

static int mbr_get_info(label_t *label, label_info_t *linfo)
{
	memset(linfo, 0, sizeof(label_info_t));
	linfo->dcnt = dc_label;
	linfo->ltype = lt_mbr;
	linfo->ablock0 = label->ablock0;
	linfo->anblocks = label->anblocks;
	return EOK;
}

static label_part_t *mbr_part_first(label_t *label)
{
	link_t *link;

	link = list_first(&label->parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, label_part_t, llabel);
}

static label_part_t *mbr_part_next(label_part_t *part)
{
	link_t *link;

	link = list_next(&part->llabel, &part->label->parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, label_part_t, llabel);
}

static void mbr_part_get_info(label_part_t *part, label_part_info_t *pinfo)
{
	pinfo->index = part->index;
	pinfo->block0 = part->block0;
	pinfo->nblocks = part->nblocks;
}

static int mbr_part_create(label_t *label, label_part_spec_t *pspec,
    label_part_t **rpart)
{
	label_part_t *part;
	mbr_pte_t pte;
	int rc;

	part = calloc(1, sizeof(label_part_t));
	if (part == NULL)
		return ENOMEM;

	/* XXX Verify index, block0, nblocks */

	if (pspec->index < 1 || pspec->index > label->pri_entries) {
		rc = EINVAL;
		goto error;
	}

	/* XXX Check if index is used */

	part->label = label;
	part->index = pspec->index;
	part->block0 = pspec->block0;
	part->nblocks = pspec->nblocks;
	part->ptype = pspec->ptype;

	rc = mbr_part_to_pte(part, &pte);
	if (rc != EOK) {
		rc = EINVAL;
		goto error;
	}

	rc = mbr_pte_update(label, &pte, pspec->index - 1);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	list_append(&part->llabel, &label->parts);

	*rpart = part;
	return EOK;
error:
	free(part);
	return rc;
}

static int mbr_part_destroy(label_part_t *part)
{
	mbr_pte_t pte;
	int rc;

	/* Prepare unused partition table entry */
	mbr_unused_pte(&pte);

	/* Modify partition table */
	rc = mbr_pte_update(part->label, &pte, part->index - 1);
	if (rc != EOK)
		return EIO;

	list_remove(&part->llabel);
	free(part);
	return EOK;
}

static void mbr_unused_pte(mbr_pte_t *pte)
{
	memset(pte, 0, sizeof(mbr_pte_t));
}

static int mbr_part_to_pte(label_part_t *part, mbr_pte_t *pte)
{
	if ((part->block0 >> 32) != 0)
		return EINVAL;
	if ((part->nblocks >> 32) != 0)
		return EINVAL;
	if ((part->ptype >> 8) != 0)
		return EINVAL;

	memset(pte, 0, sizeof(mbr_pte_t));
	pte->ptype = part->ptype;
	pte->first_lba = host2uint32_t_le(part->block0);
	pte->length = host2uint32_t_le(part->nblocks);
	return EOK;
}

static int mbr_pte_to_part(label_t *label, mbr_pte_t *pte, int index)
{
	label_part_t *part;
	uint32_t block0;
	uint32_t nblocks;

	block0 = uint32_t_le2host(pte->first_lba);
	nblocks = uint32_t_le2host(pte->length);

	/* See UEFI specification 2.0 section 5.2.1 Legacy Master Boot Record */
	if (pte->ptype == mbr_pt_unused || pte->ptype == mbr_pt_extended ||
	    nblocks == 0)
		return EOK;

	part = calloc(1, sizeof(label_part_t));
	if (part == NULL)
		return ENOMEM;

	part->index = index;
	part->block0 = block0;
	part->nblocks = nblocks;

	/*
	 * TODO: Verify
	 *   - partition must reside on disk
	 *   - partition must not overlap any other partition
	 */

	part->label = label;
	list_append(&part->llabel, &label->parts);
	return EOK;
}

/** Update partition table entry at specified index.
 *
 * Replace partition entry at index @a index with the contents of
 * @a pte.
 */
static int mbr_pte_update(label_t *label, mbr_pte_t *pte, int index)
{
	mbr_br_block_t *br;
	int rc;

	br = calloc(1, label->block_size);
	if (br == NULL)
		return ENOMEM;

	rc = block_read_direct(label->svcid, mbr_ba, 1, br);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	br->pte[index] = *pte;

	rc = block_write_direct(label->svcid, mbr_ba, 1, br);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	free(br);
	return EOK;
error:
	free(br);
	return rc;
}

/** @}
 */
