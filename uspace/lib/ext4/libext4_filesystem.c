/*
 * Copyright (c) 2011 Frantisek Princ
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

/** @addtogroup libext4
 * @{
 */ 

/**
 * @file	libext4_filesystem.c
 * @brief	TODO
 */

#include <errno.h>
#include <malloc.h>
#include "libext4_filesystem.h"

/**
 * TODO doxy
 */
int ext4_filesystem_init(ext4_filesystem_t *fs, service_id_t service_id)
{

	int rc;
	ext4_superblock_t *temp_superblock;
	size_t block_size;

	fs->device = service_id;

	// TODO what does constant 2048 mean?
	rc = block_init(EXCHANGE_SERIALIZE, fs->device, 2048);
	if (rc != EOK) {
		return rc;
	}

	/* Read superblock from device */
	rc = ext4_superblock_read_direct(fs->device, &temp_superblock);
	if (rc != EOK) {
		block_fini(fs->device);
		return rc;
	}

	/* Read block size from superblock and check */
	block_size = ext4_superblock_get_block_size(temp_superblock);
	if (block_size > EXT4_MAX_BLOCK_SIZE) {
		block_fini(fs->device);
		return ENOTSUP;
	}

	/* Initialize block caching */
	rc = block_cache_init(service_id, block_size, 0, CACHE_MODE_WT);
	if (rc != EOK) {
		block_fini(fs->device);
		return rc;
	}

	/* Return loaded superblock */
	fs->superblock = temp_superblock;

	return EOK;
}

/**
 * TODO doxy
 */
int ext4_filesystem_check_sanity(ext4_filesystem_t *fs)
{
	int rc;

	rc = ext4_superblock_check_sanity(fs->superblock);
	if (rc != EOK) {
		return rc;
	}

	return EOK;
}

/**
 * TODO doxy
 */
int ext4_filesystem_check_features(ext4_filesystem_t *fs, bool *o_read_only)
{
	/* Feature flags are present in rev 1 and later */
	if (ext4_superblock_get_rev_level(fs->superblock) == 0) {
		*o_read_only = false;
		return EOK;
	}

	uint32_t incompatible_features;
	incompatible_features = ext4_superblock_get_features_incompatible(fs->superblock);
	incompatible_features &= ~EXT4_FEATURE_INCOMPAT_SUPP;
	if (incompatible_features > 0) {
		*o_read_only = true;
		return ENOTSUP;
	}

	uint32_t compatible_read_only;
	compatible_read_only = ext4_superblock_get_features_read_only(fs->superblock);
	compatible_read_only &= ~EXT4_FEATURE_RO_COMPAT_SUPP;
	if (compatible_read_only > 0) {
		*o_read_only = true;
	}

	return EOK;
}

/**
 * TODO doxy
 */
void ext4_filesystem_fini(ext4_filesystem_t *fs)
{
	free(fs->superblock);
	block_fini(fs->device);
}


/**
 * @}
 */ 
