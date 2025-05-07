/*
 * Copyright (c) 2025 Miroslav Cimerman
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

/** @addtogroup hr
 * @{
 */
/**
 * @file
 */

#include <adt/list.h>
#include <block.h>
#include <byteorder.h>
#include <errno.h>
#include <inttypes.h>
#include <io/log.h>
#include <loc.h>
#include <mem.h>
#include <uuid.h>
#include <stdlib.h>
#include <stdio.h>
#include <str.h>
#include <types/uuid.h>

#include "../../../util.h"
#include "../../../var.h"

#include "g_stripe.h"

static void		*meta_gstripe_alloc_struct(void);
static errno_t		 meta_gstripe_init_vol2meta(const hr_volume_t *, void *);
static errno_t		 meta_gstripe_init_meta2vol(const list_t *,
    hr_volume_t *);
static void		 meta_gstripe_encode(void *, void *);
static errno_t		 meta_gstripe_decode(const void *, void *);
static errno_t		 meta_gstripe_get_block(service_id_t, void **);
static errno_t		 meta_gstripe_write_block(service_id_t, const void *);
static bool		 meta_gstripe_has_valid_magic(const void *);
static bool		 meta_gstripe_compare_uuids(const void *, const void *);
static void		 meta_gstripe_inc_counter(void *);
static errno_t		 meta_gstripe_save(hr_volume_t *, bool);
static const char	*meta_gstripe_get_devname(const void *);
static hr_level_t	 meta_gstripe_get_level(const void *);
static uint64_t		 meta_gstripe_get_data_offset(void);
static size_t		 meta_gstripe_get_size(void);
static uint8_t		 meta_gstripe_get_flags(void);
static hr_metadata_type_t	 meta_gstripe_get_type(void);
static void		 meta_gstripe_dump(const void *);

hr_superblock_ops_t metadata_gstripe_ops = {
	.alloc_struct		= meta_gstripe_alloc_struct,
	.init_vol2meta		= meta_gstripe_init_vol2meta,
	.init_meta2vol		= meta_gstripe_init_meta2vol,
	.encode			= meta_gstripe_encode,
	.decode			= meta_gstripe_decode,
	.get_block		= meta_gstripe_get_block,
	.write_block		= meta_gstripe_write_block,
	.has_valid_magic	= meta_gstripe_has_valid_magic,
	.compare_uuids		= meta_gstripe_compare_uuids,
	.inc_counter		= meta_gstripe_inc_counter,
	.save			= meta_gstripe_save,
	.get_devname		= meta_gstripe_get_devname,
	.get_level		= meta_gstripe_get_level,
	.get_data_offset	= meta_gstripe_get_data_offset,
	.get_size		= meta_gstripe_get_size,
	.get_flags		= meta_gstripe_get_flags,
	.get_type		= meta_gstripe_get_type,
	.dump			= meta_gstripe_dump
};

static void *meta_gstripe_alloc_struct(void)
{
	return calloc(1, sizeof(struct g_stripe_metadata));
}

static errno_t meta_gstripe_init_vol2meta(const hr_volume_t *vol, void *md_v)
{
	(void)vol;
	(void)md_v;
	return ENOTSUP;
}

static errno_t meta_gstripe_init_meta2vol(const list_t *list, hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;

	/* get bsize */
	size_t bsize;
	struct dev_list_member *memb = list_get_instance(list_first(list),
	    struct dev_list_member, link);
	rc = block_get_bsize(memb->svc_id, &bsize);
	if (rc != EOK)
		goto error;

	vol->bsize = bsize;

	uint64_t smallest_provider_size = ~0ULL;
	struct g_stripe_metadata *main_meta = NULL;

	list_foreach(*list, link, struct dev_list_member, iter) {
		struct g_stripe_metadata *iter_meta = iter->md;

		meta_gstripe_dump(iter_meta);

		if (iter_meta->md_provsize < smallest_provider_size) {
			smallest_provider_size = iter_meta->md_provsize;
			main_meta = iter_meta;
		}
	}

	assert(main_meta != NULL);

	vol->truncated_blkno =
	    main_meta->md_provsize / bsize;

	vol->extent_no = main_meta->md_all;

	vol->data_blkno = (vol->truncated_blkno - 1) * vol->extent_no;

	vol->data_offset = 0;

	if (main_meta->md_all > HR_MAX_EXTENTS) {
		HR_DEBUG("Assembled volume has %u extents (max = %u)",
		    (unsigned)main_meta->md_all, HR_MAX_EXTENTS);
		rc = EINVAL;
		goto error;
	}

	vol->strip_size = main_meta->md_stripesize;

	vol->layout = HR_RLQ_NONE;

	memcpy(vol->in_mem_md, main_meta, sizeof(struct g_stripe_metadata));

	list_foreach(*list, link, struct dev_list_member, iter) {
		struct g_stripe_metadata *iter_meta = iter->md;
		uint16_t index = iter_meta->md_no;

		vol->extents[index].svc_id = iter->svc_id;
		iter->fini = false;

		vol->extents[index].state = HR_EXT_ONLINE;
	}

	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].state == HR_EXT_NONE)
			vol->extents[i].state = HR_EXT_MISSING;
	}

