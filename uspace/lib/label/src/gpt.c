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

static void gpt_unused_pte(gpt_entry_t *);
static int gpt_part_to_pte(label_part_t *, gpt_entry_t *);
static int gpt_pte_to_part(label_t *, gpt_entry_t *, int);
static int gpt_pte_update(label_t *, gpt_entry_t *, int);

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
	gpt_header_t *gpt_hdr[2];
	gpt_entry_t *eptr;
	uint8_t *etable = NULL;
	size_t bsize;
	uint32_t num_entries;
	uint32_t esize;
	uint32_t bcnt;
	uint64_t ptba[2];
	uint64_t h1ba;
	uint32_t entry;
	uint64_t ba_min, ba_max;
	int i, j;
	int rc;

	gpt_hdr[0] = NULL;
	gpt_hdr[1] = NULL;

	rc = block_get_bsize(sid, &bsize);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	if (bsize < 512 || (bsize % 512) != 0) {
		rc = EINVAL;
		goto error;
	}

	gpt_hdr[0] = calloc(1, bsize);
	if (gpt_hdr[0] == NULL) {
		rc = ENOMEM;
		goto error;
	}

	gpt_hdr[1] = calloc(1, bsize);
	if (gpt_hdr[1] == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = block_read_direct(sid, gpt_hdr_ba, 1, gpt_hdr[0]);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	h1ba = uint64_t_le2host(gpt_hdr[0]->alternate_lba);

	rc = block_read_direct(sid, h1ba, 1, gpt_hdr[1]);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	label = calloc(1, sizeof(label_t));
	if (label == NULL)
		return ENOMEM;

	list_initialize(&label->parts);

	for (j = 0; j < 2; j++) {
		for (i = 0; i < 8; ++i) {
			if (gpt_hdr[j]->efi_signature[i] != efi_signature[i]) {
				rc = EINVAL;
				goto error;
			}
		}
	}

	num_entries = uint32_t_le2host(gpt_hdr[0]->num_entries);
	esize = uint32_t_le2host(gpt_hdr[0]->entry_size);
	bcnt = (num_entries + esize - 1) / esize;
	ptba[0] = uint64_t_le2host(gpt_hdr[0]->entry_lba);
	ptba[1] = uint64_t_le2host(gpt_hdr[1]->entry_lba);
	ba_min = uint64_t_le2host(gpt_hdr[0]->first_usable_lba);
	ba_max = uint64_t_le2host(gpt_hdr[0]->last_usable_lba);

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

	rc = block_read_direct(sid, ptba[0], bcnt, etable);
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
	free(gpt_hdr[0]);
	gpt_hdr[0] = NULL;
	free(gpt_hdr[1]);
	gpt_hdr[1] = NULL;

	label->ops = &gpt_label_ops;
	label->ltype = lt_gpt;
	label->svc_id = sid;
	label->ablock0 = ba_min;
	label->anblocks = ba_max - ba_min + 1;
	label->pri_entries = num_entries;
	label->block_size = bsize;

	label->lt.gpt.hdr_ba[0] = gpt_hdr_ba;
	label->lt.gpt.hdr_ba[1] = h1ba;
	label->lt.gpt.ptable_ba[0] = ptba[0];
	label->lt.gpt.ptable_ba[1] = ptba[1];
	label->lt.gpt.esize = esize;

	*rlabel = label;
	return EOK;
error:
	free(etable);
	free(gpt_hdr[0]);
	free(gpt_hdr[1]);
	free(label);
	return rc;
}

static int gpt_create(service_id_t sid, label_t **rlabel)
{
	label_t *label = NULL;
	gpt_header_t *gpt_hdr = NULL;
	uint8_t *etable = NULL;
	size_t bsize;
	uint32_t num_entries;
	uint32_t esize;
	uint64_t ptba[2];
	uint64_t hdr_ba[2];
	uint64_t pt_blocks;
	uint64_t ba_min, ba_max;
	aoff64_t nblocks;
	uint64_t resv_blocks;
	int i, j;
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

	rc = block_get_nblocks(sid, &nblocks);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	/* Number of blocks of a partition table */
	pt_blocks = gpt_ptable_min_size / bsize;
	/* Minimum number of reserved (not allocatable) blocks */
	resv_blocks = 3 + 2 * pt_blocks;

	if (nblocks <= resv_blocks) {
		rc = ENOSPC;
		goto error;
	}

	hdr_ba[0] = gpt_hdr_ba;
	hdr_ba[1] = nblocks - 1;
	ptba[0] = 2;
	ptba[1] = nblocks - 1 - pt_blocks;
	ba_min = ptba[0] + pt_blocks;
	ba_max = ptba[1] - 1;
	esize = sizeof(gpt_entry_t);

	num_entries = pt_blocks * bsize / sizeof(gpt_entry_t);

	for (i = 0; i < 2; i++) {
		gpt_hdr = calloc(1, bsize);
		if (gpt_hdr == NULL) {
			rc = ENOMEM;
			goto error;
		}

		for (j = 0; j < 8; ++j)
			gpt_hdr->efi_signature[j] = efi_signature[j];
		gpt_hdr->revision = host2uint32_t_le(gpt_revision);
		gpt_hdr->header_size = host2uint32_t_le(sizeof(gpt_header_t));
		gpt_hdr->header_crc32 = 0; /* XXX */
		gpt_hdr->my_lba = host2uint64_t_le(hdr_ba[i]);
		gpt_hdr->alternate_lba = host2uint64_t_le(hdr_ba[1 - i]);
		gpt_hdr->first_usable_lba = host2uint64_t_le(ba_min);
		gpt_hdr->last_usable_lba = host2uint64_t_le(ba_max);
		//gpt_hdr->disk_guid
		gpt_hdr->entry_lba = host2uint64_t_le(ptba[i]);
		gpt_hdr->num_entries = host2uint32_t_le(num_entries);
		gpt_hdr->entry_size = host2uint32_t_le(esize);
		gpt_hdr->pe_array_crc32 = 0; /* XXXX */

		rc = block_write_direct(sid, hdr_ba[i], 1, gpt_hdr);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		free(gpt_hdr);
		gpt_hdr = NULL;

		etable = calloc(num_entries, esize);
		if (etable == NULL) {
			rc = ENOMEM;
			goto error;
		}

		rc = block_write_direct(sid, ptba[i], pt_blocks, etable);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		free(etable);
		etable = 0;
	}

	label = calloc(1, sizeof(label_t));
	if (label == NULL)
		return ENOMEM;

	list_initialize(&label->parts);

	label->ops = &gpt_label_ops;
	label->ltype = lt_gpt;
	label->svc_id = sid;
	label->ablock0 = ba_min;
	label->anblocks = ba_max - ba_min + 1;
	label->pri_entries = num_entries;
	label->block_size = bsize;

	label->lt.gpt.hdr_ba[0] = hdr_ba[0];
	label->lt.gpt.hdr_ba[1] = hdr_ba[1];
	label->lt.gpt.ptable_ba[0] = ptba[0];
	label->lt.gpt.ptable_ba[1] = ptba[1];
	label->lt.gpt.esize = esize;

	*rlabel = label;
	return EOK;
error:
	free(etable);
	free(gpt_hdr);
	free(label);
	return rc;
}

