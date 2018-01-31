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

#include <adt/checksum.h>
#include <byteorder.h>
#include <errno.h>
#include <mem.h>
#include <stdint.h>
#include <stdlib.h>
#include <uuid.h>

#include "std/mbr.h"
#include "std/gpt.h"
#include "gpt.h"

static errno_t gpt_open(label_bd_t *, label_t **);
static errno_t gpt_create(label_bd_t *, label_t **);
static void gpt_close(label_t *);
static errno_t gpt_destroy(label_t *);
static errno_t gpt_get_info(label_t *, label_info_t *);
static label_part_t *gpt_part_first(label_t *);
static label_part_t *gpt_part_next(label_part_t *);
static void gpt_part_get_info(label_part_t *, label_part_info_t *);
static errno_t gpt_part_create(label_t *, label_part_spec_t *, label_part_t **);
static errno_t gpt_part_destroy(label_part_t *);
static errno_t gpt_suggest_ptype(label_t *, label_pcnt_t, label_ptype_t *);

static errno_t gpt_check_free_idx(label_t *, int);
static errno_t gpt_check_free_range(label_t *, uint64_t, uint64_t);

static void gpt_unused_pte(gpt_entry_t *);
static errno_t gpt_part_to_pte(label_part_t *, gpt_entry_t *);
static errno_t gpt_pte_to_part(label_t *, gpt_entry_t *, int);
static errno_t gpt_pte_update(label_t *, gpt_entry_t *, int);

static errno_t gpt_update_pt_crc(label_t *, uint32_t);
static void gpt_hdr_compute_crc(gpt_header_t *, size_t);
static errno_t gpt_hdr_get_crc(gpt_header_t *, size_t, uint32_t *);

static errno_t gpt_pmbr_create(label_bd_t *, size_t, uint64_t);
static errno_t gpt_pmbr_destroy(label_bd_t *, size_t);

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
	.part_destroy = gpt_part_destroy,
	.suggest_ptype = gpt_suggest_ptype
};

