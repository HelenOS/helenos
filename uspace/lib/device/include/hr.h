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

/** @addtogroup libdevice
 * @{
 */
/**
 * @file
 */

#ifndef LIBDEVICE_HR_H
#define LIBDEVICE_HR_H

#include <async.h>
#include <errno.h>
#include <loc.h>

/* for now */
#define HR_MAXDEVS 4

#define HR_DEVNAME_LEN 32

typedef struct hr {
	async_sess_t *sess;
} hr_t;

typedef enum hr_level {
	hr_l_0 = 0,
	hr_l_1 = 1,
	hr_l_4 = 4,
	hr_l_5 = 5,
	hr_l_linear = 254,
	hr_l_empty = 255
} hr_level_t;

typedef struct hr_config {
	char devname[HR_DEVNAME_LEN];
	service_id_t devs[HR_MAXDEVS];
	size_t dev_no;
	hr_level_t level;
} hr_config_t;

typedef struct hr_extent {
	service_id_t svc_id;
	int status;
} hr_extent_t;

typedef struct hr_vol_info {
	hr_extent_t extents[HR_MAXDEVS];
	size_t extent_no;
	service_id_t svc_id;
	hr_level_t level;
	uint64_t nblocks;
	uint32_t strip_size;
	size_t bsize;
} hr_vol_info_t;

extern errno_t hr_sess_init(hr_t **);
extern void hr_sess_destroy(hr_t *);

extern errno_t hr_create(hr_t *, hr_config_t *, bool);
extern errno_t hr_stop(const char *);
extern errno_t hr_print_status(void);

#endif

/** @}
 */
