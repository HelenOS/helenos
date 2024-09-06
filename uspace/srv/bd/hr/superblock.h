/*
 * Copyright (c) 2024 Miroslav Cimerman
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

#define HR_META_SIZE 1	/* in blocks */
#define HR_META_OFF 7	/* in blocks */

#define HR_MAGIC 0x4420492041205248LLU

typedef struct hr_metadata {
	uint64_t magic;
	uint32_t extent_no;
	uint32_t level;
	uint64_t nblocks;	/* all blocks */
	uint64_t data_blkno;	/* usable blocks */
	uint32_t data_offset;	/* block where data starts */
	uint32_t index;		/* index of disk in array */
	uint32_t strip_size;
	uint32_t status;	/* yet unused */
	uint8_t uuid[16];
	char devname[32];
} hr_metadata_t;

extern errno_t hr_write_meta_to_vol(hr_volume_t *);
extern errno_t hr_get_vol_from_meta(hr_config_t *, hr_volume_t *);

#endif

/** @}
 */