static errno_t gpt_open(label_bd_t *bd, label_t **rlabel)
{
	label_t *label = NULL;
	gpt_header_t *gpt_hdr[2];
	gpt_entry_t *eptr;
	uint8_t *etable[2];
	size_t bsize;
	aoff64_t nblocks;
	uint32_t num_entries;
	uint32_t esize;
	uint32_t pt_blocks;
	uint64_t ptba[2];
	uint64_t h1ba;
	uint32_t entry;
	uint32_t pt_crc;
	uint64_t ba_min, ba_max;
	uint32_t hdr_size;
	uint32_t hdr_crc;
	int i, j;
	errno_t rc;

	gpt_hdr[0] = NULL;
	gpt_hdr[1] = NULL;
	etable[0] = NULL;
	etable[1] = NULL;

	rc = bd->ops->get_bsize(bd->arg, &bsize);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = bd->ops->get_nblocks(bd->arg, &nblocks);
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

	rc = bd->ops->read(bd->arg, gpt_hdr_ba, 1, gpt_hdr[0]);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	h1ba = uint64_t_le2host(gpt_hdr[0]->alternate_lba);

	if (h1ba >= nblocks) {
		rc = EINVAL;
		goto error;
	}

	rc = bd->ops->read(bd->arg, h1ba, 1, gpt_hdr[1]);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	label = calloc(1, sizeof(label_t));
	if (label == NULL)
		return ENOMEM;

	list_initialize(&label->parts);
	list_initialize(&label->pri_parts);
	list_initialize(&label->log_parts);

	for (j = 0; j < 2; j++) {
		for (i = 0; i < 8; ++i) {
			if (gpt_hdr[j]->efi_signature[i] != efi_signature[i]) {
				rc = EINVAL;
				goto error;
			}
		}
	}

	if (uint32_t_le2host(gpt_hdr[0]->revision) !=
	    uint32_t_le2host(gpt_hdr[1]->revision)) {
		rc = EINVAL;
		goto error;
	}

	if (uint32_t_le2host(gpt_hdr[0]->header_size) !=
	    uint32_t_le2host(gpt_hdr[1]->header_size)) {
		rc = EINVAL;
		goto error;
	}

	hdr_size = uint32_t_le2host(gpt_hdr[0]->header_size);
	if (hdr_size < sizeof(gpt_header_t) ||
	    hdr_size > bsize) {
		rc = EINVAL;
		goto error;
	}

	for (j = 0; j < 2; j++) {
		rc = gpt_hdr_get_crc(gpt_hdr[j], hdr_size, &hdr_crc);
		if (rc != EOK)
			goto error;

		if (uint32_t_le2host(gpt_hdr[j]->header_crc32) != hdr_crc) {
		        rc = EINVAL;
			goto error;
		}
	}

	if (uint64_t_le2host(gpt_hdr[0]->my_lba) != gpt_hdr_ba) {
		rc = EINVAL;
		goto error;
	}

	if (uint64_t_le2host(gpt_hdr[1]->my_lba) != h1ba) {
		rc = EINVAL;
		goto error;
	}

	if (uint64_t_le2host(gpt_hdr[1]->alternate_lba) != gpt_hdr_ba) {
		rc = EINVAL;
		goto error;
	}

	num_entries = uint32_t_le2host(gpt_hdr[0]->num_entries);
	esize = uint32_t_le2host(gpt_hdr[0]->entry_size);
	pt_blocks = (num_entries * esize + bsize - 1) / bsize;
	ptba[0] = uint64_t_le2host(gpt_hdr[0]->entry_lba);
	ptba[1] = uint64_t_le2host(gpt_hdr[1]->entry_lba);
	ba_min = uint64_t_le2host(gpt_hdr[0]->first_usable_lba);
	ba_max = uint64_t_le2host(gpt_hdr[0]->last_usable_lba);
	pt_crc = uint32_t_le2host(gpt_hdr[0]->pe_array_crc32);

	if (uint64_t_le2host(gpt_hdr[1]->first_usable_lba) != ba_min) {
		rc = EINVAL;
		goto error;
	}

	if (uint64_t_le2host(gpt_hdr[1]->last_usable_lba) != ba_max) {
		rc = EINVAL;
		goto error;
	}

	for (i = 0; i < 16; i++) {
		if (gpt_hdr[1]->disk_guid[i] != gpt_hdr[0]->disk_guid[i]) {
			rc = EINVAL;
			goto error;
		}
	}

	if (uint32_t_le2host(gpt_hdr[1]->num_entries) != num_entries) {
		rc = EINVAL;
		goto error;
	}

	if (uint32_t_le2host(gpt_hdr[1]->entry_size) != esize) {
		rc = EINVAL;
		goto error;
	}

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

	/* Check fields in backup header for validity */

	for (j = 0; j < 2; j++) {
		etable[j] = calloc(1, pt_blocks * bsize);
		if (etable[j] == NULL) {
			rc = ENOMEM;
			goto error;
		}

		rc = bd->ops->read(bd->arg, ptba[j], pt_blocks / 2, etable[j]);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		if (compute_crc32(etable[j], num_entries * esize) != pt_crc) {
			rc = EIO;
			goto error;
		}

	}

	for (entry = 0; entry < num_entries; entry++) {
		eptr = (gpt_entry_t *)(etable[0] + entry * esize);
		rc = gpt_pte_to_part(label, eptr, entry + 1);
		if (rc != EOK)
			goto error;
	}

	free(etable[0]);
	etable[0] = NULL;
	free(etable[1]);
	etable[1] = NULL;
	free(gpt_hdr[0]);
	gpt_hdr[0] = NULL;
	free(gpt_hdr[1]);
	gpt_hdr[1] = NULL;

	label->ops = &gpt_label_ops;
	label->ltype = lt_gpt;
	label->bd = *bd;
	label->ablock0 = ba_min;
	label->anblocks = ba_max - ba_min + 1;
	label->pri_entries = num_entries;
	label->block_size = bsize;

	label->lt.gpt.hdr_ba[0] = gpt_hdr_ba;
	label->lt.gpt.hdr_ba[1] = h1ba;
	label->lt.gpt.ptable_ba[0] = ptba[0];
	label->lt.gpt.ptable_ba[1] = ptba[1];
	label->lt.gpt.esize = esize;
	label->lt.gpt.pt_blocks = pt_blocks;
	label->lt.gpt.pt_crc = pt_crc;
	label->lt.gpt.hdr_size = hdr_size;

	*rlabel = label;
	return EOK;
error:
	free(etable[0]);
	free(etable[1]);
	free(gpt_hdr[0]);
	free(gpt_hdr[1]);
	free(label);
	return rc;
}

