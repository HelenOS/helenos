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
static int mbr_open_ext(label_t *);
static int mbr_create(service_id_t, label_t **);
static void mbr_close(label_t *);
static int mbr_destroy(label_t *);
static int mbr_get_info(label_t *, label_info_t *);
static label_part_t *mbr_part_first(label_t *);
static label_part_t *mbr_part_next(label_part_t *);
static void mbr_part_get_info(label_part_t *, label_part_info_t *);
static int mbr_part_create(label_t *, label_part_spec_t *, label_part_t **);
static int mbr_part_destroy(label_part_t *);
static int mbr_suggest_ptype(label_t *, label_pcnt_t, label_ptype_t *);

static void mbr_unused_pte(mbr_pte_t *);
static int mbr_part_to_pte(label_part_t *, mbr_pte_t *);
static int mbr_pte_to_part(label_t *, mbr_pte_t *, int);
static int mbr_pte_to_log_part(label_t *, uint64_t, mbr_pte_t *);
static void mbr_log_part_to_ptes(label_part_t *, mbr_pte_t *, mbr_pte_t *);
static int mbr_pte_update(label_t *, mbr_pte_t *, int);
static int mbr_log_part_insert(label_t *, label_part_t *);
static int mbr_ebr_create(label_t *, label_part_t *);
static int mbr_ebr_delete(label_t *, label_part_t *);
static int mbr_ebr_update_next(label_t *, label_part_t *);
static void mbr_update_log_indices(label_t *);

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
	.part_destroy = mbr_part_destroy,
	.suggest_ptype = mbr_suggest_ptype
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
	list_initialize(&label->pri_parts);
	list_initialize(&label->log_parts);

	/* Verify boot record signature */
	sgn = uint16_t_le2host(mbr->signature);
	if (sgn != mbr_br_signature) {
		rc = EIO;
		goto error;
	}


	label->ext_part = NULL;
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
	label->svc_id = sid;
	label->block_size = bsize;
	label->ablock0 = mbr_ablock0;
	label->anblocks = nblocks - mbr_ablock0;
	label->pri_entries = mbr_nprimary;

	if (label->ext_part != NULL) {
		/* Open extended partition */
		rc = mbr_open_ext(label);
		if (rc != EOK)
			goto error;
	}

	*rlabel = label;
	return EOK;
error:
	free(mbr);
	mbr_close(label);
	return rc;
}

