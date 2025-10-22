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

#include <loc.h>
#include <stdio.h>

#include "../superblock.h"
#include "../util.h"
#include "../var.h"

static errno_t meta_noop_probe(service_id_t, void **);
static errno_t meta_noop_init_vol2meta(hr_volume_t *);
static errno_t meta_noop_init_meta2vol(const list_t *, hr_volume_t *);
static errno_t meta_noop_erase_block(service_id_t);
static bool meta_noop_compare_uuids(const void *, const void *);
static void meta_noop_inc_counter(hr_volume_t *);
static errno_t meta_noop_save(hr_volume_t *, bool);
static errno_t meta_noop_save_ext(hr_volume_t *, size_t, bool);
static const char *meta_noop_get_devname(const void *);
static hr_level_t meta_noop_get_level(const void *);
static uint64_t meta_noop_get_data_offset(void);
static size_t meta_noop_get_size(void);
static uint8_t meta_noop_get_flags(void);
static hr_metadata_type_t meta_noop_get_type(void);
static void meta_noop_dump(const void *);

hr_superblock_ops_t noop_ops = {
	.probe = meta_noop_probe,
	.init_vol2meta = meta_noop_init_vol2meta,
	.init_meta2vol = meta_noop_init_meta2vol,
	.erase_block = meta_noop_erase_block,
	.compare_uuids = meta_noop_compare_uuids,
	.inc_counter = meta_noop_inc_counter,
	.save = meta_noop_save,
	.save_ext = meta_noop_save_ext,
	.get_devname = meta_noop_get_devname,
	.get_level = meta_noop_get_level,
	.get_data_offset = meta_noop_get_data_offset,
	.get_size = meta_noop_get_size,
	.get_flags = meta_noop_get_flags,
	.get_type = meta_noop_get_type,
	.dump = meta_noop_dump
};

static errno_t meta_noop_probe(service_id_t svc_id, void **rmd)
{
	HR_DEBUG("%s()", __func__);

	return ENOTSUP;
}

static errno_t meta_noop_init_vol2meta(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	return EOK;
}

static errno_t meta_noop_init_meta2vol(const list_t *list, hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	return ENOTSUP;
}

static errno_t meta_noop_erase_block(service_id_t dev)
{
	HR_DEBUG("%s()", __func__);

	return EOK;
}

static bool meta_noop_compare_uuids(const void *m1p, const void *m2p)
{
	return false;
}

static void meta_noop_inc_counter(hr_volume_t *vol)
{
	(void)vol;
}

static errno_t meta_noop_save(hr_volume_t *vol, bool with_state_callback)
{
	HR_DEBUG("%s()", __func__);

	return EOK;
}

static errno_t meta_noop_save_ext(hr_volume_t *vol, size_t ext_idx,
    bool with_state_callback)
{
	HR_DEBUG("%s()", __func__);

	return EOK;
}

static const char *meta_noop_get_devname(const void *md_v)
{
	return NULL;
}

static hr_level_t meta_noop_get_level(const void *md_v)
{
	return HR_LVL_UNKNOWN;
}

static uint64_t meta_noop_get_data_offset(void)
{
	return 0;
}

static size_t meta_noop_get_size(void)
{
	return 0;
}

static uint8_t meta_noop_get_flags(void)
{
	HR_DEBUG("%s()", __func__);

	uint8_t flags = 0;

	flags |= HR_METADATA_HOTSPARE_SUPPORT;
	flags |= HR_METADATA_ALLOW_REBUILD;

	return flags;
}

static hr_metadata_type_t meta_noop_get_type(void)
{
	HR_DEBUG("%s()", __func__);

	return HR_METADATA_NOOP;
}

static void meta_noop_dump(const void *md_v)
{
	HR_DEBUG("%s()", __func__);

	printf("NOOP Metadata\n");
}

/** @}
 */