static errno_t gpt_create(label_bd_t *bd, label_t **rlabel)
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
	uint32_t pt_crc;
	uuid_t disk_uuid;
	int i, j;
	errno_t rc;

	rc = bd->ops->get_bsize(bd->arg, &bsize);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	if (bsize < 512 || (bsize % 512) != 0) {
		rc = EINVAL;
		goto error;
	}

	rc = bd->ops->get_nblocks(bd->arg, &nblocks);
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

	rc = gpt_pmbr_create(bd, bsize, nblocks);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	uuid_generate(&disk_uuid);

	hdr_ba[0] = gpt_hdr_ba;
	hdr_ba[1] = nblocks - 1;
	ptba[0] = 2;
	ptba[1] = nblocks - 1 - pt_blocks;
	ba_min = ptba[0] + pt_blocks;
	ba_max = ptba[1] - 1;
	esize = sizeof(gpt_entry_t);

	num_entries = pt_blocks * bsize / sizeof(gpt_entry_t);

	for (i = 0; i < 2; i++) {
		etable = calloc(1, pt_blocks * bsize);
		if (etable == NULL) {
			rc = ENOMEM;
			goto error;
		}

		rc = bd->ops->write(bd->arg, ptba[i], pt_blocks, etable);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		pt_crc = compute_crc32((uint8_t *)etable,
		    num_entries * esize);

		free(etable);
		etable = NULL;

		gpt_hdr = calloc(1, bsize);
		if (gpt_hdr == NULL) {
			rc = ENOMEM;
			goto error;
		}

		for (j = 0; j < 8; ++j)
			gpt_hdr->efi_signature[j] = efi_signature[j];
		gpt_hdr->revision = host2uint32_t_le(gpt_revision);
		gpt_hdr->header_size = host2uint32_t_le(sizeof(gpt_header_t));
		gpt_hdr->header_crc32 = 0;
		gpt_hdr->my_lba = host2uint64_t_le(hdr_ba[i]);
		gpt_hdr->alternate_lba = host2uint64_t_le(hdr_ba[1 - i]);
		gpt_hdr->first_usable_lba = host2uint64_t_le(ba_min);
		gpt_hdr->last_usable_lba = host2uint64_t_le(ba_max);
		uuid_encode(&disk_uuid, gpt_hdr->disk_guid);
		gpt_hdr->entry_lba = host2uint64_t_le(ptba[i]);
		gpt_hdr->num_entries = host2uint32_t_le(num_entries);
		gpt_hdr->entry_size = host2uint32_t_le(esize);
		gpt_hdr->pe_array_crc32 = pt_crc;

		gpt_hdr_compute_crc(gpt_hdr, sizeof(gpt_header_t));

		rc = bd->ops->write(bd->arg, hdr_ba[i], 1, gpt_hdr);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		free(gpt_hdr);
		gpt_hdr = NULL;
	}

	label = calloc(1, sizeof(label_t));
	if (label == NULL)
		return ENOMEM;

	list_initialize(&label->parts);
	list_initialize(&label->pri_parts);
	list_initialize(&label->log_parts);

	label->ops = &gpt_label_ops;
	label->ltype = lt_gpt;
	label->bd = *bd;
	label->ablock0 = ba_min;
	label->anblocks = ba_max - ba_min + 1;
	label->pri_entries = num_entries;
	label->block_size = bsize;

	label->lt.gpt.hdr_ba[0] = hdr_ba[0];
	label->lt.gpt.hdr_ba[1] = hdr_ba[1];
	label->lt.gpt.ptable_ba[0] = ptba[0];
	label->lt.gpt.ptable_ba[1] = ptba[1];
	label->lt.gpt.esize = esize;
	label->lt.gpt.pt_blocks = pt_blocks;
	label->lt.gpt.pt_crc = pt_crc;
	label->lt.gpt.hdr_size = sizeof(gpt_header_t);

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
		list_remove(&part->lparts);
		list_remove(&part->lpri);
		free(part);
		part = gpt_part_first(label);
	}

	free(label);
}