/** Open extended partition */
static int mbr_open_ext(label_t *label)
{
	mbr_br_block_t *ebr = NULL;
	mbr_pte_t *ethis;
	mbr_pte_t *enext;
	uint64_t ebr_b0;
	uint64_t ebr_nblocks_min;
	uint64_t ebr_nblocks_max;
	uint64_t pebr_b0;
	uint64_t pebr_nblocks;
	uint64_t pb0;
	uint64_t pnblocks;
	uint64_t ep_b0;
	int rc;

	ebr = calloc(1, label->block_size);
	if (ebr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	/* First block of extended partition */
	ep_b0 = label->ext_part->block0;

	/* First block of current EBR */
	ebr_b0 = label->ext_part->block0;

	/*
	 * We don't have bounds for the first EBR, so for purpose of
	 * verification let's say it contains at least one block and
	 * at most all blocks from the extended partition.
	 */
	ebr_nblocks_min = 1;
	ebr_nblocks_max = label->ext_part->nblocks;

	while (true) {
		/* Read EBR */
		rc = block_read_direct(label->svc_id, ebr_b0, 1, ebr);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		ethis = &ebr->pte[mbr_ebr_pte_this];
		enext = &ebr->pte[mbr_ebr_pte_next];

		pb0 = ebr_b0 + uint32_t_le2host(ethis->first_lba);
		pnblocks = uint32_t_le2host(ethis->length);

		if (ethis->ptype == mbr_pt_unused || pnblocks == 0)
			break;

		/* Verify partition lies within the range of EBR */
		if (pb0 + pnblocks > ebr_b0 + ebr_nblocks_max) {
			rc = EIO;
			goto error;
		}

		/* Create partition structure */
		rc = mbr_pte_to_log_part(label, ebr_b0, ethis);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		/* Save previous EBR range */
		pebr_b0 = ebr_b0;
		pebr_nblocks = ebr_nblocks_min;

		/* Proceed to next EBR */
		ebr_b0 = ep_b0 + uint32_t_le2host(enext->first_lba);
		ebr_nblocks_min = uint32_t_le2host(enext->length);
		ebr_nblocks_max = ebr_nblocks_min;

		if (enext->ptype == mbr_pt_unused || ebr_nblocks_min == 0)
			break;

		/* Verify next EBR does not overlap this EBR */
		if (ebr_b0 < pebr_b0 + pebr_nblocks) {
			rc = EIO;
			goto error;
		}

		/* Verify next EBR does not extend beyond end of label */
		if (ebr_b0 + ebr_nblocks_max > label->ablock0 + label->anblocks) {
			rc = EIO;
			goto error;
		}
	}

	free(ebr);
	return EOK;
error:
	/* Note that logical partitions need to be deleted by caller */
	free(ebr);
	return rc;
}

static int mbr_create(service_id_t sid, label_t **rlabel)
{
	label_t *label = NULL;
	mbr_br_block_t *mbr = NULL;
	aoff64_t nblocks;
	size_t bsize;
	int i;
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

	mbr = calloc(1, bsize);
	if (mbr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	label = calloc(1, sizeof(label_t));
	if (label == NULL)
		return ENOMEM;

	list_initialize(&label->parts);
	list_initialize(&label->pri_parts);
	list_initialize(&label->log_parts);

	mbr->media_id = 0;
	mbr->pad0 = 0;
	for (i = 0; i < mbr_nprimary; i++)
		mbr_unused_pte(&mbr->pte[i]);
	mbr->signature = host2uint16_t_le(mbr_br_signature);

	rc = block_write_direct(sid, mbr_ba, 1, mbr);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	free(mbr);
	mbr = NULL;

	label->ops = &mbr_label_ops;
	label->ltype = lt_mbr;
	label->block_size = bsize;
	label->svc_id = sid;
	label->ablock0 = mbr_ablock0;
	label->anblocks = nblocks - mbr_ablock0;
	label->pri_entries = mbr_nprimary;
	label->ext_part = NULL;

	*rlabel = label;
	return EOK;
error:
	free(mbr);
	free(label);
	return rc;
}

static void mbr_close(label_t *label)
{
	label_part_t *part;

	if (label == NULL)
		return;

	part = mbr_part_first(label);
	while (part != NULL) {
		list_remove(&part->lparts);
		if (link_used(&part->lpri))
			list_remove(&part->lpri);
		if (link_used(&part->llog))
			list_remove(&part->llog);
		free(part);

		part = mbr_part_first(label);
	}

	free(label);
}

static int mbr_destroy(label_t *label)
{
	mbr_br_block_t *mbr = NULL;
	label_part_t *part;
	int rc;

	part = mbr_part_first(label);
	if (part != NULL) {
		rc = ENOTEMPTY;
		goto error;
	}

	mbr = calloc(1, label->block_size);
	if (mbr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = block_write_direct(label->svc_id, mbr_ba, 1, mbr);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	free(mbr);
	mbr = NULL;

	free(label);
	return EOK;
error:
	free(mbr);
	return rc;
}

static bool mbr_can_delete_part(label_t *label)
{
	return list_count(&label->parts) > 0;
}

static int mbr_get_info(label_t *label, label_info_t *linfo)
{
	memset(linfo, 0, sizeof(label_info_t));
	linfo->ltype = lt_mbr;

	/* We support extended partitions */
	linfo->flags = lf_ext_supp;

	/** Can create primary if there is a free slot */
	if (list_count(&label->pri_parts) < mbr_nprimary)
		linfo->flags |= lf_can_create_pri;
	/* Can create extended if there is a free slot and no extended */
	if ((linfo->flags & lf_can_create_pri) != 0 && label->ext_part == NULL)
		linfo->flags |= lf_can_create_ext;
	/* Can create logical if there is an extended partition */
	if (label->ext_part != NULL)
		linfo->flags |= lf_can_create_log;
	/* Can delete partition */
	if (mbr_can_delete_part(label))
		linfo->flags |= lf_can_delete_part;

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

	return list_get_instance(link, label_part_t, lparts);
}

static label_part_t *mbr_part_next(label_part_t *part)
{
	link_t *link;

	link = list_next(&part->lparts, &part->label->parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, label_part_t, lparts);
}

static label_part_t *mbr_log_part_first(label_t *label)
{
	link_t *link;

	link = list_first(&label->log_parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, label_part_t, llog);
}

static label_part_t *mbr_log_part_next(label_part_t *part)
{
	link_t *link;

	link = list_next(&part->llog, &part->label->log_parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, label_part_t, llog);
}

static label_part_t *mbr_log_part_prev(label_part_t *part)
{
	link_t *link;

	link = list_prev(&part->llog, &part->label->log_parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, label_part_t, llog);
}

#include <io/log.h>
static void mbr_part_get_info(label_part_t *part, label_part_info_t *pinfo)
{
	pinfo->index = part->index;
	pinfo->block0 = part->block0;
	pinfo->nblocks = part->nblocks;

	log_msg(LOG_DEFAULT, LVL_NOTE, "mbr_part_get_info: index=%d ptype=%d",
	    (int)part->index, (int)part->ptype.t.num);
	if (link_used(&part->llog))
		pinfo->pkind = lpk_logical;
	else if (part->ptype.t.num == mbr_pt_extended)
		pinfo->pkind = lpk_extended;
	else
		pinfo->pkind = lpk_primary;
}

static int mbr_part_create(label_t *label, label_part_spec_t *pspec,
    label_part_t **rpart)
{
	label_part_t *part;
	label_part_t *prev;
	label_part_t *next;
	mbr_pte_t pte;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "mbr_part_create");
	if (pspec->ptype.fmt != lptf_num)
		return EINVAL;

	part = calloc(1, sizeof(label_part_t));
	if (part == NULL)
		return ENOMEM;


	/* XXX Check if index is used */
	part->label = label;
	part->index = pspec->index;
	part->block0 = pspec->block0;
	part->nblocks = pspec->nblocks;
	part->hdr_blocks = pspec->hdr_blocks;

	switch (pspec->pkind) {
	case lpk_primary:
		part->ptype = pspec->ptype;
		break;
	case lpk_extended:
		part->ptype.fmt = lptf_num;
		part->ptype.t.num = mbr_pt_extended;
		if (pspec->ptype.t.num != 0) {
			rc = EINVAL;
			goto error;
		}
		if (label->ext_part != NULL) {
			rc = EEXISTS;
			goto error;
		}
		break;
	case lpk_logical:
		part->ptype = pspec->ptype;
		if (pspec->index != 0) {
			rc = EINVAL;
			goto error;
		}
		break;
	}

	if (pspec->pkind != lpk_logical) {
		/* Primary or extended partition */
		/* XXX Verify index, block0, nblocks */

		if (pspec->index < 1 || pspec->index > label->pri_entries) {
			rc = EINVAL;
			goto error;
		}

		if (pspec->hdr_blocks != 0) {
			rc = EINVAL;
			goto error;
		}

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

		list_append(&part->lparts, &label->parts);
		list_append(&part->lpri, &label->pri_parts);

		if (pspec->pkind == lpk_extended)
			label->ext_part = part;
	} else {
		/* Logical partition */
		rc = mbr_log_part_insert(label, part);
		if (rc != EOK)
			goto error;

		/* Create EBR for new partition */
		rc = mbr_ebr_create(label, part);
		if (rc != EOK)
			goto error;

		prev = mbr_log_part_prev(part);
		if (prev != NULL) {
			/* Update 'next' PTE in EBR of previous partition */
			rc = mbr_ebr_update_next(label, prev);
			if (rc != EOK) {
				goto error;
			}
		} else {
			/* New partition is now the first one */
			next = mbr_log_part_next(part);
			if (next != NULL) {
				/*
				 * Create new, relocated EBR for the former
				 * first partition
				 */
				next->hdr_blocks = pspec->hdr_blocks;
				rc = mbr_ebr_create(label, next);
				if (rc != EOK)
					goto error;
			}
		}

		/* This also sets index for the new partition. */
		mbr_update_log_indices(label);
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "mbr_part_create success");
	*rpart = part;
	return EOK;
error:
	free(part);
	return rc;
}

static int mbr_part_destroy(label_part_t *part)
{
	mbr_pte_t pte;
	label_part_t *prev;
	label_part_t *next;
	uint64_t ep_b0;
	int rc;

	if (link_used(&part->lpri)) {
		/* Primary/extended partition */

		/* Prepare unused partition table entry */
		mbr_unused_pte(&pte);

		/* Modify partition table */
		rc = mbr_pte_update(part->label, &pte, part->index - 1);
		if (rc != EOK)
			return EIO;

		/* If it was the extended partition, clear ext. part. pointer */
		if (part == part->label->ext_part)
			part->label->ext_part = NULL;

		list_remove(&part->lpri);
	} else {
		/* Logical partition */

		prev = mbr_log_part_prev(part);
		if (prev != NULL) {
			/* Update next link in previous EBR */
			list_remove(&part->llog);

			rc = mbr_ebr_update_next(part->label, prev);
			if (rc != EOK) {
				/* Roll back */
				list_insert_after(&part->llog, &prev->llog);
				return EIO;
			}

			/* Delete EBR */
			rc = mbr_ebr_delete(part->label, part);
			if (rc != EOK)
				return EIO;
		} else {
			next = mbr_log_part_next(part);
			list_remove(&part->llog);

			if (next != NULL) {
				/*
				 * Relocate next partition's EBR to the beginning
				 * of the extended partition. This also
				 * overwrites the EBR of the former first
				 * partition.
				 */

				/* First block of extended partition */
				ep_b0 = part->label->ext_part->block0;

				next->hdr_blocks = next->block0 - ep_b0;

				rc = mbr_ebr_create(part->label, next);
				if (rc != EOK) {
					list_prepend(&part->llog, &part->label->log_parts);
					return EIO;
				}
			} else {
				/* Delete EBR */
				rc = mbr_ebr_delete(part->label, part);
				if (rc != EOK)
					return EIO;
			}
		}

		/* Update indices */
		mbr_update_log_indices(part->label);
	}

	list_remove(&part->lparts);
	free(part);
	return EOK;
}

static int mbr_suggest_ptype(label_t *label, label_pcnt_t pcnt,
    label_ptype_t *ptype)
{
	ptype->fmt = lptf_num;
	ptype->t.num = 0;

	switch (pcnt) {
	case lpc_exfat:
		ptype->t.num = mbr_pt_ms_advanced;
		break;
	case lpc_ext4:
		ptype->t.num = mbr_pt_linux;
		break;
	case lpc_fat12_16:
		ptype->t.num = mbr_pt_fat16_lba;
		break;
	case lpc_fat32:
		ptype->t.num = mbr_pt_fat32_lba;
		break;
	case lpc_minix:
		ptype->t.num = mbr_pt_minix;
		break;
	}

	if (ptype->t.num == 0)
		return EINVAL;

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
	if ((part->ptype.t.num >> 8) != 0)
		return EINVAL;

	log_msg(LOG_DEFAULT, LVL_NOTE, "mbr_part_to_pte: a0=%" PRIu64
	    " len=%" PRIu64 " ptype=%d", part->block0, part->nblocks,
	    (int)part->ptype.t.num);
	memset(pte, 0, sizeof(mbr_pte_t));
	pte->ptype = part->ptype.t.num;
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
	if (pte->ptype == mbr_pt_unused || nblocks == 0)
		return EOK;

	part = calloc(1, sizeof(label_part_t));
	if (part == NULL)
		return ENOMEM;

	part->ptype.fmt = lptf_num;
	part->ptype.t.num = pte->ptype;
	part->index = index;
	part->block0 = block0;
	part->nblocks = nblocks;

	/*
	 * TODO: Verify
	 *   - partition must reside on disk
	 *   - partition must not overlap any other partition
	 */

	part->label = label;
	list_append(&part->lparts, &label->parts);
	list_append(&part->lpri, &label->pri_parts);

	if (pte->ptype == mbr_pt_extended)
		label->ext_part = part;
	return EOK;
}

static int mbr_pte_to_log_part(label_t *label, uint64_t ebr_b0,
    mbr_pte_t *pte)
{
	label_part_t *part;
	uint32_t block0;
	uint32_t nblocks;
	size_t nlparts;

	block0 = ebr_b0 + uint32_t_le2host(pte->first_lba);
	nblocks = uint32_t_le2host(pte->length);

	if (pte->ptype == mbr_pt_unused || nblocks == 0)
		return EOK;

	part = calloc(1, sizeof(label_part_t));
	if (part == NULL)
		return ENOMEM;

	nlparts = list_count(&label->log_parts);

	part->ptype.fmt = lptf_num;
	part->ptype.t.num = pte->ptype;
	part->index = mbr_nprimary + 1 + nlparts;
	part->block0 = block0;
	part->nblocks = nblocks;
	part->hdr_blocks = block0 - ebr_b0;

	part->label = label;
	list_append(&part->lparts, &label->parts);
	list_append(&part->llog, &label->log_parts);

	return EOK;
}

static void mbr_log_part_to_ptes(label_part_t *part, mbr_pte_t *pthis,
    mbr_pte_t *pnext)
{
	label_part_t *next;
	uint64_t ep_b0;
	uint64_t totsize;

	/* First block of extended partition */
	ep_b0 = part->label->ext_part->block0;

	assert(link_used(&part->llog));
	assert(part->block0 >= ep_b0);
	assert(part->hdr_blocks <= part->block0 - ep_b0);

	/* 'This' EBR entry */
	if (pthis != NULL) {
		memset(pthis, 0, sizeof(mbr_pte_t));
		pthis->ptype = part->ptype.t.num;
		pthis->first_lba = host2uint32_t_le(part->hdr_blocks);
		pthis->length = host2uint32_t_le(part->nblocks);
	}

	/* 'Next' EBR entry */
	if (pnext != NULL) {
		next = mbr_log_part_next(part);

		memset(pnext, 0, sizeof(mbr_pte_t));
		if (next != NULL) {
			/* Total size of EBR + partition */
			totsize = next->hdr_blocks + next->nblocks;

			pnext->ptype = mbr_pt_extended;
			pnext->first_lba = host2uint32_t_le(next->block0 -
			    next->hdr_blocks - ep_b0);
			pnext->length = host2uint32_t_le(totsize);
		}
	}
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

	rc = block_read_direct(label->svc_id, mbr_ba, 1, br);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	br->pte[index] = *pte;

	rc = block_write_direct(label->svc_id, mbr_ba, 1, br);
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

/** Insert logical partition into logical partition list. */
static int mbr_log_part_insert(label_t *label, label_part_t *part)
{
	label_part_t *cur;

	cur = mbr_log_part_first(label);
	while (cur != NULL) {
		if (cur->block0 + cur->nblocks > part->block0)
			break;
		cur = mbr_log_part_next(part);
	}

	if (cur != NULL)
		list_insert_before(&part->llog, &cur->llog);
	else
		list_append(&part->llog, &label->log_parts);

	return EOK;
}

/** Create EBR for partition. */
static int mbr_ebr_create(label_t *label, label_part_t *part)
{
	mbr_br_block_t *br;
	uint64_t ba;
	int rc;

	br = calloc(1, label->block_size);
	if (br == NULL)
		return ENOMEM;

	mbr_log_part_to_ptes(part, &br->pte[mbr_ebr_pte_this],
	    &br->pte[mbr_ebr_pte_next]);
	br->signature = host2uint16_t_le(mbr_br_signature);

	ba = part->block0 - part->hdr_blocks;

	log_msg(LOG_DEFAULT, LVL_NOTE, "Write EBR to ba=%" PRIu64, ba);
	rc = block_write_direct(label->svc_id, ba, 1, br);
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

static int mbr_ebr_delete(label_t *label, label_part_t *part)
{
	mbr_br_block_t *br;
	uint64_t ba;
	int rc;

	br = calloc(1, label->block_size);
	if (br == NULL)
		return ENOMEM;

	ba = part->block0 - part->hdr_blocks;

	rc = block_write_direct(label->svc_id, ba, 1, br);
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

/** Update 'next' PTE in EBR of partition. */
static int mbr_ebr_update_next(label_t *label, label_part_t *part)
{
	mbr_br_block_t *br;
	uint64_t ba;
	uint16_t sgn;
	int rc;

	ba = part->block0 - part->hdr_blocks;

	log_msg(LOG_DEFAULT, LVL_NOTE, "mbr_ebr_update_next ba=%" PRIu64,
	    ba);

	br = calloc(1, label->block_size);
	if (br == NULL)
		return ENOMEM;

	log_msg(LOG_DEFAULT, LVL_NOTE, "mbr_ebr_update_next read ba=%" PRIu64,
	    ba);

	rc = block_read_direct(label->svc_id, ba, 1, br);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	/* Verify boot record signature */
	sgn = uint16_t_le2host(br->signature);
	if (sgn != mbr_br_signature) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "mbr_ebr_update_next signature error");
		rc = EIO;
		goto error;
	}

	mbr_log_part_to_ptes(part, NULL, &br->pte[mbr_ebr_pte_next]);

	log_msg(LOG_DEFAULT, LVL_NOTE, "mbr_ebr_update_next write ba=%" PRIu64,
	    ba);

	rc = block_write_direct(label->svc_id, ba, 1, br);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "mbr_ebr_update_next success");

	free(br);
	return EOK;
error:
	free(br);
	return rc;
}

/** Update indices of logical partitions.
 *
 * Logical partition indices are unstable, i.e. they can change during
 * the lifetime of a logical partition. Since the index corresponds to the
 * position of the partition in order of block address, anytime a partition
 * is created or deleted, indices of all partitions at higher addresses
 * change.
 */
static void mbr_update_log_indices(label_t *label)
{
	label_part_t *part;
	int idx;

	idx = mbr_nprimary + 1;

	part = mbr_log_part_first(label);
	while (part != NULL) {
		part->index = idx++;
		part = mbr_log_part_next(part);
	}
}

/** @}
 */
