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

#ifndef _HR_VAR_H
#define _HR_VAR_H

#include <bd_srv.h>
#include <errno.h>
#include <hr.h>

#define NAME "hr"

#define HR_STRIP_SIZE DATA_XFER_LIMIT

typedef struct hr_volume hr_volume_t;

typedef struct hr_ops {
	errno_t (*create)(hr_volume_t *);
	errno_t (*init)(hr_volume_t *);
} hr_ops_t;

typedef struct hr_volume {
	hr_ops_t hr_ops;
	bd_srvs_t hr_bds;
	link_t lvolumes;
	char devname[32];
	hr_extent_t extents[HR_MAXDEVS];
	uint64_t nblocks;
	uint64_t data_blkno;
	uint32_t data_offset; /* in blocks */
	uint32_t strip_size;
	service_id_t svc_id;
	size_t bsize;
	size_t dev_no;
	hr_level_t level;
} hr_volume_t;

extern errno_t hr_init_devs(hr_volume_t *);
extern void hr_fini_devs(hr_volume_t *);

extern errno_t hr_raid0_create(hr_volume_t *);
extern errno_t hr_raid1_create(hr_volume_t *);
extern errno_t hr_raid4_create(hr_volume_t *);
extern errno_t hr_raid5_create(hr_volume_t *);

extern errno_t hr_raid0_init(hr_volume_t *);
extern errno_t hr_raid1_init(hr_volume_t *);
extern errno_t hr_raid4_init(hr_volume_t *);
extern errno_t hr_raid5_init(hr_volume_t *);

#endif

/** @}
 */