static errno_t gpt_destroy(label_t *label)
{
	gpt_header_t *gpt_hdr = NULL;
	uint8_t *etable = NULL;
	label_part_t *part;
	int i;
	errno_t rc;

	part = gpt_part_first(label);
	if (part != NULL) {
		rc = ENOTEMPTY;
		goto error;
	}

	for (i = 0; i < 2; i++) {
		gpt_hdr = calloc(1, label->block_size);
		if (gpt_hdr == NULL) {
			rc = ENOMEM;
			goto error;
		}

		rc = label->bd.ops->write(label->bd.arg, label->lt.gpt.hdr_ba[i],
		    1, gpt_hdr);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		free(gpt_hdr);
		gpt_hdr = NULL;

		etable = calloc(1, label->lt.gpt.pt_blocks *
		    label->block_size);
		if (etable == NULL) {
			rc = ENOMEM;
			goto error;
		}

		rc = label->bd.ops->write(label->bd.arg,
		    label->lt.gpt.ptable_ba[i], label->lt.gpt.pt_blocks,
		    etable);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		free(etable);
		etable = 0;
	}

	rc = gpt_pmbr_destroy(&label->bd, label->block_size);
	if (rc != EOK)
		goto error;

	free(label);
	return EOK;
error:
	return rc;
}

static bool gpt_can_create_pri(label_t *label)
{
	return list_count(&label->parts) < (size_t)label->pri_entries;
}

static bool gpt_can_delete_part(label_t *label)
{
	return list_count(&label->parts) > 0;
}

static errno_t gpt_get_info(label_t *label, label_info_t *linfo)
{
	memset(linfo, 0, sizeof(label_info_t));
	linfo->ltype = lt_gpt;
	linfo->flags = lf_ptype_uuid; /* Partition type is in UUID format */
	if (gpt_can_create_pri(label))
		linfo->flags = linfo->flags | lf_can_create_pri;
	if (gpt_can_delete_part(label))
		linfo->flags = linfo->flags | lf_can_delete_part;
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

	return list_get_instance(link, label_part_t, lparts);
}

static label_part_t *gpt_part_next(label_part_t *part)
{
	link_t *link;

	link = list_next(&part->lparts, &part->label->parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, label_part_t, lparts);
}

static void gpt_part_get_info(label_part_t *part, label_part_info_t *pinfo)
{
	pinfo->index = part->index;
	pinfo->pkind = lpk_primary;
	pinfo->block0 = part->block0;
	pinfo->nblocks = part->nblocks;
}

static errno_t gpt_part_create(label_t *label, label_part_spec_t *pspec,
    label_part_t **rpart)
{
	label_part_t *part;
	gpt_entry_t pte;
	errno_t rc;

	part = calloc(1, sizeof(label_part_t));
	if (part == NULL) {
		rc = ENOMEM;
		goto error;
	}

	/* Verify index is within bounds and free */
	rc = gpt_check_free_idx(label, pspec->index);
	if (rc != EOK) {
		rc = EINVAL;
		goto error;
	}

	/* Verify range is within bounds and free */
	rc = gpt_check_free_range(label, pspec->block0, pspec->nblocks);
	if (rc != EOK) {
		rc = EINVAL;
		goto error;
	}

	/* GPT only has primary partitions */
	if (pspec->pkind != lpk_primary) {
		rc = EINVAL;
		goto error;
	}

	/* Partition type must be in UUID format */
	if (pspec->ptype.fmt != lptf_uuid) {
		rc = EINVAL;
		goto error;
	}

	/* XXX Check if index is used */

	part->label = label;
	part->index = pspec->index;
	part->block0 = pspec->block0;
	part->nblocks = pspec->nblocks;
	part->ptype = pspec->ptype;
	uuid_generate(&part->part_uuid);

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

	list_append(&part->lparts, &label->parts);
	list_append(&part->lpri, &label->pri_parts);

	*rpart = part;
	return EOK;
error:
	free(part);
	return rc;
}

static errno_t gpt_part_destroy(label_part_t *part)
{
	gpt_entry_t pte;
	errno_t rc;

	/* Prepare unused partition table entry */
	gpt_unused_pte(&pte);

	/* Modify partition tables */
	rc = gpt_pte_update(part->label, &pte, part->index - 1);
	if (rc != EOK)
		return EIO;

	list_remove(&part->lparts);
	list_remove(&part->lpri);
	free(part);
	return EOK;
}

