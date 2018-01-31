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
 * @file Dummy label (for disks that have no recognized label)
 */

#include <errno.h>
#include <mem.h>
#include <stdlib.h>

#include "dummy.h"

static errno_t dummy_open(label_bd_t *, label_t **);
static errno_t dummy_create(label_bd_t *, label_t **);
static void dummy_close(label_t *);
static errno_t dummy_destroy(label_t *);
static errno_t dummy_get_info(label_t *, label_info_t *);
static label_part_t *dummy_part_first(label_t *);
static label_part_t *dummy_part_next(label_part_t *);
static void dummy_part_get_info(label_part_t *, label_part_info_t *);
static errno_t dummy_part_create(label_t *, label_part_spec_t *, label_part_t **);
static errno_t dummy_part_destroy(label_part_t *);
static errno_t dummy_suggest_ptype(label_t *, label_pcnt_t, label_ptype_t *);

label_ops_t dummy_label_ops = {
	.open = dummy_open,
	.create = dummy_create,
	.close = dummy_close,
	.destroy = dummy_destroy,
	.get_info = dummy_get_info,
	.part_first = dummy_part_first,
	.part_next = dummy_part_next,
	.part_get_info = dummy_part_get_info,
	.part_create = dummy_part_create,
	.part_destroy = dummy_part_destroy,
	.suggest_ptype = dummy_suggest_ptype
};

static errno_t dummy_open(label_bd_t *bd, label_t **rlabel)
{
	label_t *label = NULL;
	label_part_t *part = NULL;
	size_t bsize;
	aoff64_t nblocks;
	uint64_t ba_min, ba_max;
	errno_t rc;

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

	label = calloc(1, sizeof(label_t));
	if (label == NULL)
		return ENOMEM;

	list_initialize(&label->parts);
	list_initialize(&label->pri_parts);
	list_initialize(&label->log_parts);

	ba_min = 0;
	ba_max = nblocks;

	label->ops = &dummy_label_ops;
	label->ltype = lt_none;
	label->bd = *bd;
	label->ablock0 = ba_min;
	label->anblocks = ba_max - ba_min + 1;
	label->pri_entries = 0;
	label->block_size = bsize;

	part = calloc(1, sizeof(label_part_t));
	if (part == NULL)
		return ENOMEM;

	part->index = 0;
	part->block0 = ba_min;
	part->nblocks = ba_max - ba_min;
	part->ptype.fmt = lptf_num;

	part->label = label;
	list_append(&part->lparts, &label->parts);
	list_append(&part->lpri, &label->pri_parts);


	*rlabel = label;
	return EOK;
error:
	free(part);
	free(label);
	return rc;
}

static errno_t dummy_create(label_bd_t *bd, label_t **rlabel)
{
	return ENOTSUP;
}

static void dummy_close(label_t *label)
{
	label_part_t *part;

	part = dummy_part_first(label);
	while (part != NULL) {
		list_remove(&part->lparts);
		list_remove(&part->lpri);
		free(part);
		part = dummy_part_first(label);
	}

	free(label);
}

static errno_t dummy_destroy(label_t *label)
{
	return ENOTSUP;
}

static errno_t dummy_get_info(label_t *label, label_info_t *linfo)
{
	memset(linfo, 0, sizeof(label_info_t));
	linfo->ltype = lt_none;
	linfo->flags = 0;
	linfo->ablock0 = label->ablock0;
	linfo->anblocks = label->anblocks;
	return EOK;
}

static label_part_t *dummy_part_first(label_t *label)
{
	link_t *link;

	link = list_first(&label->parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, label_part_t, lparts);
}

static label_part_t *dummy_part_next(label_part_t *part)
{
	link_t *link;

	link = list_next(&part->lparts, &part->label->parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, label_part_t, lparts);
}

static void dummy_part_get_info(label_part_t *part, label_part_info_t *pinfo)
{
	pinfo->index = part->index;
	pinfo->pkind = lpk_primary;
	pinfo->block0 = part->block0;
	pinfo->nblocks = part->nblocks;
}

static errno_t dummy_part_create(label_t *label, label_part_spec_t *pspec,
    label_part_t **rpart)
{
	return ENOTSUP;
}

static errno_t dummy_part_destroy(label_part_t *part)
{
	return ENOTSUP;
}

static errno_t dummy_suggest_ptype(label_t *label, label_pcnt_t pcnt,
    label_ptype_t *ptype)
{
	return ENOTSUP;
}

/** @}
 */