error:
	return rc;
}

static void meta_gstripe_encode(void *md_v, void *block)
{
	HR_DEBUG("%s()", __func__);

	stripe_metadata_encode(md_v, block);
}

static errno_t meta_gstripe_decode(const void *block, void *md_v)
{
	HR_DEBUG("%s()", __func__);

	stripe_metadata_decode(block, md_v);

	return EOK;
}

static errno_t meta_gstripe_get_block(service_id_t dev, void **rblock)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	uint64_t blkno;
	size_t bsize;
	void *block;

	if (rblock == NULL)
		return EINVAL;

	rc = block_get_bsize(dev, &bsize);
	if (rc != EOK)
		return rc;

	if (bsize < sizeof(struct g_stripe_metadata))
		return EINVAL;

	rc = block_get_nblocks(dev, &blkno);
	if (rc != EOK)
		return rc;

	if (blkno < 1)
		return EINVAL;

	block = malloc(bsize);
	if (block == NULL)
		return ENOMEM;

	rc = block_read_direct(dev, blkno - 1, 1, block);
	/*
	 * XXX: here maybe call vol state event or the state callback...
	 *
	 * but need to pass vol pointer
	 */
	if (rc != EOK) {
		free(block);
		return rc;
	}

	*rblock = block;
	return EOK;
}

static errno_t meta_gstripe_write_block(service_id_t dev, const void *block)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	uint64_t blkno;
	size_t bsize;

	rc = block_get_bsize(dev, &bsize);
	if (rc != EOK)
		return rc;

	if (bsize < sizeof(struct g_stripe_metadata))
		return EINVAL;

	rc = block_get_nblocks(dev, &blkno);
	if (rc != EOK)
		return rc;

	if (blkno < 1)
		return EINVAL;

	rc = block_write_direct(dev, blkno - 1, 1, block);

	return rc;
}

static bool meta_gstripe_has_valid_magic(const void *md_v)
{
	HR_DEBUG("%s()", __func__);

	const struct g_stripe_metadata *md = md_v;

	if (str_lcmp(md->md_magic, G_STRIPE_MAGIC, 16) != 0)
		return false;

	return true;
}

static bool meta_gstripe_compare_uuids(const void *md1_v, const void *md2_v)
{
	const struct g_stripe_metadata *md1 = md1_v;
	const struct g_stripe_metadata *md2 = md2_v;
	if (md1->md_id == md2->md_id)
		return true;

	return false;
}

static void meta_gstripe_inc_counter(void *md_v)
{
	(void)md_v;
}

static errno_t meta_gstripe_save(hr_volume_t *vol, bool with_state_callback)
{
	HR_DEBUG("%s()", __func__);

	return EOK;
}

static const char *meta_gstripe_get_devname(const void *md_v)
{
	const struct g_stripe_metadata *md = md_v;

	return md->md_name;
}

static hr_level_t meta_gstripe_get_level(const void *md_v)
{
	(void)md_v;

	return HR_LVL_0;
}

static uint64_t meta_gstripe_get_data_offset(void)
{
	return 0;
}

static size_t meta_gstripe_get_size(void)
{
	return 1;
}

static uint8_t meta_gstripe_get_flags(void)
{
	uint8_t flags = 0;

	return flags;
}

static hr_metadata_type_t meta_gstripe_get_type(void)
{
	return 	HR_METADATA_GEOM_STRIPE;
}

static void meta_gstripe_dump(const void *md_v)
{
	HR_DEBUG("%s()", __func__);

	const struct g_stripe_metadata *md = md_v;

	printf("     magic: %s\n", md->md_magic);
	printf("   version: %u\n", (u_int)md->md_version);
	printf("      name: %s\n", md->md_name);
	printf("        id: %u\n", (u_int)md->md_id);
	printf("        no: %u\n", (u_int)md->md_no);
	printf("       all: %u\n", (u_int)md->md_all);
	printf("stripesize: %u\n", (u_int)md->md_stripesize);
	printf(" mediasize: %jd\n", (intmax_t)md->md_provsize);
}

/** @}
 */
