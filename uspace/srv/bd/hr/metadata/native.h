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

#ifndef _HR_METADATA_NATIVE_H
#define _HR_METADATA_NATIVE_H

#include "../var.h"

/*
 * Metadata is stored on the last block of an extent.
 */
#define HR_NATIVE_META_SIZE		1	/* in blocks */
#define HR_NATIVE_DATA_OFF		0

#define HR_NATIVE_MAGIC_STR		"HelenRAID"
#define HR_NATIVE_MAGIC_SIZE		16
#define HR_NATIVE_UUID_LEN		16
#define HR_NATIVE_METADATA_VERSION	1

struct hr_metadata {
	char		magic[HR_NATIVE_MAGIC_SIZE];

	uint8_t		uuid[HR_NATIVE_UUID_LEN];

	uint64_t	data_blkno;		/* usable blocks */
	uint64_t	truncated_blkno;	/* size of smallest extent */

	uint64_t	data_offset;
	uint64_t	counter;

	uint32_t	version;		/* XXX: yet unused */
	uint32_t	extent_no;
	uint32_t	index;			/* index of extent in volume */
	uint32_t	level;

	uint32_t	layout;
	uint32_t	strip_size;

	uint32_t	bsize;

	char		devname[HR_DEVNAME_LEN];
} __attribute__((packed));

#endif

/** @}
 */