static void gpt_close(label_t *label)
{
	label_part_t *part;

	part = gpt_part_first(label);
	while (part != NULL) {
		list_remove(&part->llabel);
		free(part);
		part = gpt_part_first(label);
	}

	free(label);
}

static int gpt_destroy(label_t *label)
{
	gpt_header_t *gpt_hdr = NULL;
	uint8_t *etable = NULL;
	label_part_t *part;
	uint64_t pt_blocks;
	int i;
	int rc;

	part = gpt_part_first(label);
	if (part != NULL) {
		rc = ENOTEMPTY;
		goto error;
	}

	pt_blocks = label->pri_entries * label->lt.gpt.esize /
	    label->block_size;

	for (i = 0; i < 2; i++) {
		gpt_hdr = calloc(1, label->block_size);
		if (gpt_hdr == NULL) {
			rc = ENOMEM;
			goto error;
		}

		rc = block_write_direct(label->svc_id, label->lt.gpt.hdr_ba[i],
		    1, gpt_hdr);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		free(gpt_hdr);
		gpt_hdr = NULL;

		etable = calloc(label->pri_entries, label->lt.gpt.esize);
		if (etable == NULL) {
			rc = ENOMEM;
			goto error;
		}

		rc = block_write_direct(label->svc_id,
		    label->lt.gpt.ptable_ba[i], pt_blocks, etable);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		free(etable);
		etable = 0;
	}

	free(label);
	return EOK;
error:
	return rc;
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
	label_part_t *part;
	gpt_entry_t pte;
	int rc;

	part = calloc(1, sizeof(label_part_t));
	if (part == NULL) {
		rc = ENOMEM;
		goto error;
	}

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

	/* Prepare partition table entry */
	rc = gpt_part_to_pte(part, &pte);
	if (rc != EOK) {
		rc = EINVAL;
		goto error;
	}

	/* Modify partition tables */
	rc = gpt_pte_update(label, &pte, pspec->index - 1);
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

static int gpt_part_destroy(label_part_t *part)
{
	gpt_entry_t pte;
	int rc;

	/* Prepare unused partition table entry */
	gpt_unused_pte(&pte);

	/* Modify partition tables */
	rc = gpt_pte_update(part->label, &pte, part->index - 1);
	if (rc != EOK)
		return EIO;

	list_remove(&part->llabel);
	free(part);
	return EOK;
}

static void gpt_unused_pte(gpt_entry_t *pte)
{
	memset(pte, 0, sizeof(gpt_entry_t));
}

static int gpt_part_to_pte(label_part_t *part, gpt_entry_t *pte)
{
	uint64_t eblock;

	eblock = part->block0 + part->nblocks - 1;
	if (eblock < part->block0)
		return EINVAL;

	memset(pte, 0, sizeof(gpt_entry_t));
	pte->part_type[0] = 0x12;
	pte->part_id[0] = 0x34;
	pte->start_lba = host2uint64_t_le(part->block0);
	pte->end_lba = host2uint64_t_le(eblock);
//	pte->attributes
//	pte->part_name
	return EOK;
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

/** Update partition table entry at specified index.
 *
 * Replace partition entry at index @a index with the contents of
 * @a pte.
 */
static int gpt_pte_update(label_t *label, gpt_entry_t *pte, int index)
{
	size_t pos;
	size_t offs;
	uint64_t ba;
	uint8_t *buf;
	gpt_entry_t *e;
	int i;
	int rc;

	pos = index * label->lt.gpt.esize;
	offs = pos % label->block_size;

	buf = calloc(1, label->block_size);
	if (buf == NULL)
		return ENOMEM;

	/* For both partition tables: read, modify, write */
	for (i = 0; i < 2; i++) {
		ba = label->lt.gpt.ptable_ba[i] +
		    pos / label->block_size;

		rc = block_read_direct(label->svc_id, ba, 1, buf);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		/* Replace single entry */
		e = (gpt_entry_t *)(&buf[offs]);
		*e = *pte;

		rc = block_write_direct(label->svc_id, ba, 1, buf);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}
	}

	free(buf);
	return EOK;
error:
	free(buf);
	return rc;
}

/** @}
 */
