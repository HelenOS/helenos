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

typedef struct hr_volume hr_volume_t;

#define HR_METADATA_HOTSPARE_SUPPORT 0x01

typedef struct hr_superblock_ops {
	void		*(*alloc_struct)(void);
	errno_t		 (*init_vol2meta)(const hr_volume_t *, void *);
	errno_t		 (*init_meta2vol)(const list_t *, hr_volume_t *);
	void		 (*encode)(void *, void *);
	errno_t		 (*decode)(const void *, void *);
	errno_t		 (*get_block)(service_id_t, void **);
	errno_t		 (*write_block)(service_id_t, const void *);
	bool		 (*has_valid_magic)(const void *);
	bool		 (*compare_uuids)(const void *, const void *);
	void		 (*inc_counter)(void *);
	errno_t		 (*save)(hr_volume_t *, bool);
	const char	*(*get_devname)(const void *);
	hr_level_t	 (*get_level)(const void *);
	uint64_t	 (*get_data_offset)(void);
	size_t		 (*get_size)(void);
	uint8_t		 (*get_flags)(void);
	void		 (*dump)(const void *);
	hr_metadata_type_t (*get_type)(void);
} hr_superblock_ops_t;

extern hr_superblock_ops_t *get_type_ops(hr_metadata_type_t);
extern errno_t	find_metadata(service_id_t, void **, hr_metadata_type_t *);

#endif

/** @}
 */
