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

#ifndef _HR_SUPERBLOCK_H
#define _HR_SUPERBLOCK_H

#include "var.h"

/*
 * Metadata is stored on the last block of an extent.
 */
#define HR_META_SIZE		1	/* in blocks */
#define HR_DATA_OFF		0

#define HR_MAGIC_STR		"HelenRAID"
#define HR_MAGIC_SIZE		16
#define HR_UUID_LEN		16
#define HR_METADATA_VERSION	1

typedef struct hr_metadata hr_metadata_t;
typedef struct hr_volume hr_volume_t;

struct hr_metadata {
	char		magic[HR_MAGIC_SIZE];

	uint8_t		uuid[HR_UUID_LEN];

	/* TODO: change to blkno */
	uint64_t	nblocks;		/* all blocks */
	uint64_t	data_blkno;		/* usable blocks */

	uint64_t	truncated_blkno;	/* usable blocks */
	uint64_t	data_offset;

	uint64_t	counter;		/* XXX: yet unused */
	uint32_t	version;		/* XXX: yet unused */
	uint32_t	extent_no;

	uint32_t	index;			/* index of extent in volume */
	uint32_t	level;
	uint32_t	layout;
	uint32_t	strip_size;

	uint32_t	bsize;

	char		devname[HR_DEVNAME_LEN];
} __attribute__((packed));

extern errno_t	hr_metadata_init(hr_volume_t *, hr_metadata_t *);
extern errno_t	hr_metadata_save(hr_volume_t *);
extern errno_t	hr_write_metadata_block(service_id_t, const void *);
extern errno_t	hr_get_metadata_block(service_id_t, void **);
extern void	hr_encode_metadata_to_block(hr_metadata_t *, void *);
extern void	hr_decode_metadata_from_block(const void *, hr_metadata_t *);
extern void	hr_metadata_dump(hr_metadata_t *);
extern bool	hr_valid_md_magic(hr_metadata_t *);

#endif

/** @}
 */
