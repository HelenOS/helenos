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

typedef enum hr_level {
	HR_LVL_0	= 0x00, /* striping, no redundancy */
	HR_LVL_1	= 0x01, /* n-way mirroring */
	HR_LVL_4	= 0x04, /* dedicated parity */
	HR_LVL_5	= 0x05, /* distributed parity */
	HR_LVL_UNKNOWN	= 0xFF
} hr_level_t;

typedef enum hr_vol_status {
	HR_VOL_ONLINE,	/* OK, OPTIMAL */
	HR_VOL_FAULTY,
	HR_VOL_WEAKENED	/* used for partial, but usable mirror */
} hr_vol_status_t;

typedef enum hr_ext_status {
	HR_EXT_ONLINE,	/* OK */
	HR_EXT_MISSING,
	HR_EXT_FAILED
} hr_ext_status_t;

typedef struct hr {
	async_sess_t *sess;
} hr_t;

typedef struct hr_config {
	char devname[HR_DEVNAME_LEN];
	service_id_t devs[HR_MAXDEVS];
	size_t dev_no;
	hr_level_t level;
} hr_config_t;

typedef struct hr_extent {
	service_id_t svc_id;
	hr_ext_status_t status;
} hr_extent_t;

typedef struct hr_vol_info {
	hr_extent_t extents[HR_MAXDEVS];
	size_t extent_no;
	service_id_t svc_id;
	hr_level_t level;
	uint64_t nblocks;
	uint32_t strip_size;
	size_t bsize;
	hr_vol_status_t status;
} hr_vol_info_t;

extern errno_t hr_sess_init(hr_t **);
extern void hr_sess_destroy(hr_t *);

extern errno_t hr_create(hr_t *, hr_config_t *, bool);
extern errno_t hr_stop(const char *);
extern errno_t hr_print_status(void);

extern const char *hr_get_vol_status_msg(hr_vol_status_t);
extern const char *hr_get_ext_status_msg(hr_ext_status_t);

#endif

/** @}
 */