static errno_t gpt_suggest_ptype(label_t *label, label_pcnt_t pcnt,
    label_ptype_t *ptype)
{
	const char *ptid;
	errno_t rc;

	ptid = NULL;

	switch (pcnt) {
	case lpc_fat12_16:
	case lpc_exfat:
	case lpc_fat32:
		ptid = GPT_MS_BASIC_DATA;
		break;
	case lpc_ext4:
		ptid = GPT_LINUX_FS_DATA;
		break;
	case lpc_minix:
		ptid = GPT_MINIX_FAKE;
		break;
	}

	if (ptid == NULL)
		return EINVAL;

	ptype->fmt = lptf_uuid;
	rc = uuid_parse(ptid, &ptype->t.uuid, NULL);
	assert(rc == EOK);

	return EOK;
}

/** Verify that the specified index is valid and free. */
static errno_t gpt_check_free_idx(label_t *label, int index)
{
	label_part_t *part;

	if (index < 1 || index > label->pri_entries)
		return EINVAL;

	part = gpt_part_first(label);
	while (part != NULL) {
		if (part->index == index)
			return EEXIST;
		part = gpt_part_next(part);
	}

	return EOK;
}

/** Determine if two block address ranges overlap. */
static bool gpt_overlap(uint64_t a0, uint64_t an, uint64_t b0, uint64_t bn)
{
	return !(a0 + an <= b0 || b0 + bn <= a0);
}

static errno_t gpt_check_free_range(label_t *label, uint64_t block0,
    uint64_t nblocks)
{
	label_part_t *part;

	if (block0 < label->ablock0)
		return EINVAL;
	if (block0 + nblocks > label->ablock0 + label->anblocks)
		return EINVAL;

	part = gpt_part_first(label);
	while (part != NULL) {
		if (gpt_overlap(block0, nblocks, part->block0, part->nblocks))
			return EEXIST;
		part = gpt_part_next(part);
	}

	return EOK;
}

static void gpt_unused_pte(gpt_entry_t *pte)
{
	memset(pte, 0, sizeof(gpt_entry_t));
}

static errno_t gpt_part_to_pte(label_part_t *part, gpt_entry_t *pte)
{
	uint64_t eblock;

	eblock = part->block0 + part->nblocks - 1;
	if (eblock < part->block0)
		return EINVAL;

	memset(pte, 0, sizeof(gpt_entry_t));
	uuid_encode(&part->ptype.t.uuid, pte->part_type);
	uuid_encode(&part->part_uuid, pte->part_id);
	pte->start_lba = host2uint64_t_le(part->block0);
	pte->end_lba = host2uint64_t_le(eblock);
//	pte->attributes
//	pte->part_name
	return EOK;
}

static errno_t gpt_pte_to_part(label_t *label, gpt_entry_t *pte, int index)
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

	b0 = uint64_t_le2host(pte->start_lba);
	b1 = uint64_t_le2host(pte->end_lba);
	if (b1 <= b0)
		return EINVAL;

	part = calloc(1, sizeof(label_part_t));
	if (part == NULL)
		return ENOMEM;

	part->index = index;
	part->block0 = b0;
	part->nblocks = b1 - b0 + 1;
	part->ptype.fmt = lptf_uuid;
	uuid_decode(pte->part_type, &part->ptype.t.uuid);
	uuid_decode(pte->part_id, &part->part_uuid);

	part->label = label;
	list_append(&part->lparts, &label->parts);
	list_append(&part->lpri, &label->pri_parts);
	return EOK;
}

/** Update partition table entry at specified index.
 *
 * Replace partition entry at index @a index with the contents of
 * @a pte.
 */
static errno_t gpt_pte_update(label_t *label, gpt_entry_t *pte, int index)
{
	size_t pos;
	uint64_t ba;
	uint64_t nblocks;
	size_t ptbytes;
	uint8_t *buf;
	gpt_entry_t *e;
	uint32_t crc;
	int i;
	errno_t rc;

	/* Byte offset of partition entry */
	pos = index * label->lt.gpt.esize;
	/* Number of bytes in partition table */
	ptbytes = label->pri_entries * label->lt.gpt.esize;

	buf = calloc(1, label->block_size * label->lt.gpt.pt_blocks);
	if (buf == NULL)
		return ENOMEM;

	/* For both partition tables: read, modify, write */
	for (i = 0; i < 2; i++) {
		ba = label->lt.gpt.ptable_ba[i];
		nblocks = label->lt.gpt.pt_blocks;

		rc = label->bd.ops->read(label->bd.arg, ba, nblocks, buf);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		crc = compute_crc32(buf, ptbytes);
		if (crc != label->lt.gpt.pt_crc) {
			/* Corruption detected */
			rc = EIO;
			goto error;
		}

		/* Replace single entry */
		e = (gpt_entry_t *)(&buf[pos]);
		*e = *pte;

		rc = label->bd.ops->write(label->bd.arg, ba, nblocks, buf);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		crc = compute_crc32(buf, ptbytes);
		rc = gpt_update_pt_crc(label, crc);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}
	}

	label->lt.gpt.pt_crc = crc;
	free(buf);
	return EOK;
error:
	free(buf);
	return rc;
}

static errno_t gpt_update_pt_crc(label_t *label, uint32_t crc)
{
	gpt_header_t *gpt_hdr;
	errno_t rc;
	int i;

	gpt_hdr = calloc(1, label->block_size);
	if (gpt_hdr == NULL) {
		rc = ENOMEM;
		goto exit;
	}

	for (i = 0; i < 2; i++) {
		rc = label->bd.ops->read(label->bd.arg,
		    label->lt.gpt.hdr_ba[i], 1, gpt_hdr);
		if (rc != EOK) {
			rc = EIO;
			goto exit;
		}

		gpt_hdr->pe_array_crc32 = host2uint32_t_le(crc);
		gpt_hdr_compute_crc(gpt_hdr, label->lt.gpt.hdr_size);

		rc = label->bd.ops->write(label->bd.arg,
		    label->lt.gpt.hdr_ba[i], 1, gpt_hdr);
		if (rc != EOK) {
			rc = EIO;
			goto exit;
		}
	}

	rc = EOK;

exit:
	free(gpt_hdr);
	return rc;
}

static void gpt_hdr_compute_crc(gpt_header_t *hdr, size_t hdr_size)
{
	uint32_t crc;

	hdr->header_crc32 = 0;
	crc = compute_crc32((uint8_t *)hdr, hdr_size);
	hdr->header_crc32 = crc;
}

static errno_t gpt_hdr_get_crc(gpt_header_t *hdr, size_t hdr_size, uint32_t *crc)
{
	gpt_header_t *c;

	c = calloc(1, hdr_size);
	if (c == NULL)
		return ENOMEM;

	memcpy(c, hdr, hdr_size);
	c->header_crc32 = 0;
	*crc = compute_crc32((uint8_t *)c, hdr_size);
	free(c);

	return EOK;
}

/** Create GPT Protective MBR */
static errno_t gpt_pmbr_create(label_bd_t *bd, size_t bsize, uint64_t nblocks)
{
	mbr_br_block_t *pmbr = NULL;
	uint64_t pmbr_nb;
	errno_t rc;

	pmbr = calloc(1, bsize);
	if (pmbr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	pmbr_nb = nblocks - gpt_hdr_ba;

	pmbr->pte[0].ptype = mbr_pt_gpt_protect;
	pmbr->pte[0].first_lba = gpt_hdr_ba;

	if (pmbr_nb <= UINT32_MAX)
		pmbr->pte[0].length = host2uint32_t_le((uint32_t)pmbr_nb);
	else
		pmbr->pte[0].length = host2uint32_t_le(UINT32_MAX);

	pmbr->signature = host2uint16_t_le(mbr_br_signature);

	rc = bd->ops->write(bd->arg, mbr_ba, 1, pmbr);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	free(pmbr);
	return EOK;
error:
	free(pmbr);
	return rc;
}

/** Destroy GPT Protective MBR */
static errno_t gpt_pmbr_destroy(label_bd_t *bd, size_t bsize)
{
	mbr_br_block_t *pmbr = NULL;
	errno_t rc;

	pmbr = calloc(1, bsize);
	if (pmbr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = bd->ops->write(bd->arg, mbr_ba, 1, pmbr);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	free(pmbr);
	return EOK;
error:
	free(pmbr);
	return rc;
}

/** @}
 */
